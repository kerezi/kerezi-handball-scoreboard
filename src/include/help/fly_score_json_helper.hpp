#ifndef FLY_SCORE_JSON_HELPER
#define FLY_SCORE_JSON_HELPER

#pragma once

#include<QJsonObject>
#include<QString>

class FlyJsonHelper {

public:
    static int jsonInt(const QJsonObject &o, const QString &key, int fallback = 0);
    static qint64 jsonInt64(const QJsonObject &o, const QString &key, qint64 fallback = 0);
    static bool jsonBool(const QJsonObject &o, const QString &key, bool fallback = false);
    static QString jsonString(const QJsonObject &o, const QString &key);
    static uint32_t jsonColor(const QJsonObject &o, const QString &key, uint32_t fallback);
};

#endif