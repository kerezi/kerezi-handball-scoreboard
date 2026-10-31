#include "fly_score_player_stat_dialog.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][player-stat-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_logo_helpers.hpp"
#include "fly_score_horizontal_tab_bar.hpp"
#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"

#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStyle>
#include <QColorDialog>
#include <QTabWidget>
#include <QSpinBox>
#include <QCheckBox>

FlyPlayerStatDialog::FlyPlayerStatDialog(const QString &dataDir, FlyScoreState::FlyPlayer *player, bool isHome,
					 FlyScoreState::FlyState &state, QWidget *parent)
	: FlyBaseDialog(fly_i18n("Dock.Players"), fly_i18n("Players.Hint"),
			QString::number(player->playerTshirtNumber) + " " + player->playerName, dataDir, state, parent),
	  player_(player),
	  isHome_(isHome)
{
	nStat_ = new FlyScoreState::FlyPlayerStat{};
	nStat_->moment_ms = state_.timers[0].current_moment_ms +
			    (state_.timers[0].running ? FlyQtHelper::fly_now_ms() - state_.timers[0].last_tick_ms : 0);
	resize(660, 820);

	auto *tabs = new FlyHorizontalTabWidget();
	tabs->setTabPosition(QTabWidget::West);
	tabs->tabBar()->setStyleSheet("QTabBar {font-size:22pt;}");
	tabs->tabBar()->setAutoHide(true);
	tabs->setObjectName(fly_i18n("Dock.Players"));
	tabs->setDocumentMode(true);
	tabs->setMovable(false);
	tabs->setUsesScrollButtons(true);

	std::vector<std::function<FlyPlayerStatTabUI *(QWidget * statTabWidget)>> playerStatsTabs = {
		[this](QWidget *statTabWidget) -> FlyPlayerStatTabUI * { return playerStatWidget(statTabWidget); },
		[this](QWidget *statTabWidget) -> FlyPlayerStatTabUI * { return playerPitchWidget(statTabWidget); },
		[this](QWidget *statTabWidget) -> FlyPlayerStatTabUI * {
			return playerListWidget(statTabWidget, true);
		},
		[this](QWidget *statTabWidget) -> FlyPlayerStatTabUI * {
			return playerListWidget(statTabWidget, false);
		},
		[this](QWidget *statTabWidget) -> FlyPlayerStatTabUI * { return playerStatsWidget(statTabWidget); }};

	for (const auto &getWidget : playerStatsTabs) {
		QWidget *statTabWidget = new QWidget();
		FlyPlayerStatDialog::FlyPlayerStatTabUI *statTab = getWidget(statTabWidget);
		tabs->addTab(statTabWidget, statTab->title);
	}

	gl_->addWidget(tabs);

	// connect(home_->addPlayersBtn, &QPushButton::clicked, this, &FlyPlayerStatDialog::onAddHomePlayer);
	// connect(guest_->addPlayersBtn, &QPushButton::clicked, this, &FlyPlayerStatDialog::onAddGuestPlayer);

	addButtons();
}

FlyPlayerStatDialog::~FlyPlayerStatDialog() = default;

