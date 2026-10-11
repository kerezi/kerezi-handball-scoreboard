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
	: FlyBaseDialog(fly_i18n("MatchInfo.Title"),
					fly_i18n("MatchInfo.Hint"),
					fly_i18n("MatchInfo.Match"),
					dataDir, state, parent)
{
	competition_ = new QLineEdit(gb_);
	competitionRound_   = new QLineEdit(gb_);
	matchNumber_ = new QSpinBox(gb_);
	matchNumber_->setRange(1000, 9999);
	matchNumber_->setMinimumWidth(70);
	matchNumber_->setMaximumWidth(90);

	datetime_ = new QDateTimeEdit(gb_);
	city_  = new QLineEdit(gb_);
	venue_  = new QLineEdit(gb_);
	spectators_ =  new QSpinBox(gb_);
	spectators_->setRange(0, 99999);
	spectators_->setMinimumWidth(70);
	spectators_->setMaximumWidth(150);

	gl_->addWidget(new QLabel(fly_i18n("MatchInfo.Competition"), gb_), 0, 0);
	gl_->addWidget(competition_, 0, 1, 1, 2);

	gl_->addWidget(new QLabel(fly_i18n("MatchInfo.CompetitionRound"), gb_), 1, 0);
	gl_->addWidget(competitionRound_, 1, 1);

	gl_->addWidget(new QLabel(fly_i18n("MatchInfo.MatchNumber"), gb_), 2, 0);
	gl_->addWidget(matchNumber_, 2, 1);

	gl_->addWidget(new QLabel(fly_i18n("MatchInfo.DateAndTime"), gb_), 3, 0);
	gl_->addWidget(datetime_, 3, 1);

	gl_->addWidget(new QLabel(fly_i18n("MatchInfo.City"), gb_), 4, 0);
	gl_->addWidget(city_, 4, 1);

	gl_->addWidget(new QLabel(fly_i18n("MatchInfo.Venue"), gb_), 5, 0);
	gl_->addWidget(venue_, 5, 1);

    gl_->addWidget(new QLabel(fly_i18n("MatchInfo.Spectators"), gb_), 6, 0);
    gl_->addWidget(spectators_, 6, 1);

	FlyBaseDialog::addButtons();
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