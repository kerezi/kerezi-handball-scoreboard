#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][dock]"
#include "fly_score_log.hpp"

#include "fly_score_horizontal_tab_bar.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_state.hpp"
#include "fly_score_const.hpp"
#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_logo_helpers.hpp"
#include "fly_score_match_info_dialog.hpp"
#include "fly_score_teams_dialog.hpp"
#include "fly_score_players_dialog.hpp"
#include "fly_score_fields_dialog.hpp"
#include "fly_score_timers_dialog.hpp"
#include "fly_score_hotkeys_dialog.hpp"
#include "fly_score_paths.hpp"

#include "fly_score_config_tab.hpp"

#ifdef ENABLE_EMBEDDED_DEFAULTS
#include "embedded_assets.hpp"
#endif

#include <obs.h>
#ifdef ENABLE_FRONTEND_API
#include <obs-frontend-api.h>
#endif

#include <QAbstractButton>
#include <QBoxLayout>
#include <QCheckBox>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QSettings>
#include <QGridLayout>
#include <QGroupBox>
#include <QHash>
#include <QJsonObject>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QComboBox>
#include <QDesktopServices>
#include <QMetaObject>
#include <QShortcut>
#include <QSizePolicy>
#include <QSpinBox>
#include <QTabWidget>
#include <QTextStream>
#include <QSpacerItem>
#include <algorithm>
#include <limits>
#include <QToolButton>
#include <QUrl>

#ifdef ENABLE_FRONTEND_API
static void fly_on_frontend_event(enum obs_frontend_event event, void *data)
{
	auto *self = static_cast<FlyScoreDock *>(data);
	if (!self)
		return;
	QMetaObject::invokeMethod(
		self, [self, event]() { self->handleFrontendEvent(static_cast<int>(event)); }, Qt::QueuedConnection);
}
#endif

FlyScoreDock::FlyScoreDock(QWidget *parent) : QWidget(parent)
{
	QSizePolicy sp = sizePolicy();
	sp.setHorizontalPolicy(QSizePolicy::Preferred);
	sp.setVerticalPolicy(QSizePolicy::Expanding);
	setSizePolicy(sp);
}