FlyPlayerStatDialog::FlyPlayerStatTabUI *FlyPlayerStatDialog::playerStatWidget(QWidget *playerStatTabWidget)
{
	stat_ = new FlyPlayerStatTabUI{};
	stat_->playerStatRows = new QVector<FlyPlayerStatUIRow>();
	stat_->title = fly_i18n("Players.Stat");

	stat_->playerStatLayout = new QVBoxLayout(playerStatTabWidget);
	stat_->playerStatLayout->setContentsMargins(0, 4, 0, 0);
	stat_->playerStatLayout->setSpacing(6);
	stat_->playerStatLayout->setAlignment(Qt::AlignTop);

	QColor c = FlyQtHelper::colorFromU32(isHome_ ? state_.home.color : state_.guest.color);
	QColor cc = FlyQtHelper::getHighContrastColor(c);
	QString css = "QPushButton { font-weight:bold; } QPushButton[plStatType=\"true\"] { background-color: " +
		      c.name() + "; color: " + cc.name() + "; }";

	auto *playerStatWidget = new QWidget();
	QGridLayout *l = new QGridLayout(playerStatWidget);

	playerStatWidget->resize(881, 569);

	playerStatWidget->setStyleSheet(
		"QWidget {"
		"   background-image: url('C:\\Users\\gamer\\Documents\\ws\\ls\\dev\\kerezi-handball-scoreboard\\specs\\goal.png');"
		"   background-repeat: no-repeat;"
		"   background-position: center;"
		"}");

	QPushButton *stealBtn = new QPushButton(fly_i18n("Players.Stat.Steal"));
	stealBtn->setStyleSheet(css);
	QPushButton *goalBtn = new QPushButton(fly_i18n("Players.Stat.Goal"));
	goalBtn->setStyleSheet(css);
	QPushButton *penBtn = new QPushButton(fly_i18n("Players.Stat.Shoot.Pen"));
	penBtn->setStyleSheet(css);
	QPushButton *freeBtn = new QPushButton(fly_i18n("Players.Stat.Shoot.Free"));
	freeBtn->setStyleSheet(css);
	QPushButton *gk1Btn = new QPushButton(fly_i18n("Players.Stat.GK_1"));
	QPushButton *gk2Btn = new QPushButton(fly_i18n("Players.Stat.GK_2"));
	QPushButton *gk3Btn = new QPushButton(fly_i18n("Players.Stat.GK_3"));
	QPushButton *gk4Btn = new QPushButton(fly_i18n("Players.Stat.GK_4"));
	QPushButton *gk5Btn = new QPushButton(fly_i18n("Players.Stat.GK_5"));
	QPushButton *gk6Btn = new QPushButton(fly_i18n("Players.Stat.GK_6"));

	l->addWidget(stealBtn, 0, 0, 1, 3);
	l->addWidget(goalBtn, 1, 0, 1, 1);
	l->addWidget(penBtn, 1, 1, 1, 1);
	l->addWidget(freeBtn, 1, 2, 1, 1);
	l->addWidget(gk1Btn, 2, 0);
	l->addWidget(gk2Btn, 2, 1);
	l->addWidget(gk3Btn, 2, 2);
	l->addWidget(gk4Btn, 3, 0);
	l->addWidget(gk5Btn, 3, 1);
	l->addWidget(gk6Btn, 3, 2);

	stat_->playerStatLayout->addWidget(playerStatWidget, 1);

	playerStatTabWidget->setLayout(stat_->playerStatLayout);

	connect(stealBtn, &QPushButton::clicked, this,
		[this, stealBtn, goalBtn, penBtn, freeBtn, gk1Btn, gk2Btn, gk3Btn, gk4Btn, gk5Btn, gk6Btn]() {
			setPropPlStatType(stealBtn, true);
			for (QPushButton *btn :
			     {goalBtn, penBtn, freeBtn, gk1Btn, gk2Btn, gk3Btn, gk4Btn, gk5Btn, gk6Btn}) {
				setPropPlStatType(btn, false);
			}
			nStat_->statType = "steal";
		});

	connect(goalBtn, &QPushButton::clicked, this, [this, stealBtn, goalBtn]() {
		setPropPlStatType(stealBtn, false);
		nStat_->success = !nStat_->success;
		setProp(goalBtn, "plStatType", nStat_->success);
	});

	connect(penBtn, &QPushButton::clicked, this, [this, stealBtn, penBtn, freeBtn]() {
		setPropPlStatType(stealBtn, false);
		setPropPlStatType(penBtn, true);
		setPropPlStatType(freeBtn, false);
		nStat_->statType = "shoot - penalty";
	});

	connect(freeBtn, &QPushButton::clicked, this, [this, stealBtn, penBtn, freeBtn]() {
		setPropPlStatType(stealBtn, false);
		setPropPlStatType(penBtn, false);
		setPropPlStatType(freeBtn, true);
		nStat_->statType = "shoot - free throw";
	});

	int i = 0;
	for (QPushButton *btn : {gk1Btn, gk2Btn, gk3Btn, gk4Btn, gk5Btn, gk6Btn}) {
		const int p = ++i;
		btn->setStyleSheet(css);

		if (!nStat_->statType.startsWith("shoot")) {
			nStat_->statType = "shoot - free throw";
		}

		connect(btn, &QPushButton::clicked, this,
			[this, p, stealBtn, btn, gk1Btn, gk2Btn, gk3Btn, gk4Btn, gk5Btn, gk6Btn]() {
				setPropPlStatType(stealBtn, false);
				for (QPushButton *ibtn : {gk1Btn, gk2Btn, gk3Btn, gk4Btn, gk5Btn, gk6Btn}) {
					if (ibtn != btn)
						setPropPlStatType(ibtn, false);
				}
				setPropPlStatType(btn, true);
				nStat_->goalPosition = p;
			});
	}

	return stat_;
}

