#include "fly_score_config_tab.hpp"
#include "fly_score_log.hpp"
#include "fly_score_const.hpp"
#include "config.hpp"

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
#include <QMessageBox>
#include <QLabel>
#include <QDir>
#include <QDesktopServices>
#include <QShortcut>
#include <QSettings>
#include <QFileDialog>

FlyConfigTab::FlyConfigTab(const QString &title, const QString &dataDir, FlyScoreState::FlyState &state, FlyScoreDock *dock,
			   QWidget *parent)
	: QWidget(parent),
	  dataDir_(dataDir),
	  state_(state),
	  dock_(dock)
{
	init();
	buildUI();
}

void FlyConfigTab::init()
{
	hotkeyBindings_ = fly_hotkeys_load(dataDir_);
	applyHotkeyBindings(hotkeyBindings_);

	webSocketServer_ = new FlyScoreWebSocketServer(this);
	connect(webSocketServer_, &FlyScoreWebSocketServer::commandReceived, this, &FlyConfigTab::handleRemoteCommand);
	connect(webSocketServer_, &FlyScoreWebSocketServer::statusChanged, this, &FlyConfigTab::updateWebSocketStatus);
	webSocketServer_->start(FlyScoreWebSocketServer::fly_load_websocket_port());
	updateWebSocketStatus();
}

FlyConfigTab::~FlyConfigTab() = default;