bool FlyScoreDock::init()
{
	dataDir_ = fly_get_data_root();

	loadState();
	lastLoggedState_ = st_;
	ensureResourcesDefaults();

	setObjectName(fly_dock_name());
	setAttribute(Qt::WA_StyledBackground, true);
	setStyleSheet(QStringLiteral("FlyScoreDock { background: rgba(39, 42, 51, 1.0)}"));

	auto *tabs = new FlyHorizontalTabWidget(this);
	tabs->setTabPosition(QTabWidget::West);
	tabs->setObjectName(QStringLiteral("flyScoreTabs"));
	tabs->setDocumentMode(true);
	tabs->setMovable(false);
	tabs->setUsesScrollButtons(false);
	tabs->setWindowTitle(fly_i18n("Common.Scoreboard"));
	tabs->tabBar()->setStyleSheet("QTabBar {font-size:22pt;}");
	tabs->tabBar()->setAutoHide(true);

	configTab_ = new FlyConfigTab("tabs", dataDir_, st_, this, tabs);

	auto *teamTab = new QWidget(tabs);
	auto *teamVBox = new QVBoxLayout(teamTab);
	teamVBox->setContentsMargins(0, 0, 0, 0);
	teamVBox->setSpacing(4);
	teamVBox->setAlignment(Qt::AlignTop);
	customFieldsLayout_ = teamVBox;

	auto *singleTab = new QWidget(tabs);
	auto *singleVBox = new QVBoxLayout(singleTab);
	singleVBox->setContentsMargins(0, 0, 0, 0);
	singleVBox->setSpacing(4);
	singleVBox->setAlignment(Qt::AlignTop);
	singleStatsLayout_ = singleVBox;

	auto *timersTab = new QWidget(tabs);
	auto *timersVBox = new QVBoxLayout(timersTab);
	timersVBox->setContentsMargins(0, 0, 0, 0);
	timersVBox->setSpacing(4);
	timersVBox->setAlignment(Qt::AlignTop);
	timersLayout_ = timersVBox;

	auto *eventsTab = new QWidget(tabs);
	auto *eventsVBox = new QVBoxLayout(eventsTab);
	eventsVBox->setContentsMargins(6, 6, 6, 6);
	eventsVBox->setSpacing(6);

	auto *handballTab = new QWidget(tabs);
	auto *handballGL = new QGridLayout(handballTab);
	handballGL->setContentsMargins(0, 0, 0, 0);
	handballGL->setHorizontalSpacing(6);
	handballGL->setVerticalSpacing(0);
	// handballLayout_ = handballGL;

	auto *eventsTop = new QHBoxLayout();
	eventLogStatus_ = new QLabel(eventsTab);
	eventTimestampMode_ = new QComboBox(eventsTab);
	eventTimestampMode_->addItem(fly_i18n("Events.RelativeTime"), QStringLiteral("relative"));
	eventTimestampMode_->addItem(fly_i18n("Events.WallClock"), QStringLiteral("clock"));
	QSettings eventSettings(fly_settings_org_name(), fly_settings_app_name());
	const QString savedMode =
		eventSettings.value(fly_settings_key_event_timestamp_mode(), QStringLiteral("relative")).toString();
	eventTimestampMode_->setCurrentIndex(savedMode == QLatin1String("clock") ? 1 : 0);
	eventLog_.setTimestampMode(savedMode == QLatin1String("clock") ? FlyScoreEventLog::TimestampMode::WallClock
								       : FlyScoreEventLog::TimestampMode::Relative);
	eventLogToggle_ = new QPushButton(eventsTab);
	auto *eventsFolder = new QPushButton(fly_i18n("Events.OpenFolder"), eventsTab);
	connect(eventsFolder, &QPushButton::clicked, this, [this]() {
		QDir().mkpath(eventLogsDirectory());
		QDesktopServices::openUrl(QUrl::fromLocalFile(eventLogsDirectory()));
	});

	eventsTop->addWidget(eventLogStatus_, 1);
	eventsTop->addWidget(eventTimestampMode_);
	eventsTop->addWidget(eventLogToggle_);
	eventsTop->addWidget(eventsFolder);

	auto *eventsEntry = new QHBoxLayout();
	eventText_ = new QLineEdit(eventsTab);
	eventText_->setPlaceholderText(fly_i18n("Events.Placeholder"));
	eventAdd_ = new QPushButton(fly_i18n("Events.Add"), eventsTab);
	eventsEntry->addWidget(eventText_, 1);
	eventsEntry->addWidget(eventAdd_);

	auto *eventsHint = new QLabel(fly_i18n("Events.Hint"), eventsTab);
	eventsHint->setWordWrap(true);
	eventsVBox->addLayout(eventsTop);
	eventsVBox->addLayout(eventsEntry);
	eventsVBox->addWidget(eventsHint);
	eventsVBox->addStretch(1);

	//tabs->addTab(configTab, fly_i18n("Common.Configuration"));
	//tabs->addTab(teamTab, fly_i18n("Common.TeamStats"));
	//tabs->addTab(singleTab, fly_i18n("Common.SingleStats"));
	//tabs->addTab(timersTab, fly_i18n("Common.Timers"));
	//tabs->addTab(eventsTab, fly_i18n("Common.Events"));
	//tabs->addTab(handballTab, fly_i18n("Common.Handball"));

	// 👥
	tabs->addTab(configTab_, QStringLiteral("⚙️"));
	tabs->addTab(teamTab, QStringLiteral("📊"));
	tabs->addTab(singleTab, QStringLiteral("📈"));
	tabs->addTab(timersTab, QStringLiteral("⏱️"));
	tabs->addTab(eventsTab, QStringLiteral("🎫"));
	tabs->addTab(handballTab, QStringLiteral("🤾‍♀️"));

	connect(eventLogToggle_, &QPushButton::clicked, this, [this]() {
		if (eventLog_.isActive())
			stopEventLog();
		else
			startEventLog();
	});

	connect(eventAdd_, &QPushButton::clicked, this, [this]() {
		if (!eventText_)
			return;
		appendEvent(eventText_->text());
		eventText_->clear();
	});
	connect(eventText_, &QLineEdit::returnPressed, eventAdd_, &QPushButton::click);
	connect(eventTimestampMode_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int index) {
		const bool wallClock = index == 1;
		eventLog_.setTimestampMode(wallClock ? FlyScoreEventLog::TimestampMode::WallClock
						     : FlyScoreEventLog::TimestampMode::Relative);
		QSettings settings(fly_settings_org_name(), fly_settings_app_name());
		settings.setValue(fly_settings_key_event_timestamp_mode(),
				  wallClock ? QStringLiteral("clock") : QStringLiteral("relative"));
	});
	refreshEventLogUi();

#ifdef ENABLE_FRONTEND_API
	obs_frontend_add_event_callback(fly_on_frontend_event, this);
	frontendEventCallbackConnected_ = true;
	if (obs_frontend_streaming_active())
		startEventLog(QStringLiteral("Stream Start"));
	else if (obs_frontend_recording_active())
		startEventLog(QStringLiteral("Recording Start"));
#endif

	refreshUiFromState(false);
	configTab_->broadcastCurrentState();

	return true;
}

FlyScoreDock::~FlyScoreDock()
{
#ifdef ENABLE_FRONTEND_API
	if (frontendEventCallbackConnected_) {
		obs_frontend_remove_event_callback(fly_on_frontend_event, this);
		frontendEventCallbackConnected_ = false;
	}
#endif
}

QString FlyScoreDock::eventLogsDirectory() const
{
	return QDir(FlyScoreState::fly_data_dir()).filePath(QStringLiteral("event-logs"));
}

void FlyScoreDock::refreshEventLogUi()
{
	if (!eventLogStatus_ || !eventLogToggle_)
		return;

	if (eventLog_.isActive()) {
		const QFileInfo info(eventLog_.filePath());
		eventLogStatus_->setText(fly_i18n("Events.Recording").arg(info.fileName()));
		eventLogStatus_->setToolTip(eventLog_.filePath());
		eventLogToggle_->setText(fly_i18n("Events.Stop"));
	} else {
		eventLogStatus_->setText(fly_i18n("Events.NotRecording"));
		eventLogStatus_->setToolTip(eventLog_.filePath());
		eventLogToggle_->setText(fly_i18n("Events.Start"));
	}
	if (eventTimestampMode_)
		eventTimestampMode_->setEnabled(!eventLog_.isActive());
}

