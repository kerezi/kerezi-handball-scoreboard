#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][state]"
#include "fly_score_log.hpp"

#include "fly_score_i18n.hpp"
#include "fly_score_state.hpp"

#include <obs-module.h>
#include <util/platform.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <QSettings>

QString FlyScoreState::moduleBaseDirFromConfigFile()
{
	char *p = obs_module_config_path("plugin.json");
	QString filePath = p ? QString::fromUtf8(p) : QString();
	if (p)
		bfree(p);
	return filePath.isEmpty() ? QDir::homePath() : QFileInfo(filePath).absolutePath();
}

QString FlyScoreState::overlay_dir_path(const QString &base_dir)
{
	return QDir(base_dir).absolutePath();
}

QString FlyScoreState::overlay_plugin_json(const QString &base_dir)
{
	return QDir(overlay_dir_path(base_dir)).filePath(QStringLiteral("plugin.json"));
}

QString FlyScoreState::fly_data_dir()
{
	return QDir::cleanPath(FlyScoreState::moduleBaseDirFromConfigFile());
}

bool FlyScoreState::fly_ensure_webroot(QString *outBaseDir)
{
	const QString base = FlyScoreState::fly_data_dir();
	const QString webroot = FlyScoreState::overlay_dir_path(base);
	QDir().mkpath(webroot);
	if (outBaseDir)
		*outBaseDir = base;
	return true;
}

FlyScoreState::FlyTimer FlyScoreState::makeDefaultMainTimer()
{
	FlyScoreState::FlyTimer t;
	t.label = fly_i18n("Default.Timer.HalfTime");
	t.mode = QStringLiteral("countup");
	t.running = false;
	t.initial_ms = 0;
	t.duration_ms = 1800000;
	t.current_moment_ms = 0;
	t.last_tick_ms = 0;
	t.visible = true;
	return t;
}

FlyScoreState::FlyTimer FlyScoreState::makeDefaultTimeoutTimer()
{
	FlyScoreState::FlyTimer t;
	t.label = fly_i18n("Default.Timer.Timeout");
	t.mode = QStringLiteral("countdown");
	t.running = false;
	t.initial_ms = 60000;
	t.duration_ms = 60000;
	t.current_moment_ms = 60000;
	t.last_tick_ms = 0;
	t.visible = false;
	return t;
}

void FlyScoreState::ensureDefaultTeamsStats(FlyScoreState::FlyState &st)
{
	if (st.stTeamsStats.isEmpty()) {
		const QStringList labels = {
			fly_i18n("Default.TeamStat.Score"),       fly_i18n("Default.TeamStat.Fouls"),
			fly_i18n("Default.TeamStat.YellowCards"), fly_i18n("Default.TeamStat.RedCards"),
			fly_i18n("Default.TeamStat.Corners"),
		};
		for (const QString &label : labels) {
			FlyScoreState::FlyStat stat;
			stat.label = label;
			stat.visible = true;
			st.stTeamsStats.push_back(stat);
		}
		return;
	}

	if (st.stTeamsStats[0].label.isEmpty())
		st.stTeamsStats[0].label = fly_i18n("Default.TeamStat.Score");
}

void FlyScoreState::ensureDefaultMatchStats(FlyScoreState::FlyState &st)
{
	if (st.stMatchStats.isEmpty()) {
		FlyScoreState::FlyStat ss;
		ss.label = fly_i18n("Default.MatchStat.Period");
		ss.first = 0;
		ss.visible = true;
		st.stMatchStats.push_back(ss);
		return;
	}

	if (st.stMatchStats[0].label.isEmpty())
		st.stMatchStats[0].label = fly_i18n("Default.MatchStat.Period");
}

QJsonObject FlyScoreState::timerToJson(const FlyScoreState::FlyTimer &t)
{
	QJsonObject o;
	o["label"] = t.label;
	o["mode"] = t.mode;
	o["running"] = t.running;
	o["initial_ms"] = QString::number(t.initial_ms);
	o["duration_ms"] = QString::number(t.duration_ms);
	o["current_moment_ms"] = QString::number(t.current_moment_ms);
	o["last_tick_ms"] = QString::number(t.last_tick_ms);
	o["visible"] = t.visible;
	return o;
}

