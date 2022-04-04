#pragma once
#include "ModModule.h"

struct ModMetadata {
	const char* name;
	const char* author;
	const char* version;
};

struct ModHandle {
	ModMetadata* metadata;
	void* handle;
};

typedef ModMetadata* (*Ship_Init)();
typedef void* (*BindFunction)(void* ref);
typedef void* (*DumpFunction)();

#define SHIP_MAIN_FUNC extern "C" __declspec(dllexport) ModMetadata* MainFunc

namespace Ship {
	class CoreLoader : public ModModule {
	public:
		explicit CoreLoader(ModManager* Manager) : ModModule(Manager) {}
		static void Log(const char* message);
	private:
		void Init() override;
		void Exit() override;
		void Open(std::shared_ptr<Ship::Archive> archive) override;
		void Close(Ship::Archive mod) override;
	protected:
		std::vector<ModHandle> handles;
		void ScanModsFolder();
		void LoadMod(std::string path);
	};
}
