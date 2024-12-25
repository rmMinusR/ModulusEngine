#pragma once

#include <filesystem>
#include <ReflectionSpec.hpp>
#include "dllapi.h"

class SerialNode;

class STIX_DISABLE_IMAGE_CAPTURE SerialFile
{
	SerialNode* rootNode;
public:
	ENGINEDATA_API SerialFile(const std::filesystem::path&);
	ENGINEDATA_API ~SerialFile();
	ENGINEDATA_API SerialNode const* getRootNode() const;
};
