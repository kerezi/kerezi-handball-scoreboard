#ifndef FLY_SCORE_STATS_TAB
#define FLY_SCORE_STATS_TAB

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

class FlyStatsTab : public FlyDockTab {
public:
	explicit FlyStatsTab(const QString &title, const bool isMatchStat, const QString &dataDir,
			     FlyScoreState::FlyState &state, FlyScoreDock *dock, QWidget *parent)
		: FlyDockTab(title, dataDir, state, dock, parent), isMatchStat(isMatchStat)
	{
		this->init();
		this->buildUI();
	};

	~FlyStatsTab() override;

public:
	void refreshUiFromState(bool onlyTimeIfRunning) override;
	void syncUiFromState() override;
	void syncStateFromUi() override;
	void bumpStat(int index, int delta, bool isHome);
	void toggleStatVisible(int index);

public slots:

private slots:

private:
	struct FlyStatUIRow {
		QWidget *row = nullptr;
		QCheckBox *visibleCheck = nullptr;
		QLabel *labelLbl = nullptr;
		QSpinBox *firstSpin = nullptr;
		QToolButton *minusFirstSpin = nullptr;
		QToolButton *plusFirstSpin = nullptr;
		QSpinBox *secondSpin = nullptr;
		QToolButton *minusSecondSpin = nullptr;
		QToolButton *plusSecondSpin = nullptr;
	};

	QVBoxLayout *layout = nullptr;
	QList<FlyStatUIRow> stats;
	bool isMatchStat;
	void clearAllStatsRows();

protected:
	void init() override;
	void buildUI() override;
};

#endif
