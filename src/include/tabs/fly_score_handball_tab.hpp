#ifndef FLY_SCORE_HANDBALL_TAB
#define FLY_SCORE_HANDBALL_TAB

#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QKeySequence>
#include <QToolButton>
#include <QVector>

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
class QPushButton;

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

private:
	struct FlyTeamPlayerUI {
		QLabel *teamPlayerTShirtNumber = nullptr;
		QLabel *teamPlayerTShirtName = nullptr;

		QPushButton *teamPlayerWarningTime = nullptr;
		QPushButton *teamPlayerFrst2minSuspTimer = nullptr;
		QPushButton *teamPlayerScnd2minSuspTimer = nullptr;
		QPushButton *teamPlayerThrd2minSuspTimer = nullptr;
		QPushButton *teamPlayerDisqualificationTime = nullptr;
		QPushButton *teamPlayerBlueCardTime = nullptr;
		QPushButton *teamPlayerGoals = nullptr;
	};

	struct FlyTeamUI {
		QLabel *teamLongName = nullptr;
		QLabel *teamFrst2minSuspTimer = nullptr;
		QLabel *teamScnd2minSuspTimer = nullptr;
		QLabel *teamThrd2minSuspTimer = nullptr;

		QLabel *teamShrtName = nullptr;
		QLabel *teamScores = nullptr;
		QLabel *teamNumOfYllCard = nullptr;
		QLabel *teamNumOfFrst2minSup = nullptr;
		QLabel *teamNumOfScnd2minSup = nullptr;
		QLabel *teamNumOfThrd2minSup = nullptr;
		QLabel *teamNumOfRedCard = nullptr;
		QLabel *teamNumOfBluCard = nullptr;

		QVector<FlyTeamPlayerUI *> *players = new QVector<FlyTeamPlayerUI *>();

		QPushButton *teamTimeout = nullptr;
		QLabel *teamOfficials = nullptr;
		QLabel *teamOfficialsNumOfYllCard = nullptr;
		QLabel *teamOfficialsNumOf2minSup = nullptr;
		QLabel *teamOfficialsNumOfRedCard = nullptr;
		QLabel *teamOfficialsNumOfBluCard = nullptr;

		QVector<FlyTeamPlayerUI *> *officials = new QVector<FlyTeamPlayerUI *>();
	};

	int posRow = 0;
	int posCol = 0;
	int curHei = 0;
	int curWid = 0;

	int debugId = 0;

	FlyTeamUI *home = nullptr;
	FlyTeamUI *guest = nullptr;

	QPushButton *mt = nullptr;
	QLabel *mr = nullptr;

	void buildUI_matchInfo();
	void buildUI_matchInfo_Team_FirstLine(bool isHome);
	void buildUI_matchInfo_Team_SecondLine(bool isHome);

	void buildUI_players();
	void buildUI_team_player(int row, bool isHome);
	void buildUI_matchOfficialInfo(bool isHome);
	void buildUI_matchOfficials();
	void buildUI_team_official(int row, bool isHome);

	QLabel *newQLabel(QString title, int height, int width, bool isHome, bool isTeam = false, QFlags<Qt::AlignmentFlag> alf = Qt::AlignCenter);
	QPushButton *newFlyClickableLabel(QString title, int height, int width, bool isHome, bool isTeam = false, QFlags<Qt::AlignmentFlag> alf = Qt::AlignCenter);

	void connectUi();
	void syncLabelFromState(QLabel *l, bool disable, bool out, QString text);
	void syncButtonFromState(QPushButton *b, bool disable, bool out, QString text);

protected:
	void init() override;
	void buildUI() override;

	QGridLayout *handballTabLayout_ = nullptr;
};

#endif