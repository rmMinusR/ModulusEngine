#include "CapstoneWrapper.hpp"

#include <cassert>

#include "SemanticValue.hpp"
#include "MachineState.hpp"

csh capstone_instance;

void capstone_check_error(cs_err code)
{
	if (code != CS_ERR_OK)
	{
		printf("Error with Capstone: %s\n", cs_strerror(code));
		assert(false);
	}
}

csh capstone_get_instance()
{
	assert(cs_support(cs_arch::CS_ARCH_OURS));

	if (!capstone_instance)
	{
		cs_err status = cs_open(
			cs_arch::CS_ARCH_OURS,
			sizeof(void*) == 8 ? cs_mode::CS_MODE_64 : cs_mode::CS_MODE_32,
			&capstone_instance
		);
		capstone_check_error(status);

		status = cs_option(capstone_instance, cs_opt_type::CS_OPT_DETAIL, cs_opt_value::CS_OPT_ON);
		capstone_check_error(status);
	}

	return capstone_instance;
}

void capstone_cleanup_instance()
{
	if (capstone_instance)
	{
		cs_err status = cs_close(&capstone_instance);
		capstone_check_error(status);
	}
}


bool insn_in_group(const cs_insn& insn, uint8_t group)
{
	return carray_contains(insn.detail->groups, insn.detail->groups_count, group);
}


int printInstructionCursor(const cs_insn* insn)
{
	int bytesWritten = 0;
	//for (int i = 0; i < callLevel   ; ++i) bytesWritten += printf(" |  "); //Indent
	                                       bytesWritten += printf("%p: ", (void*)insn->address); //Write address
	//for (int i = 0; i < insn->size  ; ++i) bytesWritten += printf(" %02x", insn->bytes[i]); //Write raw bytes
	//for (int i = 0; i < 8-insn->size; ++i) bytesWritten += printf("   "); //Pad
	                                       bytesWritten += printf("%s %s", insn->mnemonic, insn->op_str); //Write disassembly
	return bytesWritten;
}

void foreachSubFunction(void(*fn)(), const std::function<void( void(*)() )>& visitor)
{
	cs_insn* insn = cs_malloc(capstone_get_instance());
	
	const uint8_t* cursor = (uint8_t*)fn;
	uint_addr_t addr = (uint_addr_t)cursor;

	while (true)
	{
		//Disassemble one instruction at a time
		size_t allowedToProcess = sizeof(cs_insn::bytes);
		bool disassemblyGood = cs_disasm_iter(capstone_get_instance(), &cursor, &allowedToProcess, &addr, insn);
		assert(disassemblyGood && "An internal error occurred with the Capstone disassembler.");

		if (carray_contains(insn->detail->groups, insn->detail->groups_count, cs_group_type::CS_GRP_CALL))
		{
			MachineState dummyState(true);
			dummyState.setInsnPtr(addr);
			SemanticValue tgt = dummyState.getOperand(insn, 0);
			if (SemanticKnownConst* k = tgt.tryGetKnownConst()) visitor( (void(*)())(uint_addr_t)k->bound() );
		}
		else if (carray_contains(insn->detail->groups, insn->detail->groups_count, cs_group_type::CS_GRP_BRANCH_RELATIVE))
		{
			const void* branchTarget = nullptr;
			if (!platform_isIf(*insn) && (branchTarget = platform_getJumpTarget(*insn)) && branchTarget >= cursor)
			{
				cursor = (const uint8_t*)branchTarget;
				addr = (uint_addr_t)branchTarget;
			}
			else
			{
				assert(false && "Cannot determine subfunction: caller is non-linear");
				break;
			}
		}
		else if (carray_contains(insn->detail->groups, insn->detail->groups_count, cs_group_type::CS_GRP_RET))
		{
			//Returned. Cannot process further.
			break;
		}
	}

	cs_free(insn, 1);
}

void* getSubFunction(void(*fn)(), int index)
{
	void* out = nullptr;
	foreachSubFunction(fn,
		[&](void(*i)())
		{
			if (index == 0) out = (void*)i;
			--index;
		}
	);
	return out;
}

void* getLastSubFunction(void(*fn)(), size_t index)
{
	void* out[index+1];
	memset(out, 0, (index+1)*sizeof(void*));
	foreachSubFunction(fn,
		[&](void(*i)())
		{
			for(int i = 0; i < index; ++i) out[i] = out[i+1];
			out[index] = (void*)i;
		}
	);
	return out[0];
}
