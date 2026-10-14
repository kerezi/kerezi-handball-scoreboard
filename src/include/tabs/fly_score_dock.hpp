#pragma once

#include <QWidget>
#include <QString>
#include <QList>
#include <QKeySequence>
#include <QToolButton>

#include "fly_score_state.hpp"
#include "fly_score_event_log.hpp"

class QPushButton;
class QSpinBox;
class QLineEdit;
class QCheckBox;
class QComboBox;
class QVBoxLayout;
class QLabel;
class QShortcut;
class QJsonObject;
class FlyScoreWebSocketServer;
class FlyConfigTab;
class FlyTeamsStatsTab;

struct FlyMatchStatUi {
	QWidget *row = nullptr;
	QCheckBox *visibleCheck = nullptr;
	QLabel *labelLbl = nullptr;
	QSpinBox *valueSpin = nullptr;
	QToolButton *minusBtn = nullptr;
	QToolButton *plusBtn = nullptr;
};

struct FlyTimerUi {
	QWidget *row = nullptr;
	QLabel *labelLbl = nullptr;
	QLineEdit *timeEdit = nullptr;
	QPushButton *startStop = nullptr;
	QPushButton *reset = nullptr;
	QCheckBox *visibleCheck = nullptr;
};

struct FlyHotkeyBinding;

class FlyScoreDock : public QWidget {
	Q_OBJECT
public:
	explicit FlyScoreDock(QWidget *parent = nullptr);
	~FlyScoreDock() override;
	bool init();

	void handleFrontendEvent(int event);
	void loadState();
	void saveState();
	void bumpTeamStat(int index, int delta, bool isHome);
	void toggleTeamStatVisible(int index);
	
	void bumpMatchStat(int index, int delta);
	void activateTemplateFolder(const QString &path);
	void refreshUiFromState(bool onlyTimeIfRunning = false);
public slots:
	void toggleMatchStatVisible(int index);
	void ensureResourcesDefaults();
	void appendEvent(const QString &event);
	void startEventLog(const QString &firstEvent = QStringLiteral("Stream Start"));
	void stopEventLog(const QString &lastEvent = QStringLiteral("Stream End"));
	void logStateChanges(const FlyScoreState::FlyState &before, const FlyScoreState::FlyState &after);

private:
	void clearAllMatchStatRows();
	void loadMatchStatControlsFromState();
	void syncMatchStatControlsToState();

	void clearAllTimerRows();
	void loadTimerControlsFromState();
	// void loadHandballFromState();

	void refreshEventLogUi();
	QString eventLogsDirectory() const;

private:
	QString dataDir_;
	FlyScoreState::FlyState st_;

	FlyScoreState::FlyState lastLoggedState_;

	FlyConfigTab *configTab_ = nullptr;
	FlyTeamsStatsTab *teamsStatsTab_ = nullptr;

	QVBoxLayout *singleStatsLayout_ = nullptr;
	QList<FlyMatchStatUi> singleStats_;

	QVBoxLayout *timersLayout_ = nullptr;
	QList<FlyTimerUi> timers_;

	FlyScoreEventLog eventLog_;
	QLabel *eventLogStatus_ = nullptr;
	QComboBox *eventTimestampMode_ = nullptr;
	QLineEdit *eventText_ = nullptr;
	QPushButton *eventLogToggle_ = nullptr;
	QPushButton *eventAdd_ = nullptr;

	bool frontendEventCallbackConnected_ = false;
};