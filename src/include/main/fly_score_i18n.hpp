#pragma once

#define LOG_TAG "[" PLUGIN_NAME "][i18n]"

#include <obs-module.h>
#include "fly_score_log.hpp"

#include <QString>

inline QString fly_i18n(const char *key)
{
	const char * s = new char[100];
	if (!obs_module_get_string(key, &s))
		LOGI("String [%s] is not FOUMDED!!!", key);
	return QString::fromUtf8(obs_module_text(key));
}

