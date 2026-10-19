#ifndef FLY_SCORE_CLICKABLE_LABEL
#define FLY_SCORE_CLICKABLE_LABEL

#pragma once

#include "fly_score_state.hpp"

#include <QLabel>
#include <QMouseEvent>

class FlyClickableLabel : public QLabel {
    Q_OBJECT

public:
    explicit FlyClickableLabel(FlyScoreState::FlyState &state, QWidget* parent = nullptr);
    explicit FlyClickableLabel(FlyScoreState::FlyState &state, QString s):QLabel(s),state_(state) {};

    void refreshUI();
signals:
    void clicked();

protected:
    void mousePressEvent(QMouseEvent* event) override;

private:
	FlyScoreState::FlyState &state_;
    void changeStyle();
};

#endif // CLICKABLELABEL_H
