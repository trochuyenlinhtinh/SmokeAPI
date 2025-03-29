#include <smoke_api/smoke_api.hpp>
#include <build_config.h>
#include <smoke_api/config.hpp>
#include <core/globals.hpp>
#include <core/paths.hpp>
#include <common/steamclient_exports.hpp>
#include <koalabox/globals.hpp>
#include <koalabox/logger.hpp>
#include <koalabox/hook.hpp>
#include <koalabox/loader.hpp>
#include <koalabox/win_util.hpp>
#include <koalabox/util.hpp>

#if COMPILE_STORE_MODE
#include <store_mode/store.hpp>
#endif

// Hooking steam_api has shown itself to be less desirable than steamclient
// for the reasons outlined below:
//
// Calling original in flat functions will actually call the hooked functions
// because the original function redirects the execution to a function taken
// from self pointer, which would have been hooked by SteamInternal_*Interface
// functions.
//
// Furthermore, turns out that many flat functions share the same body,
// which looks like the following snippet:
//
//   mov rax, qword ptr ds:[rcx]
//   jmp qword ptr ds:[rax+immediate]
//
// This means that we end up inadvertently hooking unintended functions.
// Given that hooking steam_api has no apparent benefits, but has inherent flaws,
// the support for it has been dropped from this project.

void init_proxy_mode() {
    LOG_INFO("🔀 Detected proxy mode")

    globals::steamapi_module = koalabox::loader::load_original_library(paths::get_self_path(), STEAMAPI_DLL);
}

namespace smoke_api {

    void init(HMODULE module_handle) {
        try {
            DisableThreadLibraryCalls(module_handle);

            koalabox::globals::init_globals(module_handle, PROJECT_NAME);

            globals::smokeapi_handle = module_handle;

            config::init_config();

            // This kind of timestamp is reliable only for CI builds, as it will reflect the compilation
            // time stamp only when this file gets recompiled.
            LOG_INFO("🐨 {} v{} | Compiled at '{}'", PROJECT_NAME, PROJECT_VERSION, __TIMESTAMP__)

            const auto exe_path = Path(koalabox::win_util::get_module_file_name_or_throw(nullptr));
            const auto exe_name = exe_path.filename().string();

            LOG_DEBUG("Process name: '{}' [{}-bit]", exe_name, BITNESS)

            init_proxy_mode();

            LOG_INFO("🚀 Initialization complete")
        } catch (const Exception& ex) {
            koalabox::util::panic(fmt::format("Initialization error: {}", ex.what()));
        }
    }

    void shutdown() {
        try {
            if (globals::steamapi_module != nullptr) {
                koalabox::win_util::free_library(globals::steamapi_module);
            }

            LOG_INFO("💀 Shutdown complete")
        } catch (const Exception& ex) {
            LOG_ERROR("Shutdown error: {}", ex.what())
        }
    }

}