FlyPlayerStatDialog::FlyPlayerStatTabUI *FlyPlayerStatDialog::playerStatsWidget(QWidget *playerStatsTabWidget)
{
	stats_ = new FlyPlayerStatTabUI{};
	stats_->playerStatRows = new QVector<FlyPlayerStatUIRow>();
	stats_->title = fly_i18n("Players.Stats");

	stats_->playerStatLayout = new QVBoxLayout(playerStatsTabWidget);
	stats_->playerStatLayout->setContentsMargins(0, 4, 0, 0);
	stats_->playerStatLayout->setSpacing(6);
	stats_->playerStatLayout->setAlignment(Qt::AlignTop);

	QColor c = FlyQtHelper::colorFromU32(isHome_ ? state_.home.color : state_.guest.color);
	QColor cc = FlyQtHelper::getHighContrastColor(c);
	QString css = "QPushButton { font-weight:bold; } QPushButton[plStatType=\"true\"] { background-color: " +
		      c.name() + "; color: " + cc.name() + "; }";

	playerStatsTabWidget->resize(881, 569);

	playerStatsTabWidget->setStyleSheet(
		"QWidget {"
		"   background-image: url('C:\\Users\\gamer\\Documents\\ws\\ls\\dev\\kerezi-handball-scoreboard\\specs\\goal.png');"
		"   background-repeat: no-repeat;"
		"   background-position: center;"
		"}");

	stats_->playerStatLayout->addWidget(playerStatsTabWidget, 1);

	playerStatsTabWidget->setLayout(stat_->playerStatLayout);

	return stats_;
}

FlyPlayerStatDialog::FlyPlayerStatTabUI *FlyPlayerStatDialog::playerListWidget(QWidget *playerAssistTabWidget,
									       bool isHome)
{
	FlyPlayerStatTabUI **st = isHome ? &assist_ : &block_;
	(*st) = new FlyPlayerStatTabUI{};
	(*st)->playerStatRows = new QVector<FlyPlayerStatUIRow>();
	(*st)->title = fly_i18n(isHome ? "Players.Stat.Assist" : "Players.Stat.Block");

	(*st)->playerStatLayout = new QVBoxLayout(playerAssistTabWidget);
	(*st)->playerStatLayout->setContentsMargins(0, 4, 0, 0);
	(*st)->playerStatLayout->setSpacing(6);
	(*st)->playerStatLayout->setAlignment(Qt::AlignTop);

	QColor c = FlyQtHelper::colorFromU32(isHome ? state_.home.color : state_.guest.color);
	QColor cc = FlyQtHelper::getHighContrastColor(c);
	QString css = "QPushButton { font-weight:bold; } QPushButton[plStatType=\"true\"] { background-color: " +
		      c.name() + "; color: " + cc.name() + "; }";

	auto *playerListWidget = new QWidget();
	QGridLayout *l = new QGridLayout(playerListWidget);

	playerListWidget->resize(881, 569);

	QVector<FlyScoreState::FlyPlayer>(*plrs) = (isHome ? &state_.home.players : &state_.guest.players);
	int row = 0;
	for (FlyScoreState::FlyPlayer p : *plrs) {
		QPushButton *plBtn = new QPushButton(QString::number(p.playerTshirtNumber) + " " + p.playerTshirtName);
		plBtn->setStyleSheet(css);

		connect(plBtn, &QPushButton::clicked, this, [this, plBtn, p, isHome]() {
			setPropPlStatType(plBtn, true);
			if (isHome)
				nStat_->assistPlayerNumber = p.playerTshirtNumber;
			else
				nStat_->blockPlayerNumber = p.playerTshirtNumber;
		});
		l->addWidget(plBtn, row++, 0);
	}

	(*st)->playerStatLayout->addWidget(playerListWidget, 1);

	playerAssistTabWidget->setLayout((*st)->playerStatLayout);

	return (*st);
}

