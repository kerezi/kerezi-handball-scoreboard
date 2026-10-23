#include "fly_score_stats_tab.hpp"
#include "fly_score_log.hpp"
#include "fly_score_const.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][stats-tab]"

#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"

#include <Qt>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QWidget>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QIcon>
#include <QString>

FlyStatsTab::~FlyStatsTab() = default;

void FlyStatsTab::init() {}

void FlyStatsTab::buildUI()
{
	auto *tab = new QWidget(this);
	auto *statsVBox = new QVBoxLayout(tab);
	statsVBox->setContentsMargins(0, 0, 0, 0);
	statsVBox->setSpacing(4);
	statsVBox->setAlignment(Qt::AlignTop);
	layout = statsVBox;

	syncUiFromState();
}

void FlyStatsTab::clearAllStatsRows()
{
	stats.clear();
	if (!layout)
		return;

	while (QLayoutItem *item = layout->takeAt(0)) {
		if (QWidget *w = item->widget())
			w->deleteLater();
		delete item;
	}
}

void FlyStatsTab::refreshUiFromState(bool onlyTimeIfRunning)
{
	Q_UNUSED(onlyTimeIfRunning);

	syncUiFromState();
}

void FlyStatsTab::syncUiFromState()
{
	clearAllStatsRows();
	if (!layout)
		return;

	stats.reserve(isMatchStat ? state_.stMatchStats.size() : state_.stTeamsStats.size());
	{
		auto *hdrRow = new QWidget(this);
		auto *grid = new QGridLayout(hdrRow);
		grid->setContentsMargins(0, 0, 0, 0);
		grid->setHorizontalSpacing(6);
		grid->setVerticalSpacing(0);

		auto *cbSpacer = new QSpacerItem(22, 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
		grid->addItem(cbSpacer, 0, 0);

		auto *statHdr = new QLabel(fly_i18n("Common.Stat"), hdrRow);
		statHdr->setStyleSheet(QStringLiteral("font-weight:bold;"));
		grid->addWidget(statHdr, 0, 1);

		auto *firstHdr = new QLabel(fly_i18n(isMatchStat ? "Common.MatchStats" : "Common.Home"), hdrRow);
		firstHdr->setStyleSheet(QStringLiteral("font-weight:bold;"));
		firstHdr->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		grid->addWidget(firstHdr, 0, 2);

		if (!isMatchStat) {
			auto *secondHdr = new QLabel(fly_i18n("Common.Guest"), hdrRow);
			secondHdr->setStyleSheet(QStringLiteral("font-weight:bold;"));
			secondHdr->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
			grid->addWidget(secondHdr, 0, 3);
		}

		grid->setColumnStretch(1, 2);
		grid->setColumnStretch(2, 1);

		if (!isMatchStat)
			grid->setColumnStretch(3, 1);

		layout->addWidget(hdrRow);
	}

	QVector<FlyScoreState::FlyStat> *stStats = isMatchStat ? &state_.stMatchStats : &state_.stTeamsStats;
	for (int i = 0; i < stStats->size(); ++i) {
		const auto &st = (*stStats)[i];
		FlyStatUIRow ui;

		auto *row = new QWidget();
		auto *grid = new QGridLayout(row);
		grid->setContentsMargins(0, 0, 0, 0);
		grid->setHorizontalSpacing(6);
		grid->setVerticalSpacing(0);

		auto *visibleCheck = new QCheckBox(row);
		visibleCheck->setChecked(st.visible);
		grid->addWidget(visibleCheck, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);

		auto *labelLbl = new QLabel(st.label.isEmpty() ? QStringLiteral("(unnamed)") : st.label, row);
		labelLbl->setMinimumWidth(120);
		grid->addWidget(labelLbl, 0, 1);

		auto makeStepBtn = [this](const QString &themeIconName, QStyle::StandardPixmap fallbackPixmap,
					  const QString &fallbackText, const QString &tooltip,
					  QWidget *parent) -> QToolButton * {
			auto *btn = new QToolButton(parent);
			btn->setToolTip(tooltip);
			btn->setCursor(Qt::PointingHandCursor);
			btn->setAutoRaise(false);
			btn->setFocusPolicy(Qt::StrongFocus);
			btn->setFixedSize(28, 28);
			btn->setIconSize(QSize(14, 14));

			QIcon icon = QIcon::fromTheme(themeIconName);
			if (icon.isNull())
				icon = style()->standardIcon(fallbackPixmap);

			if (!icon.isNull()) {
				btn->setIcon(icon);
			} else {
				btn->setText(fallbackText);
			}

			return btn;
		};

		auto *firstSpin = new QSpinBox(row);
		firstSpin->setRange(0, std::numeric_limits<int>::max());
		firstSpin->setValue(std::max(0, st.first));
		firstSpin->setMinimumWidth(86);
		firstSpin->setMaximumHeight(32);
		firstSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);

		auto *minusFirstSpin = makeStepBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown,
						   QStringLiteral("-"), fly_i18n("Dock.HomeMinusOne"), row);

		auto *plusFirstSpin = makeStepBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("+"),
						  fly_i18n("Dock.HomePlusOne"), row);

		QSpinBox *secondSpin = nullptr;
		QToolButton *minusSecondSpin = nullptr;
		QToolButton *plusSecondSpin = nullptr;
		if (!isMatchStat) {
			secondSpin = new QSpinBox(row);
			secondSpin->setRange(0, std::numeric_limits<int>::max());
			secondSpin->setValue(std::max(0, st.second));
			secondSpin->setMinimumWidth(86);
			secondSpin->setMaximumHeight(32);
			secondSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);

			minusSecondSpin = makeStepBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown,
						      QStringLiteral("-"), fly_i18n("Dock.GuestMinusOne"), row);

			plusSecondSpin = makeStepBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp,
						     QStringLiteral("+"), fly_i18n("Dock.GuestPlusOne"), row);
		}

		auto *firstBox = new QWidget(row);
		auto *firstLay = new QHBoxLayout(firstBox);
		firstLay->setContentsMargins(0, 0, 0, 0);
		firstLay->setSpacing(4);
		firstLay->addWidget(minusFirstSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);
		firstLay->addWidget(firstSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);
		firstLay->addWidget(plusFirstSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);

		QWidget *secondBox = nullptr;
		QHBoxLayout *secondLay = nullptr;
		if (!isMatchStat) {
			secondBox = new QWidget(row);
			secondLay = new QHBoxLayout(secondBox);
			secondLay->setContentsMargins(0, 0, 0, 0);
			secondLay->setSpacing(4);
			secondLay->addWidget(minusSecondSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);
			secondLay->addWidget(secondSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);
			secondLay->addWidget(plusSecondSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);
		}

		grid->addWidget(firstBox, 0, 2, Qt::AlignHCenter | Qt::AlignVCenter);
		if (!isMatchStat)
			grid->addWidget(secondBox, 0, 3, Qt::AlignHCenter | Qt::AlignVCenter);

		grid->setColumnStretch(1, 2);
		grid->setColumnStretch(2, 1);
		if (!isMatchStat)
			grid->setColumnStretch(3, 1);

		layout->addWidget(row);

		ui.row = row;
		ui.visibleCheck = visibleCheck;
		ui.labelLbl = labelLbl;
		ui.firstSpin = firstSpin;
		ui.secondSpin = secondSpin;
		ui.minusFirstSpin = minusFirstSpin;
		ui.plusFirstSpin = plusFirstSpin;
		ui.minusSecondSpin = minusSecondSpin;
		ui.plusSecondSpin = plusSecondSpin;

		auto sync = [this]() {
			syncStateFromUi();
		};

		connect(firstSpin, qOverload<int>(&QSpinBox::valueChanged), this, [sync](int) { sync(); });
		if (secondSpin != nullptr)
			connect(secondSpin, qOverload<int>(&QSpinBox::valueChanged), this, [sync](int) { sync(); });
		connect(visibleCheck, &QCheckBox::toggled, this, [sync](bool) { sync(); });

		connect(minusFirstSpin, &QToolButton::clicked, this, [firstSpin, sync]() {
			firstSpin->setValue(std::max(0, firstSpin->value() - 1));
			sync();
		});
		connect(plusFirstSpin, &QToolButton::clicked, this, [firstSpin, sync]() {
			firstSpin->setValue(firstSpin->value() + 1);
			sync();
		});

		if (!isMatchStat) {
			connect(minusSecondSpin, &QToolButton::clicked, this, [secondSpin, sync]() {
				secondSpin->setValue(std::max(0, secondSpin->value() - 1));
				sync();
			});
			connect(plusSecondSpin, &QToolButton::clicked, this, [secondSpin, sync]() {
				secondSpin->setValue(secondSpin->value() + 1);
				sync();
			});
		}

		stats.push_back(ui);
	}

	layout->addStretch(1);
}

