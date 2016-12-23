#include <obs-module.h>

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("frontend-tools", "en-US")

#include "obs-frontend-api.h"

#if defined(_WIN32) || defined(__APPLE__)
void InitSceneSwitcher();
void FreeSceneSwitcher();
#endif
void InitOutputTimer();
void FreeOutputTimer();

bool obs_module_load(void)
{
	if (obs_frontend_get_main_window() == NULL) {
		// we have no frontend. this module is not needed.
		return false;
	}

#if defined(_WIN32) || defined(__APPLE__)
	InitSceneSwitcher();
#endif
	InitOutputTimer();
	return true;
}

void obs_module_unload(void)
{
#if defined(_WIN32) || defined(__APPLE__)
	FreeSceneSwitcher();
#endif
	FreeOutputTimer();
}
