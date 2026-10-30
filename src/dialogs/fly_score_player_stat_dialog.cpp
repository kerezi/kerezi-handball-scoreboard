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
#include <QComboBox>

FlyPlayerStatDialog::FlyPlayerStatDialog(const QString &dataDir, FlyScoreState::FlyPlayer *player, bool isHome,
					 FlyScoreState::FlyState &state, QWidget *parent)
	: FlyBaseDialog(fly_i18n("Players.Title"), fly_i18n("Players.Hint"), fly_i18n("Dock.Players"), dataDir, state,
			parent),
	  player_(player),
	  isHome_(isHome)
{
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
		[this](QWidget *statTabWidget) -> FlyPlayerStatTabUI * { return playerShotStatWidget(statTabWidget); },
		[this](QWidget *statTabWidget) -> FlyPlayerStatTabUI * { return playerAssistStatWidget(statTabWidget); }};

	for (const auto &getWidget : playerStatsTabs) {
		QWidget *statTabWidget = new QWidget();
		FlyPlayerStatDialog::FlyPlayerStatTabUI *statTab = getWidget(statTabWidget);
		addPlayerStatTabUI(tabs, statTab, statTabWidget);
	}

	gl_->addWidget(tabs);

	// connect(home_->addPlayersBtn, &QPushButton::clicked, this, &FlyPlayerStatDialog::onAddHomePlayer);
	// connect(guest_->addPlayersBtn, &QPushButton::clicked, this, &FlyPlayerStatDialog::onAddGuestPlayer);

	addButtons();
}

FlyPlayerStatDialog::~FlyPlayerStatDialog() = default;

FlyPlayerStatDialog::FlyPlayerStatTabUI *FlyPlayerStatDialog::playerShotStatWidget(QWidget *playerShotTabWidget)
{
	shot_ = new FlyPlayerStatTabUI{};
	shot_->playerStatRows = new QVector<FlyPlayerStatUIRow>();
	shot_->title = fly_i18n("Players.Stat.Shot");

	shot_->playerStatLayout = new QVBoxLayout(playerShotTabWidget);
	shot_->playerStatLayout->setContentsMargins(0, 4, 0, 0);
	shot_->playerStatLayout->setSpacing(6);
	shot_->playerStatLayout->setAlignment(Qt::AlignTop);

	QColor c = FlyQtHelper::colorFromU32(isHome_ ? state_.home.color : state_.guest.color);
	QColor cc = FlyQtHelper::getHighContrastColor(c);
	QString css = "background-color: " + c.name() + "; color: " + cc.name() + ";";

	auto *playerShotWidget = new QWidget();
	QGridLayout *l = new QGridLayout(playerShotWidget);

	playerShotWidget->resize(881, 569);

	playerShotWidget->setStyleSheet(
		"QWidget {"
		"   background-image: url('C:\\Users\\gamer\\Documents\\ws\\ls\\dev\\kerezi-handball-scoreboard\\specs\\goal.png');"
		"   background-repeat: no-repeat;"
		"   background-position: center;"
		"}");

	QPushButton *btn1 = new QPushButton("Бутон 1");
	QPushButton *btn2 = new QPushButton("Бутон 2");
	QPushButton *btn3 = new QPushButton("Бутон 3");
	QPushButton *btn4 = new QPushButton("Бутон 4");
	QPushButton *btn5 = new QPushButton("Бутон 5");
	QPushButton *btn6 = new QPushButton("Бутон 6");

	l->addWidget(btn1, 0, 0);
	l->addWidget(btn2, 0, 1);
	l->addWidget(btn3, 0, 2);
	l->addWidget(btn4, 1, 0);
	l->addWidget(btn5, 1, 1);
	l->addWidget(btn6, 1, 2);

	shot_->playerStatLayout->addWidget(playerShotWidget, 1);

	playerShotTabWidget->setLayout(shot_->playerStatLayout);

	return shot_;
}