FlyPlayerStatDialog::FlyPlayerStatTabUI *FlyPlayerStatDialog::playerPitchWidget(QWidget *playerPitchTabWidget)
{
	pitch_ = new FlyPlayerStatTabUI{};
	pitch_->playerStatRows = new QVector<FlyPlayerStatUIRow>();
	pitch_->title = fly_i18n("Players.Stat.Pitch");

	pitch_->playerStatLayout = new QVBoxLayout(playerPitchTabWidget);
	pitch_->playerStatLayout->setContentsMargins(0, 4, 0, 0);
	pitch_->playerStatLayout->setSpacing(6);
	pitch_->playerStatLayout->setAlignment(Qt::AlignTop);

	QColor c = FlyQtHelper::colorFromU32(isHome_ ? state_.home.color : state_.guest.color);
	QColor cc = FlyQtHelper::getHighContrastColor(c);
	QString css = "QPushButton { font-weight:bold; } QPushButton[plStatType=\"true\"] { background-color: " +
		      c.name() + "; color: " + cc.name() + "; }";

	auto *playerPitchWidget = new QWidget();
	QGridLayout *l = new QGridLayout(playerPitchWidget);

	playerPitchWidget->resize(881, 569);

	QPushButton *pitch00btn = new QPushButton(fly_i18n("Players.Stat.Pitch_00"));
	QPushButton *pitch01btn = new QPushButton(fly_i18n("Players.Stat.Pitch_01"));
	QPushButton *pitch02btn = new QPushButton(fly_i18n("Players.Stat.Pitch_02"));
	QPushButton *pitch03btn = new QPushButton(fly_i18n("Players.Stat.Pitch_03"));
	QPushButton *pitch04btn = new QPushButton(fly_i18n("Players.Stat.Pitch_04"));
	QPushButton *pitch05btn = new QPushButton(fly_i18n("Players.Stat.Pitch_05"));
	QPushButton *pitch06btn = new QPushButton(fly_i18n("Players.Stat.Pitch_06"));
	QPushButton *pitch07btn = new QPushButton(fly_i18n("Players.Stat.Pitch_07"));
	QPushButton *pitch08btn = new QPushButton(fly_i18n("Players.Stat.Pitch_08"));
	QPushButton *pitch09btn = new QPushButton(fly_i18n("Players.Stat.Pitch_09"));
	QPushButton *pitch10btn = new QPushButton(fly_i18n("Players.Stat.Pitch_10"));
	QPushButton *pitch11btn = new QPushButton(fly_i18n("Players.Stat.Pitch_11"));
	QPushButton *pitch12btn = new QPushButton(fly_i18n("Players.Stat.Pitch_12"));
	QPushButton *pitch13btn = new QPushButton(fly_i18n("Players.Stat.Pitch_13"));

	l->addWidget(pitch01btn, 0, 0, 1, 1);
	l->addWidget(pitch00btn, 0, 1, 1, 1);
	l->addWidget(pitch05btn, 0, 2, 1, 1);

	l->addWidget(pitch02btn, 1, 0, 1, 1);
	l->addWidget(pitch03btn, 1, 1, 1, 1);
	l->addWidget(pitch04btn, 1, 2, 1, 1);

	l->addWidget(pitch08btn, 2, 0, 1, 1);
	l->addWidget(pitch07btn, 2, 1, 1, 1);
	l->addWidget(pitch06btn, 2, 2, 1, 1);

	l->addWidget(pitch09btn, 3, 0, 1, 1);
	l->addWidget(pitch10btn, 3, 1, 1, 1);
	l->addWidget(pitch11btn, 3, 2, 1, 1);

	l->addWidget(pitch12btn, 4, 1, 1, 1);

	l->addWidget(pitch13btn, 5, 1, 1, 1);

	pitch_->playerStatLayout->addWidget(playerPitchWidget, 1);

	playerPitchTabWidget->setLayout(pitch_->playerStatLayout);

	int i = 0;
	for (QPushButton *btn : {pitch00btn, pitch01btn, pitch02btn, pitch03btn, pitch04btn, pitch05btn, pitch06btn,
				 pitch07btn, pitch08btn, pitch09btn, pitch10btn, pitch11btn, pitch12btn, pitch13btn}) {
		const int p = i;
		i++;
		btn->setStyleSheet(css);
		connect(btn, &QPushButton::clicked, this,
			[this, p, btn, pitch00btn, pitch01btn, pitch02btn, pitch03btn, pitch04btn, pitch05btn,
			 pitch06btn, pitch07btn, pitch08btn, pitch09btn, pitch10btn, pitch11btn, pitch12btn,
			 pitch13btn]() {
				setPropPlStatType(btn, true);
				for (QPushButton *ibtn : {pitch00btn, pitch01btn, pitch02btn, pitch03btn, pitch04btn,
							  pitch05btn, pitch06btn, pitch07btn, pitch08btn, pitch09btn,
							  pitch10btn, pitch11btn, pitch12btn, pitch13btn}) {
					setPropPlStatType(ibtn, false);
				}
				nStat_->pitchPosition = p;
				// LOGI("Clicked :: " + QString::number(p).toUtf8());
			});
	}
	return pitch_;
}

