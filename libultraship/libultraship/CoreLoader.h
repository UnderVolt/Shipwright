#pragma once
#include "ModModule.h"
#include "GameVersions.h"

#define OOT_ANY_VERSION 0xFFFFFFFF

enum ModVersion {
	UNK, VERSION_B0
};

struct ModMetadata {
	int mod_version;
	uint32_t game_version;
	ModVersion int_version;
	std::string main;
	std::string dll;
	std::string name;
	std::string author;
	std::string description;
};

struct ModHandle {
	ModMetadata metadata;
	void* handle;
};

typedef const ModMetadata (*Barrel_MData)();
typedef void  (*Ship_Init)();
typedef void* (*BindFunction)(void* ref);
typedef void* (*DumpFunction)();

#define EXPORT_FUNC extern "C" __declspec(dllexport)

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
