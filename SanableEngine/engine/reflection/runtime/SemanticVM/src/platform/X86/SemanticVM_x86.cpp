#include "SemanticVM.hpp"

#include <cassert>

#include "CapstoneWrapper.hpp"
#include "FunctionBytecodeWalker.hpp"

void pad(int& charsPrinted, int desiredWidth)
{
	if (charsPrinted < desiredWidth)
	{
		printf("%*c", desiredWidth-charsPrinted, ' ');
		charsPrinted = desiredWidth;
	}
}

int pad_inline(int charsPrinted, int desiredWidth)
{
	pad(charsPrinted, desiredWidth);
	return charsPrinted;
}

int debugPrintOperand(const MachineState& state, const cs_insn* insn, int opId)
{
	if (insn->detail->x86.operands[opId].type == X86_OP_MEM)
	{
		return printf("*(") + state.decodeMemAddr(insn->detail->x86.operands[opId].mem).debugPrintValue(false) + printf(") ");
	}
	else return 0;
}

bool isOperandIdentity(const cs_insn* insn, int id1, int id2)
{
	cs_x86_op* ops = insn->detail->x86.operands;
	
	if (ops[id1].type != ops[id2].type) return false;

	switch (ops[id1].type)
	{
	case X86_OP_IMM: return memcmp(&ops[id1].imm, &ops[id2].imm, sizeof(ops[id1].imm)) == 0;
	case X86_OP_REG: return memcmp(&ops[id1].reg, &ops[id2].reg, sizeof(ops[id1].reg)) == 0;
	case X86_OP_MEM: return memcmp(&ops[id1].mem, &ops[id2].mem, sizeof(ops[id1].mem)) == 0;

	default: assert(false); return false;
	}
}


