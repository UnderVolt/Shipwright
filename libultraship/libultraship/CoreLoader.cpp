#include "CoreLoader.h"
#include "GameSettings.h"
#include "SohImGuiImpl.h"
#include "filesystem"
#include "Lib/nlohmann/json.hpp"

// Thanks Microsoft for being non posix compliant
#if defined(_WIN32)
#include <Windows.h>
#define dlerror() ""
#define dlsym(handle, func) (void*) GetProcAddress(reinterpret_cast<HMODULE>(handle), func)
#define dlclose(handle) FreeLibrary(reinterpret_cast<HMODULE>(handle))
#define RTLD_LAZY 0

void* dlopen(const char* path, int flag) {
	char text[] = "something";
	size_t len = strlen(path);
	wchar_t* w_path = new wchar_t[len + 1];
	mbstowcs(w_path, path, len + 1);
	return LoadLibrary(w_path);
}

#define FILE_EXT ".dll"
#elif defined(__APPLE__)
#include <dlfcn.h>
#define FILE_EXT ".dylib"
#elif defined(__linux__) || defined(__FreeBSD__) // lets make the bold assumption for FreeBSD
#include <dlfcn.h>
#define FILE_EXT ".so"
#endif
#include "TextureMod.h"

namespace fs = std::filesystem;

void CPrint(const char* demo) {
	INFO(demo);
}

namespace Ship {

	void CoreLoader::Init() {
		this->ScanModsFolder();
	}

	void CoreLoader::Exit() {
		this->handles.clear();
	}

	void CoreLoader::ScanModsFolder() {
		const std::string path = "mods";

        for (const auto& entry : fs::directory_iterator(path)) {
            std::string file = entry.path().string();
            std::string ext = file.substr(file.find_last_of('.'));
            if (ext == ".otr") {
				this->LoadMod(file);
            }
        }
	}
	
	void CoreLoader::LoadMod(std::string path) {
		std::shared_ptr<Archive> otr = std::make_shared<Archive>(path, "", false);
		std::shared_ptr<File> raw_data = std::make_shared<File>();
		std::shared_ptr<File> raw_code_data = std::make_shared<File>();
		std::string metadata_file = "BarrelMod";

		if (!otr->HasFile(metadata_file)) {
			ERROR("Failed to load metadata info from: %s", path.c_str());
			return;
		}

		otr->LoadFile(metadata_file, false, raw_data);

		if (!raw_data->bIsLoaded) {
			ERROR("Failed to load metadata file");
			return;
		}

		auto tdata = new char[raw_data->dwBufferSize];
		memcpy(tdata, raw_data->buffer.get(), raw_data->dwBufferSize + 1);
		tdata[raw_data->dwBufferSize] = '\0';

		auto json = nlohmann::json::parse(tdata);

		if(json.is_discarded()) {
			ERROR("Failed to parse metadata file");
			return;
		}

		std::string dll_path = json["dll"];
		dll_path.append(FILE_EXT);

		ModMetadata metadata = {
			json.value("version", 0),
			json.value("oot_version", OOT_ANY_VERSION),
			json.value("mdv", ModVersion::VERSION_B0),
			json["main"],
			dll_path,
			json["name"],
			json.value("author", "Unknown"),
			json.value("description", ""),
		};
		

		if (!otr->HasFile(metadata.dll)) {
			ERROR("Failed to load dll from: %s", metadata.dll.c_str());
			return;
		}

		otr->LoadFile(metadata.dll, false, raw_code_data);

		if (!raw_code_data->bIsLoaded) {
			ERROR("Failed to load metadata file");
			return;
		}

		std::string tmp = (fs::temp_directory_path() / (metadata.name + FILE_EXT)).string();

		std::ofstream file;
		file.open(tmp, std::ios_base::binary);
		file.write(raw_code_data->buffer.get(), raw_code_data->dwBufferSize);
		file.close();

		void* handle = dlopen(tmp.c_str(), RTLD_LAZY);

		if(!handle) {
			ERROR("Failed to load mod: %s", path.c_str());
			return;
		}

		BindFunction bind_console = (BindFunction)dlsym(handle, "BindConsole");
		if (!bind_console) {
			ERROR("Failed to bind console!");
			return;
		}

		bind_console(&SohImGui::console);

		Ship_Init main_func = (Ship_Init) dlsym(handle, metadata.main.c_str());

		if(!main_func) {
			ERROR("Failed to call main function!");
			return;
		}

		main_func();

		DumpFunction dump_table = (DumpFunction)dlsym(handle, "DumpHookMGR");
		if (!dump_table) {
			ERROR("Failed to bind hook table!");
			return;
		}

		const HookTableMap& table = *(HookTableMap*)dump_table();

		for (auto& hook : table) {
			INFO("Hooking %d", hook.first);
			HookTableMap& table = const_cast<HookTableMap&>(Ship::getHookTableMap());
			table[hook.first] = hook.second;
		}

		INFO("Loaded mod: %s", path.c_str());
		INFO("Mod [%s] - v%d", metadata.name.c_str(), metadata.mod_version);
		this->handles.push_back({metadata, handle });
	}

	void CoreLoader::Log(const char* message) {
		INFO(message);
	}
	void CoreLoader::Open(std::shared_ptr<Archive> archive){}
	void CoreLoader::Close(Archive mod) {}

}
