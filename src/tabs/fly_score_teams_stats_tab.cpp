#include "fly_score_teams_stats_tab.hpp"
#include "fly_score_log.hpp"
#include "fly_score_const.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][teams-stats-tab]"

#include "fly_score_log.hpp"

#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_json_helper.hpp"
#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_hotkeys_dialog.hpp"
#include "fly_score_fields_dialog.hpp"
#include "fly_score_timers_dialog.hpp"
#include "fly_score_teams_dialog.hpp"
#include "fly_score_players_dialog.hpp"
#include "fly_score_match_info_dialog.hpp"
#include "fly_score_paths.hpp"
#include "fly_score_websocket_server.hpp"
#include "fly_score_websocket_server.hpp"

#include <Qt>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QCheckBox>
#include <QSpinBox>
#include <QMessageBox>
#include <QLabel>
#include <QDir>
#include <QDesktopServices>
#include <QShortcut>
#include <QSettings>
#include <QFileDialog>
#include <QWidget>

FlyTeamsStatsTab::~FlyTeamsStatsTab() = default;

void FlyTeamsStatsTab::init() {}

void FlyTeamsStatsTab::buildUI()
{
	auto *tab = new QWidget(this);
	auto *teamsStatsVBox = new QVBoxLayout(tab);
	teamsStatsVBox->setContentsMargins(0, 0, 0, 0);
	teamsStatsVBox->setSpacing(4);
	teamsStatsVBox->setAlignment(Qt::AlignTop);
	layout = teamsStatsVBox;

	syncUiFromState();
}

void FlyTeamsStatsTab::clearAllTeamsStatsRows()
{
	teamsStats.clear();
	if (!layout)
		return;

	while (QLayoutItem *item = layout->takeAt(0)) {
		if (QWidget *w = item->widget())
			w->deleteLater();
		delete item;
	}
}
void FlyTeamsStatsTab::refreshUiFromState(bool onlyTimeIfRunning)
{
	Q_UNUSED(onlyTimeIfRunning);

	syncUiFromState();
}

