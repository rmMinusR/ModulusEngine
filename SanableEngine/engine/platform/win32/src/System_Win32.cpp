#include "System_Win32.hpp"

//Memory leak tracing
#define _CRTDBG_MAP_ALLOC
#include <stdlib.h>
#include <crtdbg.h>

#include <cassert>
#include <sstream>
#include <iostream>
#include <chrono>
#include <thread>

#include "application/Application.hpp"

gpr460::System_Win32::System_Win32()
{
	consolePsuedofile = nullptr;
	logFile = nullptr;
#if _DEBUG
	memset(&checkpoint, 0, sizeof(_CrtMemState));
#endif
}

gpr460::System_Win32::~System_Win32()
{
	assert(!isAlive);
}

void gpr460::System_Win32::Init(Application* engine)
{
	System::Init(engine);

	assert(!isAlive);
	isAlive = true;

	//Log memory leaks
#ifdef _DEBUG
//	_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
//	_CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_DEBUG);
	_CrtMemCheckpoint(&checkpoint);
#endif
	
	//Create console and redirect
	if (!AllocConsole()) System::ShowErrorF(L"Failed to allocate console: Code %i", GetLastError());
	freopen_s(&consolePsuedofile, "CONOUT$", "w", stdout);
	if (!consolePsuedofile) ShowError(L"Failed to redirect console output");
}

void gpr460::System_Win32::DoMainLoop()
{
	while (true)
	{
		std::chrono::time_point frameStart = std::chrono::steady_clock::now();
		
		engine->frameStep(engine);
		if (engine->quit) break;

		std::chrono::time_point frameEnd = std::chrono::steady_clock::now();

		std::chrono::duration<float> elapsed = frameEnd - frameStart;
		std::chrono::duration<float> targetTimePerFrame = std::chrono::duration<float>{ 1 } / targetFps;
		if (elapsed < targetTimePerFrame) Sleep(std::chrono::duration_cast<std::chrono::milliseconds>(targetTimePerFrame - elapsed).count());
	}
}

void gpr460::System_Win32::Shutdown()
{
	assert(isAlive);
	isAlive = false;

	//Close console redirection
	fclose(consolePsuedofile);
	consolePsuedofile = nullptr;

	if (logFile)
	{
		SetEndOfFile(logFile);
		CloseHandle(logFile);
		logFile = nullptr;
	}

	//_CrtDumpMemoryLeaks();
	_CrtMemDumpAllObjectsSince(&checkpoint);
}

void gpr460::System_Win32::DebugPause()
{
#ifdef _DEBUG
	//Pause so we can read console
	WriteConsoleW(logFile, L"\nDEBUG Paused, type any key in console to continue\n", 52, nullptr, nullptr);
	std::cin.get();
#endif
}

void gpr460::System_Win32::ShowError(const std::wstring& message)
{
	MessageBoxW(NULL, message.c_str(), L"Error", MB_OK | MB_ICONSTOP);
}

void gpr460::System_Win32::LogToErrorFile(const std::wstring& message)
{
	//Lazy init logfile
	if (!logFile)
	{
		//TODO does this need FILE_FLAG_NO_BUFFERING or FILE_FLAG_WRITE_THROUGH?
		logFile = CreateFileW(logFileName.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

		if (!logFile || (GetLastError() != 0 && GetLastError() != ERROR_ALREADY_EXISTS)) System::ShowErrorF(L"Failed to create error file (code %i)", GetLastError());
	}

	DWORD nWritten;
	if (!WriteFile(logFile, message.c_str(), (DWORD)message.length(), &nWritten, NULL))
	{
		System::ShowErrorF(L"Error code %i while logging to file", GetLastError());
	}

	if (nWritten < message.length())
	{
		System::ShowErrorF(L"Tried to write %u characters to error file, but only wrote %u", message.length(), nWritten);
	}
}

std::vector<std::filesystem::path> gpr460::System_Win32::ListPlugins(std::filesystem::path path) const
{
	std::vector<std::filesystem::path> contents;

	if (!std::filesystem::exists(path))
	{
		printf("Plugins folder does not exist, creating\n");
		std::filesystem::create_directory(path);
	}

	for (const std::filesystem::path& entry : std::filesystem::directory_iterator(path))
	{
		std::ostringstream joiner;
		joiner << entry.filename().string() << ".dll"; //Build DLL name
		contents.push_back(entry / joiner.str());
	}

	return contents;
}

std::filesystem::path gpr460::System_Win32::GetBaseDir() const
{
	//Get host assembly
	constexpr size_t bufSz = 1024;
	wchar_t buf[bufSz];
	memset(buf, 0, sizeof(wchar_t)*bufSz);
	DWORD written = GetModuleFileNameW(NULL, buf, bufSz);
	assert(written != bufSz);

	//Search for last backslash
	long found;
	for (found = (long)written; found >= 0; --found)
	{
		if (buf[found] == L'\\') break;
	}
	assert(found != -1);

	//Substring
	buf[found] = L'\0';

	return std::wstring(buf);
}