void SemanticVM::step(MachineState& state, const cs_insn* insn, const std::function<void(void*)>& pushCallStack, const std::function<void*()>& popCallStack, const std::function<void(void*)>& jump, const std::function<void(const std::vector<void*>&)>& fork)
{
	if (insn->id == x86_insn::X86_INS_LEA)
	{
		auto addr = state.getOperand(insn, 1);
		state.setOperand(insn, 0, addr);
		if (debug) { printf("   ; = ") + addr.debugPrintValue(false); }
	}
	else if (insn->id == x86_insn::X86_INS_MOV)
	{
		auto val = state.getOperand(insn, 1);

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		state.setOperand(insn, 0, val);
	}
	else if (insn->id == x86_insn::X86_INS_MOVSX || insn->id == x86_insn::X86_INS_MOVSXD)
	{
		SemanticValue val = state.getOperand(insn, 1);

		size_t targetSize = insn->detail->x86.operands[0].size;
		if (SemanticKnownConst* c = val.tryGetKnownConst()) val = c->signExtend(targetSize);
		else if (val.isUnknown()) val = SemanticUnknown(targetSize);
		else assert(false && "Value type cannot be sign-extended");

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		state.setOperand(insn, 0, val);
	}
	else if (insn->id == x86_insn::X86_INS_CMP)
	{
		std::optional<bool> cf, of, sf, zf, af, pf;
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticKnownConst* c1 = op1.tryGetKnownConst();
		SemanticKnownConst* c2 = op2.tryGetKnownConst();
		if (c1 && c2)
		{
			SemanticKnownConst cSum = *(op1+op2).tryGetKnownConst();
			SemanticKnownConst cDiff = *(op2-op1).tryGetKnownConst();
			uint64_t msbMask = 1ull << (cDiff.size*8 - 1);
			sf = cDiff.value & msbMask; //Most significant bit is sign bit in both 1's and 2's compliment machines
			pf = !(cDiff.value & 1); //Even or odd: check least significant bit
			zf = cDiff.bound() == 0;

			//CF and OF: see https://teaching.idallen.com/dat2343/10f/notes/040_overflow.txt
			{
				uint64_t parity = ((c1->value&1) + (c2->value&1)) >> 1; //Precompute first bit, as it would otherwise be lost in shift
				cf = ( (c1->bound()>>1) + (c2->bound()>>1) + parity ) & msbMask //Adding would cause rollover
					|| c2->bound() > c1->bound(); //Subtracting would cause borrow
			}
			of = (c1->bound()&msbMask) == (c2->bound()&msbMask)
			  && (c1->bound()&msbMask) != (cSum.bound()&msbMask);

			//Aux carry: only if carry/borrow was generated in lowest 4 bits. TODO check
			//See https://en.wikipedia.org/wiki/Half-carry_flag section on x86
			af = ( (c1->bound()&0x0F) + (c2->bound()&0x0F) )&0x10;
		}
		else if (op1.isUnknown() || op2.isUnknown())
		{
			cf = of = sf = zf = af = pf = std::nullopt;
		}
		else assert(false && "Cannot compare: Unhandled case");
		
		//Write back flags
		SemanticFlags flags = *state.getRegister(X86_REG_EFLAGS).tryGetFlags();
		flags.set((int)MachineState::FlagIDs::Carry   , cf);
		flags.set((int)MachineState::FlagIDs::Overflow, of);
		flags.set((int)MachineState::FlagIDs::Sign    , sf);
		flags.set((int)MachineState::FlagIDs::Zero    , zf);
		flags.set((int)MachineState::FlagIDs::AuxCarry, af);
		flags.set((int)MachineState::FlagIDs::Parity  , pf);
		state.setRegister(X86_REG_EFLAGS, flags);

		//Debug
		if (debug)
		{
			printf("   ; ") + op1.debugPrintValue(false) + printf(" ?= ") + op2.debugPrintValue(false) + printf(": ");

			printf("cf=%s ", cf.has_value() ? (cf.value() ? "Y" : "N") : "U");
			printf("of=%s ", of.has_value() ? (of.value() ? "Y" : "N") : "U");
			printf("sf=%s ", sf.has_value() ? (sf.value() ? "Y" : "N") : "U");
			printf("zf=%s ", zf.has_value() ? (zf.value() ? "Y" : "N") : "U");
			printf("af=%s ", af.has_value() ? (af.value() ? "Y" : "N") : "U");
			printf("pf=%s ", pf.has_value() ? (pf.value() ? "Y" : "N") : "U");
		}
	}
	else if (insn_in_group(*insn, cs_group_type::CS_GRP_CALL))
	{
		SemanticKnownConst fp  = *state.getOperand(insn, 0).tryGetKnownConst();
		state.pushStackFrame(fp);
		pushCallStack((uint8_t*)fp.value); //Sanity check. Also no ROP nonsense
	}
	else if (insn_in_group(*insn, cs_group_type::CS_GRP_RET))
	{
		SemanticValue returnAddr = state.popStackFrame();

		//If given an operand, pop that many bytes from the stack
		if (insn->detail->x86.op_count == 1) state.stackPop(state.getOperand(insn, 0).tryGetKnownConst()->value);

		void* poppedReturnAddr = popCallStack();
		if (poppedReturnAddr) assert(poppedReturnAddr == (void*)returnAddr.tryGetKnownConst()->value);
	}
	else if (insn->id == x86_insn::X86_INS_PUSH)
	{
		state.stackPush(state.getOperand(insn, 0));
	}
	else if (insn->id == x86_insn::X86_INS_POP)
	{
		auto val = state.stackPop(insn->detail->x86.operands[0].size);
		state.setOperand(insn, 0, val);
		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
	}
	else if (insn->id == x86_insn::X86_INS_XCHG)
	{
		auto val1 = state.getOperand(insn, 0);
		auto val2 = state.getOperand(insn, 1);
		state.setOperand(insn, 0, val2);
		state.setOperand(insn, 1, val1);

		if (debug)
		{
			printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val1.debugPrintValue(false);
			printf(" | ")   + debugPrintOperand(state, insn, 1) + printf(":= ") + val1.debugPrintValue(false);
		}
	}
	else if (insn->id == x86_insn::X86_INS_SUB)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1-op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
	}
	else if (insn->id == x86_insn::X86_INS_ADD)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1+op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
	}
	else if (insn->id == x86_insn::X86_INS_AND)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1&op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
	}
	else if (insn->id == x86_insn::X86_INS_OR)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1|op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
	}
	else if (insn->id == x86_insn::X86_INS_XOR)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = !isOperandIdentity(insn, 0, 1) ? op1^op2 : SemanticKnownConst(0, op1.getSize(), false); //Special case: XOR with self is a common trick to produce a 0
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
	}
	else if (insn->id == x86_insn::X86_INS_DEC)
	{
		auto op = state.getOperand(insn, 0);
		state.setOperand(insn, 0, op-SemanticKnownConst(1, op.getSize(), false) );
	}
	else if (insn->id == x86_insn::X86_INS_INC)
	{
		auto op = state.getOperand(insn, 0);
		state.setOperand(insn, 0, op+SemanticKnownConst(1, op.getSize(), false) );
	}
	else if (insn->id == x86_insn::X86_INS_NOP)
	{
		//Do nothing
	}
	else if (insn_in_group(*insn, cs_group_type::CS_GRP_BRANCH_RELATIVE)) //TODO: Really hope nobody uses absolute branching. Is that a thing?
	{
		std::optional<bool> conditionMet = state.isConditionMet(insn->id);

		auto* ifBranch = state.getOperand(insn, 0).tryGetKnownConst();
		auto* noBranch = state.getRegister(x86_reg::X86_REG_RIP).tryGetKnownConst();
		assert(ifBranch && "Attempted to branch to an indeterminate location!");
		assert(noBranch && "Attempted to branch to an indeterminate location!");

		//Indeterminate case: fork
		if (!conditionMet.has_value())
		{
			fork({
				(void*)noBranch->value,
				(void*)ifBranch->value
			});
		}

		//Determinate case: jump if condition met
		else
		{
			if (conditionMet.value()) jump((void*)ifBranch->value);

			if (debug) printf("   ; Determinate: %s", conditionMet.value() ? "Branching" : "Not branching");
		}
	}
	else
	{
		//Unhandled operation: Just invalidate all relevant register state
		cs_regs  regsRead,  regsWritten;
		uint8_t nRegsRead, nRegsWritten;
		cs_regs_access(capstone_get_instance(), insn,
			regsRead   , &nRegsRead,
			regsWritten, &nRegsWritten);
		for (int i = 0; i < nRegsWritten; ++i) state.setRegister((x86_reg)regsWritten[i], SemanticUnknown(sizeof(void*)) ); //TODO read correct size
	}
}

