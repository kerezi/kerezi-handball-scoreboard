#ifndef FLY_SCORE_HANDBALL_TAB
#define FLY_SCORE_HANDBALL_TAB

#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QKeySequence>
#include <QToolButton>

#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_dock_tab.hpp"

class QPushButton;
class QGridLayout;
class QCheckBox;
class QGridLayout;
class QLabel;
class QShortcut;
class FlyScoreWebSocketServer;

class FlyHandballTab : public FlyDockTab {
public:
	explicit FlyHandballTab(const QString &title, const QString &dataDir, FlyScoreState::FlyState &state,
			    FlyScoreDock *dock, QWidget *parent)
		: FlyDockTab(title, dataDir, state, dock, parent) {
			this->init();
			this->buildUI();
		};

	~FlyHandballTab() override;

public:
	void refreshUiFromState(bool onlyTimeIfRunning) override;
	void syncUiFromState() override;
	void syncStateFromUi() override;

private slots:
	void onClearTeamsAndReset();

private:

private:
	int posRow = 0;
	int posCol = 0;
	int curHei = 0;
	int curWid = 0;

    void buildUI_matchInfo();
    // void buildUI_players();
    // void buildUI_matchOfficialInfo();
    // void buildUI_matchOfficials();

protected:
	void init() override;
	void buildUI() override;

	QGridLayout *handballTabLayout_ = nullptr;
};


#endif