FlyScoreState::FlyTimer FlyScoreState::timerFromJson(const QJsonObject &o)
{
	FlyScoreState::FlyTimer t;
	t.label = o.value("label").toString();
	t.mode = o.value("mode").toString("countdown");
	t.running = o.value("running").toBool(false);
	t.initial_ms = o.value("initial_ms").toString("0").toLongLong();
	t.duration_ms = o.value("duration_ms").toString("0").toLongLong();
	t.current_moment_ms = o.value("current_moment_ms").toString("0").toLongLong();
	t.last_tick_ms = o.value("last_tick_ms").toString("0").toLongLong();
	t.visible = o.value("visible").toBool(true);
	return t;
}

QJsonObject FlyScoreState::fly_state_to_json_object(const FlyScoreState::FlyState &stIn)
{
	FlyScoreState::FlyState st = stIn;

	FlyScoreState::ensureDefaultTeamsStats(st);
	FlyScoreState::ensureDefaultMatchStats(st);

	QJsonObject j;
	j["version"] = 5;

	auto matchInfoToJson = [](const FlyScoreState::FlyMatchInfo &mi) {
		QJsonObject o;
		o["competition"] = mi.competition;
		o["competitionRound"] = mi.competitionRound;
		o["matchNumber"] = QString::number(mi.matchNumber);
		o["datetime_ms"] = QString::number(mi.datetime.toMSecsSinceEpoch());
		o["city"] = mi.city;
		o["venue"] = mi.venue;
		o["spectators"] = QString::number(mi.spectators);
		return o;
	};

	j["matchInfo"] = matchInfoToJson(st.matchInfo);

	auto teamToJson = [](const FlyScoreState::FlyTeam &tm) {
		QJsonObject o;
		o["title"] = tm.title;
		o["subtitle"] = tm.subtitle;
		o["logo"] = tm.logo;
		o["color"] = QString::number(tm.color);

		o["fstTimeout_ms"] = QString::number(tm.fstTimeout_ms);
		o["sndTimeout_ms"] = QString::number(tm.sndTimeout_ms);
		o["trdTimeout_ms"] = QString::number(tm.trdTimeout_ms);

		QJsonArray ofsArr;
		for (const auto &of : tm.officials) {
			QJsonObject oo;
			oo["officialLetter"] = QString(of.officialLetter);
			oo["officialName"] = of.officialName;
			oo["officialRole"] = of.officialRole;
			oo["warning_ms"] = QString::number(of.warning_ms);
			oo["fst2minSusp_ms"] = QString::number(of.fst2minSusp_ms);
			oo["disqualification_ms"] = QString::number(of.disqualification_ms);
			oo["blueCard_ms"] = QString::number(of.blueCard_ms);
			ofsArr.append(oo);
		}
		o["officials"] = ofsArr;

		QJsonArray plrsArr;
		for (const auto &plr : tm.players) {
			QJsonObject oo;
			oo["playerTshirtNumber"] = plr.playerTshirtNumber;
			oo["playerTshirtName"] = plr.playerTshirtName;
			oo["playerName"] = plr.playerName;
			oo["warning_ms"] = QString::number(plr.warning_ms);
			oo["fst2minSusp_ms"] = QString::number(plr.fst2minSusp_ms);
			oo["snd2minSusp_ms"] = QString::number(plr.snd2minSusp_ms);
			oo["trd2minSusp_ms"] = QString::number(plr.trd2minSusp_ms);
			oo["disqualification_ms"] = QString::number(plr.disqualification_ms);
			oo["blueCard_ms"] = QString::number(plr.blueCard_ms);

			QJsonArray plrgArr;
			for (const auto &plrg : plr.goals) {
				QJsonObject ooo;
				ooo["moment_ms"] = QString::number(plrg.moment_ms);
				ooo["goalType"] = plrg.goalType;
				plrgArr.push_back(ooo);
			}
			oo["goals"] = plrgArr;

			plrsArr.push_back(oo);
		}
		o["players"] = plrsArr;

		return o;
	};

	j["home"] = teamToJson(st.home);
	j["guest"] = teamToJson(st.guest);
	j["swap_sides"] = st.swap_sides;
	j["show_scoreboard"] = st.show_scoreboard;

	QJsonArray cfArr;
	for (const auto &cf : st.stTeamsStats) {
		QJsonObject o;
		o["label"] = cf.label;
		o["home"] = cf.first;
		o["guest"] = cf.second;
		o["visible"] = cf.visible;
		cfArr.append(o);
	}
	j["teamsStats"] = cfArr;

	QJsonArray msArr;
	for (const auto &ms : st.stMatchStats) {
		QJsonObject o;
		o["label"] = ms.label;
		o["value"] = ms.first;
		o["visible"] = ms.visible;
		msArr.append(o);
	}
	j["matchStats"] = msArr;

	QJsonArray timersArr;
	if (st.timers.isEmpty()) {
		timersArr.append(FlyScoreState::timerToJson(FlyScoreState::makeDefaultMainTimer()));
		timersArr.append(FlyScoreState::timerToJson(FlyScoreState::makeDefaultTimeoutTimer()));
	} else {
		for (const auto &tm : st.timers) {
			timersArr.append(FlyScoreState::timerToJson(tm));
		}
	}
	QJsonObject to;
	to["timeout"] = timersArr.last();
	LOGI("Set timeout timer to json file");
	timersArr.removeLast();
	to["main"] = timersArr;
	j["timers"] = to;

	return j;
}