FlyPlayerStatDialog::FlyPlayerStatTabUI *FlyPlayerStatDialog::playerAssistStatWidget(QWidget *playerAssistTabWidget)
{
	assist_ = new FlyPlayerStatTabUI{};
	assist_->playerStatRows = new QVector<FlyPlayerStatUIRow>();
	assist_->title = fly_i18n("Players.Stat.Assist");

	assist_->playerStatLayout = new QVBoxLayout(playerAssistTabWidget);
	assist_->playerStatLayout->setContentsMargins(0, 4, 0, 0);
	assist_->playerStatLayout->setSpacing(6);
	assist_->playerStatLayout->setAlignment(Qt::AlignTop);

	QColor c = FlyQtHelper::colorFromU32(isHome_ ? state_.home.color : state_.guest.color);
	QColor cc = FlyQtHelper::getHighContrastColor(c);
	QString css = "background-color: " + c.name() + "; color: " + cc.name() + ";";

	auto *playerAssistWidget = new QWidget();
	QGridLayout *l = new QGridLayout(playerAssistWidget);

	playerAssistWidget->resize(881, 569);

	QComboBox *selectPlayer = new QComboBox();
	QVector<FlyScoreState::FlyPlayer>(*plrs) = (isHome_ ? &state_.home.players : &state_.guest.players);
	for (FlyScoreState::FlyPlayer p : *plrs) {
		selectPlayer->addItem(p.playerTshirtName);
	}

	assist_->playerStatLayout->addWidget(playerAssistWidget, 1);

	playerAssistTabWidget->setLayout(assist_->playerStatLayout);

	return assist_;
}

void FlyPlayerStatDialog::addPlayerStatTabUI(FlyHorizontalTabWidget *tabs,
					     FlyPlayerStatDialog::FlyPlayerStatTabUI *statTab, QWidget *playerStatTab)
{
	QColor c = FlyQtHelper::colorFromU32(isHome_ ? state_.home.color : state_.guest.color);
	QColor cc = FlyQtHelper::getHighContrastColor(c);
	QString css = "background-color: " + c.name() + "; color: " + cc.name() + ";";

	auto *plrStatHdr = new QWidget();
	auto *plrStatHdrLay = new QHBoxLayout(plrStatHdr);
	QLabel *lbl = new QLabel(fly_i18n("Players.PlayerTshirtNumber"), plrStatHdr);
	lbl->setStyleSheet("QLabel {font-size: 20pt;min-width: 60px;min-width: 65px; " + css + "}");
	plrStatHdrLay->addWidget(lbl, 1, Qt::AlignVCenter | Qt::AlignLeft);
	plrStatHdrLay->addWidget(new QLabel(fly_i18n("Players.PlayerTshirtName"), plrStatHdr), 2, Qt::AlignCenter);
	plrStatHdrLay->addWidget(new QLabel(fly_i18n("Players.PlayerName"), plrStatHdr), 2, Qt::AlignCenter);
	statTab->addPlayerStatBtn = new QPushButton(fly_i18n("Players.PlayerAdd"));

	statTab->addPlayerStatBtn->setCursor(Qt::PointingHandCursor);
	statTab->addPlayerStatBtn->setStyleSheet("QPushButton {font-size: 18pt; min-width: 60px;min-width: 65px; " +
						 css + ";}");
	plrStatHdrLay->addWidget(statTab->addPlayerStatBtn, 1, Qt::AlignVCenter | Qt::AlignRight);
	plrStatHdr->setLayout(plrStatHdrLay);
	statTab->playerStatLayout->addWidget(plrStatHdr);

	tabs->addTab(playerStatTab, statTab->title);
	QString fcss = "FlyHorizontalTabBar::tab";
	if (isHome_)
		fcss = fcss + "";
	else
		fcss = fcss + ":last";
	fcss = fcss + ":selected { " + css + ";}";
	tabs->setStyleSheet(fcss);
};

// void FlyPlayerStatDialog::onAddPlayer(FlyPlayersUI *playersUI)
// {
	// FlyScoreState::FlyPlayer fp;
	// fp.goals = QVector<FlyScoreState::FlyGoal>();