void FlyScoreDock::startEventLog(const QString &firstEvent)
{
	if (eventLog_.isActive())
		return;

	lastLoggedState_ = st_;
	if (!eventLog_.startSession(eventLogsDirectory(), firstEvent)) {
		if (eventLogStatus_)
			eventLogStatus_->setText(fly_i18n("Events.WriteFailed"));
		return;
	}
	refreshEventLogUi();
}

void FlyScoreDock::stopEventLog(const QString &lastEvent)
{
	if (!eventLog_.isActive())
		return;
	if (!lastEvent.trimmed().isEmpty())
		eventLog_.appendEvent(lastEvent);
	eventLog_.stopSession();
	refreshEventLogUi();
}

void FlyScoreDock::appendEvent(const QString &event)
{
	const QString clean = event.simplified();
	if (clean.isEmpty())
		return;
	if (!eventLog_.isActive())
		startEventLog(QStringLiteral("Log Start"));
	if (eventLog_.isActive())
		eventLog_.appendEvent(clean);
	refreshEventLogUi();
}

void FlyScoreDock::handleFrontendEvent(int event)
{
#ifdef ENABLE_FRONTEND_API
	if (event == OBS_FRONTEND_EVENT_STREAMING_STARTED) {
		if (eventLog_.isActive())
			eventLog_.appendEvent(QStringLiteral("Stream Start"));
		else
			startEventLog(QStringLiteral("Stream Start"));
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_STARTED) {
		if (eventLog_.isActive())
			eventLog_.appendEvent(QStringLiteral("Recording Start"));
		else
			startEventLog(QStringLiteral("Recording Start"));
	} else if (event == OBS_FRONTEND_EVENT_STREAMING_STOPPED) {
		if (eventLog_.isActive()) {
			eventLog_.appendEvent(QStringLiteral("Stream End"));
			if (!obs_frontend_recording_active())
				eventLog_.stopSession();
		}
	} else if (event == OBS_FRONTEND_EVENT_RECORDING_STOPPED) {
		if (eventLog_.isActive()) {
			eventLog_.appendEvent(QStringLiteral("Recording End"));
			if (!obs_frontend_streaming_active())
				eventLog_.stopSession();
		}
	}
	refreshEventLogUi();
#else
	Q_UNUSED(event);
#endif
}

void FlyScoreDock::logStateChanges(const FlyScoreState::FlyState &before, const FlyScoreState::FlyState &after)
{
	if (!eventLog_.isActive())
		return;

	auto teamName = [](const FlyScoreState::FlyTeam &team, const QString &fallback) {
		return team.title.trimmed().isEmpty() ? fallback : team.title.trimmed();
	};
	auto labelOr = [](const QString &label, const QString &fallback) {
		return label.trimmed().isEmpty() ? fallback : label.trimmed();
	};

	if (before.home.title != after.home.title)
		eventLog_.appendEvent(
			QStringLiteral("Home team: %1").arg(teamName(after.home, QStringLiteral("Home"))));
	if (before.guest.title != after.guest.title)
		eventLog_.appendEvent(
			QStringLiteral("Guest team: %1").arg(teamName(after.guest, QStringLiteral("Guest"))));
	if (before.swap_sides != after.swap_sides)
		eventLog_.appendEvent(after.swap_sides ? QStringLiteral("Sides swapped")
						       : QStringLiteral("Sides restored"));
	if (before.show_scoreboard != after.show_scoreboard)
		eventLog_.appendEvent(after.show_scoreboard ? QStringLiteral("Scoreboard shown")
							    : QStringLiteral("Scoreboard hidden"));

	const int commonFields = static_cast<int>(std::min(before.custom_fields.size(), after.custom_fields.size()));
	for (int i = 0; i < commonFields; ++i) {
		const auto &oldField = before.custom_fields[i];
		const auto &field = after.custom_fields[i];
		const QString label = labelOr(field.label, QStringLiteral("Score %1").arg(i + 1));
		if (oldField.first != field.first) {
			eventLog_.appendEvent(QStringLiteral("%1: %2 (%3-%4)")
						      .arg(label, teamName(after.home, QStringLiteral("Home")))
						      .arg(field.first)
						      .arg(field.second));
		}
		if (oldField.second != field.second) {
			eventLog_.appendEvent(QStringLiteral("%1: %2 (%3-%4)")
						      .arg(label, teamName(after.guest, QStringLiteral("Guest")))
						      .arg(field.first)
						      .arg(field.second));
		}
		if (oldField.visible != field.visible)
			eventLog_.appendEvent(QStringLiteral("%1 %2").arg(
				label, field.visible ? QStringLiteral("shown") : QStringLiteral("hidden")));
	}
	if (after.custom_fields.size() != before.custom_fields.size())
		eventLog_.appendEvent(QStringLiteral("Team stat configuration changed"));

	const int commonSingles = static_cast<int>(std::min(before.single_stats.size(), after.single_stats.size()));
	for (int i = 0; i < commonSingles; ++i) {
		const auto &oldStat = before.single_stats[i];
		const auto &stat = after.single_stats[i];
		if (oldStat.first != stat.first)
			eventLog_.appendEvent(QStringLiteral("%1: %2")
						      .arg(labelOr(stat.label, QStringLiteral("Stat %1").arg(i + 1)))
						      .arg(stat.first));
	}
	if (after.single_stats.size() != before.single_stats.size())
		eventLog_.appendEvent(QStringLiteral("Single stat configuration changed"));

	const int commonTimers = static_cast<int>(std::min(before.timers.size(), after.timers.size()));
	for (int i = 0; i < commonTimers; ++i) {
		const auto &oldTimer = before.timers[i];
		const auto &timer = after.timers[i];
		const QString label = labelOr(timer.label, QStringLiteral("Timer %1").arg(i + 1));
		if (oldTimer.running != timer.running)
			eventLog_.appendEvent(QStringLiteral("%1 %2").arg(
				label, timer.running ? QStringLiteral("Start") : QStringLiteral("Pause")));
		else if (!timer.running && oldTimer.remaining_ms != timer.remaining_ms)
			eventLog_.appendEvent(
				QStringLiteral("%1 set to %2 seconds").arg(label).arg(timer.remaining_ms / 1000));
		if (oldTimer.visible != timer.visible)
			eventLog_.appendEvent(QStringLiteral("%1 %2").arg(
				label, timer.visible ? QStringLiteral("shown") : QStringLiteral("hidden")));
	}
	if (after.timers.size() != before.timers.size())
		eventLog_.appendEvent(QStringLiteral("Timer configuration changed"));

	lastLoggedState_ = after;
}

