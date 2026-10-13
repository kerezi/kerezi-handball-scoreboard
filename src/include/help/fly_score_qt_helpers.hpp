#pragma once

#include <QString>
#include <QIcon>
#include <QSize>
#include <QStyle>
#include <QColor>

class QWidget;
class QAbstractButton;

class FlyQtHelper {
public:
    static qint64 fly_now_ms();

    static QIcon fly_themed_icon(QWidget *w,
                      const char *name,
                      QStyle::StandardPixmap fallback);

    static void fly_style_icon_only_button(QAbstractButton *b,
                                const QIcon &icon,
                                const QString &tooltip,
                                const QSize &iconSize = QSize(20, 20));

    static qint64 fly_parse_mmss_to_ms(const QString &txt);
    static QString fly_format_ms_mmss(qint64 ms);
    static QString fly_cache_bust_url(const QString &in);
    static QColor colorFromU32(uint32_t c);
    static uint32_t u32FromColor(const QColor &qc);
    static QColor getHighContrastColor(const QColor &backgroundColor);
};