bool FlyScoreState::fly_state_from_json_object(const QJsonObject &j, FlyScoreState::FlyState &st)
{
	auto readColor = [](const QJsonObject &o, const char *key, uint32_t def = 0xFFFFFF) -> uint32_t {
		const QJsonValue v = o.value(key);

		if (v.isDouble())
			return static_cast<uint32_t>(v.toInt());

		if (v.isString()) {
			QString s = v.toString().trimmed();
			if (s.startsWith("0x", Qt::CaseInsensitive))
				return s.mid(2).toUInt(nullptr, 16);
			return s.toUInt(nullptr, 10);
		}

		return def;
	};

	auto readMatchInfo = [&](const QJsonObject &o) {
		FlyScoreState::FlyMatchInfo mi;
		mi.competition = o.value("competition").toString();
		mi.competitionRound = o.value("competitionRound").toString();
		mi.matchNumber = o.value("matchNumber").toString("0").toInt();
		qint64 datetime_ms = o.value("datetime_ms").toString("0").toLongLong();
		mi.datetime = QDateTime::fromMSecsSinceEpoch(datetime_ms);
		mi.city = o.value("city").toString();
		mi.venue = o.value("venue").toString();
		mi.spectators = o.value("spectators").toString("0").toInt();
		return mi;
	};
	st.matchInfo = readMatchInfo(j.value("matchInfo").toObject());

	auto readTeam = [&](const QJsonObject &o) {
		FlyScoreState::FlyTeam tm;
		tm.title = o.value("title").toString();
		tm.subtitle = o.value("subtitle").toString();
		tm.logo = o.value("logo").toString();
		tm.color = readColor(o, "color", 0xFFFFFF);

		tm.fstTimeout_ms = o.value("fstTimeout_ms").toString("0").toLongLong();
		tm.sndTimeout_ms = o.value("sndTimeout_ms").toString("0").toLongLong();
		tm.trdTimeout_ms = o.value("trdTimeout_ms").toString("0").toLongLong();

		const QJsonValue ofVal = o.value("officials");
		if (ofVal.isArray()) {
			const QJsonArray ofArr = ofVal.toArray();
			tm.officials.reserve(ofArr.size());

			char ofl = 'A';
			for (const QJsonValue v : ofArr) {
				if (!v.isObject())
					continue;
				const QJsonObject oo = v.toObject();
				FlyScoreState::FlyOfficial of;
				// of.officialLetter = o.value("officialLetter").toString().at(0);
				of.officialLetter = QString(ofl++).at(0);
				of.officialName = oo.value("officialName").toString();
				of.officialRole = oo.value("officialRole").toString();
				of.warning_ms = oo.value("warning_ms").toString("0").toLongLong();
				of.fst2minSusp_ms = oo.value("fst2minSusp_ms").toString("0").toLongLong();
				of.disqualification_ms = oo.value("disqualification_ms").toString("0").toLongLong();
				of.blueCard_ms = oo.value("blueCard_ms").toString("0").toLongLong();

				tm.officials.push_back(of);
			}
		}

		const QJsonValue opVal = o.value("players");
		if (opVal.isArray()) {
			const QJsonArray opArr = opVal.toArray();
			tm.players.reserve(opArr.size());

			for (const QJsonValue v : opArr) {
				if (!v.isObject())
					continue;
				const QJsonObject oo = v.toObject();
				FlyScoreState::FlyPlayer op;

				op.playerTshirtNumber = oo.value("playerTshirtNumber").toInt();
				op.playerTshirtName = oo.value("playerTshirtName").toString();
				op.playerName = oo.value("playerName").toString();
				op.warning_ms = oo.value("warning_ms").toString("0").toLongLong();
				op.fst2minSusp_ms = oo.value("fst2minSusp_ms").toString("0").toLongLong();
				op.snd2minSusp_ms = oo.value("snd2minSusp_ms").toString("0").toLongLong();
				op.trd2minSusp_ms = oo.value("trd2minSusp_ms").toString("0").toLongLong();
				op.disqualification_ms = oo.value("disqualification_ms").toString("0").toLongLong();
				op.blueCard_ms = oo.value("blueCard_ms").toString("0").toLongLong();

				const QJsonValue opgVal = oo.value("goals");
				if (opgVal.isArray()) {
					const QJsonArray opgArr = opgVal.toArray();
					op.goals.reserve(opgArr.size());

					for (const QJsonValue vv : opgArr) {
						if (!vv.isObject())
							continue;
						const QJsonObject ooo = vv.toObject();
						FlyScoreState::FlyGoal opg;
						opg.moment_ms = ooo.value("moment_ms").toString("0").toLongLong();
						opg.goalType = ooo.value("goalType").toString();
						op.goals.push_back(opg);
					}
				}

				tm.players.push_back(op);
			}
		}

		return tm;
	};

	st.home = readTeam(j.value("home").toObject());
	st.guest = readTeam(j.value("guest").toObject());
	st.swap_sides = j.value("swap_sides").toBool(false);
	st.show_scoreboard = j.value("show_scoreboard").toBool(true);
	st.stTeamsStats.clear();

	const QJsonValue cfVal = j.value("teamsStats");
	if (cfVal.isArray()) {
		const QJsonArray cfArr = cfVal.toArray();
		st.stTeamsStats.reserve(cfArr.size());

		for (const QJsonValue v : cfArr) {
			if (!v.isObject())
				continue;

			const QJsonObject o = v.toObject();

			FlyScoreState::FlyStat cf;
			cf.label = o.value("label").toString();
			cf.first = o.value("home").toInt(0);
			cf.second = o.value("guest").toInt(0);
			cf.visible = o.value("visible").toBool(true);

			st.stTeamsStats.push_back(cf);
		}
	}

	st.stMatchStats.clear();

	const QJsonValue ssVal = j.value("matchStats");
	if (ssVal.isArray()) {
		const QJsonArray msArr = ssVal.toArray();
		st.stMatchStats.reserve(msArr.size());

		for (const QJsonValue v : msArr) {
			if (!v.isObject())
				continue;

			const QJsonObject o = v.toObject();

			FlyScoreState::FlyStat ms;
			ms.label = o.value("label").toString();
			ms.first = o.value("value").toInt(0);
			ms.visible = o.value("visible").toBool(true);

			st.stMatchStats.push_back(ms);
		}
	}

	FlyScoreState::ensureDefaultMatchStats(st);
	FlyScoreState::ensureDefaultTeamsStats(st);

	st.timers.clear();

	const QJsonValue timersVal = j.value("timers.main");
	if (timersVal.isArray()) {
		const QJsonArray timersArr = timersVal.toArray();

		if (!timersArr.isEmpty()) {
			st.timers.reserve(timersArr.size());

			for (const QJsonValue v : timersArr) {
				if (!v.isObject())
					continue;

				st.timers.push_back(FlyScoreState::timerFromJson(v.toObject()));
			}
		}
	} else {
		const QJsonObject tObj = j.value("timer").toObject();
		if (!tObj.isEmpty())
			st.timers.push_back(FlyScoreState::timerFromJson(tObj));
	}

	const QJsonValue timerTimeoutVal = j.value("timers.timeout");
	if (timerTimeoutVal.isObject()) {
		LOGI("Push timeout timer to state");
		st.timers.push_back(FlyScoreState::timerFromJson(timersVal.toObject()));
	}

	if (st.timers.isEmpty()) {
		st.timers.push_back(FlyScoreState::makeDefaultMainTimer());
		st.timers.push_back(FlyScoreState::makeDefaultTimeoutTimer());
	}

	FlyScoreState::FlyTimer &main = st.timers[0];
	if (main.mode.isEmpty())
		main.mode = "countdown";

	return true;
}

