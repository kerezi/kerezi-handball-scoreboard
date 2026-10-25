#include "fly_score_handball_tab.hpp"
#include "fly_score_log.hpp"
#include "fly_score_const.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][handball-tab]"

#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_clickable_label.hpp"

#include <Qt>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QLabel>
#include <QWidget>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QIcon>
#include <QString>
#include <QPushButton>

FlyHandballTab::~FlyHandballTab() = default;

void FlyHandballTab::init() {}

void FlyHandballTab::buildUI()
{
	resize(1200, 1000);
	bool setL = false;
	if (handballTabLayout_ == nullptr) {
		auto *handballGL = new QGridLayout();
		handballGL->setContentsMargins(4, 4, 4, 4);
		handballGL->setVerticalSpacing(4);
		handballGL->setHorizontalSpacing(8);
		handballTabLayout_ = handballGL;
		setL = true;
	}

	buildUI_matchInfo();
	buildUI_players();
	posRow += 1;
	posCol = 0;
	buildUI_matchOfficialInfo(true);
	buildUI_matchOfficialInfo(false);
	buildUI_matchOfficials();

	if (setL) {
		this->setLayout(handballTabLayout_);
		for (int i = 0; i < posCol; i++) {
			handballTabLayout_->setColumnStretch(i, 1);
		}
		connectUi();
	}
}
void FlyHandballTab::connectUi()
{
	connect(mt, &QPushButton::clicked, this, [this]() {
		if (!state_.timers[0].running) {
			qint64 ms = state_.timers[1].initial_ms;
			if (ms < 0)
				ms = 0;

			state_.timers[1].current_moment_ms = ms;
			state_.timers[1].running = false;
			state_.timers[1].last_tick_ms = 0;
		}
		dock_->toggleTimerRunning(0);
	});
}

QPushButton *FlyHandballTab::newFlyClickableLabel(QString title, int height, int width, bool isHome, bool isTeam,
						  QFlags<Qt::AlignmentFlag> alf)
{
	QPushButton *newQLabel = new QPushButton();
	QColor c = FlyQtHelper::colorFromU32(isHome ? state_.home.color : state_.guest.color);
	QString css = "QPushButton { font-weight:bold; } QPushButton[team=\"out\"] { background-color: red; color: " +
		      c.darker().name() +
		      "; } QPushButton[team=\"yellow\"] { color: yellow; background-color: " + c.darker().name() +
		      "; } QPushButton[team=\"2minSusp\"] { color: red; background-color: " + c.darker().name() +
		      "; }  QPushButton[team=\"true\"] {color : " + FlyQtHelper::getHighContrastColor(c).name() +
		      "; background-color: " + c.darker().name() +
		      ";} QPushButton[trun=\"true\"] {color: green; } QPushButton[trun=\"false\"] {color: red; }";

	newQLabel->setStyleSheet(css);
	newQLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	newQLabel->setProperty("team", isTeam);
	posCol += width * (isHome ? 0 : -1);
	handballTabLayout_->addWidget(newQLabel, posRow, posCol, height, width, alf);
	posCol += width * (isHome ? 1 : 0);

	return newQLabel;
}

QLabel *FlyHandballTab::newQLabel(QString title, int height, int width, bool isHome, bool isTeam,
				  QFlags<Qt::AlignmentFlag> alf)
{
	QLabel *newQLabel = new QLabel(title);
	QColor c = FlyQtHelper::colorFromU32(isHome ? state_.home.color : state_.guest.color);
	QString css = "QLabel { font-weight:bold; } QLabel[team=\"yellow\"] { color: yellow; background-color: " +
		      c.darker().name() + "; }  QLabel[team=\"out\"] { background-color: red; color: " +
		      c.darker().name() + "; } QLabel[team=\"true\"] { background-color: " + c.darker().name() +
		      "; color: " + FlyQtHelper::getHighContrastColor(c).name() + "; }";
	newQLabel->setStyleSheet(css);
	newQLabel->setToolTip(title);
	newQLabel->setAlignment(alf);
	newQLabel->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);
	newQLabel->setProperty("team", isTeam);
	posCol += width * (isHome ? 0 : -1);
	handballTabLayout_->addWidget(newQLabel, posRow, posCol, height, width);
	posCol += width * (isHome ? 1 : 0);

	return newQLabel;
}

