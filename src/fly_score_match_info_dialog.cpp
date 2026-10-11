#include "fly_score_match_info_dialog.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][teams-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"

#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QToolButton>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStyle>

FlyMatchInfoDialog::FlyMatchInfoDialog(const QString &dataDir, FlyState &state, QWidget *parent)
	: QDialog(parent),
	  dataDir_(dataDir),
	  state_(state)
{
	setObjectName(QStringLiteral("FlyMatchInfoDialog"));
	setWindowTitle(fly_i18n("MatchInfo.Title"));
	setModal(false);

	setMinimumWidth(480);
	resize(520, 420);
	setSizeGripEnabled(false);

	auto *root = new QVBoxLayout(this);
	root->setContentsMargins(14, 14, 14, 14);
	root->setSpacing(10);

	// --->
	auto *hint = new QLabel(fly_i18n("MatchInfo.Hint"),
		this);
	hint->setObjectName(QStringLiteral("matchInfoHint"));
	hint->setWordWrap(true);
	root->addWidget(hint);

	auto *gbMatch = new QGroupBox(fly_i18n("MatchInfo.Match"), this);
	gbMatch->setObjectName(QStringLiteral("matchGroup"));

	auto *mi = new QGridLayout(gbMatch);
	mi->setContentsMargins(10, 10, 10, 10);
	mi->setHorizontalSpacing(8);
	mi->setVerticalSpacing(6);

	competition_ = new QLineEdit(gbMatch);
	competitionRound_   = new QLineEdit(gbMatch);
	matchNumber_ = new QSpinBox(gbMatch);
	matchNumber_->setRange(1000, 9999);
	matchNumber_->setMinimumWidth(70);
	matchNumber_->setMaximumWidth(90);

	datetime_ = new QDateTimeEdit(gbMatch);
	city_  = new QLineEdit(gbMatch);
	venue_  = new QLineEdit(gbMatch);
	spectators_ =  new QSpinBox(gbMatch);
	spectators_->setRange(0, 99999);
	spectators_->setMinimumWidth(70);
	spectators_->setMaximumWidth(150);

	mi->addWidget(new QLabel(fly_i18n("MatchInfo.Competition"), gbMatch), 0, 0);
	mi->addWidget(competition_, 0, 1, 1, 2);

	mi->addWidget(new QLabel(fly_i18n("MatchInfo.CompetitionRound"), gbMatch), 1, 0);
	mi->addWidget(competitionRound_, 1, 1);

	mi->addWidget(new QLabel(fly_i18n("MatchInfo.MatchNumber"), gbMatch), 2, 0);
	mi->addWidget(matchNumber_, 2, 1);

	mi->addWidget(new QLabel(fly_i18n("MatchInfo.DateAndTime"), gbMatch), 3, 0);
	mi->addWidget(datetime_, 3, 1);

	mi->addWidget(new QLabel(fly_i18n("MatchInfo.City"), gbMatch), 4, 0);
	mi->addWidget(city_, 4, 1);

	mi->addWidget(new QLabel(fly_i18n("MatchInfo.Venue"), gbMatch), 5, 0);
	mi->addWidget(venue_, 5, 1);

    mi->addWidget(new QLabel(fly_i18n("MatchInfo.Spectators"), gbMatch), 6, 0);
    mi->addWidget(spectators_, 6, 1);

	gbMatch->setLayout(mi);


	auto *buttonsRow = new QHBoxLayout();
	buttonsRow->setContentsMargins(0, 0, 0, 0);
	buttonsRow->setSpacing(8);

	buttonsRow->addStretch(1);

	applyBtn_ = new QPushButton(fly_i18n("Common.SaveAndClose"), this);
	auto *closeBtn = new QPushButton(fly_i18n("Common.Close"), this);

	applyBtn_->setCursor(Qt::PointingHandCursor);
	closeBtn->setCursor(Qt::PointingHandCursor);

	buttonsRow->addWidget(applyBtn_);
	buttonsRow->addWidget(closeBtn);

	root->addWidget(gbMatch);
	root->addLayout(buttonsRow);
	setLayout(root);

	syncUiFromState();

	connect(applyBtn_, &QPushButton::clicked, this, &FlyMatchInfoDialog::onApply);
	connect(closeBtn, &QPushButton::clicked, this, &FlyMatchInfoDialog::close);
}

FlyMatchInfoDialog::~FlyMatchInfoDialog() = default;

void FlyMatchInfoDialog::syncUiFromState()
{
	if (competition_)
		competition_->setText(state_.matchInfo.competition);
	if (competitionRound_)
		competitionRound_->setText(state_.matchInfo.competitionRound);
	if (matchNumber_)
		matchNumber_->setValue(state_.matchInfo.matchNumber);
	if (datetime_)
		datetime_->setDateTime(state_.matchInfo.datetime);
	if (city_)
		city_->setText(state_.matchInfo.city);
	if (venue_)
		venue_->setText(state_.matchInfo.venue);
	if (spectators_)
		spectators_->setValue(state_.matchInfo.spectators);
}

void FlyMatchInfoDialog::syncStateFromUi()
{
	if (competition_)
		state_.matchInfo.competition = competition_->text();
	if (competitionRound_)
		state_.matchInfo.competitionRound = competitionRound_->text();
	if (matchNumber_)
		state_.matchInfo.matchNumber = matchNumber_->value();
	if (datetime_)
		state_.matchInfo.datetime = datetime_->dateTime();
	if (city_)
		state_.matchInfo.city = city_->text();
	if (venue_)
		state_.matchInfo.venue = venue_->text();
	if (spectators_)
		state_.matchInfo.spectators = spectators_->value();
}

void FlyMatchInfoDialog::onApply()
{
	syncStateFromUi();
	fly_state_save(dataDir_, state_);
	LOGI("Match Info dialog: competition/round/match number/datetime/city/venue/spectators saved.");
	accept();
}
