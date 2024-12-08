#include "SemanticVM.hpp"

#include <cassert>

#include "CapstoneWrapper.hpp"
#include "FunctionBytecodeWalker.hpp"

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


#pragma region Per-instruction-group step functions

bool SemanticVM::step_dataflow(MachineState& state, const cs_insn* insn, const std::function<void(const std::string&)>& reportError)
{
	if (insn->id == x86_insn::X86_INS_LEA)
	{
		auto addr = state.getOperand(insn, 1);
		state.setOperand(insn, 0, addr);
		if (debug) { printf("   ; = ") + addr.debugPrintValue(false); }
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_MOV)
	{
		auto val = state.getOperand(insn, 1);
		state.setOperand(insn, 0, val);
		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_MOVABS)
	{
		auto val = state.getOperand(insn, 1);
		state.setOperand(insn, 0, val);
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_MOVZX)
	{
		auto val = state.getOperand(insn, 1);

		if (SemanticKnownConst* c = val.tryGetKnownConst()) *c = c->zeroExtend(insn->detail->x86.operands[0].size);
		else if (SemanticFlags* f = val.tryGetFlags())
		{
			uint64_t mask = (~0ull)<<(val.getSize()*8) & ~( (~0ull)<<(insn->detail->x86.operands[0].size*8) );
			f->bits &= ~mask;
			f->bitsKnown |= mask;
		}

		state.setOperand(insn, 0, val);
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_MOVSX || insn->id == x86_insn::X86_INS_MOVSXD)
	{
		SemanticValue val = state.getOperand(insn, 1);

		size_t targetSize = insn->detail->x86.operands[0].size;
		if (SemanticKnownConst* c = val.tryGetKnownConst()) val = c->signExtend(targetSize);
		else if (val.isUnknown()) val = SemanticUnknown(targetSize);
		else reportError("Value type cannot be sign-extended");

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		state.setOperand(insn, 0, val);
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_PUSH)
	{
		state.stackPush(state.getOperand(insn, 0));
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_POP)
	{
		auto val = state.stackPop(insn->detail->x86.operands[0].size);
		state.setOperand(insn, 0, val);
		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }
		return true;
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

		return true;
	}
	else return false;
}