void FlyHandballTab::buildUI_matchInfo_Team_FirstLine(bool isHome)
{
	bool makeNew = isHome ? home == nullptr : guest == nullptr;
	FlyTeamUI *team = makeNew ? new FlyTeamUI{} : isHome ? home : guest;
	makeNew = team->teamLongName == nullptr;

	FlyScoreState::FlyTeam *t = (isHome ? &state_.home : &state_.guest);

	posCol += (isHome ? 0 : 8);

	if (makeNew) {
		team->teamLongName =
			newQLabel(fly_i18n("Common.Home"), 2, 4, isHome, true, Qt::AlignTop | Qt::AlignHCenter);
		team->teamLongName->setWordWrap(true);
		team->teamFrst2minSuspTimer =
			newQLabel(fly_i18n("Common.F2MinSusp"), 2, 1, isHome, true, Qt::AlignTop | Qt::AlignHCenter);
		team->teamScnd2minSuspTimer =
			newQLabel(fly_i18n("Common.F2MinSusp"), 2, 1, isHome, true, Qt::AlignTop | Qt::AlignHCenter);
		team->teamThrd2minSuspTimer =
			newQLabel(fly_i18n("Common.F2MinSusp"), 2, 1, isHome, true, Qt::AlignTop | Qt::AlignHCenter);
		team->teamTimeout = newFlyClickableLabel(fly_i18n("Common.TimeOut"), 2, 1, isHome, true,
							 Qt::AlignTop | Qt::AlignHCenter);

		connect(team->teamTimeout, &QPushButton::clicked, this, [this, isHome, team]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			bool tog = false;
			if (t->fstTimeout_ms == 0 && !state_.timers[1].running) {
				t->fstTimeout_ms = state_.timers[0].current_moment_ms +
						   (state_.timers[0].running
							    ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms
							    : 0);
				tog = true;
			} else if (t->sndTimeout_ms == 0 && !state_.timers[1].running) {
				t->sndTimeout_ms = state_.timers[0].current_moment_ms +
						   (state_.timers[0].running
							    ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms
							    : 0);
				tog = true;
			} else if (t->trdTimeout_ms == 0 && !state_.timers[1].running) {
				t->trdTimeout_ms = state_.timers[0].current_moment_ms +
						   (state_.timers[0].running
							    ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms
							    : 0);
				tog = true;
				team->teamTimeout->setDisabled(true);
			}

			if (tog) {
				if (state_.timers[0].running) dock_->toggleTimerRunning(0);
				dock_->toggleTimerRunning(1);
			}
		});
	}
	team->teamLongName->setText(t->title);
	// add 2 min susp ...
	team->teamFrst2minSuspTimer->setText("");
	team->teamScnd2minSuspTimer->setText("");
	team->teamThrd2minSuspTimer->setText("");
	bool f2 = false, s2 = false, t2 = false;
	auto set2min = [&](const FlyScoreState::FlyPlayer *o, long long delta) {
		if (!f2) {
			team->teamFrst2minSuspTimer->setText(FlyQtHelper::fly_format_ms_mmss(delta) + "\n" +
							     QString::number(o->playerTshirtNumber));
			f2 = true;
		} else if (!s2) {
			team->teamScnd2minSuspTimer->setText(FlyQtHelper::fly_format_ms_mmss(delta) + "\n" +
							     QString::number(o->playerTshirtNumber));
			s2 = true;
		} else if (!t2) {
			team->teamThrd2minSuspTimer->setText(FlyQtHelper::fly_format_ms_mmss(delta) + "\n" +
							     QString::number(o->playerTshirtNumber));
			t2 = true;
		}
	};

	for (FlyScoreState::FlyPlayer p : t->players) {
		long long ct =
			state_.timers[0].current_moment_ms +
			(state_.timers[0].running ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms : 0);
		if (p.fst2minSusp_ms > 0 && p.fst2minSusp_ms + 120000 >= ct) {
			set2min(&p, ct - p.fst2minSusp_ms);
		} else if (p.snd2minSusp_ms > 0 && p.snd2minSusp_ms + 120000 >= ct) {
			set2min(&p, ct - p.snd2minSusp_ms);
		} else if (p.trd2minSusp_ms > 0 && p.trd2minSusp_ms + 120000 >= ct) {
			set2min(&p, ct - p.trd2minSusp_ms);
		} else if (p.disqualification_ms > 0 && p.disqualification_ms + 120000 >= ct) {
			set2min(&p, ct - p.disqualification_ms);
		} else if (p.blueCard_ms > 0 && p.blueCard_ms + 120000 >= ct) {
			set2min(&p, ct - p.blueCard_ms);
		}
	}

	int numOfTo = 0;
	numOfTo += t->fstTimeout_ms > 0 ? 1 : 0;
	numOfTo += t->sndTimeout_ms > 0 ? 1 : 0;
	numOfTo += t->trdTimeout_ms > 0 ? 1 : 0;
	team->teamTimeout->setText(fly_i18n("Common.TimeOut") + " " + QString::number(numOfTo));

	posCol += (isHome ? 0 : 10);

	if (makeNew) {
		if (isHome)
			home = team;
		else
			guest = team;
	}
}