void FlyConfigTab::buildUI()
{
	auto *configTabVBoxLayout = new QVBoxLayout(this);
	configTabVBoxLayout->setContentsMargins(0, 0, 0, 0);
	configTabVBoxLayout->setSpacing(4);
	configTabVBoxLayout->setAlignment(Qt::AlignTop);
	{
		auto *headerRow = new QHBoxLayout();
		headerRow->setContentsMargins(0, 0, 0, 0);
		headerRow->setSpacing(6);

		matchInfoBtn_ = new QPushButton(fly_i18n("Dock.MatchInfo"), this);
		matchInfoBtn_->setMinimumWidth(110);
		matchInfoBtn_->setMaximumWidth(130);
		matchInfoBtn_->setCursor(Qt::PointingHandCursor);

		teamsBtn_ = new QPushButton(fly_i18n("Dock.Teams"), this);
		teamsBtn_->setMinimumWidth(110);
		teamsBtn_->setMaximumWidth(130);
		teamsBtn_->setCursor(Qt::PointingHandCursor);

		playersBtn_ = new QPushButton(fly_i18n("Dock.Players"), this);
		playersBtn_->setMinimumWidth(110);
		playersBtn_->setMaximumWidth(130);
		playersBtn_->setCursor(Qt::PointingHandCursor);

		editFieldsBtn_ = new QPushButton(fly_i18n("Dock.StatsButton"), this);
		editFieldsBtn_->setMinimumWidth(110);
		editFieldsBtn_->setMaximumWidth(130);
		editFieldsBtn_->setCursor(Qt::PointingHandCursor);
		editFieldsBtn_->setToolTip(fly_i18n("Dock.ConfigureMatchStats"));

		editTimersBtn_ = new QPushButton(fly_i18n("Dock.TimersButton"), this);
		editTimersBtn_->setMinimumWidth(110);
		editTimersBtn_->setMaximumWidth(130);
		editTimersBtn_->setCursor(Qt::PointingHandCursor);
		editTimersBtn_->setToolTip(fly_i18n("Dock.ConfigureTimers"));

		headerRow->addWidget(matchInfoBtn_);
		headerRow->addWidget(teamsBtn_);
		headerRow->addWidget(playersBtn_);
		headerRow->addWidget(editFieldsBtn_);
		headerRow->addWidget(editTimersBtn_);
		headerRow->addStretch(1);

		configTabVBoxLayout->addLayout(headerRow);
	}

	{
		auto *togglesRow = new QHBoxLayout();
		togglesRow->setContentsMargins(0, 0, 0, 0);
		togglesRow->setSpacing(10);

		swapSides_ = new QCheckBox(fly_i18n("Hotkey.SwapSides"), this);
		showScoreboard_ = new QCheckBox(fly_i18n("Dock.ShowScoreboard"), this);

		togglesRow->addWidget(swapSides_);
		togglesRow->addWidget(showScoreboard_);
		togglesRow->addStretch(1);

		configTabVBoxLayout->addLayout(togglesRow);
	}
	auto *openOverlayFolderBtn = new QPushButton(fly_i18n("Dock.OpenOverlayFolder"), this);
	openOverlayFolderBtn->setCursor(Qt::PointingHandCursor);
	openOverlayFolderBtn->setToolTip(fly_i18n("Dock.OpenOverlayFolderTooltip"));

	auto *clearBtn = new QPushButton(QStringLiteral("🧹"), this);
	clearBtn->setCursor(Qt::PointingHandCursor);
	clearBtn->setToolTip(fly_i18n("Dock.ResetTooltip"));

	auto *hotkeysBtn = new QPushButton(QStringLiteral("⌨️"), this);
	hotkeysBtn->setCursor(Qt::PointingHandCursor);
	hotkeysBtn->setToolTip(fly_i18n("Dock.ConfigureHotkeys"));
	{
		auto *templateRow = new QHBoxLayout();
		templateRow->setContentsMargins(0, 0, 0, 0);
		templateRow->setSpacing(6);

		selectTemplateFolderBtn_ = new QPushButton(fly_i18n("Dock.SelectTemplateFolder"), this);
		selectTemplateFolderBtn_->setCursor(Qt::PointingHandCursor);
		selectTemplateFolderBtn_->setToolTip(fly_i18n("Dock.SelectTemplateFolderTooltip"));
		activeTemplateLabel_ = new QLabel(this);
		activeTemplateLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
		activeTemplateLabel_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

		webSocketStatus_ = new QLabel(this);
		webSocketStatus_->setTextInteractionFlags(Qt::TextSelectableByMouse);
		webSocketStatus_->setToolTip(fly_i18n("Dock.WebSocketTooltip"));

		templateRow->addWidget(selectTemplateFolderBtn_);
		templateRow->addWidget(activeTemplateLabel_, 1);
		templateRow->addWidget(openOverlayFolderBtn);
		templateRow->addWidget(webSocketStatus_);
		configTabVBoxLayout->addLayout(templateRow);

		auto *bottomRow = new QHBoxLayout();
		bottomRow->setContentsMargins(0, 0, 0, 0);
		bottomRow->setSpacing(6);

		bottomRow->addWidget(clearBtn);
		bottomRow->addStretch(1);
		bottomRow->addWidget(hotkeysBtn);

		configTabVBoxLayout->addLayout(bottomRow);
		configTabVBoxLayout->addStretch(1);
	}
	configTabLayout_ = configTabVBoxLayout;
	hotkeyBindings_ = buildMergedHotkeyBindings();

	refreshActiveTemplateLabel();

	connect(swapSides_, &QCheckBox::toggled, this, [this](bool on) {
		state_.swap_sides = on;
		dock_->saveState();
	});

	connect(clearBtn, &QPushButton::clicked, this, &FlyConfigTab::onClearTeamsAndReset);
	connect(openOverlayFolderBtn, &QPushButton::clicked, this, [this]() {
		const QString path = dataDir_;
		if (!path.isEmpty() && QDir(path).exists())
			QDesktopServices::openUrl(QUrl::fromLocalFile(path));
	});
	connect(hotkeysBtn, &QPushButton::clicked, this, &FlyConfigTab::openHotkeysDialog);
	connect(showScoreboard_, &QCheckBox::toggled, this, [this](bool on) {
		state_.show_scoreboard = on;
		dock_->saveState();
	});

	connect(selectTemplateFolderBtn_, &QPushButton::clicked, this, [this](bool on) {
		const QString cur = dataDir_.isEmpty() ? fly_get_data_root_no_ui() : dataDir_;
		const QString picked = QFileDialog::getExistingDirectory(this, fly_i18n("Dock.SelectTemplateFolderTitle"), cur);
		if (picked.isEmpty())
			return;

		dock_->activateTemplateFolder(picked);
	});

	connect(matchInfoBtn_, &QPushButton::clicked, this, [this](bool on) {onOpenDialog(new FlyMatchInfoDialog(dataDir_, state_, this));});
	connect(teamsBtn_, &QPushButton::clicked, this, [this](bool on) {onOpenDialog(new FlyTeamsDialog(dataDir_, state_, this));});
	connect(playersBtn_, &QPushButton::clicked, this, [this](bool on) {onOpenDialog(new FlyPlayersDialog(dataDir_, state_, this));});
	connect(editFieldsBtn_, &QPushButton::clicked, this, [this](bool on) {onOpenDialog(new FlyFieldsDialog(dataDir_, state_, this));});
	connect(editTimersBtn_, &QPushButton::clicked, this, [this](bool on) {onOpenDialog(new FlyTimersDialog(dataDir_, state_, this));});
}

