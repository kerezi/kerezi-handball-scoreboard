#pragma once

inline constexpr const char *kFlyDockId = "FlyScoreDock";
inline constexpr const char *kFlyDockTitle = "Fly Score";

static inline QString fly_settings_org_name()
{
	return QStringLiteral("MMLTech");
}
static inline QString fly_settings_app_name()
{
	return QStringLiteral("fly-scoreboard");
}
static inline QString fly_settings_key_websocket_port()
{
	return QStringLiteral("websocket/port");
}

static inline QString fly_settings_key_event_timestamp_mode()
{
	return QStringLiteral("events/timestamp_mode");
}