void FlyHandballTab::buildUI_matchInfo_Team_SecondLine(bool isHome)
{
	bool makeNew = isHome ? home == nullptr : guest == nullptr;
	FlyTeamUI *team = makeNew ? new FlyTeamUI{} : isHome ? home : guest;
	makeNew = team->teamShrtName == nullptr;

	FlyScoreState::FlyTeam *t = (isHome ? &state_.home : &state_.guest);
	posCol += (isHome ? 0 : 9);

	if (makeNew) {
		team->teamShrtName =
			newQLabel(fly_i18n("Common.Home") + " " + fly_i18n("Dock.Players"), 1, 3, isHome, true);
		team->teamNumOfYllCard = newQLabel(fly_i18n("Common.NumberOfYellow"), 1, 1, isHome, true);
		team->teamNumOfFrst2minSup = newQLabel(fly_i18n("Common.NumberOfFirst2minSusp"), 1, 1, isHome, true);
		team->teamNumOfScnd2minSup = newQLabel(fly_i18n("Common.NumberOfSecond2minSusp"), 1, 1, isHome, true);
		team->teamNumOfThrd2minSup = newQLabel(fly_i18n("Common.NumberOfThird2minSusp"), 1, 1, isHome, true);
		team->teamNumOfRedCard = newQLabel(fly_i18n("Common.NumberOfDisq"), 1, 1, isHome, true);
		team->teamNumOfBluCard = newQLabel(fly_i18n("Common.NumberOfBlue"), 1, 1, isHome, true);
	}

	team->teamShrtName->setText(t->subtitle);

	int numOf = 0;
	for (int i = 0; i < t->players.size(); i++) {
		numOf += (t->players[i].warning_ms > 0) ? 1 : 0;
	}
	team->teamNumOfYllCard->setText(fly_i18n("Common.NumberOfYellow") + " " + QString::number(numOf));

	numOf = 0;
	for (int i = 0; i < t->players.size(); i++) {
		numOf += (t->players[i].fst2minSusp_ms > 0) ? 1 : 0;
	}
	team->teamNumOfFrst2minSup->setText(fly_i18n("Common.NumberOfFirst2minSusp") + " " + QString::number(numOf));

	numOf = 0;
	for (int i = 0; i < t->players.size(); i++) {
		numOf += (t->players[i].snd2minSusp_ms > 0) ? 1 : 0;
	}
	team->teamNumOfScnd2minSup->setText(fly_i18n("Common.NumberOfSecond2minSusp") + " " + QString::number(numOf));

	numOf = 0;
	for (int i = 0; i < t->players.size(); i++) {
		numOf += (t->players[i].trd2minSusp_ms > 0) ? 1 : 0;
	}
	team->teamNumOfThrd2minSup->setText(fly_i18n("Common.NumberOfThird2minSusp") + " " + QString::number(numOf));

	numOf = 0;
	for (int i = 0; i < t->players.size(); i++) {
		numOf += (t->players[i].disqualification_ms > 0) ? 1 : 0;
	}
	team->teamNumOfRedCard->setText(fly_i18n("Common.NumberOfDisq") + " " + QString::number(numOf));

	numOf = 0;
	for (int i = 0; i < t->players.size(); i++) {
		numOf += (t->players[i].blueCard_ms > 0) ? 1 : 0;
	}
	team->teamNumOfBluCard->setText(fly_i18n("Common.NumberOfBlue") + " " + QString::number(numOf));

	if (makeNew) {
		if (isHome)
			home = team;
		else
			guest = team;
	}

	posCol += (isHome ? 0 : 10);
}

