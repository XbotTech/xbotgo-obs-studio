#include <obs-module.h>
namespace xbotgo {
extern obs_source_info falconm_source_info;
}
OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("xbotogo-falconM", "en-US")
bool obs_module_load(void)
{
	obs_register_source(&xbotgo::falconm_source_info);
	return true;
}
