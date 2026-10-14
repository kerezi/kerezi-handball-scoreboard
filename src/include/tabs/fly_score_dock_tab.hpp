#ifndef FLY_SCORE_DOCK_TAB
#define FLY_SCORE_DOCK_TAB

#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QKeySequence>
#include <QToolButton>

#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"

class FlyDockTab : public QWidget {
public:
	explicit FlyDockTab(const QString &title, const QString &dataDir, FlyScoreState::FlyState &state,
			    FlyScoreDock *dock, QWidget *parent)
		: QWidget(parent),
		  dataDir_(dataDir),
		  state_(state),
		  dock_(dock)
	{ };
    virtual void syncUiFromState() = 0;
    virtual void syncStateFromUi() = 0;
	virtual void refreshUiFromState(bool onlyTimeIfRunning) = 0;

protected:
	QString dataDir_;
	FlyScoreState::FlyState &state_;
	FlyScoreDock *dock_;

	virtual void init() = 0;
	virtual void buildUI() = 0;
};

#endif
