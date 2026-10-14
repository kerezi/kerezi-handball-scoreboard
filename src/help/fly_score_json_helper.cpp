#define LOG_TAG "[" PLUGIN_NAME "][json-helper]"

#include "fly_score_json_helper.hpp"

int FlyJsonHelper::jsonInt(const QJsonObject &o, const QString &key, int fallback)
{
	const QJsonValue v = o.value(key);
	if (v.isDouble())
		return v.toInt(fallback);
	if (v.isString()) {
		bool ok = false;
		const int n = v.toString().toInt(&ok);
		return ok ? n : fallback;
	}
	return fallback;
}

qint64 FlyJsonHelper::jsonInt64(const QJsonObject &o, const QString &key, qint64 fallback)
{
	const QJsonValue v = o.value(key);
	if (v.isDouble())
		return static_cast<qint64>(v.toDouble(fallback));
	if (v.isString()) {
		bool ok = false;
		const qint64 n = v.toString().toLongLong(&ok);
		return ok ? n : fallback;
	}
	return fallback;
}

bool FlyJsonHelper::jsonBool(const QJsonObject &o, const QString &key, bool fallback)
{
	const QJsonValue v = o.value(key);
	if (v.isBool())
		return v.toBool(fallback);
	if (v.isDouble())
		return v.toInt() != 0;
	if (v.isString()) {
		const QString s = v.toString().trimmed().toLower();
		if (s == QLatin1String("true") || s == QLatin1String("1") || s == QLatin1String("yes"))
			return true;
		if (s == QLatin1String("false") || s == QLatin1String("0") || s == QLatin1String("no"))
			return false;
	}
	return fallback;
}

QString FlyJsonHelper::jsonString(const QJsonObject &o, const QString &key)
{
	return o.value(key).toString().trimmed();
}

uint32_t FlyJsonHelper::jsonColor(const QJsonObject &o, const QString &key, uint32_t fallback)
{
	const QJsonValue v = o.value(key);
	if (v.isDouble())
		return static_cast<uint32_t>(v.toInt(static_cast<int>(fallback)));
	if (v.isString()) {
		QString s = v.toString().trimmed();
		if (s.startsWith(QLatin1Char('#')))
			s.remove(0, 1);
		bool ok = false;
		const uint n = s.startsWith(QStringLiteral("0x"), Qt::CaseInsensitive) ? s.mid(2).toUInt(&ok, 16)
										       : s.toUInt(&ok, 16);
		if (ok)
			return static_cast<uint32_t>(n);
		const uint dec = s.toUInt(&ok, 10);
		if (ok)
			return static_cast<uint32_t>(dec);
	}
	return fallback;
}
