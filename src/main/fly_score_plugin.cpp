#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][plugin]"
#include "fly_score_log.hpp"

#include <obs-module.h>
#include <obs-frontend-api.h>

#include <obs.h>

#include <QString>
#include <QDir>

#include "fly_score_paths.hpp"
#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_const.hpp"
#include "fly_score_i18n.hpp"
#include "fly_score_plugin.hpp"

OBS_DECLARE_MODULE();
OBS_MODULE_USE_DEFAULT_LOCALE(PLUGIN_NAME, "en-US")

MODULE_EXPORT const char *obs_module_name(void)
{
	return PLUGIN_NAME;
}

MODULE_EXPORT const char *obs_module_description(void)
{
	return obs_module_text("Plugin.Description");
}

static FlyScorePlugin* g_flyPlugin = nullptr;

// Конструктор на класа
FlyScorePlugin::FlyScorePlugin() {
    LOGI("Create plugin");
}

// Деструктор на класа (почистване на паметта)
FlyScorePlugin::~FlyScorePlugin() {
	if (!dock)
		return;

	obs_frontend_remove_dock(fly_dock_name().toUtf8());

	dock = nullptr;

	LOGI("Destroy plugin");
}

bool FlyScorePlugin::init() {

	if (dock)
		return false;

	auto *flyDock = new FlyScoreDock(nullptr);
	flyDock->init();

    obs_frontend_add_dock_by_id(fly_dock_name().toUtf8(), fly_i18n("Common.Scoreboard").toUtf8(), flyDock);


	dock = flyDock;
	LOGI("Fly Scoreboard dock created");

	return true;
}

bool obs_module_load(void)
{
	LOGI("Plugin loaded (version %s)", PLUGIN_VERSION);

    g_flyPlugin = new FlyScorePlugin();

    if (!g_flyPlugin->init()) {
        delete g_flyPlugin;
        g_flyPlugin = nullptr;
        return false;
    }

	return true;
}

void obs_module_unload(void)
{
	LOGI("Plugin unloading...");
    if (g_flyPlugin) {
        delete g_flyPlugin;
        g_flyPlugin = nullptr;
    }
	LOGI("Plugin unloaded");
}