FlyPlayerStatDialog::FlyPlayerStatUIRow FlyPlayerStatDialog::addPlayerStatUI(const FlyScoreState::FlyPlayerStat &ps,
									     bool canRemove)
{
	FlyPlayerStatUIRow r;
	r.canRemove = canRemove;

	auto *row = new QWidget();
	auto *lay = new QHBoxLayout(row);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(8);

	auto *removeBtn = new QPushButton(row);
	removeBtn->setText(fly_i18n("Players.PlayerRemove"));
	removeBtn->setToolTip(canRemove ? fly_i18n("Stats.RemoveStat") : fly_i18n("Stats.StatCannotBeRemoved"));
	removeBtn->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);

	r.moment = new QLabel(row);
	r.moment->setText(FlyQtHelper::fly_format_ms_mmss(ps.moment_ms));
	r.statRes = new QCheckBox(row);
	r.statRes->setDisabled(true);
	r.statRes->setChecked(ps.success);
	r.plStatType = new QLabel(row);
	r.plStatType->setText(ps.statType);
	r.goalPos = new QLabel(row);
	r.goalPos->setText(QString::number(ps.goalPosition));
	r.pitchPos = new QLabel(row);
	r.pitchPos->setText(QString::number(ps.pitchPosition));
	r.assistPlTSnum = new QLabel(row);
	r.assistPlTSnum->setText(QString::number(ps.assistPlayerNumber));
	r.blockPlTSnum = new QLabel(row);
	r.blockPlTSnum->setText(QString::number(ps.blockPlayerNumber));
	r.row = row;
	const int btnSize = r.moment->sizeHint().height();
	removeBtn->setFixedSize(btnSize, btnSize);
	removeBtn->setDisabled(!canRemove);
	removeBtn->setStyleSheet("QPushButton {"
				 "  font-size:18px;"
				 "  padding:0;"
				 "  min-width:60px;"
				 "  max-width:65px;"
				 "}");

	r.remove = removeBtn;

	lay->addWidget(r.moment, 1, Qt::AlignVCenter | Qt::AlignLeft);
	lay->addWidget(r.statRes, 1, Qt::AlignCenter);
	lay->addWidget(r.plStatType, 1, Qt::AlignCenter);
	lay->addWidget(r.goalPos, 1, Qt::AlignCenter);
	lay->addWidget(r.pitchPos, 1, Qt::AlignCenter);
	lay->addWidget(r.assistPlTSnum, 1, Qt::AlignCenter);
	lay->addWidget(r.blockPlTSnum, 1, Qt::AlignCenter);
	lay->addWidget(r.blockPlTSnum, 1, Qt::AlignCenter);
	lay->addWidget(r.remove, 1, Qt::AlignCenter);

	row->setLayout(lay);

	stats_->playerStatLayout->addWidget(row, Qt::AlignTop | Qt::AlignCenter);

	stats_->playerStatRows->push_back(r);

	if (canRemove) {
		connect(removeBtn, &QPushButton::clicked, this, [this, row]() {
			for (int i = 0; i < stats_->playerStatRows->size(); ++i) {
				if (stats_->playerStatRows->at(i).row == row) {
					auto rr = stats_->playerStatRows->at(i);
					stats_->playerStatRows->removeAt(i);
					if (stats_->playerStatLayout)
						stats_->playerStatLayout->removeWidget(rr.row);
					rr.row->setHidden(true);
					rr.row->deleteLater();
					break;
				}
			}
		});
	}

	return r;
}