void FlyHandballTab::buildUI_matchInfo()
{
	{
		buildUI_matchInfo_Team_FirstLine(true);

		if (mt == nullptr) {
			mt = newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 2, 4, true);
			QString css = mt->styleSheet() + QStringLiteral(" QPushButton {font-size: 24pt;}");
			mt->setStyleSheet(css);
		}
		mt->setToolTip(state_.timers[0].running ? fly_i18n("Dock.PauseTimer") : fly_i18n("Dock.StartTimer"));
		mt->setProperty("trun", state_.timers[0].running);
		FlyScoreState::FlyTimer tm = state_.timers[0];
		// LOGI("tm.current_moment_ms = %Ld", tm.current_moment_ms);
		syncButtonFromState(
			mt, false, false,
			FlyQtHelper::fly_format_ms_mmss(
				tm.current_moment_ms + (tm.running ? FlyQtHelper::fly_now_ms() - tm.last_tick_ms : 0)));

		buildUI_matchInfo_Team_FirstLine(false);
	}

	posRow += 2;
	posCol = 0;
	// 2nd row
	{
		buildUI_matchInfo_Team_SecondLine(true);

		mr = (mr == nullptr) ? newQLabel(fly_i18n("Common.MatchResult"), 1, 2, true) : mr;
		int homeScore = 0;
		for (FlyScoreState::FlyPlayer p : state_.home.players) {
			for (FlyScoreState::FlyGoal g : p.goals) {
				homeScore++;
			}
		}
		int guestScore = 0;
		for (FlyScoreState::FlyPlayer p : state_.guest.players) {
			for (FlyScoreState::FlyGoal g : p.goals) {
				guestScore++;
			}
		}
		mr->setText(QString::asprintf("%02d:%02d", homeScore, guestScore));

		buildUI_matchInfo_Team_SecondLine(false);
	}
}

