#pragma once

#include "Aliases.hpp"

#include <vector>
#include <filesystem>

class EngineCore;
class PluginManager;

namespace gpr460
{

	class System
	{
	protected:
		bool isAlive = false;
		EngineCore* engine;

		friend class EngineCore;
		virtual void Init(EngineCore*);
		virtual void DoMainLoop() = 0;
		virtual void Shutdown() = 0;

		friend class PluginManager;
		virtual std::vector<std::filesystem::path> ListPlugins(std::filesystem::path path) const = 0;

	public:
		System() = default;
		virtual ~System() = default;

		//TODO these should be combined
		virtual void ShowError(const gpr460::string& message) = 0;
		virtual void LogToErrorFile(const gpr460::string& message) = 0;

		template<typename... Ts>
		void ShowError(const gpr460::string& format, const Ts&... args)
		{
			constexpr size_t bufferSize = 256;
			wchar_t buffer[bufferSize];
			int nValid = swprintf_s(buffer, bufferSize, format.c_str(), args...);

			ShowError(gpr460::string(buffer, nValid));
		}

		virtual std::filesystem::path GetBaseDir() const = 0;
	};
}