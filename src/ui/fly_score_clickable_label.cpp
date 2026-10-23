#include "fly_score_clickable_label.hpp"

#include "fly_score_state.hpp"
#include "fly_score_i18n.hpp"

#include <QStringBuilder>

FlyClickableLabel::FlyClickableLabel(FlyScoreState::FlyState &state, QWidget *parent) : QLabel(parent), state_(state) {}

void FlyClickableLabel::mousePressEvent(QMouseEvent *event)
{
	if (event->button() == Qt::LeftButton) {
		emit clicked(); // Излъчваме сигнала
	}

	this->changeStyle();

	QLabel::mousePressEvent(event);
}

void FlyClickableLabel::changeStyle()
{
	QString css = this->styleSheet() +
		      " color : " % (this->state_.timers[0].running ? QString(" green ") : QString(" red ")) % ";";
	this->setToolTip(this->state_.timers[0].running ? fly_i18n("Dock.PauseTimer") : fly_i18n("Dock.StartTimer"));
	this->setStyleSheet(css);
}

void FlyClickableLabel::refreshUI()
{
	changeStyle();
}
