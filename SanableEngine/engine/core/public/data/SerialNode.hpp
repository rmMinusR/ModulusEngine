#pragma once

#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <locale>
#include <cassert>
#include <variant>
#include <ReflectionSpec.hpp>

#include "dllapi.h"

enum class SerialFormat
{
	JSON,
	YAML
};

class STIX_DISABLE_IMAGE_CAPTURE SerialNode
{
protected:
	SerialNode() = default;

public:
	ENGINEDATA_API virtual ~SerialNode();

	ENGINEDATA_API static SerialNode* parse(std::wistream& in, SerialFormat format);

	ENGINEDATA_API static SerialNode* parseYaml(std::wistream& in, const wchar_t* indent = nullptr);
	ENGINEDATA_API static SerialNode* parseJson(std::wistream& in);
};

class SerialString : public SerialNode
{
public:
	typedef std::wstring contents_t;

private:
	contents_t contents;

public:
	ENGINEDATA_API SerialString(contents_t contents);
	ENGINEDATA_API virtual ~SerialString();
	ENGINEDATA_API static SerialString* parseJson(std::wistream& in);
};

class SerialNumber : public SerialNode
{
public:
	typedef std::variant<float, int> contents_t;

private:
	contents_t contents;

public:
	ENGINEDATA_API SerialNumber(contents_t contents);
	ENGINEDATA_API virtual ~SerialNumber();
	ENGINEDATA_API static SerialNumber* parseJson(std::wistream& in);
};

class SerialObject : public SerialNode
{
public:
	typedef std::unordered_map<std::wstring, SerialNode*> contents_t;

private:
	contents_t contents;

public:
	ENGINEDATA_API SerialObject(contents_t contents);
	ENGINEDATA_API virtual ~SerialObject();
	ENGINEDATA_API static SerialObject* parseJson(std::wistream& in);
};

class SerialArray : public SerialNode
{
public:
	typedef std::vector<SerialNode*> contents_t;
	
private:
	contents_t contents;

public:
	ENGINEDATA_API SerialArray(contents_t contents);
	ENGINEDATA_API virtual ~SerialArray();
	ENGINEDATA_API static SerialArray* parseJson(std::wistream& in);
};