void FlyConfigTab::onOpenDialog(QDialog *dlg)
{
	const FlyScoreState::FlyState before = state_;
	dlg->exec();

	dock_->loadState();
	dock_->logStateChanges(before, state_);
	refreshUiFromState(false);

	hotkeyBindings_ = buildMergedHotkeyBindings();
	applyHotkeyBindings(hotkeyBindings_);
}

void FlyConfigTab::refreshUiFromState(bool onlyTimeIfRunning)
{
	Q_UNUSED(onlyTimeIfRunning);

	if (swapSides_ && swapSides_->isChecked() != state_.swap_sides)
		swapSides_->setChecked(state_.swap_sides);
	if (showScoreboard_ && showScoreboard_->isChecked() != state_.show_scoreboard)
		showScoreboard_->setChecked(state_.show_scoreboard);
}

void FlyConfigTab::onClearTeamsAndReset()
{
	auto rc = QMessageBox::question(this, fly_i18n("Dock.ResetValuesTitle"), fly_i18n("Dock.ResetValuesMessage"));
	if (rc != QMessageBox::Yes)
		return;

	for (auto &cf : state_.custom_fields) {
		cf.first = 0;
		cf.second = 0;
	}

	for (auto &ss : state_.single_stats) {
		ss.first = 0;
	}

	for (auto &tm : state_.timers) {
		qint64 ms = tm.initial_ms;
		if (ms < 0)
			ms = 0;
		tm.remaining_ms = ms;
		tm.running = false;
		// tm.last_tick_ms = 0;
	}

	dock_->saveState();
	refreshUiFromState(false);
}

void FlyConfigTab::toggleSwap()
{
	if (swapSides_)
		swapSides_->setChecked(!swapSides_->isChecked());
	else {
		state_.swap_sides = !state_.swap_sides;
		dock_->saveState();
		refreshUiFromState(false);
	}
}

void FlyConfigTab::toggleScoreboardVisible()
{
	if (showScoreboard_)
		showScoreboard_->setChecked(!showScoreboard_->isChecked());
	else {
		state_.show_scoreboard = !state_.show_scoreboard;
		dock_->saveState();
		refreshUiFromState(false);
	}
}

void FlyConfigTab::clearAllShortcuts()
{
	for (auto *sc : shortcuts_) {
		if (sc)
			sc->deleteLater();
	}
	shortcuts_.clear();
}

QList<FlyHotkeyBinding> FlyConfigTab::buildMergedHotkeyBindings() const
{
	QList<FlyHotkeyBinding> merged = buildDefaultHotkeyBindings();

	if (hotkeyBindings_.isEmpty())
		return merged;

	QHash<QString, QKeySequence> existing;
	existing.reserve(hotkeyBindings_.size());
	for (const auto &b : hotkeyBindings_)
		existing.insert(b.actionId, b.sequence);

	for (auto &b : merged) {
		if (existing.contains(b.actionId))
			b.sequence = existing.value(b.actionId);
	}

	return merged;
}

