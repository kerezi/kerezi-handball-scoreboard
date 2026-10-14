#ifndef FLY_SCORE_TEAMS_STATS_TAB
#define FLY_SCORE_TEAMS_STATS_TAB

#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QKeySequence>
#include <QToolButton>

#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_dock_tab.hpp"
#include "fly_score_log.hpp"

class QPushButton;
class QVBoxLayout;
class QCheckBox;
class QGridLayout;
class QLabel;
class QShortcut;
class FlyScoreWebSocketServer;

class FlyTeamsStatsTab : public FlyDockTab {
public:
	explicit FlyTeamsStatsTab(const QString &title, const QString &dataDir, FlyScoreState::FlyState &state,
				  FlyScoreDock *dock, QWidget *parent)
		: FlyDockTab(title, dataDir, state, dock, parent)
	{
		this->init();
		this->buildUI();
	};

	~FlyTeamsStatsTab() override;

public:
	void refreshUiFromState(bool onlyTimeIfRunning) override;
	void syncUiFromState() override;
	void syncStateFromUi() override;
	void bumpTeamStat(int index, int delta, bool isHome);
	void toggleTeamStatVisible(int index);

public slots:

private slots:

private:
	struct FlyTeamsStatUIRow {
		QWidget *row = nullptr;
		QCheckBox *visibleCheck = nullptr;
		QLabel *labelLbl = nullptr;
		QSpinBox *homeSpin = nullptr;
		QSpinBox *guestSpin = nullptr;
		QToolButton *minusHome = nullptr;
		QToolButton *plusHome = nullptr;
		QToolButton *minusGuest = nullptr;
		QToolButton *plusGuest = nullptr;
	};

	
	QVBoxLayout *layout = nullptr;
	QList<FlyTeamsStatUIRow> teamsStats;

	void clearAllTeamsStatsRows();

protected:
	void init() override;
	void buildUI() override;

};

#endif
