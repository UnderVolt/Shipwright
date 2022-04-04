#include "ModManager.h"

#include "CoreLoader.h"
#include "ModModule.h"
#include "SohImGuiImpl.h"

namespace Ship {
	std::vector<ModModule*> modules;

	void ModManager::Init() {
		modules.push_back(new CoreLoader(this));
		// ResManager->GetArchive()->loa
		// std::shared_ptr<Ship::Archive> archive = std::make_shared<Ship::Archive>("mods/TexMods.otr", "", false);
		// Ship::registerHookListener({ RMGR_LOAD_DLIST, [](const HookEvent event) {
		// 	INFO("Loading Texture Modules");
		// }});
		for (auto& mod : modules) {
			mod->Init();
		}
	}

	void ModManager::Exit() {
		for (auto& mod : modules)
			mod->Exit();
	}
}