bool FlyScoreState::fly_state_read_json(const std::string &base_dir_s, std::string &out_json)
{
	const QString base_dir = QString::fromStdString(base_dir_s);
	const QString path = FlyScoreState::overlay_plugin_json(base_dir);
	QFile f(path);
	if (!f.exists() || !f.open(QIODevice::ReadOnly))
		return false;
	const QByteArray data = f.readAll();
	out_json.assign(data.constData(), data.size());
	return true;
}

bool FlyScoreState::fly_state_write_json(const std::string &base_dir_s, const std::string &json)
{
	const QString base_dir = QString::fromStdString(base_dir_s);
	const QString path = FlyScoreState::overlay_plugin_json(base_dir);
	QDir().mkpath(QFileInfo(path).absolutePath());
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	f.write(QByteArray::fromStdString(json));
	return true;
}

bool FlyScoreState::write_one_json(const QString &path, const QJsonDocument &doc)
{
	QDir().mkpath(QFileInfo(path).absolutePath());
	QFile f(path);
	if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
		return false;
	f.write(doc.toJson(QJsonDocument::Compact));
	return true;
}

bool FlyScoreState::fly_state_load(const QString &base_dir, FlyScoreState::FlyState &out)
{
	const QString path = FlyScoreState::overlay_plugin_json(base_dir);
	QFile f(path);
	if (!f.exists() || !f.open(QIODevice::ReadOnly)) {
		return false;
	}

	const auto doc = QJsonDocument::fromJson(f.readAll());
	if (!doc.isObject()) {
		return false;
	}

	return fly_state_from_json_object(doc.object(), out);
}