bool SemanticVM::step_cmpmath(MachineState& state, const cs_insn* insn, const std::function<void(const std::string&)>& reportError)
{
	if (insn->id == x86_insn::X86_INS_CMP)
	{
		std::optional<bool> cf, of, sf, zf, af, pf;
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		if (op1.tryGetKnownConst() && op2.tryGetKnownConst())
		{
			SemanticKnownConst* c1 = op1.tryGetKnownConst();
			SemanticKnownConst* c2 = op2.tryGetKnownConst();

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
		else if (op1.tryGetMagic() && op2.tryGetMagic())
		{
			SemanticMagic* m1 = op1.tryGetMagic();
			SemanticMagic* m2 = op2.tryGetMagic();
			if (m1->id == m2->id)
			{
				SemanticKnownConst cSum(m1->offset + m2->offset, 8, false);
				SemanticKnownConst cDiff(m2->offset - m1->offset, 8, false);
				uint64_t msbMask = 1ull << (cDiff.size * 8 - 1);
				sf = cDiff.value & msbMask; //Most significant bit is sign bit in both 1's and 2's compliment machines
				pf = !(cDiff.value & 1); //Even or odd: check least significant bit
				zf = cDiff.bound() == 0;

				//CF and OF: see https://teaching.idallen.com/dat2343/10f/notes/040_overflow.txt
				{
					uint64_t parity = ((m1->offset & 1) + (m2->offset & 1)) >> 1; //Precompute first bit, as it would otherwise be lost in shift
					cf = ((m1->offset >> 1) + (m2->offset >> 1) + parity) & msbMask //Adding would cause rollover
						|| m2->offset > m1->offset; //Subtracting would cause borrow
				}
				of = (m1->offset & msbMask) == (m2->offset & msbMask)
					&& (m1->offset & msbMask) != (cSum.bound() & msbMask);

				//Aux carry: only if carry/borrow was generated in lowest 4 bits. TODO check
				//See https://en.wikipedia.org/wiki/Half-carry_flag section on x86
				af = ((m1->offset & 0x0F) + (m2->offset & 0x0F)) & 0x10;
			}
			else
			{
				reportError("Cannot compare: Magic value basis mismatched, result is indeterminate");
			}
		}
		else if (op1.isUnknown() || op2.isUnknown())
		{
			cf = of = sf = zf = af = pf = std::nullopt;
		}
		else reportError("Cannot compare: Unhandled case");
		
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

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_TEST)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticKnownConst* c1 = op1.tryGetKnownConst();
		SemanticKnownConst* c2 = op2.tryGetKnownConst();
		
		std::optional<bool> sf, zf, pf;
		if (c1 && c2)
		{
			sf = c1->isSigned() && c2->isSigned();
			zf = (c1->bound()&c2->bound())==0;
			pf = 0b1 & ~(c1->bound()^c2->bound());
		}
		else if (op1.isUnknown() || op2.isUnknown())
		{
			sf = zf = pf = std::nullopt;
		}
		else reportError("Cannot compare: Unhandled case");
		
		//Write back flags
		SemanticFlags flags = *state.getRegister(X86_REG_EFLAGS).tryGetFlags();
		flags.set((int)MachineState::FlagIDs::Sign    , sf);
		flags.set((int)MachineState::FlagIDs::Zero    , zf);
		flags.set((int)MachineState::FlagIDs::Parity  , pf);
		flags.set((int)MachineState::FlagIDs::Carry   , false);
		flags.set((int)MachineState::FlagIDs::Overflow, false);
		flags.set((int)MachineState::FlagIDs::AuxCarry, std::nullopt);
		state.setRegister(X86_REG_EFLAGS, flags);
		
		//Debug
		if (debug)
		{
			printf("   ; ") + op1.debugPrintValue(false) + printf(" ?= ") + op2.debugPrintValue(false) + printf(": ");
			
			printf("sf=%s ", sf.has_value() ? (sf.value() ? "Y" : "N") : "U");
			printf("zf=%s ", zf.has_value() ? (zf.value() ? "Y" : "N") : "U");
			printf("pf=%s ", pf.has_value() ? (pf.value() ? "Y" : "N") : "U");
		}

		return true;
	}
	else return false;
}

bool SemanticVM::step_math(MachineState& state, const cs_insn* insn, const std::function<void(const std::string&)>& reportError)
{
	if (insn->id == x86_insn::X86_INS_SUB)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1-op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_ADD)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1+op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_MUL)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = SemanticUnknown(op1.getSize());

		SemanticKnownConst* c1 = op1.tryGetKnownConst();
		SemanticKnownConst* c2 = op2.tryGetKnownConst();
		if (c1 && c2)
		{
			val = SemanticKnownConst(c1->bound()*c2->bound(), op1.getSize(), false);
		}

		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_IMUL)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = SemanticUnknown(op1.getSize());

		SemanticKnownConst* c1 = op1.tryGetKnownConst();
		SemanticKnownConst* c2 = op2.tryGetKnownConst();
		if (c1 && c2)
		{
			val = SemanticKnownConst(c1->asSigned()*c2->asSigned(), sizeof(uint64_t), false).signExtend(op1.getSize());
		}

		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_AND)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1&op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_OR)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = op1|op2;
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_XOR)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue val = !isOperandIdentity(insn, 0, 1) ? op1^op2 : SemanticKnownConst(0, op1.getSize(), false); //Special case: XOR with self is a common trick to produce a 0
		state.setOperand(insn, 0, val);
		//TODO adjust flags

		if (debug) { printf("   ; ") + debugPrintOperand(state, insn, 0) + printf(":= ") + val.debugPrintValue(false); }

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_DEC)
	{
		auto op = state.getOperand(insn, 0);
		state.setOperand(insn, 0, op-SemanticKnownConst(1, op.getSize(), false) );
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_INC)
	{
		auto op = state.getOperand(insn, 0);
		state.setOperand(insn, 0, op+SemanticKnownConst(1, op.getSize(), false) );
		return true;
	}
	else return false;
}