QList<FlyHotkeyBinding> FlyConfigTab::buildDefaultHotkeyBindings() const
{
	QList<FlyHotkeyBinding> v;

	v.push_back({"swap_sides", fly_i18n("Hotkey.SwapSides"), QKeySequence()});
	v.push_back({"toggle_scoreboard", fly_i18n("Hotkey.ToggleScoreboard"), QKeySequence()});

	for (int i = 0; i < state_.custom_fields.size(); ++i) {
		const auto &cf = state_.custom_fields[i];
		const QString label = cf.label.isEmpty() ? fly_i18n("Hotkey.CustomFieldN").arg(i + 1) : cf.label;
		const QString baseId = QStringLiteral("field_%1").arg(i);

		v.push_back({baseId + "_toggle", fly_i18n("Hotkey.CustomToggle").arg(label), QKeySequence()});
		v.push_back({baseId + "_home_inc", fly_i18n("Hotkey.CustomHomeInc").arg(label), QKeySequence()});
		v.push_back({baseId + "_home_dec", fly_i18n("Hotkey.CustomHomeDec").arg(label), QKeySequence()});
		v.push_back({baseId + "_guestate_inc", fly_i18n("Hotkey.CustomGuestsInc").arg(label), QKeySequence()});
		v.push_back({baseId + "_guestate_dec", fly_i18n("Hotkey.CustomGuestsDec").arg(label), QKeySequence()});
	}

	for (int i = 0; i < state_.single_stats.size(); ++i) {
		const auto &ss = state_.single_stats[i];
		const QString label = ss.label.isEmpty() ? fly_i18n("Hotkey.SingleStatN").arg(i + 1) : ss.label;
		const QString baseId = QStringLiteral("single_%1").arg(i);

		v.push_back({baseId + "_toggle", fly_i18n("Hotkey.SingleToggle").arg(label), QKeySequence()});
		v.push_back({baseId + "_inc", fly_i18n("Hotkey.SingleInc").arg(label), QKeySequence()});
		v.push_back({baseId + "_dec", fly_i18n("Hotkey.SingleDec").arg(label), QKeySequence()});
	}

	for (int i = 0; i < state_.timers.size(); ++i) {
		const auto &tm = state_.timers[i];
		const QString label = tm.label.isEmpty() ? fly_i18n("Hotkey.TimerN").arg(i + 1) : tm.label;
		const QString baseId = QStringLiteral("timer_%1").arg(i);

		v.push_back({baseId + "_toggle", fly_i18n("Hotkey.TimerToggle").arg(label), QKeySequence()});
	}

	return v;
}

void FlyConfigTab::applyHotkeyBindings(const QList<FlyHotkeyBinding> &bindings)
{
	clearAllShortcuts();
	hotkeyBindings_ = bindings;

	fly_hotkeys_save(dataDir_, hotkeyBindings_);

	for (const auto &b : bindings) {
		if (b.sequence.isEmpty())
			continue;

		auto *sc = new QShortcut(b.sequence, this);
		sc->setContext(Qt::ApplicationShortcut);
		shortcuts_.push_back(sc);

		const QString &id = b.actionId;

		if (id == QLatin1String("swap_sides")) {
			connect(sc, &QShortcut::activated, this, [this]() { toggleSwap(); });
		} else if (id == QLatin1String("toggle_scoreboard")) {
			connect(sc, &QShortcut::activated, this, [this]() { toggleScoreboardVisible(); });

		} else if (id.startsWith(QLatin1String("field_"))) {
			const auto parts = id.split(QLatin1Char('_'));
			if (parts.size() < 3)
				continue;

			bool ok = false;
			int idx = parts[1].toInt(&ok);
			if (!ok)
				continue;

			if (parts.size() == 3 && parts[2] == QLatin1String("toggle")) {
				connect(sc, &QShortcut::activated, this,
					[this, idx]() { dock_->toggleCustomFieldVisible(idx); });
			} else if (parts.size() == 4) {
				const QString side = parts[2];
				const QString dir = parts[3];

				if (side == QLatin1String("home") && dir == QLatin1String("inc"))
					connect(sc, &QShortcut::activated, this,
						[this, idx]() { dock_->bumpCustomFieldHome(idx, +1); });
				else if (side == QLatin1String("home") && dir == QLatin1String("dec"))
					connect(sc, &QShortcut::activated, this,
						[this, idx]() { dock_->bumpCustomFieldHome(idx, -1); });
				else if (side == QLatin1String("guest") && dir == QLatin1String("inc"))
					connect(sc, &QShortcut::activated, this,
						[this, idx]() { dock_->bumpCustomFieldGuest(idx, +1); });
				else if (side == QLatin1String("guest") && dir == QLatin1String("dec"))
					connect(sc, &QShortcut::activated, this,
						[this, idx]() { dock_->bumpCustomFieldGuest(idx, -1); });
			}

		} else if (id.startsWith(QLatin1String("single_"))) {
			const auto parts = id.split(QLatin1Char('_'));
			if (parts.size() != 3)
				continue;

			bool ok = false;
			int idx = parts[1].toInt(&ok);
			if (!ok)
				continue;

			const QString action = parts[2];
			if (action == QLatin1String("toggle"))
				connect(sc, &QShortcut::activated, this,
					[this, idx]() { dock_->toggleSingleStatVisible(idx); });
			else if (action == QLatin1String("inc"))
				connect(sc, &QShortcut::activated, this, [this, idx]() { dock_->bumpSingleStat(idx, +1); });
			else if (action == QLatin1String("dec"))
				connect(sc, &QShortcut::activated, this, [this, idx]() { dock_->bumpSingleStat(idx, -1); });

		} else if (id.startsWith(QLatin1String("timer_"))) {
			const auto parts = id.split(QLatin1Char('_'));
			if (parts.size() == 3 && parts[2] == QLatin1String("toggle")) {
				bool ok = false;
				int idx = parts[1].toInt(&ok);
				if (!ok)
					continue;

				connect(sc, &QShortcut::activated, this, [this, idx]() { toggleTimerRunning(idx); });
			}
		}
	}
}

