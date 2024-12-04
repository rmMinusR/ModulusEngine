#include "ThunkUtils.hpp"

#include <cassert>

#include "CapstoneWrapper.hpp"
#include "FunctionBytecodeWalker.hpp"
#include "SemanticVM.hpp"

/*
ptrdiff_t _captureCastOffset(const DetectedConstants& image, void(*castThunk)())
{
	MachineState canonicalState(true);
	SemanticMagic _this; //TODO
	for (int i = 0; i < image.bytes.size(); ++i)
	{
		if (image.usage[i]) canonicalState.setMemory(_this, SemanticKnownConst(image.bytes[i], 1, false), 1);
	}

	//Simulate
	SemanticVM::execFunc(canonicalState, castThunk, {}, {});

	assert(false); //TODO implement
	return ptrdiff_t();
}
*/

DetectedConstants _captureVtablesInternal(size_t objSize, void(*thunk)(), size_t destructorCallCount, const std::vector<void(*)()>& allocators, const std::vector<void(*)()>& nofill)
{
	//Setup
	SemanticVM::ExecutionOptions options;
	options.canReadHostMemory = true;
	options.allocators = allocators;
	options.continueOnError = true;

	//Only allow constructor and thunk to run (GCC: full-constructor, which has vtables. We don't care about base constructor.)
	options.isSandboxAllowList = true;
	options.sandboxed = { thunk, (void(*)())getLastSubFunction(thunk, destructorCallCount) };

	//Unwrap aliases
	for (auto i : allocators) { auto unwrapped = unwrapAliaFunction(i); if (unwrapped != i) options.allocators.push_back(i); }
	
	//Simulate
	MachineState canonicalState(true);
	SemanticVM().execFunc(canonicalState, thunk, options);

	//Read from state.thisMemory into DetectedConstants
	DetectedConstants out(objSize);
	for (size_t i = 0; i < objSize; ++i)
	{
		SemanticValue _byte = canonicalState.getMemory(SemanticMagic(1, i, 0), 1); //Allocation for "this" object will be first, so magic ID=0
		auto* byte = _byte.tryGetKnownConst();
		if (byte && byte->isPositionIndependentAddr)
		{
			out.usage[i] = true;
			out.bytes[i] = byte->value;
		}
	}
	return out;
}