void FlyScoreDock::loadState()
{
	if (!FlyScoreState::fly_state_load(dataDir_, st_)) {
		st_ = FlyScoreState::fly_state_make_defaults();
		FlyScoreState::fly_state_save(dataDir_, st_);
	}

	if (st_.timers.isEmpty()) {
		FlyScoreState::FlyTimer main;
		main.label = fly_i18n("Default.Timer.FirstHalf");
		main.mode = QStringLiteral("countup");
		main.running = false;
		main.initial_ms = 0;
		main.remaining_ms = 0;
		main.last_tick_ms = 0;
		st_.timers.push_back(main);
		FlyScoreState::fly_state_save(dataDir_, st_);
	} else if (st_.timers[0].mode.isEmpty()) {
		st_.timers[0].mode = QStringLiteral("countup");
	}
}

void FlyScoreDock::saveState()
{
	logStateChanges(lastLoggedState_, st_);
	FlyScoreState::fly_state_save(dataDir_, st_);
	lastLoggedState_ = st_;
	configTab_->broadcastCurrentState();
}

void FlyScoreDock::refreshUiFromState(bool onlyTimeIfRunning)
{
	Q_UNUSED(onlyTimeIfRunning);

	configTab_->refreshUiFromState(onlyTimeIfRunning);

	loadCustomFieldControlsFromState();
	loadSingleStatControlsFromState();
	loadTimerControlsFromState();
	// loadHandballFromState();
}

void FlyScoreDock::clearAllCustomFieldRows()
{
	customFields_.clear();
	if (!customFieldsLayout_)
		return;

	while (QLayoutItem *item = customFieldsLayout_->takeAt(0)) {
		if (QWidget *w = item->widget())
			w->deleteLater();
		delete item;
	}
}

void FlyScoreDock::loadCustomFieldControlsFromState()
{
	clearAllCustomFieldRows();
	if (!customFieldsLayout_)
		return;

	customFields_.reserve(st_.custom_fields.size());

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

		customFieldsLayout_->addWidget(hdrRow);
	}

	for (int i = 0; i < st_.custom_fields.size(); ++i) {
		const auto &cf = st_.custom_fields[i];
		FlyCustomFieldUi ui;

		auto *row = new QWidget(this);
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

		customFieldsLayout_->addWidget(row);

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
			syncCustomFieldControlsToState();
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

		customFields_.push_back(ui);
	}

	customFieldsLayout_->addStretch(1);
}

void FlyScoreDock::syncCustomFieldControlsToState()
{
	st_.custom_fields.clear();
	st_.custom_fields.reserve(customFields_.size());

	for (const auto &ui : customFields_) {
		FlyScoreState::FlyStat cf;
		cf.label = ui.labelLbl ? ui.labelLbl->text() : QString();
		cf.first = ui.homeSpin ? ui.homeSpin->value() : 0;
		cf.second = ui.guestSpin ? ui.guestSpin->value() : 0;
		cf.visible = ui.visibleCheck ? ui.visibleCheck->isChecked() : true;
		st_.custom_fields.push_back(cf);
	}

	saveState();
}

void FlyScoreDock::clearAllSingleStatRows()
{
	if (singleStatsLayout_) {
		QLayoutItem *it = nullptr;
		while ((it = singleStatsLayout_->takeAt(0)) != nullptr) {
			if (QWidget *w = it->widget())
				w->deleteLater();
			delete it;
		}
	}

	singleStats_.clear();
}