bool SemanticVM::step_bitmath(MachineState& state, const cs_insn* insn, const std::function<void(const std::string&)>& reportError)
{
	if (insn->id == x86_insn::X86_INS_SHL)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue result = SemanticUnknown(op1.getSize());
		SemanticKnownConst* shift = op2.tryGetKnownConst();
		if (shift)
		{
			if (SemanticKnownConst* cv = op1.tryGetKnownConst())
			{
				cv->value = cv->value << shift->value;
				result = *cv;
			}
			else if (SemanticFlags* fv = op1.tryGetFlags())
			{
				fv->bits      = fv->bits      << shift->value;
				fv->bitsKnown = fv->bitsKnown << shift->value;
				for (int i = 0; i < shift->value; ++i) fv->set(i, false); //These bits are known
				result = *fv;
			}
		}
		state.setOperand(insn, 0, result);
		if (debug) { printf("   ; := ") + result.debugPrintValue(false); }
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_SHR)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue result = SemanticUnknown(op1.getSize());
		SemanticKnownConst* shift = op2.tryGetKnownConst();
		if (shift)
		{
			if (SemanticKnownConst* cv = op1.tryGetKnownConst())
			{
				cv->value = cv->value >> shift->value;
				result = *cv;
			}
			else if (SemanticFlags* fv = op1.tryGetFlags())
			{
				fv->bits      = fv->bits      >> shift->value;
				fv->bitsKnown = fv->bitsKnown >> shift->value;
				result = *fv;
			}
		}
		state.setOperand(insn, 0, result);
		if (debug) { printf("   ; := ") + result.debugPrintValue(false); }
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_SAL)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue result = SemanticUnknown(op1.getSize());
		SemanticKnownConst* shift = op2.tryGetKnownConst();
		if (shift)
		{
			if (SemanticKnownConst* cv = op1.tryGetKnownConst())
			{
				bool sign = cv->isSigned();
				cv->setSign(false);
				cv->value = cv->value << shift->value;
				cv->setSign(sign);
				result = *cv;
			}
		}
		state.setOperand(insn, 0, result);
		if (debug) { printf("   ; := ") + result.debugPrintValue(false); }
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_SAR)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue result = SemanticUnknown(op1.getSize());
		SemanticKnownConst* shift = op2.tryGetKnownConst();
		if (shift)
		{
			if (SemanticKnownConst* cv = op1.tryGetKnownConst())
			{
				bool sign = cv->isSigned();
				cv->setSign(false);
				cv->value = cv->value >> shift->value;
				cv->setSign(sign);
				result = *cv;
			}
		}
		state.setOperand(insn, 0, result);
		if (debug) { printf("   ; := ") + result.debugPrintValue(false); }
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_ROL)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue result = SemanticUnknown(op1.getSize());
		SemanticKnownConst* shift = op2.tryGetKnownConst();
		if (shift)
		{
			if (SemanticKnownConst* cv = op1.tryGetKnownConst())
			{
				int bits = op1.getSize()*8;
				cv->value = (cv->value << (shift->value%bits))
					      | (cv->value >> (bits - shift->value%bits));
				result = *cv;
			}
		}
		state.setOperand(insn, 0, result);
		if (debug) { printf("   ; := ") + result.debugPrintValue(false); }
		return true;
	}
	else if (insn->id == x86_insn::X86_INS_ROR)
	{
		SemanticValue op1 = state.getOperand(insn, 0);
		SemanticValue op2 = state.getOperand(insn, 1);
		SemanticValue result = SemanticUnknown(op1.getSize());
		SemanticKnownConst* shift = op2.tryGetKnownConst();
		if (shift)
		{
			if (SemanticKnownConst* cv = op1.tryGetKnownConst())
			{
				int bits = op1.getSize()*8;
				cv->value = (cv->value >> (shift->value%bits))
					      | (cv->value << (bits - shift->value%bits));
				result = *cv;
			}
		}
		state.setOperand(insn, 0, result);
		if (debug) { printf("   ; := ") + result.debugPrintValue(false); }
		return true;
	}
	
	else return false;
}