void FlyHandballTab::buildUI_team_player(int row, bool isHome)
{
	FlyScoreState::FlyTeam *t = (isHome ? &state_.home : &state_.guest);
	bool disable = !(row < t->players.size());
	FlyScoreState::FlyPlayer *p = (!disable ? &(t->players[row]) : new FlyScoreState::FlyPlayer());

	bool out = !disable && (t->players[row].trd2minSusp_ms > 0 || t->players[row].disqualification_ms > 0 ||
				t->players[row].blueCard_ms > 0);

	posCol += (isHome ? 0 : 10);
	FlyTeamUI *tUI = (isHome ? home : guest);

	bool makeNew = !(row < tUI->players->size());
	FlyTeamPlayerUI *pl = makeNew ? new FlyTeamPlayerUI{} : (*tUI->players)[row];

	if (makeNew) {
		pl->teamPlayerTShirtNumber = newQLabel(fly_i18n("Common.PlayerNumber"), 1, 1, isHome, !disable);
		pl->teamPlayerTShirtName = newQLabel(fly_i18n("Players.PlayerName"), 1, 2, isHome, !disable,
						     (isHome ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter);
		pl->teamPlayerWarningTime = newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(pl->teamPlayerWarningTime, &QPushButton::clicked, this, [this, isHome, pl]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->players.size(); i++) {
				if (t->players[i].playerTshirtNumber == pl->teamPlayerTShirtNumber->text().toInt()) {
					t->players[i].warning_ms =
						state_.timers[0].current_moment_ms +
						(state_.timers[0].running
							 ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms
							 : 0);
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		pl->teamPlayerFrst2minSuspTimer =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(pl->teamPlayerFrst2minSuspTimer, &QPushButton::clicked, this, [this, isHome, pl]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->players.size(); i++) {
				if (t->players[i].playerTshirtNumber == pl->teamPlayerTShirtNumber->text().toInt()) {
					t->players[i].fst2minSusp_ms =
						FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		pl->teamPlayerScnd2minSuspTimer =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(pl->teamPlayerScnd2minSuspTimer, &QPushButton::clicked, this, [this, isHome, pl]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->players.size(); i++) {
				if (t->players[i].playerTshirtNumber == pl->teamPlayerTShirtNumber->text().toInt()) {
					t->players[i].snd2minSusp_ms =
						FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		pl->teamPlayerThrd2minSuspTimer =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(pl->teamPlayerThrd2minSuspTimer, &QPushButton::clicked, this, [this, isHome, pl]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->players.size(); i++) {
				if (t->players[i].playerTshirtNumber == pl->teamPlayerTShirtNumber->text().toInt()) {
					t->players[i].trd2minSusp_ms =
						FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		pl->teamPlayerDisqualificationTime =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(pl->teamPlayerDisqualificationTime, &QPushButton::clicked, this, [this, isHome, pl]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->players.size(); i++) {
				if (t->players[i].playerTshirtNumber == pl->teamPlayerTShirtNumber->text().toInt()) {
					t->players[i].disqualification_ms =
						FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		pl->teamPlayerBlueCardTime =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(pl->teamPlayerBlueCardTime, &QPushButton::clicked, this, [this, isHome, pl]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->players.size(); i++) {
				if (t->players[i].playerTshirtNumber == pl->teamPlayerTShirtNumber->text().toInt()) {
					t->players[i].blueCard_ms = FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		pl->teamPlayerGoals = newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(pl->teamPlayerGoals, &QPushButton::clicked, this, [this, isHome, pl]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->players.size(); i++) {
				if (t->players[i].playerTshirtNumber == pl->teamPlayerTShirtNumber->text().toInt()) {
					FlyScoreState::FlyGoal g;
					g.moment_ms = state_.timers[0].current_moment_ms +
						      (state_.timers[0].running ? FlyQtHelper::fly_now_ms() -
											  state_.timers[0].last_tick_ms
										: 0);
					g.goalType = "regular";
					t->players[i].goals.push_back(g);
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});
	}

	pl->teamPlayerTShirtNumber->setProperty("team", p->warning_ms > 0 ? "yellow" : "true");
	syncLabelFromState(pl->teamPlayerTShirtNumber, disable, out, QString::number(p->playerTshirtNumber));

	pl->teamPlayerTShirtName->setProperty("team", p->warning_ms > 0 ? "yellow" : "true");
	syncLabelFromState(pl->teamPlayerTShirtName, disable, out, p->playerTshirtName);

	pl->teamPlayerWarningTime->setProperty("team", p->warning_ms > 0 ? "yellow" : "true");
	syncButtonFromState(pl->teamPlayerWarningTime, disable, out, FlyQtHelper::fly_format_ms_mmss(p->warning_ms));
	pl->teamPlayerWarningTime->setDisabled(p->fst2minSusp_ms != 0 || p->warning_ms != 0 || out || disable);

	pl->teamPlayerFrst2minSuspTimer->setProperty("team", p->fst2minSusp_ms > 0 ? "2minSusp" : "true");
	syncButtonFromState(pl->teamPlayerFrst2minSuspTimer, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(p->fst2minSusp_ms));
	pl->teamPlayerFrst2minSuspTimer->setDisabled(p->fst2minSusp_ms != 0 || out || state_.timers[0].running ||
						     disable);

	pl->teamPlayerScnd2minSuspTimer->setProperty("team", p->snd2minSusp_ms > 0 ? "2minSusp" : "true");
	syncButtonFromState(pl->teamPlayerScnd2minSuspTimer, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(p->snd2minSusp_ms));
	pl->teamPlayerScnd2minSuspTimer->setDisabled(p->fst2minSusp_ms == 0 || p->snd2minSusp_ms != 0 || out ||
						     state_.timers[0].running);

	pl->teamPlayerThrd2minSuspTimer->setProperty("team", p->trd2minSusp_ms > 0 ? "2minSusp" : "true");
	syncButtonFromState(pl->teamPlayerThrd2minSuspTimer, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(p->trd2minSusp_ms));
	pl->teamPlayerThrd2minSuspTimer->setDisabled(p->snd2minSusp_ms == 0 || p->trd2minSusp_ms != 0 || out ||
						     state_.timers[0].running);

	pl->teamPlayerDisqualificationTime->setProperty("team", p->disqualification_ms > 0 ? "2minSusp" : "true");
	syncButtonFromState(pl->teamPlayerDisqualificationTime, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(p->disqualification_ms));
	pl->teamPlayerDisqualificationTime->setDisabled(out || state_.timers[0].running || disable);

	pl->teamPlayerBlueCardTime->setProperty("team", p->blueCard_ms > 0 ? "2minSusp" : "true");
	syncButtonFromState(pl->teamPlayerBlueCardTime, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(p->blueCard_ms));
	pl->teamPlayerBlueCardTime->setDisabled(out || state_.timers[0].running || disable);

	pl->teamPlayerGoals->setProperty("team", p->warning_ms > 0 ? "yellow" : "true");
	syncButtonFromState(pl->teamPlayerGoals, disable, out, QString::number(p->goals.size()));

	posCol += (isHome ? 0 : 10);

	if (makeNew)
		tUI->players->push_back(pl);
}

void FlyHandballTab::buildUI_players()
{

	posRow += 1;
	posCol = 0;
	int plCount = std::max(state_.home.players.size(), state_.guest.players.size());
	// 3rd row to 18th row
	for (int i = 0; i < plCount; i++) {
		posRow++;
		posCol = 0;

		buildUI_team_player(i, true);
		buildUI_team_player(i, false);
	}
}

void FlyHandballTab::buildUI_matchOfficials()
{
	posRow += 1;
	posCol = 0;
	int ofCount = std::max(state_.home.officials.size(), state_.guest.officials.size());
	// 3rd row to 18th row
	for (int i = 0; i < ofCount; i++) {
		posRow++;
		posCol = 0;

		buildUI_team_official(i, true);
		buildUI_team_official(i, false);
	}
}

void FlyHandballTab::buildUI_team_official(int row, bool isHome)
{
	FlyScoreState::FlyTeam *t = (isHome ? &state_.home : &state_.guest);
	bool disable = !(row < t->officials.size());
	FlyScoreState::FlyOfficial *o = (!disable ? &(t->officials[row]) : new FlyScoreState::FlyOfficial());

	bool out = !disable && (t->officials[row].disqualification_ms > 0 || t->officials[row].blueCard_ms > 0);

	posCol += (isHome ? 0 : 10);
	FlyTeamUI *tUI = (isHome ? home : guest);

	bool makeNew = !(row < tUI->officials->size());
	FlyTeamPlayerUI *of = makeNew ? new FlyTeamPlayerUI{} : (*tUI->officials)[row];

	if (makeNew) {
		of->teamPlayerTShirtNumber = newQLabel(fly_i18n("Common.OfficialChar"), 1, 1, isHome, !disable);
		of->teamPlayerTShirtName = newQLabel(fly_i18n("Players.PlayerName"), 1, 2, isHome, !disable,
						     (isHome ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignVCenter);
		of->teamPlayerWarningTime = newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 1, isHome, !disable);
		connect(of->teamPlayerWarningTime, &QPushButton::clicked, this, [this, isHome, of]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->officials.size(); i++) {
				if (t->officials[i].officialLetter == of->teamPlayerTShirtNumber->text().at(0)) {
					t->officials[i].warning_ms =
						state_.timers[0].current_moment_ms +
						(state_.timers[0].running
							 ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms
							 : 0);
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		of->teamPlayerFrst2minSuspTimer =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 2, isHome, !disable);
		connect(of->teamPlayerFrst2minSuspTimer, &QPushButton::clicked, this, [this, isHome, of]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->officials.size(); i++) {
				if (t->officials[i].officialLetter == of->teamPlayerTShirtNumber->text().at(0)) {
					t->officials[i].fst2minSusp_ms =
						FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		of->teamPlayerDisqualificationTime =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 2, isHome, !disable);
		connect(of->teamPlayerDisqualificationTime, &QPushButton::clicked, this, [this, isHome, of]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->officials.size(); i++) {
				if (t->officials[i].officialLetter == of->teamPlayerTShirtNumber->text().at(0)) {
					t->officials[i].disqualification_ms =
						FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});

		of->teamPlayerBlueCardTime =
			newFlyClickableLabel(fly_i18n("Common.MatchTimer"), 1, 2, isHome, !disable);
		connect(of->teamPlayerBlueCardTime, &QPushButton::clicked, this, [this, isHome, of]() {
			FlyScoreState::FlyTeam *t = isHome ? &state_.home : &state_.guest;
			for (int i = 0; i < t->officials.size(); i++) {
				if (t->officials[i].officialLetter == of->teamPlayerTShirtNumber->text().at(0)) {
					t->officials[i].blueCard_ms =
						FlyQtHelper::fly_parse_mmss_to_ms(this->mt->text());
					this->dock_->saveState();
					this->dock_->refreshUiFromState();
					return;
				}
			}
		});
	}

	syncLabelFromState(of->teamPlayerTShirtNumber, disable, out, o->officialLetter);

	syncLabelFromState(of->teamPlayerTShirtName, disable, out, o->officialName);

	of->teamPlayerWarningTime->setProperty("team", o->warning_ms > 0 ? "yellow"
									 : of->teamPlayerWarningTime->property("team"));
	syncButtonFromState(of->teamPlayerWarningTime, disable, out, FlyQtHelper::fly_format_ms_mmss(o->warning_ms));
	of->teamPlayerWarningTime->setDisabled(o->fst2minSusp_ms != 0 || o->warning_ms != 0 || out || disable);

	syncButtonFromState(of->teamPlayerFrst2minSuspTimer, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(o->fst2minSusp_ms));
	of->teamPlayerFrst2minSuspTimer->setDisabled(o->fst2minSusp_ms != 0 || out || state_.timers[0].running ||
						     disable);

	syncButtonFromState(of->teamPlayerDisqualificationTime, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(o->disqualification_ms));
	of->teamPlayerDisqualificationTime->setDisabled(out || state_.timers[0].running || disable);

	syncButtonFromState(of->teamPlayerBlueCardTime, disable || state_.timers[0].running, out,
			    FlyQtHelper::fly_format_ms_mmss(o->blueCard_ms));
	of->teamPlayerBlueCardTime->setDisabled(out || state_.timers[0].running || disable);

	posCol += (isHome ? 0 : 10);

	if (makeNew)
		tUI->officials->push_back(of);
}

void FlyHandballTab::refreshUiFromState(bool onlyTimeIfRunning)
{
	syncUiFromState();
}

void FlyHandballTab::syncUiFromState()
{
	buildUI();
}

void FlyHandballTab::syncLabelFromState(QLabel *l, bool disable, bool out, QString text)
{
	l->setText(disable ? "" : text);

	l->setDisabled(disable || out);
	if (out)
		l->setProperty("team", "out");

	l->style()->unpolish(l);
	l->style()->polish(l);
	l->update();
}

void FlyHandballTab::syncButtonFromState(QPushButton *b, bool disable, bool out, QString text)
{
	b->setText(disable ? "" : text);

	b->setDisabled(disable || out);
	if (out)
		b->setProperty("team", "out");

	b->style()->unpolish(b);
	b->style()->polish(b);
	b->update();
}

void FlyHandballTab::syncStateFromUi() {}

void FlyHandballTab::buildUI_matchOfficialInfo(bool isHome)
{
	bool makeNew = isHome ? home == nullptr : guest == nullptr;
	FlyTeamUI *team = makeNew ? new FlyTeamUI{} : isHome ? home : guest;
	makeNew = team->teamOfficials == nullptr;

	FlyScoreState::FlyTeam *t = (isHome ? &state_.home : &state_.guest);

	posCol += (isHome ? 0 : 10);

	if (makeNew) {
		team->teamOfficials = newQLabel(fly_i18n("Teams.Officials"), 1, 3, isHome, true);
		team->teamOfficialsNumOfYllCard = newQLabel(fly_i18n("Common.NumberOfYellow"), 1, 1, isHome, true,
							    Qt::AlignTop | Qt::AlignHCenter);
		team->teamOfficialsNumOf2minSup = newQLabel(fly_i18n("Common.NumberOfFirst2minSusp"), 1, 2, isHome,
							    true, Qt::AlignTop | Qt::AlignHCenter);
		team->teamOfficialsNumOfRedCard =
			newQLabel(fly_i18n("Common.NumberOfDisq"), 1, 2, isHome, true, Qt::AlignTop | Qt::AlignHCenter);
		team->teamOfficialsNumOfBluCard =
			newQLabel(fly_i18n("Common.NumberOfBlue"), 1, 2, isHome, true, Qt::AlignTop | Qt::AlignHCenter);
	}
	team->teamOfficialsNumOfYllCard->setText("");
	team->teamOfficialsNumOf2minSup->setText("");
	team->teamOfficialsNumOfRedCard->setText("");
	team->teamOfficialsNumOfBluCard->setText("");

	int numOf = 0;
	for (FlyScoreState::FlyOfficial o : t->officials) {
		long long ct =
			state_.timers[0].current_moment_ms +
			(state_.timers[0].running ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms : 0);
		if (o.warning_ms > 0 && o.warning_ms + 120000 >= ct) {
			numOf++;
		} else if (o.fst2minSusp_ms > 0 && o.fst2minSusp_ms + 120000 >= ct) {
			team->teamOfficialsNumOf2minSup->setText(QString::asprintf(
				"%c - %s", o.officialLetter, FlyQtHelper::fly_format_ms_mmss(ct - o.fst2minSusp_ms)));
		} else if (o.disqualification_ms > 0 && o.disqualification_ms + 120000 >= ct) {
			team->teamOfficialsNumOf2minSup->setText(
				QString::asprintf("%c - %s", o.officialLetter,
						  FlyQtHelper::fly_format_ms_mmss(ct - o.disqualification_ms)));
		} else if (o.blueCard_ms > 0 && o.blueCard_ms + 120000 >= ct) {
			team->teamOfficialsNumOf2minSup->setText(QString::asprintf(
				"%c - %s", o.officialLetter, FlyQtHelper::fly_format_ms_mmss(ct - o.blueCard_ms)));
		}
	}
	team->teamOfficialsNumOfYllCard->setText(fly_i18n("Common.NumberOfYellow") + " " + QString::number(numOf));

	posCol += (isHome ? 0 : 10);

	if (makeNew) {
		if (isHome)
			home = team;
		else
			guest = team;
	}
}