void FlyScoreDock::loadSingleStatControlsFromState()
{
	clearAllSingleStatRows();
	if (!singleStatsLayout_)
		return;

	singleStats_.reserve(st_.single_stats.size());

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
			icon = this->style()->standardIcon(fallbackPixmap);

		if (!icon.isNull())
			btn->setIcon(icon);
		else
			btn->setText(fallbackText);

		return btn;
	};

	for (int i = 0; i < st_.single_stats.size(); ++i) {
		const auto &ss = st_.single_stats[i];
		FlySingleStatUi ui;

		auto *row = new QWidget(this);
		auto *lay = new QHBoxLayout(row);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(6);

		auto *visibleCheck = new QCheckBox(row);
		visibleCheck->setChecked(ss.visible);

		auto *labelLbl =
			new QLabel(ss.label.isEmpty() ? fly_i18n("Hotkey.SingleStatN").arg(i + 1) : ss.label, row);
		labelLbl->setMinimumWidth(120);

		auto *valueSpin = new QSpinBox(row);
		valueSpin->setRange(-9999, 9999);
		valueSpin->setValue(ss.first);
		valueSpin->setMinimumWidth(60);

		auto *minusBtn = makeStepBtn(QStringLiteral("list-remove"), QStyle::SP_ArrowDown, QStringLiteral("-"),
					     fly_i18n("Dock.LabelMinusOne").arg(labelLbl->text()), row);

		auto *plusBtn = makeStepBtn(QStringLiteral("list-add"), QStyle::SP_ArrowUp, QStringLiteral("+"),
					    fly_i18n("Dock.LabelPlusOne").arg(labelLbl->text()), row);

		const int h = valueSpin->sizeHint().height();
		minusBtn->setFixedSize(h, h);
		plusBtn->setFixedSize(h, h);

		lay->addWidget(visibleCheck, 0, Qt::AlignVCenter);
		lay->addWidget(labelLbl);
		lay->addStretch(1);
		lay->addWidget(minusBtn, 0, Qt::AlignVCenter);
		lay->addWidget(valueSpin, 0, Qt::AlignVCenter);
		lay->addWidget(plusBtn, 0, Qt::AlignVCenter);

		singleStatsLayout_->addWidget(row);

		ui.row = row;
		ui.visibleCheck = visibleCheck;
		ui.labelLbl = labelLbl;
		ui.valueSpin = valueSpin;
		ui.minusBtn = minusBtn;
		ui.plusBtn = plusBtn;

		auto sync = [this]() {
			syncSingleStatControlsToState();
		};

		connect(visibleCheck, &QCheckBox::toggled, this, [sync](bool) { sync(); });
		connect(valueSpin, qOverload<int>(&QSpinBox::valueChanged), this, [sync](int) { sync(); });

		connect(minusBtn, &QToolButton::clicked, this, [valueSpin, sync]() {
			valueSpin->setValue(valueSpin->value() - 1);
			sync();
		});

		connect(plusBtn, &QToolButton::clicked, this, [valueSpin, sync]() {
			valueSpin->setValue(valueSpin->value() + 1);
			sync();
		});

		singleStats_.push_back(ui);
	}

	singleStatsLayout_->addStretch(1);
}

void FlyScoreDock::syncSingleStatControlsToState()
{
	st_.single_stats.clear();
	st_.single_stats.reserve(singleStats_.size());

	for (const auto &ui : singleStats_) {
		FlyScoreState::FlyStat ss;
		ss.label = ui.labelLbl ? ui.labelLbl->text() : QString();
		ss.first = ui.valueSpin ? ui.valueSpin->value() : 0;
		ss.visible = ui.visibleCheck ? ui.visibleCheck->isChecked() : true;
		st_.single_stats.push_back(ss);
	}

	saveState();
}

void FlyScoreDock::clearAllTimerRows()
{
	if (timersLayout_) {
		QLayoutItem *it = nullptr;
		while ((it = timersLayout_->takeAt(0)) != nullptr) {
			if (QWidget *w = it->widget())
				w->deleteLater();
			delete it;
		}
	}
	timers_.clear();
}