bool SemanticVM::step_execflow(MachineState& state, const cs_insn* insn, const std::function<void(const std::string&)>& reportError, const std::function<void(void*)>& pushCallStack, const std::function<void* ()>& popCallStack, const std::function<void(void*)>& jump, const std::function<void(const std::vector<void*>&)>& fork)
{
	if (insn_in_group(*insn, cs_group_type::CS_GRP_CALL))
	{
		SemanticKnownConst fp  = *state.getOperand(insn, 0).tryGetKnownConst();
		state.pushStackFrame(fp);
		pushCallStack((uint8_t*)fp.value); //Sanity check. Also no ROP nonsense
		return true;
	}
	else if (insn_in_group(*insn, cs_group_type::CS_GRP_RET))
	{
		SemanticValue returnAddr = state.popStackFrame();

		//If given an operand, pop that many bytes from the stack
		if (insn->detail->x86.op_count == 1) state.stackPop(state.getOperand(insn, 0).tryGetKnownConst()->value);

		void* poppedReturnAddr = popCallStack();
		if (poppedReturnAddr && poppedReturnAddr != (void*)returnAddr.tryGetKnownConst()->value) reportError("Attempted to return, but return address did not match");

		return true;
	}
	else if (insn->id == x86_insn::X86_INS_NOP)
	{
		//Do nothing
		return true;
	}
	else if (insn_in_group(*insn, cs_group_type::CS_GRP_BRANCH_RELATIVE)) //TODO: Really hope nobody uses absolute branching. Is that a thing?
	{
		std::optional<bool> conditionMet = state.isConditionMet(insn->id);

		auto* ifBranch = state.getOperand(insn, 0).tryGetKnownConst();
		auto* noBranch = state.getRegister(x86_reg::X86_REG_RIP).tryGetKnownConst();
		if (!ifBranch) reportError("Attempted to branch to an indeterminate location!");
		if (!noBranch) reportError("Attempted to branch to an indeterminate location!");

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

			if (debug) printf("   ; Determinate: %s", conditionMet.value() ? "Jumping" : "Not jumping");
		}

		return true;
	}
	else if (insn_in_group(*insn, cs_group_type::CS_GRP_JUMP))
	{
		SemanticValue addr = state.getOperand(insn, 0);
		if (addr.getType() != SemanticValue::Type::KnownConst) reportError("Cannot jump to an unknown/indeterminate location");
		else jump((void*) addr.tryGetKnownConst()->value);
		return true;
	}
	else return false;
}

void SemanticVM::step_invalidate(MachineState& state, const cs_insn* insn)
{
	//Unhandled operation: Just invalidate all relevant register state
	cs_regs  regsRead,  regsWritten;
	uint8_t nRegsRead, nRegsWritten;
	cs_regs_access(capstone_get_instance(), insn,
		regsRead   , &nRegsRead,
		regsWritten, &nRegsWritten);
	for (int i = 0; i < nRegsWritten; ++i) state.setRegister((x86_reg)regsWritten[i], SemanticUnknown(sizeof(void*)) ); //TODO read correct size
}

#pragma endregion

void SemanticVM::execFunc(MachineState& state, void(*fn)(), const ExecutionOptions& opt)
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
	execFunc_internal(state, fn, nullptr, 0, opt);

	//Ensure output parity
	SemanticValue rsp = state.getRegister(X86_REG_RSP);
	SemanticKnownConst* const_rsp = rsp.tryGetKnownConst();
	assert(const_rsp && const_rsp->value == 0); //TODO handle return value
}
