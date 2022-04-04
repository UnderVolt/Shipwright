#include "CoreLoader.h"
#include "GameSettings.h"
#include "SohImGuiImpl.h"
#include "filesystem"

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
            if (ext == FILE_EXT) {
				this->LoadMod(file);
            }
        }
	}
	
	void CoreLoader::LoadMod(std::string path) {
		void* handle = dlopen(path.c_str(), RTLD_LAZY);

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

		Ship_Init main_func = (Ship_Init) dlsym(handle, "MainFunc");

		if(!main_func) {
			ERROR("Failed to call handle!");
			return;
		}

		ModMetadata* metadata = main_func();
		if(metadata != nullptr) {
			INFO("Loaded mod: %s", path.c_str());
			INFO("Mod [%s] - %s", metadata->name, metadata->version);
			this->handles.emplace_back(metadata, handle);
		}

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
	}

	void CoreLoader::Log(const char* message) {
		INFO(message);
	}
	void CoreLoader::Open(std::shared_ptr<Archive> archive){}
	void CoreLoader::Close(Archive mod) {}

}