void SemanticVM::execFunc_internal(MachineState& state, void(*fn)(), void(*expectedReturnAddress)(), int indentLevel, const std::vector<void(*)()>& allocators, const std::vector<void(*)()>& sandboxed)
{
	//Prepare capstone disassembler
	cs_insn* insn = cs_malloc(capstone_get_instance());

	//Prepare branch list
	struct branch_t
	{
		const uint8_t* cursor;
		MachineState state;
		bool keepExecuting;
	};
	std::vector<branch_t> branches = {
		{ (uint8_t*)fn, state, true }
	};

	while (true)
	{
		//Canonize branches with the same cursor
		for (int i = 0; i < branches.size(); ++i)
		{
			for (int j = i+1; j < branches.size(); ++j)
			{
				if (branches[i].cursor == branches[j].cursor && branches[i].keepExecuting && branches[j].keepExecuting)
				{
					if (debug) printf("Branch #%i merged into #%i\n", j, i);
					branches[i].state = MachineState::merge({ &branches[i].state, &branches[j].state }); //Canonize state
					branches.erase(branches.begin()+i); //Erase copy
					j--; //Don't skip!
				}
			}
		}

		//Execute the cursor that's furthest behind
		int toExecIndex = -1;
		for (int i = 0; i < branches.size(); ++i)
		{
			if (branches[i].keepExecuting && (toExecIndex == -1 || branches[i].cursor < branches[toExecIndex].cursor))
			{
				toExecIndex = i;
			}
		}
		if (toExecIndex == -1) break; //All branches have terminated
		branch_t* toExec = &branches[toExecIndex];

		//Advance cursor and interpret next
		uint64_t addr = (uint64_t)(uint_addr_t)(toExec->cursor);
		size_t allowedToProcess = sizeof(cs_insn::bytes); //No way to know for sure, but we can do some stuff with JUMP/RET detection to figure it out
		bool disassemblyGood = cs_disasm_iter(capstone_get_instance(), &toExec->cursor, &allowedToProcess, &addr, insn);
		assert(disassemblyGood && "An internal error occurred with the Capstone disassembler.");
		
		//Update emulated instruction pointer
		SemanticValue rip = toExec->state.getRegister(X86_REG_RIP);
		toExec->state.setRegister(X86_REG_RIP, SemanticKnownConst(addr, rip.getSize(), true) ); //Ensure RIP is up to date

		//DEBUG
		if (debug)
		{
			toExec->state.debugPrintWorkingSet();
			for (int i = 0; i < indentLevel; ++i) printf(" |  "); //Indent
			printf("[Branch %2i] ", toExecIndex);
			printInstructionCursor(insn);
		}

		//Execute current call frame, one opcode at a time
		void(*callTarget)() = nullptr;
		std::vector<void*> jmpTargets; //Empty if no JMP, 1 element if determinate JMP, 2+ elements if indeterminate JMP
		step(toExec->state, insn,
			[&](void* fn) { callTarget = (void(*)()) fn; }, //On CALL
			[&]() { //On RET
				toExec->keepExecuting = false;
				if (debug) printf("   ; execution terminated", toExecIndex, toExec->cursor);
				return expectedReturnAddress;
			},
			[&](void* jmp) { jmpTargets = { jmp }; }, //On jump
			[&](const std::vector<void*>& forks) { jmpTargets = forks; } //On fork
		);
		
		//Handle jumping/branching
		if (jmpTargets.size() == 1) toExec->cursor = (uint8_t*)jmpTargets[0]; //Determinate case: jump
		else if (jmpTargets.size() > 1) //Indeterminate case: branch
		{
			for (void* i : jmpTargets) assert(i >= toExec->cursor && "Indeterminate looping not supported");

			if (debug) printf("   ; Spawned branches ", toExecIndex, toExec->cursor);
			toExec->cursor = (uint8_t*)jmpTargets[0];
			for (int i = 1; i < jmpTargets.size(); ++i)
			{
				if (debug) printf("#%i@%x ", i, jmpTargets[i]);
				branches.push_back({ (uint8_t*)jmpTargets[i], branches[toExecIndex].state, true }); //Can't reference toExec here in case the backing block reallocates
			}
		}

		//DEBUG
		if (debug) printf("\n");
		assert(toExec->state.getRegister(X86_REG_RSP).tryGetKnownConst() && "Lost track of RSP! This should never happen!");

		//If we're supposed to call another function, do so
		if (callTarget)
		{
			if (std::find(allocators.begin(), allocators.end(), callTarget) != allocators.end())
			{
				//No need to call function, just mark that we're creating a new heap object
				
				//TODO proper register detection
				//std::vector<x86_reg> covariants = ???;
				//assert(covariants.size() == 1);
				//toExec->state.setRegister(covariants[0], SemanticThisPtr(0)); //TODO dynamic heap-object counting
				toExec->state.setRegister(x86_reg::X86_REG_EAX, SemanticThisPtr(0)); //TODO dynamic heap-object counting
				toExec->state.popStackFrame(); //Undo pushing stack frame
				if (debug) printf("Allocated heap object #%i. Allocator function will not be simulated.\n", 0);
			}
			else if (std::find(sandboxed.begin(), sandboxed.end(), callTarget) != sandboxed.end())
			{
				//TODO implement
				toExec->state.popStackFrame(); //TEMP: Undo pushing stack frame
				if (debug) printf("Function is sandboxed, and will not be simulated.\n");
			}
			else
			{
				execFunc_internal(toExec->state, callTarget, (void(*)())toExec->cursor, indentLevel+1, allocators, sandboxed);
			}
		}
	}

	//Canonize all remaining states
	std::vector<const MachineState*> divergentStates;
	for (const branch_t& branch : branches) divergentStates.push_back(&branch.state);
	state = MachineState::merge(divergentStates); //TODO handle sandboxing
	
	//Cleanup capstone disassembler
	cs_free(insn, 1);
}

