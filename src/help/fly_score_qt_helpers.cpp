#include "fly_score_qt_helpers.hpp"

#include <QDateTime>
#include <QRegularExpression>
#include <QWidget>
#include <QStyle>
#include <QUrl>
#include <QUrlQuery>
#include <QAbstractButton>
#include <QPushButton>
#include <QColor>

qint64 FlyQtHelper::fly_now_ms()
{
    return QDateTime::currentMSecsSinceEpoch();
}

QIcon FlyQtHelper::fly_themed_icon(QWidget *w, const char *name, QStyle::StandardPixmap fallback)
{
    QIcon ic = QIcon::fromTheme(QString::fromUtf8(name));
    if (!ic.isNull())
        return ic;
    return w->style()->standardIcon(fallback);
}

void FlyQtHelper::fly_style_icon_only_button(QAbstractButton *b,
                                const QIcon &icon,
                                const QString &tooltip,
                                const QSize &iconSize)
{
    b->setText(QString());
    b->setIcon(icon);
    b->setIconSize(iconSize);
    b->setToolTip(tooltip);
    b->setCursor(Qt::PointingHandCursor);
    if (auto *pb = qobject_cast<QPushButton *>(b))
        pb->setFlat(true);
}

qint64 FlyQtHelper::fly_parse_mmss_to_ms(const QString &txt)
{
    static const QRegularExpression re(R"(^\s*(\d+)\s*:\s*([0-5]\d)\s*$)");
    auto m = re.match(txt);
    if (!m.hasMatch())
        return -1;
    const qint64 minutes = m.captured(1).toLongLong();
    const qint64 seconds = m.captured(2).toLongLong();
    return (minutes * 60 + seconds) * 1000;
}

QString FlyQtHelper::fly_format_ms_mmss(qint64 ms)
{
    if (ms < 0)
        ms = 0;
    qint64 total = ms / 1000;
    qint64 m = total / 60;
    qint64 s = total % 60;
    return QString::number(m) + ":" + QString::number(s).rightJustified(2, '0');
}

QString FlyQtHelper::fly_cache_bust_url(const QString &in)
{
    QUrl u(in);
    QUrlQuery q(u);
    q.removeAllQueryItems(QStringLiteral("cb"));
    q.addQueryItem(QStringLiteral("cb"),
                   QString::number(QDateTime::currentMSecsSinceEpoch()));
    u.setQuery(q);
    return u.toString();
}


QColor FlyQtHelper::colorFromU32(uint32_t c)
{
	int r = (c >> 16) & 0xFF;
	int g = (c >> 8) & 0xFF;
	int b = (c >> 0) & 0xFF;
	return QColor(r, g, b);
}

uint32_t FlyQtHelper::u32FromColor(const QColor &qc)
{
	uint32_t r = (uint32_t)qc.red() & 0xFF;
	uint32_t g = (uint32_t)qc.green() & 0xFF;
	uint32_t b = (uint32_t)qc.blue() & 0xFF;
	return (r << 16) | (g << 8) | b;
}

QColor FlyQtHelper::getHighContrastColor(const QColor &backgroundColor)
{
    int r = backgroundColor.red();
    int g = backgroundColor.green();
    int b = backgroundColor.blue();
    double brightness = (r * 0.299) + (g * 0.587) + (b * 0.114);

    return (brightness > 128.0) ? QColor(0, 0, 0) : QColor(255, 255, 255);
}