bool FlyScoreState::fly_state_save(const QString &base_dir, const FlyScoreState::FlyState &st)
{
	const QJsonDocument doc(FlyScoreState::fly_state_to_json_object(st));
	const QString path = FlyScoreState::overlay_plugin_json(base_dir);
	return FlyScoreState::write_one_json(path, doc);
}

QByteArray FlyScoreState::fly_state_to_json_bytes(const FlyScoreState::FlyState &st, bool compact)
{
	const QJsonDocument doc(FlyScoreState::fly_state_to_json_object(st));
	return doc.toJson(compact ? QJsonDocument::Compact : QJsonDocument::Indented);
}

FlyScoreState::FlyState FlyScoreState::fly_state_make_defaults()
{
	FlyScoreState::FlyState st;
	st.matchInfo = FlyScoreState::FlyMatchInfo{};
	st.home = FlyScoreState::FlyTeam{};
	st.guest = FlyScoreState::FlyTeam{};
	st.swap_sides = false;
	st.show_scoreboard = true;

	st.stTeamsStats.clear();
	st.stMatchStats.clear();
	st.timers.clear();

	FlyScoreState::ensureDefaultTeamsStats(st);
	FlyScoreState::ensureDefaultMatchStats(st);

	st.timers.push_back(FlyScoreState::makeDefaultMainTimer());
	st.timers.push_back(FlyScoreState::makeDefaultTimeoutTimer());

	return st;
}

