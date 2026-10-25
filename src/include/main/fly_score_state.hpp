#pragma once

#include <QString>
#include <QVector>
#include <QJsonObject>
#include <QByteArray>
#include <string>

class FlyScoreState {

public:
	struct FlyThemeManifest {
		QString title;
		QString author;
		QString authorUrl;
		QString description;
		QString version;
	};

	struct FlyThemeInfo {
		FlyThemeManifest manifest;
		QString path;
		QString folderName;
		QString error;
		bool hasIndex = false;
		bool hasManifest = false;
		bool valid = false;
	};

	struct FlyMatchInfo {
		QString competition;
		QString competitionRound;
		int matchNumber;
		QDateTime datetime;
		QString city;
		QString venue;
		int spectators;
	};

	struct FlyOfficial {
		QChar officialLetter;
		QString officialName;
		QString officialRole;
		long long warning_ms = 0;
		long long fst2minSusp_ms = 0;
		long long disqualification_ms = 0;
		long long blueCard_ms = 0;
	};

	struct FlyGoal {
		long long moment_ms;
		QString goalType;
	};

	struct FlyPlayer {
		int playerTshirtNumber = 0;
		QString playerTshirtName = "";
		QString playerName = "";
		long long warning_ms = 0;
		long long fst2minSusp_ms = 0;
		long long snd2minSusp_ms = 0;
		long long trd2minSusp_ms = 0;
		long long disqualification_ms = 0;
		long long blueCard_ms = 0;
		QVector<FlyGoal> goals;
	};

	struct FlyTeam {
		QString title;
		QString subtitle;
		QString logo;
		uint32_t color = 0xFFFFFF;
		QVector<FlyOfficial> officials;
		QVector<FlyPlayer> players;
		long long fstTimeout_ms = 0;
		long long sndTimeout_ms = 0;
		long long trdTimeout_ms = 0;
	};

	struct FlyTimer {
		QString label;
		QString mode;
		bool running = false;
		long long initial_ms = 0;
		long long duration_ms = 0;
		long long current_moment_ms = 0;
		long long last_tick_ms = 0;
		bool visible = true;
	};

	struct FlyStat {
		QString label;
		int first = 0;
		int second = 0;
		bool visible = true;
	};

	struct FlyState {
		FlyMatchInfo matchInfo;
		FlyTeam home;
		FlyTeam guest;

		bool swap_sides = false;
		bool show_scoreboard = true;

		QVector<FlyStat> stTeamsStats;
		QVector<FlyStat> stMatchStats;
		QVector<FlyTimer> timers;
	};

	static bool fly_state_read_json(const std::string &base_dir, std::string &out_json);
	static bool fly_state_write_json(const std::string &base_dir, const std::string &json);
	static QJsonObject fly_state_to_json_object(const FlyState &st);
	static bool fly_state_from_json_object(const QJsonObject &j, FlyState &st);
	static QByteArray fly_state_to_json_bytes(const FlyState &st, bool compact = true);
	static bool fly_state_load(const QString &base_dir, FlyState &out);
	static bool fly_state_save(const QString &base_dir, const FlyState &st);
	static QString fly_data_dir();
	static bool fly_ensure_webroot(QString *outBaseDir = nullptr);
	static FlyState fly_state_make_defaults();
	static bool fly_state_reset_defaults(const QString &base_dir);
	static bool fly_state_ensure_json_exists(const QString &base_dir, const FlyState *writeState = nullptr);

	static FlyThemeInfo fly_read_theme_info(const QString &themePath);
	static void setTimerStoppedAtInitial(FlyTimer &timer);
	static QString selectedTemplateName(QString dataDir);
	static bool fly_read_theme_manifest(const QString &themePath, FlyScoreState::FlyThemeManifest &out);
	static void fly_read_theme_manifest_text(const QString &manifestPath, FlyThemeManifest &out);
	static bool fly_is_valid_theme_folder(const QString &themePath, FlyThemeManifest *manifest = nullptr);
	static QString fly_normalize_manifest_key(QString key, const QString &section);
	static FlyScoreState::FlyTimer makeDefaultMainTimer();
	static FlyScoreState::FlyTimer makeDefaultTimeoutTimer();
	static void ensureDefaultMatchStats(FlyState &st);
	static QString moduleBaseDirFromConfigFile();
	static QString overlay_dir_path(const QString &base_dir);
	static QString overlay_plugin_json(const QString &base_dir);
	static void ensureDefaultTeamsStats(FlyState &st);
	static QJsonObject timerToJson(const FlyTimer &t);
	static FlyTimer timerFromJson(const QJsonObject &o);
	static bool write_one_json(const QString &path, const QJsonDocument &doc);
	static QString fly_theme_manifest_path(const QString &themePath);
};
