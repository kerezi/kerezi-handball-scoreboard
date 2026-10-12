#include "fly_score_players_dialog.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][teams-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_logo_helpers.hpp"
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

FlyPlayersDialog::FlyPlayersDialog(const QString &dataDir, FlyState &state, QWidget *parent)
	: FlyBaseDialog(fly_i18n("Players.Title"), fly_i18n("Players.Hint"), fly_i18n("Dock.Players"), dataDir, state, parent)
{
	auto *tabs = new QTabWidget(this);
	tabs->setObjectName(fly_i18n("Dock.Players"));
	tabs->setDocumentMode(true);
	tabs->setMovable(false);
	tabs->setUsesScrollButtons(true);

	home_ = new FlyPlayersUI{};
	home_->playersRows = new QVector<FlyPlayerUIRow>();
	addPlayersUI(tabs, home_, "Teams.HomeTeam");

	guest_ = new FlyPlayersUI{};
	guest_->playersRows = new QVector<FlyPlayerUIRow>();
	addPlayersUI(tabs, guest_, "Teams.GuestTeam");

	gl_->addWidget(tabs);

	connect(home_->addPlayersBtn, &QPushButton::clicked, this, &FlyPlayersDialog::onAddHomePlayer);
	connect(guest_->addPlayersBtn, &QPushButton::clicked, this, &FlyPlayersDialog::onAddGuestPlayer);

	addButtons();
}

FlyPlayersDialog::~FlyPlayersDialog() = default;

void FlyPlayersDialog::addPlayersUI(QTabWidget *tabs, FlyPlayersUI *playersUI, const char *name)
{
	auto *playersTab = new QGroupBox(tabs);

	playersUI->playersLayout = new QVBoxLayout(tabs);
	playersUI->playersLayout->setContentsMargins(0, 4, 0, 0);
	playersUI->playersLayout->setSpacing(6);
	playersUI->playersLayout->setAlignment(Qt::AlignTop);

	auto *plrsHdr = new QWidget(tabs);
	auto *plrsHdrLay = new QHBoxLayout(playersTab);
	QLabel *lbl = new QLabel(fly_i18n("Players.PlayerTshirtNumber"), plrsHdr);
	lbl->setStyleSheet("QLabel {font-size: 20pt;min-width: 60px;min-width: 65px;}");
	plrsHdrLay->addWidget(lbl, 1, Qt::AlignVCenter|Qt::AlignLeft);
	plrsHdrLay->addWidget(new QLabel(fly_i18n("Players.PlayerTshirtName"), plrsHdr), 2, Qt::AlignCenter);
	plrsHdrLay->addWidget(new QLabel(fly_i18n("Players.PlayerName"), plrsHdr), 2, Qt::AlignCenter);
	playersUI->addPlayersBtn =
		new QPushButton(fly_i18n("Players.PlayerAdd"), playersTab);
	playersUI->addPlayersBtn->setCursor(Qt::PointingHandCursor);
	playersUI->addPlayersBtn->setStyleSheet("QPushButton {font-size: 18pt; min-width: 60px;min-width: 65px;}");
	plrsHdrLay->addWidget(playersUI->addPlayersBtn, 1, Qt::AlignVCenter|Qt::AlignRight);
	plrsHdr->setLayout(plrsHdrLay);
	playersUI->playersLayout->addWidget(plrsHdr);

	playersTab->setLayout(playersUI->playersLayout);

	tabs->addTab(playersTab, fly_i18n(name));
};


void FlyPlayersDialog::onAddPlayer(FlyPlayersUI *playersUI)
{
	FlyPlayer fp;
	fp.goals = QVector<FlyGoal>();

	addPlayerUI(playersUI, fp, playersUI->playersRows->size() > 10);
	playersUI->addPlayersBtn->setDisabled(playersUI->playersRows->size() > 15);
	refreshRemoveButtons(playersUI->playersRows);
}

void FlyPlayersDialog::onAddHomePlayer()
{
	onAddPlayer(home_);
}

void FlyPlayersDialog::onAddGuestPlayer()
{
	onAddPlayer(guest_);
}

void FlyPlayersDialog::refreshRemoveButtons(QVector<FlyPlayerUIRow> *rows)
{
	const bool canRemove = rows->size() > 10;
	for (auto &r : *rows) {
		r.canRemove = canRemove;
		if (r.remove) {
			r.remove->setDisabled(!canRemove);
			r.remove->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);
			r.remove->setToolTip(canRemove ? fly_i18n("Fields.RemoveField")
							   : fly_i18n("Fields.OneTeamStatRequired"));
		}
	}
}