bool FlyScoreState::fly_state_reset_defaults(const QString &base_dir)
{
	const QString pj = FlyScoreState::overlay_plugin_json(base_dir);
	QFile::remove(pj);

	const FlyScoreState::FlyState def = FlyScoreState::fly_state_make_defaults();
	return FlyScoreState::fly_state_save(base_dir, def);
}

bool FlyScoreState::fly_state_ensure_json_exists(const QString &base_dir, const FlyScoreState::FlyState *writeState)
{
	const QString path = FlyScoreState::overlay_plugin_json(base_dir);
	if (QFileInfo::exists(path))
		return true;

	FlyScoreState::FlyState st = writeState ? *writeState : fly_state_make_defaults();
	const bool ok = fly_state_save(base_dir, st);

	if (ok)
		LOGI("Created default plugin.json: %s", path.toUtf8().constData());
	else
		LOGW("Failed to create default plugin.json: %s", path.toUtf8().constData());

	return ok;
}

void FlyScoreState::setTimerStoppedAtInitial(FlyScoreState::FlyTimer &timer)
{
	timer.running = false;
	timer.last_tick_ms = 0;
	if (timer.initial_ms < 0)
		timer.initial_ms = 0;
	timer.current_moment_ms = timer.initial_ms;
}

QString FlyScoreState::fly_theme_manifest_path(const QString &themePath)
{
	return QDir(themePath).filePath(QStringLiteral("manifest.ini"));
}

bool FlyScoreState::fly_read_theme_manifest(const QString &themePath, FlyScoreState::FlyThemeManifest &out)
{
	const QString manifestPath = FlyScoreState::fly_theme_manifest_path(themePath);
	if (!QFileInfo::exists(manifestPath))
		return false;

	QSettings manifest(manifestPath, QSettings::IniFormat);
	out.title = manifest.value(QStringLiteral("title")).toString().trimmed();
	out.author = manifest.value(QStringLiteral("author")).toString().trimmed();
	out.authorUrl = manifest.value(QStringLiteral("author_url")).toString().trimmed();
	out.description = manifest.value(QStringLiteral("description")).toString().trimmed();
	out.version = manifest.value(QStringLiteral("version")).toString().trimmed();

	if (out.title.isEmpty())
		out.title = manifest.value(QStringLiteral("theme/title")).toString().trimmed();
	if (out.author.isEmpty())
		out.author = manifest.value(QStringLiteral("theme/author")).toString().trimmed();
	if (out.authorUrl.isEmpty())
		out.authorUrl = manifest.value(QStringLiteral("theme/author_url")).toString().trimmed();
	if (out.description.isEmpty())
		out.description = manifest.value(QStringLiteral("theme/description")).toString().trimmed();
	if (out.version.isEmpty())
		out.version = manifest.value(QStringLiteral("theme/version")).toString().trimmed();
	if (out.title.isEmpty())
		out.title = manifest.value(QStringLiteral("Theme/title")).toString().trimmed();
	if (out.author.isEmpty())
		out.author = manifest.value(QStringLiteral("Theme/author")).toString().trimmed();
	if (out.authorUrl.isEmpty())
		out.authorUrl = manifest.value(QStringLiteral("Theme/author_url")).toString().trimmed();
	if (out.description.isEmpty())
		out.description = manifest.value(QStringLiteral("Theme/description")).toString().trimmed();
	if (out.version.isEmpty())
		out.version = manifest.value(QStringLiteral("Theme/version")).toString().trimmed();

	FlyScoreState::fly_read_theme_manifest_text(manifestPath, out);

	return !out.title.isEmpty() && !out.author.isEmpty() && !out.authorUrl.isEmpty() &&
	       !out.description.isEmpty() && !out.version.isEmpty();
}