void FlyTeamsStatsTab::syncUiFromState()
{
	clearAllTeamsStatsRows();
	if (!layout)
		return;

	teamsStats.reserve(state_.stTeamsStats.size());

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

		auto *homeHdr = new QLabel(fly_i18n("Common.Home"), hdrRow);
		homeHdr->setStyleSheet(QStringLiteral("font-weight:bold;"));
		homeHdr->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		grid->addWidget(homeHdr, 0, 2);

		auto *guestHdr = new QLabel(fly_i18n("Common.Guest"), hdrRow);
		guestHdr->setStyleSheet(QStringLiteral("font-weight:bold;"));
		guestHdr->setAlignment(Qt::AlignHCenter | Qt::AlignVCenter);
		grid->addWidget(guestHdr, 0, 3);

		grid->setColumnStretch(1, 2);
		grid->setColumnStretch(2, 1);
		grid->setColumnStretch(3, 1);

		layout->addWidget(hdrRow);
	}

	for (int i = 0; i < state_.stTeamsStats.size(); ++i) {
		const auto &cf = state_.stTeamsStats[i];
		FlyTeamsStatUIRow ui;

		auto *row = new QWidget();
		auto *grid = new QGridLayout(row);
		grid->setContentsMargins(0, 0, 0, 0);
		grid->setHorizontalSpacing(6);
		grid->setVerticalSpacing(0);

		auto *visibleCheck = new QCheckBox(row);
		visibleCheck->setChecked(cf.visible);
		grid->addWidget(visibleCheck, 0, 0, Qt::AlignLeft | Qt::AlignVCenter);

		auto *labelLbl = new QLabel(cf.label.isEmpty() ? QStringLiteral("(unnamed)") : cf.label, row);
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

		auto *homeSpin = new QSpinBox(row);
		homeSpin->setRange(0, std::numeric_limits<int>::max());
		homeSpin->setValue(std::max(0, cf.first));
		homeSpin->setMinimumWidth(86);
		homeSpin->setMaximumHeight(32);
		homeSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);

		auto *minusHome = makeStepBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("-"),
					      fly_i18n("Dock.HomeMinusOne"), row);

		auto *plusHome = makeStepBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("+"),
					     fly_i18n("Dock.HomePlusOne"), row);

		auto *guestSpin = new QSpinBox(row);
		guestSpin->setRange(0, std::numeric_limits<int>::max());
		guestSpin->setValue(std::max(0, cf.second));
		guestSpin->setMinimumWidth(86);
		guestSpin->setMaximumHeight(32);
		guestSpin->setButtonSymbols(QAbstractSpinBox::NoButtons);

		auto *minusGuest = makeStepBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("-"),
					       fly_i18n("Dock.GuestsMinusOne"), row);

		auto *plusGuest = makeStepBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("+"),
					      fly_i18n("Dock.GuestsPlusOne"), row);

		auto *homeBox = new QWidget(row);
		auto *homeLay = new QHBoxLayout(homeBox);
		homeLay->setContentsMargins(0, 0, 0, 0);
		homeLay->setSpacing(4);
		homeLay->addWidget(minusHome, 0, Qt::AlignHCenter | Qt::AlignVCenter);
		homeLay->addWidget(homeSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);
		homeLay->addWidget(plusHome, 0, Qt::AlignHCenter | Qt::AlignVCenter);

		auto *guestBox = new QWidget(row);
		auto *guestLay = new QHBoxLayout(guestBox);
		guestLay->setContentsMargins(0, 0, 0, 0);
		guestLay->setSpacing(4);
		guestLay->addWidget(minusGuest, 0, Qt::AlignHCenter | Qt::AlignVCenter);
		guestLay->addWidget(guestSpin, 0, Qt::AlignHCenter | Qt::AlignVCenter);
		guestLay->addWidget(plusGuest, 0, Qt::AlignHCenter | Qt::AlignVCenter);

		grid->addWidget(homeBox, 0, 2, Qt::AlignHCenter | Qt::AlignVCenter);
		grid->addWidget(guestBox, 0, 3, Qt::AlignHCenter | Qt::AlignVCenter);

		grid->setColumnStretch(1, 2);
		grid->setColumnStretch(2, 1);
		grid->setColumnStretch(3, 1);

		layout->addWidget(row);

		ui.row = row;
		ui.visibleCheck = visibleCheck;
		ui.labelLbl = labelLbl;
		ui.homeSpin = homeSpin;
		ui.guestSpin = guestSpin;
		ui.minusHome = minusHome;
		ui.plusHome = plusHome;
		ui.minusGuest = minusGuest;
		ui.plusGuest = plusGuest;

		auto sync = [this]() {
			syncStateFromUi();
		};

		connect(homeSpin, qOverload<int>(&QSpinBox::valueChanged), this, [sync](int) { sync(); });
		connect(guestSpin, qOverload<int>(&QSpinBox::valueChanged), this, [sync](int) { sync(); });
		connect(visibleCheck, &QCheckBox::toggled, this, [sync](bool) { sync(); });

		connect(minusHome, &QToolButton::clicked, this, [homeSpin, sync]() {
			homeSpin->setValue(std::max(0, homeSpin->value() - 1));
			sync();
		});
		connect(plusHome, &QToolButton::clicked, this, [homeSpin, sync]() {
			homeSpin->setValue(homeSpin->value() + 1);
			sync();
		});
		connect(minusGuest, &QToolButton::clicked, this, [guestSpin, sync]() {
			guestSpin->setValue(std::max(0, guestSpin->value() - 1));
			sync();
		});
		connect(plusGuest, &QToolButton::clicked, this, [guestSpin, sync]() {
			guestSpin->setValue(guestSpin->value() + 1);
			sync();
		});

		teamsStats.push_back(ui);
	}

	layout->addStretch(1);
}

void FlyTeamsStatsTab::syncStateFromUi()
{
	state_.stTeamsStats.clear();
	state_.stTeamsStats.reserve(teamsStats.size());

	for (const auto &ui : teamsStats) {
		FlyScoreState::FlyStat cf;
		cf.label = ui.labelLbl ? ui.labelLbl->text() : QString();
		cf.first = ui.homeSpin ? ui.homeSpin->value() : 0;
		cf.second = ui.guestSpin ? ui.guestSpin->value() : 0;
		cf.visible = ui.visibleCheck ? ui.visibleCheck->isChecked() : true;
		state_.stTeamsStats.push_back(cf);
	}

	dock_->saveState();
}

void FlyTeamsStatsTab::bumpTeamStat(int index, int delta, bool isHome)
{
	if (index < 0 || index >= state_.stTeamsStats.size())
		return;

	if (index < teamsStats.size() &&
	    ((isHome && teamsStats[index].homeSpin) || (!isHome && teamsStats[index].guestSpin))) {
		auto *spin = isHome ? teamsStats[index].homeSpin : teamsStats[index].guestSpin;
		spin->setValue(std::max(0, spin->value() + delta));
		return;
	}

	state_.stTeamsStats[index].first = std::max(0, state_.stTeamsStats[index].first + delta);
	dock_->saveState();
}

void FlyTeamsStatsTab::toggleTeamStatVisible(int index)
{
	if (index < 0 || index >= state_.stTeamsStats.size())
		return;

	if (index < teamsStats.size() && teamsStats[index].visibleCheck) {
		auto *chk = teamsStats[index].visibleCheck;
		chk->setChecked(!chk->isChecked());
		return;
	}

	state_.stTeamsStats[index].visible = !state_.stTeamsStats[index].visible;
	dock_->saveState();
}
