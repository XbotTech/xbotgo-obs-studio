#include "falconm.hpp"
#include "runtime/xbotgo-plugin-runtime.hpp"

#include <obs-module.h>
#include <memory>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("xbotogo-falconM", "en-US")

namespace {
std::unique_ptr<xbotgo::XBotGoPluginRuntime> xbotgoRuntime;
}

bool obs_module_load(void)
{
	obs_register_source(&xbotgo::falconm_source_info);
	xbotgo::falconm_scene_fitting_init();
	xbotgoRuntime = std::make_unique<xbotgo::XBotGoPluginRuntime>();
	xbotgoRuntime->requestInitialize();
	return true;
}

void obs_module_unload(void)
{
	if (xbotgoRuntime) {
		xbotgoRuntime->shutdown();
		xbotgoRuntime.reset();
	}
	xbotgo::falconm_scene_fitting_shutdown();
}