void FlyScoreDock::loadTimerControlsFromState()
{
	clearAllTimerRows();
	if (!timersLayout_)
		return;

	if (st_.timers.isEmpty()) {
		FlyScoreState::FlyTimer main;
		main.label = fly_i18n("Default.Timer.FirstHalf");
		main.mode = QStringLiteral("countup");
		main.running = false;
		main.initial_ms = 0;
		main.remaining_ms = 0;
		main.last_tick_ms = 0;
		main.visible = true;
		st_.timers.push_back(main);
	}

	timers_.reserve(st_.timers.size());

	auto makeEmojiBtn = [](const QString &emoji, const QString &tooltip, QWidget *parent) {
		auto *btn = new QPushButton(parent);
		btn->setText(emoji);
		btn->setToolTip(tooltip);
		btn->setCursor(Qt::PointingHandCursor);
		btn->setStyleSheet("QPushButton {"
				   "  font-family:'Segoe UI Emoji','Noto Color Emoji','Apple Color Emoji',sans-serif;"
				   "  font-size:12px;"
				   "  padding:0;"
				   "}");
		return btn;
	};

	for (int i = 0; i < st_.timers.size(); ++i) {
		const auto &tm = st_.timers[i];
		FlyTimerUi ui;

		auto *row = new QWidget(this);
		auto *lay = new QHBoxLayout(row);
		lay->setContentsMargins(0, 0, 0, 0);
		lay->setSpacing(6);

		auto *visibleCheck = new QCheckBox(row);
		visibleCheck->setChecked(tm.visible);

		auto *labelLbl = new QLabel(tm.label.isEmpty() ? QStringLiteral("(unnamed)") : tm.label, row);
		labelLbl->setMinimumWidth(120);

		auto *timeEdit = new QLineEdit(row);
		timeEdit->setPlaceholderText(QStringLiteral("mm:ss"));
		timeEdit->setClearButtonEnabled(true);
		timeEdit->setMaxLength(8);
		timeEdit->setMinimumWidth(60);
		timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(tm.remaining_ms));

		auto *startStopBtn =
			makeEmojiBtn(tm.running ? QStringLiteral("⏸️") : QStringLiteral("▶️"),
				     tm.running ? fly_i18n("Dock.PauseTimer") : fly_i18n("Dock.StartTimer"), row);

		auto *resetBtn = makeEmojiBtn(QStringLiteral("🔄️"), fly_i18n("Dock.ResetTimer"), row);

		const int h = timeEdit->sizeHint().height();
		startStopBtn->setFixedSize(h, h);
		resetBtn->setFixedSize(h, h);

		lay->addWidget(visibleCheck, 0, Qt::AlignVCenter);
		lay->addWidget(labelLbl);
		lay->addStretch(1);
		lay->addWidget(timeEdit, 0, Qt::AlignVCenter);
		lay->addWidget(startStopBtn, 0, Qt::AlignVCenter);
		lay->addWidget(resetBtn, 0, Qt::AlignVCenter);

		row->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
		timersLayout_->addWidget(row);

		ui.row = row;
		ui.labelLbl = labelLbl;
		ui.timeEdit = timeEdit;
		ui.startStop = startStopBtn;
		ui.reset = resetBtn;
		ui.visibleCheck = visibleCheck;

		connect(visibleCheck, &QCheckBox::toggled, this, [this, i](bool on) {
			if (i < 0 || i >= st_.timers.size())
				return;
			st_.timers[i].visible = on;
			saveState();
		});

		connect(timeEdit, &QLineEdit::editingFinished, this, [this, i, timeEdit]() {
			if (i < 0 || i >= st_.timers.size())
				return;

			FlyScoreState::FlyTimer &t = st_.timers[i];
			if (t.running) {
				timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(t.remaining_ms));
				return;
			}

			qint64 ms = FlyQtHelper::fly_parse_mmss_to_ms(timeEdit->text());
			if (ms < 0) {
				timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(t.remaining_ms));
				return;
			}

			t.initial_ms = ms;
			t.remaining_ms = ms;
			saveState();
			timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(t.remaining_ms));
		});

		connect(startStopBtn, &QPushButton::clicked, this, [this, i]() { configTab_->toggleTimerRunning(i); });

		connect(resetBtn, &QPushButton::clicked, this, [this, i]() {
			if (i < 0 || i >= st_.timers.size())
				return;

			FlyScoreState::FlyTimer &t = st_.timers[i];
			qint64 ms = t.initial_ms;
			if (ms < 0)
				ms = 0;

			t.remaining_ms = ms;
			t.running = false;
			t.last_tick_ms = 0;

			saveState();
			refreshUiFromState(false);
		});

		timers_.push_back(ui);
	}
	timersLayout_->addStretch(1);
}

// void FlyScoreDock::loadHandballFromState()
// {
// 	// handballLayout_->
// 	// 1st row
// 	int posRow = 0;
// 	int posCol = 0;
// 	int curHei = 0;
// 	int curWid = 0;
// 	{
// 		QLabel *htln = new QLabel(fly_i18n("Common.Home"), handballLayout_);
// 		htln->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htln->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 4;
// 		handballLayout_->addWidget(htln, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htf2s = new QLabel(fly_i18n("Common.F2MinSusp"), handballLayout_);
// 		htf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htf2s->setAlignment(Qt::AlignCenter);
// 		curHei = 2;
// 		curWid = 1;
// 		handballLayout_->addWidget(htf2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *hts2s = new QLabel(fly_i18n("Common.F2MinSusp"), handballLayout_);
// 		hts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		hts2s->setAlignment(Qt::AlignCenter);
// 		curHei = 2;
// 		curWid = 1;
// 		handballLayout_->addWidget(hts2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htt2s = new QLabel(fly_i18n("Common.F2MinSusp"), handballLayout_);
// 		htt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htt2s->setAlignment(Qt::AlignCenter);
// 		curHei = 2;
// 		curWid = 1;
// 		handballLayout_->addWidget(htt2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *mt = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		mt->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight:bold;"));
// 		mt->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 6;
// 		handballLayout_->addWidget(mt, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtt2s = new QLabel(fly_i18n("Common.F2MinSusp"), handballLayout_);
// 		gtt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtt2s->setAlignment(Qt::AlignCenter);
// 		curHei = 2;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtt2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gts2s = new QLabel(fly_i18n("Common.F2MinSusp"), handballLayout_);
// 		gts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gts2s->setAlignment(Qt::AlignCenter);
// 		curHei = 2;
// 		curWid = 1;
// 		handballLayout_->addWidget(gts2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtf2s = new QLabel(fly_i18n("Common.F2MinSusp"), handballLayout_);
// 		gtf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtf2s->setAlignment(Qt::AlignCenter);
// 		curHei = 2;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtf2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtln = new QLabel(fly_i18n("Common.Guest"), handballLayout_);
// 		gtln->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtln->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 4;
// 		handballLayout_->addWidget(gtln, posRow, posCol, curHei, curWid);
// 		posCol += curWid;
// 	}