// 
	// addPlayerUI(playersUI, fp, playersUI->playersRows->size() > 10);
	// playersUI->addPlayersBtn->setDisabled(playersUI->playersRows->size() > 15);
	// refreshRemoveButtons(playersUI->playersRows);
// }

// void FlyPlayerStatDialog::onAddHomePlayer()
// {
	// onAddPlayer(home_);
// }

// void FlyPlayerStatDialog::onAddGuestPlayer()
// {
	// onAddPlayer(guest_);
// }

void FlyPlayerStatDialog::refreshRemoveButtons(QVector<FlyPlayerStatUIRow> *rows)
{
	const bool canRemove = rows->size() > 10;
	for (auto &r : *rows) {
		r.canRemove = canRemove;
		if (r.remove) {
			r.remove->setDisabled(!canRemove);
			r.remove->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);
			r.remove->setToolTip(canRemove ? fly_i18n("Stats.RemoveStat")
						       : fly_i18n("Stats.OneTeamStatRequired"));
		}
	}
}

// FlyPlayerStatDialog::FlyPlayerUIRow FlyPlayerStatDialog::addPlayerUI(const FlyPlayersUI *playersUI,
// 								     const FlyScoreState::FlyPlayer &fp, bool canRemove)
// {
// 	FlyPlayerUIRow r;
// 	r.canRemove = canRemove;

// 	auto *row = new QWidget();
// 	auto *lay = new QHBoxLayout(row);
// 	lay->setContentsMargins(0, 0, 0, 0);
// 	lay->setSpacing(8);

// 	auto *playerTshirtNumberSB = new QSpinBox(row);
// 	playerTshirtNumberSB->setRange(1, 99);
// 	playerTshirtNumberSB->setMinimumWidth(70);
// 	playerTshirtNumberSB->setMaximumWidth(90);
// 	playerTshirtNumberSB->setValue(fp.playerTshirtNumber);
// 	if (fp.playerTshirtNumber == 0) {
// 		playerTshirtNumberSB->setValue(playersUI->playersRows->size() + 1);
// 	}

// 	auto *playerTshirtNameEdit = new QLineEdit(row);
// 	playerTshirtNameEdit->setPlaceholderText(fly_i18n("Players.PlayerNameLabelPlaceholder"));
// 	playerTshirtNameEdit->setText(fp.playerTshirtName);
// 	playerTshirtNameEdit->setMinimumWidth(180);
// 	playerTshirtNameEdit->setMaximumWidth(260);
// 	if (fp.playerTshirtName == nullptr || fp.playerTshirtName == "") {
// 		playerTshirtNameEdit->setText(fly_i18n("Players.PlayerTshirtName") + " # " +
// 					      QString::number(playersUI->playersRows->size() + 1));
// 	}

// 	auto *playerNameEdit = new QLineEdit(row);
// 	playerNameEdit->setPlaceholderText(fly_i18n("Players.PlayerNameLabelPlaceholder"));
// 	playerNameEdit->setText(fp.playerName);
// 	playerNameEdit->setMinimumWidth(180);
// 	playerNameEdit->setMaximumWidth(260);
// 	if (fp.playerName == nullptr || fp.playerName == "") {
// 		playerNameEdit->setText(fly_i18n("Players.PlayerName") + " # " +
// 					QString::number(playersUI->playersRows->size() + 1));
// 	}

// 	auto *removeBtn = new QPushButton(row);
// 	removeBtn->setText(fly_i18n("Players.PlayerRemove"));
// 	removeBtn->setToolTip(canRemove ? fly_i18n("Stats.RemoveStat") : fly_i18n("Stats.StatCannotBeRemoved"));
// 	removeBtn->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);

// 	const int btnSize = playerNameEdit->sizeHint().height();
// 	removeBtn->setFixedSize(btnSize, btnSize);
// 	removeBtn->setDisabled(!canRemove);
// 	removeBtn->setStyleSheet("QPushButton {"
// 				 "  font-size:18px;"
// 				 "  padding:0;"
// 				 "  min-width:60px;"
// 				 "  max-width:65px;"
// 				 "}");