void FlyStatsTab::syncStateFromUi()
{
	QVector<FlyScoreState::FlyStat> *stStats = isMatchStat ? &state_.stMatchStats : &state_.stTeamsStats;

	stStats->clear();
	stStats->reserve(stats.size());

	for (const auto &ui : stats) {
		FlyScoreState::FlyStat st;
		st.label = ui.labelLbl ? ui.labelLbl->text() : QString();
		st.first = ui.firstSpin ? ui.firstSpin->value() : 0;
		st.second = ui.secondSpin ? ui.secondSpin->value() : 0;
		st.visible = ui.visibleCheck ? ui.visibleCheck->isChecked() : true;
		state_.stTeamsStats.push_back(st);
	}

	dock_->saveState();
}

void FlyStatsTab::bumpStat(int index, int delta, bool isHome)
{
	QVector<FlyScoreState::FlyStat> *stStats = isMatchStat ? &state_.stMatchStats : &state_.stTeamsStats;
	if (index < 0 || index >= stStats->size())
		return;

	if (index < stats.size() &&
	    ((((isHome && stats[index].firstSpin) || (!isHome && stats[index].secondSpin)) && !isMatchStat) ||
	     (stats[index].firstSpin && isMatchStat))) {
		auto *spin = isHome || isMatchStat ? stats[index].firstSpin : stats[index].secondSpin;
		spin->setValue(std::max(0, spin->value() + delta));
		return;
	}

	(*stStats)[index].first = std::max(0, (*stStats)[index].first + delta);
	dock_->saveState();
}

void FlyStatsTab::toggleStatVisible(int index)
{
	QVector<FlyScoreState::FlyStat> *stStats = isMatchStat ? &state_.stMatchStats : &state_.stTeamsStats;
	if (index < 0 || index >= stStats->size())
		return;

	if (index < stats.size() && stats[index].visibleCheck) {
		auto *chk = stats[index].visibleCheck;
		chk->setChecked(!chk->isChecked());
		return;
	}

	(*stStats)[index].visible = !(*stStats)[index].visible;
	dock_->saveState();
}
