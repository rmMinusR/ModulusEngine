#pragma once

#include <vector>

#include "dllapi.h"


struct DetectedConstants
{
	std::vector<uint8_t> bytes;
	std::vector<bool> usage;

	STIX_API DetectedConstants(size_t sz);
	STIX_API void resize(size_t sz);

	//Keeps our detected constants only if our counterpart has also
	//detected the same constant. Used for vtable detection.
	STIX_API void merge(const DetectedConstants& other);

	STIX_API DetectedConstants();
	STIX_API ~DetectedConstants();
	STIX_API DetectedConstants(const DetectedConstants& cpy);
	STIX_API DetectedConstants(DetectedConstants&& mov);
	STIX_API DetectedConstants& operator=(const DetectedConstants& cpy);
	STIX_API DetectedConstants& operator=(DetectedConstants&& mov);
};