// 	lay->addWidget(playerTshirtNumberSB, 1, Qt::AlignVCenter | Qt::AlignLeft);
// 	lay->addWidget(playerTshirtNameEdit, 2, Qt::AlignCenter);
// 	lay->addWidget(playerNameEdit, 2, Qt::AlignCenter);
// 	lay->addWidget(removeBtn, 1, Qt::AlignVCenter | Qt::AlignRight);

// 	row->setLayout(lay);
// 	playersUI->playersLayout->addWidget(row, Qt::AlignTop | Qt::AlignCenter);

// 	r.row = row;
// 	r.playerTshirtNumberSB = playerTshirtNumberSB;
// 	r.playerTshirtNameEdit = playerTshirtNameEdit;
// 	r.playerNameEdit = playerNameEdit;
// 	r.remove = removeBtn;
// 	playersUI->playersRows->push_back(r);

// 	if (canRemove) {
// 		connect(removeBtn, &QPushButton::clicked, this, [this, playersUI, row]() {
// 			if (playersUI->playersRows->size() <= 10) {
// 				QMessageBox::warning(this, fly_i18n("Common.CannotRemove"),
// 						     fly_i18n("Stats.OneTeamStatRequired"));
// 				return;
// 			}

// 			for (int i = 0; i < playersUI->playersRows->size(); ++i) {
// 				if (playersUI->playersRows->at(i).row == row) {
// 					auto rr = playersUI->playersRows->at(i);
// 					playersUI->playersRows->removeAt(i);
// 					if (playersUI->playersLayout)
// 						playersUI->playersLayout->removeWidget(rr.row);
// 					rr.row->setHidden(true);
// 					rr.row->deleteLater();
// 					break;
// 				}
// 			}

// 			playersUI->addPlayersBtn->setDisabled(playersUI->playersRows->size() > 15);
// 			refreshRemoveButtons(playersUI->playersRows);
// 		});
// 	}

// 	return r;
// }

// void FlyPlayerStatDialog::syncTeamUiFromState(FlyPlayerStatUIRow *playersUI, FlyScoreState::FlyTeam *team)
// {
// 	for (auto &r : *(playersUI->playersRows)) {
// 		if (playersUI->playersLayout && r.row)
// 			playersUI->playersLayout->removeWidget(r.row);
// 		if (r.row) {
// 			r.row->setHidden(true);
// 			r.row->deleteLater();
// 		}
// 	}
// 	playersUI->playersRows->clear();

// 	if (!playersUI->playersLayout)
// 		return;

// 	for (int i = 0; i < team->players.size(); ++i) {
// 		const FlyScoreState::FlyPlayer &fp = team->players[i];
// 		const bool canRemove = team->players.size() > 10;
// 		addPlayerUI(playersUI, fp, canRemove);
// 		playersUI->addPlayersBtn->setDisabled(playersUI->playersRows->size() > 15);
// 	}

// 	refreshRemoveButtons(playersUI->playersRows);
// }

// void FlyPlayerStatDialog::syncStateFromTeamUi(FlyPlayersUI *playersUI, FlyScoreState::FlyTeam *team)
// {
// 	team->players.clear();
// 	for (int i = 0; i < playersUI->playersRows->size(); ++i) {
// 		const FlyPlayerUIRow &opr = playersUI->playersRows->at(i);
// 		FlyScoreState::FlyPlayer fp = FlyScoreState::FlyPlayer{};
// 		fp.playerTshirtNumber = opr.playerTshirtNumberSB->value();
// 		fp.playerTshirtName = opr.playerTshirtNameEdit->text();
// 		fp.playerName = opr.playerNameEdit->text();
// 		team->players.push_back(fp);
// 	}

// 	refreshRemoveButtons(playersUI->playersRows);
// }

void FlyPlayerStatDialog::syncUiFromState()
{
	// syncTeamUiFromState(home_, &state_.home);
	// syncTeamUiFromState(guest_, &state_.guest);
}

void FlyPlayerStatDialog::syncStateFromUi()
{
	// syncStateFromTeamUi(home_, &state_.home);
	// syncStateFromTeamUi(guest_, &state_.guest);
}