void FlyConfigTab::broadcastCurrentState()
{
	if (!webSocketServer_ || !webSocketServer_->isListening())
		return;
	webSocketServer_->broadcastState(state_, FlyScoreState::selectedTemplateName(dataDir_), dataDir_);
}

void FlyConfigTab::openHotkeysDialog()
{
	QList<FlyHotkeyBinding> initial = buildMergedHotkeyBindings();

	auto *dlg = new FlyHotkeysDialog(initial, this);
	dlg->setAttribute(Qt::WA_DeleteOnClose, true);

	connect(dlg, &FlyHotkeysDialog::bindingsChanged, this,
		[this](const QList<FlyHotkeyBinding> &b) { applyHotkeyBindings(b); });

	dlg->show();
}


void FlyConfigTab::onSelectTemplateFolder()
{
	const QString cur = dataDir_.isEmpty() ? fly_get_data_root_no_ui() : dataDir_;
	const QString picked = QFileDialog::getExistingDirectory(this, fly_i18n("Dock.SelectTemplateFolderTitle"), cur);
	if (picked.isEmpty())
		return;

	dock_->activateTemplateFolder(picked);
}

void FlyConfigTab::updateWebSocketStatus()
{
	if (!webSocketStatus_ || !webSocketServer_)
		return;

	const QString text =
		webSocketServer_->isListening()
			? fly_i18n("Dock.WSOnline").arg(webSocketServer_->url()).arg(webSocketServer_->clientCount())
			: fly_i18n("Dock.WSOffline");
	webSocketStatus_->setText(text);
}

void FlyConfigTab::refreshActiveTemplateLabel()
{
	if (!activeTemplateLabel_)
		return;

	const QString title = FlyScoreState::selectedTemplateName(dataDir_);
	activeTemplateLabel_->setText(title.isEmpty() ? fly_i18n("Dock.NoTemplateFolder") : title);
	activeTemplateLabel_->setToolTip(dataDir_);
}