// 	posRow += 1;
// 	posCol = 0;
// 	// 2nd row
// 	{
// 		QLabel *htsn = new QLabel(fly_i18n("Common.Home") + " " +fly_i18n("Dock.Players"), handballLayout_);
// 		htsn->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htsn->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 3;
// 		handballLayout_->addWidget(htsn, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htny = new QLabel(fly_i18n("Common.NumberOfYellow"), handballLayout_);
// 		htny->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htny->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htny, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		posCol += 3;

// 		QLabel *htnd = new QLabel(fly_i18n("Common.NumberOfDisq"), handballLayout_);
// 		htnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htnb = new QLabel(fly_i18n("Common.NumberOfBlue"), handballLayout_);
// 		htnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *mr = new QLabel(fly_i18n("Common.MatchResult"), handballLayout_);
// 		mr->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		mr->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnb = new QLabel(fly_i18n("Common.NumberOfBlue"), handballLayout_);
// 		gtnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnd = new QLabel(fly_i18n("Common.NumberOfDisq"), handballLayout_);
// 		gtnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		posCol += 3;

// 		QLabel *gtny = new QLabel(fly_i18n("Common.NumberOfYellow"), handballLayout_);
// 		gtny->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtny->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtny, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtsn = new QLabel(fly_i18n("Common.Guest") + " " +fly_i18n("Dock.Players"), handballLayout_);
// 		gtsn->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtsn->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 3;
// 		handballLayout_->addWidget(gtsn, posRow, posCol, curHei, curWid);
// 		posCol += curWid;
// 	}

// 	posRow += 1;
// 	posCol = 0;
// 	// 3rd row to 18th row
// 	for (; posRow < 18; posRow++)
// 	{
// 		posCol = 0;

// 		QLabel *htptn = new QLabel(fly_i18n("Common.PlayerNumber"), handballLayout_);
// 		htptn->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htptn->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htptn, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htptna = new QLabel(fly_i18n("Common.Home") + " " + fly_i18n("Dock.Players") + " " + fly_i18n("Common.PlayerNumber") + " " + QString::number(posRow), handballLayout_);
// 		htptna->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htptna->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htptna, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htpy = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htpy->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htpy->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htpy, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htf2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htf2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htf2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *hts2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		hts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		hts2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(hts2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htt2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htt2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htt2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htnd = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htnb = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htng = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htng->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htng->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(htng, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtng = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtng->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtng->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtng, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnb = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnd = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtt2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtt2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtt2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gts2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gts2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gts2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtf2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtf2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtf2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtpy = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtpy->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtpy->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtpy, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtptna = new QLabel(fly_i18n("Common.Guest") + " " + fly_i18n("Dock.Players") + " " + fly_i18n("Common.PlayerNumber") + " " + QString::number(posRow), handballLayout_);
// 		gtptna->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtptna->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtptna, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtptn = new QLabel(fly_i18n("Common.PlayerNumber"), handballLayout_);
// 		gtptn->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtptn->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 1;
// 		handballLayout_->addWidget(gtptn, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 	}

// 	// posRow += 1;
// 	posCol = 0;
// 	// 19th row
// 	{
// 		QLabel *htsn = new QLabel(fly_i18n("Common.Home") + " " +fly_i18n("Teams.handballLayout_ =s"), handballLayout_);
// 		htsn->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htsn->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htsn, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htny = new QLabel(fly_i18n("Common.NumberOfYellow"), handballLayout_);
// 		htny->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htny->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htny, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *hto2m = new QLabel(fly_i18n("Common.handballLayout_ =2min"), handballLayout_);
// 		hto2m->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		hto2m->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(hto2m, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htnd = new QLabel(fly_i18n("Common.Official2min"), handballLayout_);
// 		htnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htnb = new QLabel(fly_i18n("Common.Official2min"), handballLayout_);
// 		htnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnb = new QLabel(fly_i18n("Common.Official2min"), handballLayout_);
// 		gtnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnd = new QLabel(fly_i18n("Common.Official2min"), handballLayout_);
// 		gtnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gt2os = new QLabel(fly_i18n("Common.Official2min"), handballLayout_);
// 		gt2os->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gt2os->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gt2os, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtny = new QLabel(fly_i18n("Common.NumberOfYellow"), handballLayout_);
// 		gtny->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtny->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtny, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtsn = new QLabel(fly_i18n("Common.Guest") + " " +fly_i18n("Teams.Officials"), handballLayout_);
// 		gtsn->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtsn->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtsn, posRow, posCol, curHei, curWid);
// 		posCol += curWid;
// 	}

// 	posRow += 1;
// 	posCol = 0;
// 	// 20th row to 24th row
// 	for (; posRow < 24; posRow++)
// 	{
// 		posCol = 0;

// 		QLabel *htptna = new QLabel(fly_i18n("Common.OfficialChar") + " " +fly_i18n("Common.Home") + " " + fly_i18n("Teams.Officials"), handballLayout_);
// 		htptna->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htptna->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htptna, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htpy = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htpy->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htpy->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htpy, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htf2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htf2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htf2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htnd = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *htnb = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		htnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		htnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnb = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnb->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtnb, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtnd = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtnd->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtnd, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtf2s = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtf2s->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtf2s, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtpy = new QLabel(fly_i18n("Common.MatchTimer"), handballLayout_);
// 		gtpy->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtpy->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtpy, posRow, posCol, curHei, curWid);
// 		posCol += curWid;

