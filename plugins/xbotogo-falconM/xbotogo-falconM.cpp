#include "falconm.hpp"

#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("xbotogo-falconM", "en-US")
bool obs_module_load(void)
{
	obs_register_source(&xbotgo::falconm_source_info);
	xbotgo::falconm_scene_fitting_init();
	return true;
}

void obs_module_unload(void)
{
	xbotgo::falconm_scene_fitting_shutdown();
}