void FlyPlayerStatDialog::syncPlayerStatsUiFromState()
{
	for (auto &r : *(stats_->playerStatRows)) {
		if (stats_->playerStatLayout && r.row)
			stats_->playerStatLayout->removeWidget(r.row);
		if (r.row) {
			r.row->setHidden(true);
			r.row->deleteLater();
		}
	}
	stats_->playerStatRows->clear();

	if (!stats_->playerStatRows)
		return;

	for (int i = 0; i < player_->stats.size(); ++i) {
		const FlyScoreState::FlyPlayerStat &ps = player_->stats[i];
		const bool canRemove = true;
		addPlayerStatUI(ps, canRemove);
	}
}

void FlyPlayerStatDialog::syncStateFromPlayerStatsUi()
{
	player_->stats.clear();
	for (int i = 0; i < stats_->playerStatRows->size(); ++i) {
		const FlyPlayerStatUIRow &opr = stats_->playerStatRows->at(i);
		FlyScoreState::FlyPlayerStat ps = FlyScoreState::FlyPlayerStat{};
		ps.moment_ms = FlyQtHelper::fly_parse_mmss_to_ms(opr.moment->text());
		ps.success = opr.statRes->isChecked();
		ps.statType = opr.plStatType->text();
		ps.goalPosition = opr.goalPos->text().toInt();
		ps.pitchPosition = opr.pitchPos->text().toInt();
		ps.assistPlayerNumber = opr.assistPlTSnum->text().toInt();
		ps.blockPlayerNumber = opr.blockPlTSnum->text().toInt();
		player_->stats.push_back(ps);
	}
}

void FlyPlayerStatDialog::syncUiFromState()
{
	syncPlayerStatsUiFromState();
}

void FlyPlayerStatDialog::syncStateFromUi()
{
	syncStateFromPlayerStatsUi();
	player_->stats.push_back(*nStat_);
}

void FlyPlayerStatDialog::setPropPlStatType(QWidget *w, bool val)
{
	setProp(w, "plStatType", val);
}

void FlyPlayerStatDialog::setProp(QWidget *w, const char *prop, QVariant &&val)
{
	w->setProperty(prop, val);
	w->style()->unpolish(w);
	w->style()->polish(w);
	w->update();
}