FlyPlayersDialog::FlyPlayerUIRow FlyPlayersDialog::addPlayerUI(const FlyPlayersUI *playersUI, const FlyPlayer &fp,
							       bool canRemove)
{
	FlyPlayerUIRow r;
	r.canRemove = canRemove;

	auto *row = new QWidget(this);
	auto *lay = new QHBoxLayout(row);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(8);

	auto *playerTshirtNumberSB = new QSpinBox(gb_);
	playerTshirtNumberSB->setRange(1, 99);
	playerTshirtNumberSB->setMinimumWidth(70);
	playerTshirtNumberSB->setMaximumWidth(90);
	playerTshirtNumberSB->setValue(fp.playerTshirtNumber);
	if (fp.playerTshirtNumber == 0) {
		playerTshirtNumberSB->setValue(playersUI->playersRows->size() + 1);
	}

	auto *playerTshirtNameEdit = new QLineEdit(row);
	playerTshirtNameEdit->setPlaceholderText(fly_i18n("Players.PlayerNameLabelPlaceholder"));
	playerTshirtNameEdit->setText(fp.playerName);
	playerTshirtNameEdit->setMinimumWidth(180);
	playerTshirtNameEdit->setMaximumWidth(260);
	if (fp.playerTshirtName == nullptr || fp.playerTshirtName == "") {
		playerTshirtNameEdit->setText(fly_i18n("Players.PlayerTshirtName") + " # " + QString::number(playersUI->playersRows->size() + 1));
	}

	auto *playerNameEdit = new QLineEdit(row);
	playerNameEdit->setPlaceholderText(fly_i18n("Players.PlayerNameLabelPlaceholder"));
	playerNameEdit->setText(fp.playerName);
	playerNameEdit->setMinimumWidth(180);
	playerNameEdit->setMaximumWidth(260);
	if (fp.playerName == nullptr || fp.playerName == "") {
		playerNameEdit->setText(fly_i18n("Players.PlayerName") + " # " + QString::number(playersUI->playersRows->size() + 1));
	}

	auto *removeBtn = new QPushButton(row);
	removeBtn->setText(fly_i18n("Players.PlayerRemove"));
	removeBtn->setToolTip(canRemove ? fly_i18n("Fields.RemoveField") : fly_i18n("Fields.FieldCannotBeRemoved"));
	removeBtn->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);

	const int btnSize = playerNameEdit->sizeHint().height();
	removeBtn->setFixedSize(btnSize, btnSize);
	removeBtn->setDisabled(!canRemove);
	removeBtn->setStyleSheet("QPushButton {"
				 "  font-size:18px;"
				 "  padding:0;"
				 "  min-width:60px;"
				 "  max-width:65px;"
				 "}");

	lay->addWidget(playerTshirtNumberSB, 1, Qt::AlignVCenter | Qt::AlignLeft);
	lay->addWidget(playerTshirtNameEdit, 2, Qt::AlignCenter);
	lay->addWidget(playerNameEdit, 2, Qt::AlignCenter);
	lay->addWidget(removeBtn, 1, Qt::AlignVCenter | Qt::AlignRight);

	row->setLayout(lay);
	playersUI->playersLayout->addWidget(row);

	r.row = row;
	r.playerTshirtNumberSB = playerTshirtNumberSB;
	r.playerTshirtNameEdit = playerTshirtNameEdit;
	r.playerNameEdit = playerNameEdit;
	r.remove = removeBtn;
	playersUI->playersRows->push_back(r);

	if (canRemove) {
		connect(removeBtn, &QPushButton::clicked, this, [this, playersUI, row]() {
			if (playersUI->playersRows->size() <= 1) {
				QMessageBox::warning(this, fly_i18n("Common.CannotRemove"),
						     fly_i18n("Fields.OneTeamStatRequired"));
				return;
			}

			for (int i = 0; i < playersUI->playersRows->size(); ++i) {
				if (playersUI->playersRows->at(i).row == row) {
					auto rr = playersUI->playersRows->at(i);
					playersUI->playersRows->removeAt(i);
					if (playersUI->playersLayout)
						playersUI->playersLayout->removeWidget(rr.row);
					rr.row->deleteLater();
					break;
				}
			}

			playersUI->addPlayersBtn->setDisabled(playersUI->playersRows->size() > 15);
			refreshRemoveButtons(playersUI->playersRows);
		});
	}

	return r;
}

void FlyPlayersDialog::syncTeamUiFromState(FlyPlayersUI *playersUI, FlyTeam *team)
{
	for (auto &r : *(playersUI->playersRows)) {
		if (playersUI->playersLayout && r.row)
			playersUI->playersLayout->removeWidget(r.row);
		if (r.row)
			r.row->deleteLater();
	}

	playersUI->playersRows->clear();
	// vis_.clear();

	if (!playersUI->playersLayout)
		return;

	for (int i = 0; i < team->players.size(); ++i) {
		const FlyPlayer &fp = team->players[i];
		const bool canRemove = team->players.size() > 10;
		addPlayerUI(playersUI, fp, canRemove);
		playersUI->addPlayersBtn->setDisabled(playersUI->playersRows->size() > 15);
	}

	refreshRemoveButtons(playersUI->playersRows);
}

void FlyPlayersDialog::syncStateFromTeamUi(FlyPlayersUI *playersUI, FlyTeam *team)
{
	team->players.clear();
	for (int i = 0; i < playersUI->playersRows->size(); ++i) {
		const FlyPlayerUIRow &opr = playersUI->playersRows->at(i);
		FlyPlayer fp = FlyPlayer{};
		fp.playerTshirtNumber = opr.playerTshirtNumberSB->value();
		fp.playerTshirtName = opr.playerTshirtNameEdit->text();
		fp.playerName = opr.playerNameEdit->text();
		team->players.push_back(fp);
	}

	refreshRemoveButtons(playersUI->playersRows);
}

void FlyPlayersDialog::syncUiFromState()
{
	syncTeamUiFromState(home_, &state_.home);
	syncTeamUiFromState(guest_, &state_.guest);
}

void FlyPlayersDialog::syncStateFromUi()
{
	syncStateFromTeamUi(home_, &state_.home);
	syncStateFromTeamUi(guest_, &state_.guest);
}