void SemanticVM::execFunc(MachineState& state, void(*fn)(), const std::vector<void(*)()>& allocators, const std::vector<void(*)()>& sandboxed)
{
	//assert(callConv == CallConv::ThisCall); //That's all we're supporting right now

	//Setup: set flags
	state.setRegister(X86_REG_RIP, SemanticUnknown(sizeof(void*)) ); //TODO: 32-bit-on-64 support?
	state.setRegister(X86_REG_RBP, SemanticUnknown(sizeof(void*)) ); //Caller is indeterminate. TODO: 32-bit-on-64 support?
	state.setRegister(X86_REG_RSP, SemanticKnownConst(-2 * sizeof(void*), sizeof(void*), false)); //TODO: This is a magic value, the size of one stack frame. Should be treated similarly to ThisPtr instead.
	{
		SemanticFlags flags;
		flags.bits = 0;
		flags.bitsKnown = ~0ull;
		state.setRegister(X86_REG_EFLAGS, flags);
	}

	//Invoke
	execFunc_internal(state, fn, nullptr, 0, allocators, sandboxed);

	//Ensure output parity
	SemanticValue rsp = state.getRegister(X86_REG_RSP);
	SemanticKnownConst* const_rsp = rsp.tryGetKnownConst();
	assert(const_rsp && const_rsp->value == 0); //TODO handle return value
}
