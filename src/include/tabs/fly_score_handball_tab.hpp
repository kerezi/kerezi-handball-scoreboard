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
class FlyClickableLabel;


class FlyHandballTab : public FlyDockTab {
public:
	explicit FlyHandballTab(const QString &title, const QString &dataDir, FlyScoreState::FlyState &state,
				FlyScoreDock *dock, QWidget *parent)
		: FlyDockTab(title, dataDir, state, dock, parent)
	{
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
	int posRow = 0;
	int posCol = 0;
	int curHei = 0;
	int curWid = 0;

	QLabel *htln = nullptr;
	QLabel *htf2s = nullptr;
	QLabel *hts2s = nullptr;
	QLabel *htt2s = nullptr;
	FlyClickableLabel *mt = nullptr;
	QLabel *gtt2s = nullptr;
	QLabel *gts2s = nullptr;
	QLabel *gtf2s = nullptr;
	QLabel *gtln = nullptr;
	QLabel *htsn = nullptr;
	QLabel *htny = nullptr;
	QLabel *htnf2ms = nullptr;
	QLabel *htns2ms = nullptr;
	QLabel *htnt2ms = nullptr;
	QLabel *htnd = nullptr;
	QLabel *htnb = nullptr;
	QLabel *mr = nullptr;
	QLabel *gtnb = nullptr;
	QLabel *gtnd = nullptr;
	QLabel *gtny = nullptr;
	QLabel *gtnf2ms = nullptr;
	QLabel *gtns2ms = nullptr;
	QLabel *gtnt2ms = nullptr;
	QLabel *gtsn = nullptr;

	void buildUI_matchInfo();
	// void buildUI_players();
	// void buildUI_matchOfficialInfo();
	// void buildUI_matchOfficials();

	void connectUi();

protected:
	void init() override;
	void buildUI() override;

	QGridLayout *handballTabLayout_ = nullptr;
};

#endif