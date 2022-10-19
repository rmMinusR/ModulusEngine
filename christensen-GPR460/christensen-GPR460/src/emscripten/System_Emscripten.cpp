#include "System_Emscripten.hpp"

#ifndef _WIN32

#include <cassert>

gpr460::System_Emscripten::System_Emscripten()
{
}

gpr460::System_Emscripten::~System_Emscripten()
{
	assert(!isAlive);
}

void gpr460::System_Emscripten::Init()
{
	assert(!isAlive);
	isAlive = true;
}

void gpr460::System_Emscripten::Shutdown()
{
	assert(isAlive);
	isAlive = false;
}

//extern "C" void jsnative_alert(const wchar_t* message);

void gpr460::System_Emscripten::ShowError(const gpr460::string& message)
{
	//jsnative_alert(message.c_str());

	//constexpr size_t bufferSize = 256;
	//wchar_t buffer[bufferSize];
	//memset(buffer, '\0', bufferSize);
	//int nValid = swprintf_s(buffer, bufferSize, L"alert($%s)", message.c_str());
	
	//emscripten_run_script(buffer);
	//EM_ASM({
	//	alert($0);
	//}, message);
}

//extern "C" void jsnative_console_error(const wchar_t* message);

void gpr460::System_Emscripten::LogToErrorFile(const gpr460::string& message)
{
	//jsnative_console_error(message.c_str());

	//EM_ASM({
	//	console.log($0);
	//}, message);
}

#endif _WIN32