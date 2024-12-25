#include "CapstoneWrapper.hpp"

#include <cassert>

#include "ThunkUtils.hpp"
#include "MachineState.hpp"

void* platform_getRelAddr(const cs_insn& insn)
{
	return reinterpret_cast<void*>((uint_addr_t)X86_REL_ADDR(insn));
}

void_func_ptr platform_getRelFunc(const cs_insn& insn)
{
	return reinterpret_cast<void(*)()>((uint_addr_t)X86_REL_ADDR(insn));
}

bool platform_isIf(const cs_insn& insn)
{
	return !insn_in_group(insn, cs_group_type::CS_GRP_CALL)
		&& insn.id != X86_INS_JMP //Unconditional JMP isn't a branchable
		&& (
			insn_in_group(insn, cs_group_type::CS_GRP_BRANCH_RELATIVE)
			|| insn_in_group(insn, cs_group_type::CS_GRP_JUMP)
		);
}

bool platform_isInterrupt(const cs_insn& insn)
{
	return insn.id == x86_insn::X86_INS_INT3 || insn.id == x86_insn::X86_INS_INT || insn.id == x86_insn::X86_INS_INTO || insn.id == x86_insn::X86_INS_INT1;
}

void* platform_getJumpTarget(const cs_insn& insn)
{
	if (insn.id == x86_insn::X86_INS_LEA || insn_in_group(insn, x86_insn_group::X86_GRP_BRANCH_RELATIVE))
	{
		return platform_getRelAddr(insn);
	}
	else if (insn.detail->x86.operands[0].type == x86_op_type::X86_OP_IMM) // CALL, JMP, JCC
	{
		return (void*)insn.detail->x86.operands[0].imm;
	}
	else
	{
		return nullptr;
	}
}

void* unwrapAliaFunction(void(*fn)())
{
	cs_insn* insn = cs_malloc(capstone_get_instance());

	//Disassemble one instruction
	const uint8_t* cursor = (uint8_t*)fn;
	uint_addr_t addr = (uint_addr_t)cursor;
	size_t allowedToProcess = sizeof(cs_insn::bytes);
	bool disassemblyGood = cs_disasm_iter(capstone_get_instance(), &cursor, &allowedToProcess, &addr, insn);
	assert(disassemblyGood && "An internal error occurred with the Capstone disassembler.");

	if (insn->id == X86_INS_JMP || insn->id == X86_INS_LJMP) //Unconditional, indirect jump. TODO this might not work outside of Clang/MSVC
	{
		MachineState dummyState(true);
		dummyState.setInsnPtr(addr);
		SemanticValue tgt = dummyState.getOperand(insn, 0);
		if (SemanticKnownConst* k = tgt.tryGetKnownConst()) fn = (void(*)())(uint_addr_t)k->bound();
	}

	cs_free(insn, 1);
	return (void*)fn;
}