// 		QLabel *gtptna = new QLabel(fly_i18n("Common.OfficialChar") + " " +fly_i18n("Common.Guest") + " " + fly_i18n("Teams.Officials"), handballLayout_);
// 		gtptna->setStyleSheet(QStringLiteral("font-weight:bold;"));
// 		gtptna->setAlignment(Qt::AlignCenter);
// 		curHei = 1;
// 		curWid = 2;
// 		handballLayout_->addWidget(gtptna, posRow, posCol, curHei, curWid);
// 		posCol += curWid;
// 	}
// }

void FlyScoreDock::bumpCustomFieldHome(int index, int delta)
{
	if (index < 0 || index >= st_.custom_fields.size())
		return;

	if (index < customFields_.size() && customFields_[index].homeSpin) {
		auto *spin = customFields_[index].homeSpin;
		spin->setValue(std::max(0, spin->value() + delta));
		return;
	}

	st_.custom_fields[index].first = std::max(0, st_.custom_fields[index].first + delta);
	saveState();
}

void FlyScoreDock::bumpCustomFieldGuest(int index, int delta)
{
	if (index < 0 || index >= st_.custom_fields.size())
		return;

	if (index < customFields_.size() && customFields_[index].guestSpin) {
		auto *spin = customFields_[index].guestSpin;
		spin->setValue(std::max(0, spin->value() + delta));
		return;
	}

	st_.custom_fields[index].second = std::max(0, st_.custom_fields[index].second + delta);
	saveState();
}

void FlyScoreDock::bumpSingleStat(int idx, int delta)
{
	if (idx < 0 || idx >= st_.single_stats.size())
		return;

	FlyScoreState::FlyStat &ss = st_.single_stats[idx];
	ss.first += delta;

	saveState();
	loadSingleStatControlsFromState();
}

void FlyScoreDock::toggleSingleStatVisible(int idx)
{
	if (idx < 0 || idx >= st_.single_stats.size())
		return;

	FlyScoreState::FlyStat &ss = st_.single_stats[idx];
	ss.visible = !ss.visible;

	saveState();
	loadSingleStatControlsFromState();
}

void FlyScoreDock::toggleCustomFieldVisible(int index)
{
	if (index < 0 || index >= st_.custom_fields.size())
		return;

	if (index < customFields_.size() && customFields_[index].visibleCheck) {
		auto *chk = customFields_[index].visibleCheck;
		chk->setChecked(!chk->isChecked());
		return;
	}

	st_.custom_fields[index].visible = !st_.custom_fields[index].visible;
	saveState();
}

void FlyScoreDock::ensureResourcesDefaults()
{
	QString resDir = fly_get_data_root_no_ui();
	if (resDir.isEmpty())
		resDir = dataDir_;

#ifdef ENABLE_EMBEDDED_DEFAULTS
	auto writeDefaultText = [](const QString &path, const char *contents) {
		if (QFileInfo::exists(path) || !contents)
			return;
		QDir().mkpath(QFileInfo(path).absolutePath());
		QFile f(path);
		if (f.open(QIODevice::WriteOnly | QIODevice::Truncate))
			f.write(contents);
	};

	writeDefaultText(QDir(resDir).filePath(QStringLiteral("index.html")), fly_score_embedded::index_html);
	writeDefaultText(QDir(resDir).filePath(QStringLiteral("fouls.html")), fly_score_embedded::fouls_html);
	writeDefaultText(QDir(resDir).filePath(QStringLiteral("cards.html")), fly_score_embedded::cards_html);
	writeDefaultText(QDir(resDir).filePath(QStringLiteral("corners.html")), fly_score_embedded::corners_html);
	writeDefaultText(QDir(resDir).filePath(QStringLiteral("manifest.ini")), fly_score_embedded::manifest_ini);
	writeDefaultText(QDir(resDir).filePath(QStringLiteral("style.css")), fly_score_embedded::style_css);
	writeDefaultText(QDir(resDir).filePath(QStringLiteral("script.js")), fly_score_embedded::script_js);
#endif

	FlyScoreState::fly_state_ensure_json_exists(resDir, &st_);
}

void FlyScoreDock::activateTemplateFolder(const QString &path)
{
	if (path.isEmpty())
		return;

	FlyScoreState::FlyThemeInfo info = FlyScoreState::fly_read_theme_info(path);
	if (!info.valid) {
		LOGW("Invalid theme selected: path='%s', hasIndex=%d, hasManifest=%d, reason='%s'",
		     info.path.toUtf8().constData(), info.hasIndex ? 1 : 0, info.hasManifest ? 1 : 0,
		     info.error.toUtf8().constData());
		QMessageBox::warning(this, fly_i18n("Dock.TemplateInvalidTitle"),
				     fly_i18n("Dock.TemplateInvalidMessage"));
		return;
	}

	fly_set_data_root(path);
	dataDir_ = fly_get_data_root_no_ui();
	FlyScoreState::fly_state_ensure_json_exists(dataDir_, &st_);
	loadState();
	lastLoggedState_ = st_;
	if (eventLog_.isActive())
		eventLog_.appendEvent(QStringLiteral("Template folder activated: %1").arg(info.manifest.title));
	refreshUiFromState(false);
	configTab_->refreshActiveTemplateLabel();
	configTab_->broadcastCurrentState();
}