FlyScoreState::FlyThemeInfo FlyScoreState::fly_read_theme_info(const QString &themePath)
{
	FlyScoreState::FlyThemeInfo info;
	const QFileInfo themeDirInfo(themePath);
	info.path = QDir(themePath).absolutePath();
	info.folderName = themeDirInfo.fileName();
	info.hasIndex = QFileInfo::exists(QDir(themePath).filePath(QStringLiteral("index.html")));
	info.hasManifest = QFileInfo::exists(FlyScoreState::fly_theme_manifest_path(themePath));

	const bool manifestOk = FlyScoreState::fly_read_theme_manifest(themePath, info.manifest);
	info.valid = info.hasIndex && manifestOk;

	if (!info.hasIndex)
		info.error = fly_i18n("Dock.TemplateMissingIndex");
	else if (!info.hasManifest)
		info.error = fly_i18n("Dock.TemplateMissingManifest");
	else if (!manifestOk)
		info.error = fly_i18n("Dock.TemplateInvalidManifest");

	if (info.manifest.title.isEmpty())
		info.manifest.title = info.folderName.isEmpty() ? info.path : info.folderName;

	return info;
}

QString FlyScoreState::selectedTemplateName(QString dataDir)
{
	FlyScoreState::FlyThemeManifest manifest;
	if (FlyScoreState::fly_is_valid_theme_folder(dataDir, &manifest))
		return manifest.title;
	return QFileInfo(dataDir).fileName();
}

QString FlyScoreState::fly_normalize_manifest_key(QString key, const QString &section)
{
	key = key.trimmed().toLower();
	if (!section.trimmed().isEmpty())
		key = section.trimmed().toLower() + QStringLiteral("/") + key;
	return key;
}

void fly_apply_manifest_value(FlyScoreState::FlyThemeManifest &out, const QString &key, const QString &value)
{
	const QString k = key.trimmed().toLower();
	const QString v = value.trimmed();
	if ((k == QLatin1String("title") || k == QLatin1String("theme/title")) && out.title.isEmpty())
		out.title = v;
	else if ((k == QLatin1String("author") || k == QLatin1String("theme/author")) && out.author.isEmpty())
		out.author = v;
	else if ((k == QLatin1String("author_url") || k == QLatin1String("theme/author_url")) &&
		 out.authorUrl.isEmpty())
		out.authorUrl = v;
	else if ((k == QLatin1String("description") || k == QLatin1String("theme/description")) &&
		 out.description.isEmpty())
		out.description = v;
	else if ((k == QLatin1String("version") || k == QLatin1String("theme/version")) && out.version.isEmpty())
		out.version = v;
}

void FlyScoreState::fly_read_theme_manifest_text(const QString &manifestPath, FlyScoreState::FlyThemeManifest &out)
{
	QFile file(manifestPath);
	if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
		return;

	QTextStream stream(&file);
	QString section;
	while (!stream.atEnd()) {
		QString line = stream.readLine().trimmed();
		if (line.startsWith(QChar(0xFEFF)))
			line.remove(0, 1);
		if (line.isEmpty() || line.startsWith(QLatin1Char(';')) || line.startsWith(QLatin1Char('#')))
			continue;
		if (line.startsWith(QLatin1Char('[')) && line.endsWith(QLatin1Char(']'))) {
			section = line.mid(1, line.size() - 2).trimmed();
			continue;
		}

		int pos = line.indexOf(QLatin1Char('='));
		if (pos < 0)
			pos = line.indexOf(QLatin1Char(':'));
		if (pos < 0)
			continue;

		QString value = line.mid(pos + 1).trimmed();
		if ((value.startsWith(QLatin1Char('"')) && value.endsWith(QLatin1Char('"'))) ||
		    (value.startsWith(QLatin1Char('\'')) && value.endsWith(QLatin1Char('\''))))
			value = value.mid(1, value.size() - 2).trimmed();

		fly_apply_manifest_value(out, FlyScoreState::fly_normalize_manifest_key(line.left(pos), section),
					 value);
	}
}

bool FlyScoreState::fly_is_valid_theme_folder(const QString &themePath, FlyScoreState::FlyThemeManifest *manifest)
{
	FlyScoreState::FlyThemeInfo info = FlyScoreState::fly_read_theme_info(themePath);
	if (!info.valid)
		return false;

	if (manifest)
		*manifest = info.manifest;
	return true;
}
