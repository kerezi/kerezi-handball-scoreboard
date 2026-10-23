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
#include "fly_score_stats_dialog.hpp"
#include "fly_score_timers_dialog.hpp"
#include "fly_score_hotkeys_dialog.hpp"
#include "fly_score_paths.hpp"

#include "fly_score_config_tab.hpp"
#include "fly_score_stats_tab.hpp"

#include "fly_score_handball_tab.hpp"

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
	teamsStatsTab_ = new FlyStatsTab("tabs", false, dataDir_, st_, this, tabs);
	matchStatsTab_ = new FlyStatsTab("tabs", true, dataDir_, st_, this, tabs);

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

	handballTab_ = new FlyHandballTab("tabs", dataDir_, st_, this, tabs);

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
	//tabs->addTab(teamsStatTab_, fly_i18n("Common.TeamStats"));
	//tabs->addTab(singleTab, fly_i18n("Common.MatchStats"));
	//tabs->addTab(timersTab, fly_i18n("Common.Timers"));
	//tabs->addTab(eventsTab, fly_i18n("Common.Events"));
	//tabs->addTab(handballTab, fly_i18n("Common.Handball"));

	// 👥
	tabs->addTab(configTab_, QStringLiteral("⚙️"));
	tabs->addTab(teamsStatsTab_, QStringLiteral("📊"));
	tabs->addTab(matchStatsTab_, QStringLiteral("📈"));
	tabs->addTab(timersTab, QStringLiteral("⏱️"));
	tabs->addTab(eventsTab, QStringLiteral("🎫"));
	tabs->addTab(handballTab_, QStringLiteral("🤾‍♀️"));

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

	const int commonFields = static_cast<int>(std::min(before.stTeamsStats.size(), after.stTeamsStats.size()));
	for (int i = 0; i < commonFields; ++i) {
		const auto &oldStat = before.stTeamsStats[i];
		const auto &stat = after.stTeamsStats[i];
		const QString label = labelOr(stat.label, QStringLiteral("Score %1").arg(i + 1));
		if (oldStat.first != stat.first) {
			eventLog_.appendEvent(QStringLiteral("%1: %2 (%3-%4)")
						      .arg(label, teamName(after.home, QStringLiteral("Home")))
						      .arg(stat.first)
						      .arg(stat.second));
		}
		if (oldStat.second != stat.second) {
			eventLog_.appendEvent(QStringLiteral("%1: %2 (%3-%4)")
						      .arg(label, teamName(after.guest, QStringLiteral("Guest")))
						      .arg(stat.first)
						      .arg(stat.second));
		}
		if (oldStat.visible != stat.visible)
			eventLog_.appendEvent(QStringLiteral("%1 %2").arg(
				label, stat.visible ? QStringLiteral("shown") : QStringLiteral("hidden")));
	}
	if (after.stTeamsStats.size() != before.stTeamsStats.size())
		eventLog_.appendEvent(QStringLiteral("Team stat configuration changed"));

	const int commonMatchStats = static_cast<int>(std::min(before.stMatchStats.size(), after.stMatchStats.size()));
	for (int i = 0; i < commonMatchStats; ++i) {
		const auto &oldStat = before.stMatchStats[i];
		const auto &stat = after.stMatchStats[i];
		if (oldStat.first != stat.first)
			eventLog_.appendEvent(QStringLiteral("%1: %2")
						      .arg(labelOr(stat.label, QStringLiteral("Stat %1").arg(i + 1)))
						      .arg(stat.first));
	}
	if (after.stMatchStats.size() != before.stMatchStats.size())
		eventLog_.appendEvent(QStringLiteral("Match stat configuration changed"));

	const int commonTimers = static_cast<int>(std::min(before.timers.size(), after.timers.size()));
	for (int i = 0; i < commonTimers; ++i) {
		const auto &oldTimer = before.timers[i];
		const auto &timer = after.timers[i];
		const QString label = labelOr(timer.label, QStringLiteral("Timer %1").arg(i + 1));
		if (oldTimer.running != timer.running)
			eventLog_.appendEvent(QStringLiteral("%1 %2").arg(
				label, timer.running ? QStringLiteral("Start") : QStringLiteral("Pause")));
		else if (!timer.running && oldTimer.current_moment_ms != timer.current_moment_ms)
			eventLog_.appendEvent(
				QStringLiteral("%1 set to %2 seconds").arg(label).arg(timer.current_moment_ms / 1000));
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
		main.current_moment_ms = 0;
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
	teamsStatsTab_->syncUiFromState();
	matchStatsTab_->syncUiFromState();
	handballTab_->syncUiFromState();

	loadTimerControlsFromState();
	// loadHandballFromState();
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
		main.current_moment_ms = 0;
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
		timeEdit->setPlaceholderText(QStringLiteral("mm:ms"));
		timeEdit->setClearButtonEnabled(true);
		timeEdit->setMaxLength(8);
		timeEdit->setMinimumWidth(60);
		timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(tm.current_moment_ms));

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
				timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(t.current_moment_ms));
				return;
			}

			qint64 ms = FlyQtHelper::fly_parse_mmss_to_ms(timeEdit->text());
			if (ms < 0) {
				timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(t.current_moment_ms));
				return;
			}

			t.initial_ms = ms;
			t.current_moment_ms = ms;
			saveState();
			timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(t.current_moment_ms));
		});

		connect(startStopBtn, &QPushButton::clicked, this, [this, i]() { configTab_->toggleTimerRunning(i); });

		connect(resetBtn, &QPushButton::clicked, this, [this, i]() {
			if (i < 0 || i >= st_.timers.size())
				return;

			FlyScoreState::FlyTimer &t = st_.timers[i];
			qint64 ms = t.initial_ms;
			if (ms < 0)
				ms = 0;

			t.current_moment_ms = ms;
			t.running = false;
			t.last_tick_ms = 0;

			saveState();
			refreshUiFromState(false);
		});

		timers_.push_back(ui);
	}
	timersLayout_->addStretch(1);
}
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


// void FlyScoreDock::loadHandballFromState()
// {



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

void FlyScoreDock::bumpStat(int index, int delta, bool isHome, bool isMatchStat)
{
	if (isMatchStat)
		matchStatsTab_->bumpStat(index, delta, isHome);
	else
		teamsStatsTab_->bumpStat(index, delta, isHome);
}

void FlyScoreDock::toggleStatVisible(int index, bool isMatchStat)
{
	if (isMatchStat)
		matchStatsTab_->toggleStatVisible(index);
	else
		teamsStatsTab_->toggleStatVisible(index);
}

void FlyScoreDock::toggleTimerRunning(int index)
{
	configTab_->toggleTimerRunning(index);
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