void FlyConfigTab::handleRemoteCommand(const QJsonObject &command)
{
	const QString action = FlyJsonHelper::jsonString(command, QStringLiteral("action")).isEmpty()
				       ? FlyJsonHelper::jsonString(command, QStringLiteral("type"))
				       : FlyJsonHelper::jsonString(command, QStringLiteral("action"));

	if (action == QLatin1String("get_state")) {
		broadcastCurrentState();
		return;
	}
	if (action == QLatin1String("log_event")) {
		dock_->appendEvent(FlyJsonHelper::jsonString(command, QStringLiteral("label")));
		return;
	}
	if (action == QLatin1String("start_event_log")) {
		const QString label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		dock_->startEventLog(label.isEmpty() ? QStringLiteral("Stream Start") : label);
		return;
	}
	if (action == QLatin1String("stop_event_log")) {
		const QString label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		dock_->stopEventLog(label.isEmpty() ? QStringLiteral("Stream End") : label);
		return;
	}

	if (action == QLatin1String("set_state") && command.value(QStringLiteral("state")).isObject()) {
		FlyScoreState::FlyState next;
		if (FlyScoreState::fly_state_from_json_object(command.value(QStringLiteral("state")).toObject(), next)) {
			state_ = next;
			dock_->saveState();
			refreshUiFromState(false);
		}
		return;
	}

	if (action == QLatin1String("swap")) {
		toggleSwap();
		return;
	}
	if (action == QLatin1String("show_scoreboard")) {
		state_.show_scoreboard = command.value(QStringLiteral("value")).toBool(state_.show_scoreboard);
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
	if (action == QLatin1String("toggle_scoreboard")) {
		toggleScoreboardVisible();
		return;
	}

	if (action == QLatin1String("set_team")) {
		const QString side = FlyJsonHelper::jsonString(command, QStringLiteral("side"));
		FlyScoreState::FlyTeam *team = nullptr;
		if (side == QLatin1String("guest") || side == QLatin1String("guest") || side == QLatin1String("guests"))
			team = &state_.guest;
		else
			team = &state_.home;

		if (command.contains(QStringLiteral("title")))
			team->title = FlyJsonHelper::jsonString(command, QStringLiteral("title"));
		if (command.contains(QStringLiteral("subtitle")))
			team->subtitle = FlyJsonHelper::jsonString(command, QStringLiteral("subtitle"));
		if (command.contains(QStringLiteral("logo")))
			team->logo = FlyJsonHelper::jsonString(command, QStringLiteral("logo"));
		if (command.contains(QStringLiteral("color")))
			team->color = FlyJsonHelper::jsonColor(command, QStringLiteral("color"), team->color);

		dock_->saveState();
		refreshUiFromState(false);
		return;
	}

	const int index = FlyJsonHelper::jsonInt(command, QStringLiteral("index"));
	if (action == QLatin1String("add_score") || action == QLatin1String("add_field")) {
		FlyScoreState::FlyStat cf;
		cf.label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		if (cf.label.isEmpty())
			cf.label = QStringLiteral("Score");
		cf.first = std::max(0, FlyJsonHelper::jsonInt(command, QStringLiteral("home")));
		cf.second = std::max(0, FlyJsonHelper::jsonInt(command, QStringLiteral("guest")));
		cf.visible = FlyJsonHelper::jsonBool(command, QStringLiteral("visible"), true);
		state_.custom_fields.push_back(cf);
		dock_->saveState();
		refreshUiFromState(false);
		hotkeyBindings_ = buildMergedHotkeyBindings();
		applyHotkeyBindings(hotkeyBindings_);
		return;
	}
	if ((action == QLatin1String("remove_score") || action == QLatin1String("remove_field")) && index >= 0 &&
	    index < state_.custom_fields.size()) {
		if (state_.custom_fields.size() > 1) {
			state_.custom_fields.removeAt(index);
			dock_->saveState();
			refreshUiFromState(false);
			hotkeyBindings_ = buildMergedHotkeyBindings();
			applyHotkeyBindings(hotkeyBindings_);
		}
		return;
	}
	if ((action == QLatin1String("set_field") || action == QLatin1String("set_score_field")) && index >= 0 &&
	    index < state_.custom_fields.size()) {
		FlyScoreState::FlyStat &cf = state_.custom_fields[index];
		if (command.contains(QStringLiteral("label")))
			cf.label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		if (command.contains(QStringLiteral("home")))
			cf.first = std::max(0, FlyJsonHelper::jsonInt(command, QStringLiteral("home")));
		if (command.contains(QStringLiteral("guest")))
			cf.second = std::max(0, FlyJsonHelper::jsonInt(command, QStringLiteral("guest")));
		if (command.contains(QStringLiteral("visible")))
			cf.visible = FlyJsonHelper::jsonBool(command, QStringLiteral("visible"), cf.visible);
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
	if ((action == QLatin1String("toggle_field") || action == QLatin1String("score_visibility")) && index >= 0 &&
	    index < state_.custom_fields.size()) {
		state_.custom_fields[index].visible = !state_.custom_fields[index].visible;
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
	if (action == QLatin1String("bump_score")) {
		const QString side = FlyJsonHelper::jsonString(command, QStringLiteral("side"));
		const int delta = FlyJsonHelper::jsonInt(command, QStringLiteral("delta"), 1);
		if (side == QLatin1String("guest") || side == QLatin1String("guest") ||
		    (side == QLatin1String("y") && !state_.swap_sides) ||
		    (side == QLatin1String("x") && state_.swap_sides))
			dock_->bumpCustomFieldGuest(index, delta);
		else
			dock_->bumpCustomFieldHome(index, delta);
		return;
	}
	if (action == QLatin1String("set_score") && index >= 0 && index < state_.custom_fields.size()) {
		const QString side = FlyJsonHelper::jsonString(command, QStringLiteral("side"));
		const int value = std::max(0, FlyJsonHelper::jsonInt(command, QStringLiteral("value")));
		const bool guest = side == QLatin1String("guest") || side == QLatin1String("guest") ||
				   (side == QLatin1String("y") && !state_.swap_sides) ||
				   (side == QLatin1String("x") && state_.swap_sides);
		if (guest)
			state_.custom_fields[index].first = value;
		else
			state_.custom_fields[index].second = value;
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
	if (action == QLatin1String("bump_single")) {
		dock_->bumpSingleStat(index, FlyJsonHelper::jsonInt(command, QStringLiteral("delta"), 1));
		return;
	}
	if (action == QLatin1String("toggle_single") && index >= 0 && index < state_.single_stats.size()) {
		dock_->toggleSingleStatVisible(index);
		return;
	}
	if (action == QLatin1String("add_single")) {
		FlyScoreState::FlyStat ss;
		ss.label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		if (ss.label.isEmpty())
			ss.label = QStringLiteral("STAT");
		ss.first = FlyJsonHelper::jsonInt(command, QStringLiteral("value"));
		ss.visible = FlyJsonHelper::jsonBool(command, QStringLiteral("visible"), true);
		state_.single_stats.push_back(ss);
		dock_->saveState();
		refreshUiFromState(false);
		hotkeyBindings_ = buildMergedHotkeyBindings();
		applyHotkeyBindings(hotkeyBindings_);
		return;
	}
	if (action == QLatin1String("remove_single") && index >= 0 && index < state_.single_stats.size()) {
		if (state_.single_stats.size() > 1) {
			state_.single_stats.removeAt(index);
			dock_->saveState();
			refreshUiFromState(false);
			hotkeyBindings_ = buildMergedHotkeyBindings();
			applyHotkeyBindings(hotkeyBindings_);
		}
		return;
	}
	if (action == QLatin1String("set_single") && index >= 0 && index < state_.single_stats.size()) {
		FlyScoreState::FlyStat &ss = state_.single_stats[index];
		if (command.contains(QStringLiteral("label")))
			ss.label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		if (command.contains(QStringLiteral("value")))
			ss.first = FlyJsonHelper::jsonInt(command, QStringLiteral("value"));
		if (command.contains(QStringLiteral("visible")))
			ss.visible = FlyJsonHelper::jsonBool(command, QStringLiteral("visible"), ss.visible);
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
	if (action == QLatin1String("add_timer")) {
		FlyScoreState::FlyTimer timer;
		timer.label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		if (timer.label.isEmpty())
			timer.label = QStringLiteral("Timer");
		timer.mode = FlyJsonHelper::jsonString(command, QStringLiteral("mode"));
		if (timer.mode != QLatin1String("countup"))
			timer.mode = QStringLiteral("countdown");
		timer.initial_ms = std::max<qint64>(0, FlyJsonHelper::jsonInt64(command, QStringLiteral("initial_ms")));
		timer.visible = FlyJsonHelper::jsonBool(command, QStringLiteral("visible"), true);
		FlyScoreState::setTimerStoppedAtInitial(timer);
		state_.timers.push_back(timer);
		dock_->saveState();
		refreshUiFromState(false);
		hotkeyBindings_ = buildMergedHotkeyBindings();
		applyHotkeyBindings(hotkeyBindings_);
		return;
	}
	if (action == QLatin1String("remove_timer") && index >= 0 && index < state_.timers.size()) {
		if (state_.timers.size() > 1) {
			state_.timers.removeAt(index);
			dock_->saveState();
			refreshUiFromState(false);
			hotkeyBindings_ = buildMergedHotkeyBindings();
			applyHotkeyBindings(hotkeyBindings_);
		}
		return;
	}
	if (action == QLatin1String("set_timer") && index >= 0 && index < state_.timers.size()) {
		FlyScoreState::FlyTimer &timer = state_.timers[index];
		const bool wasRunning = timer.running;
		if (wasRunning)
			toggleTimerRunning(index);

		if (command.contains(QStringLiteral("label")))
			timer.label = FlyJsonHelper::jsonString(command, QStringLiteral("label"));
		if (command.contains(QStringLiteral("mode"))) {
			const QString mode = FlyJsonHelper::jsonString(command, QStringLiteral("mode"));
			timer.mode = mode == QLatin1String("countup") ? QStringLiteral("countup")
								      : QStringLiteral("countdown");
		}
		if (command.contains(QStringLiteral("initial_ms")))
			timer.initial_ms =
				std::max<qint64>(0, FlyJsonHelper::jsonInt64(command, QStringLiteral("initial_ms")));
		if (command.contains(QStringLiteral("remaining_ms")))
			timer.remaining_ms =
				std::max<qint64>(0, FlyJsonHelper::jsonInt64(command, QStringLiteral("remaining_ms")));
		else if (command.contains(QStringLiteral("initial_ms")))
			timer.remaining_ms = timer.initial_ms;
		if (command.contains(QStringLiteral("visible")))
			timer.visible = FlyJsonHelper::jsonBool(command, QStringLiteral("visible"), timer.visible);

		timer.running = false;
		timer.last_tick_ms = 0;
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
	if (action == QLatin1String("timer_toggle")) {
		toggleTimerRunning(index);
		return;
	}
	if (action == QLatin1String("timer_visibility") && index >= 0 && index < state_.timers.size()) {
		state_.timers[index].visible = !state_.timers[index].visible;
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
	if ((action == QLatin1String("timer_start") || action == QLatin1String("timer_pause")) && index >= 0 &&
	    index < state_.timers.size()) {
		const bool shouldRun = action == QLatin1String("timer_start");
		if (state_.timers[index].running != shouldRun)
			toggleTimerRunning(index);
		return;
	}
	if (action == QLatin1String("timer_reset") && index >= 0 && index < state_.timers.size()) {
		FlyScoreState::FlyTimer &t = state_.timers[index];
		t.remaining_ms = std::max<qint64>(0, t.initial_ms);
		t.running = false;
		t.last_tick_ms = 0;
		dock_->saveState();
		refreshUiFromState(false);
		return;
	}
}

void FlyConfigTab::toggleTimerRunning(int index)
{
	if (index < 0 || index >= state_.timers.size())
		return;

	FlyScoreState::FlyTimer &tm = state_.timers[index];
	const qint64 now = FlyQtHelper::fly_now_ms();

	if (!tm.running) {
		if (tm.remaining_ms < 0) {
			if (tm.mode == QStringLiteral("countdown"))
				tm.remaining_ms = (tm.initial_ms > 0) ? tm.initial_ms : 0;
			else
				tm.remaining_ms = 0;
		}
		tm.last_tick_ms = now;
		tm.running = true;
	} else {
		if (tm.last_tick_ms > 0) {
			qint64 elapsed = now - tm.last_tick_ms;
			if (elapsed < 0)
				elapsed = 0;

			if (tm.mode == QStringLiteral("countup"))
				tm.remaining_ms += elapsed;
			else {
				tm.remaining_ms -= elapsed;
				if (tm.remaining_ms < 0)
					tm.remaining_ms = 0;
			}
		}
		tm.running = false;
	}

	dock_->saveState();
	refreshUiFromState(false);
}
