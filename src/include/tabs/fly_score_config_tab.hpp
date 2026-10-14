#ifndef FLY_SCORE_CONFIG_TAB
#define FLY_SCORE_CONFIG_TAB

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
class QVBoxLayout;
class QCheckBox;
class QGridLayout;
class QLabel;
class QVBoxLayout;
class QShortcut;
class FlyScoreWebSocketServer;

class FlyConfigTab : public FlyDockTab {
public:
	explicit FlyConfigTab(const QString &title, const QString &dataDir, FlyScoreState::FlyState &state,
			    FlyScoreDock *dock, QWidget *parent)
		: FlyDockTab(title, dataDir, state, dock, parent) {
			init();
			buildUI();
		};

	~FlyConfigTab() override;

public:
	void refreshUiFromState(bool onlyTimeIfRunning);
	void toggleSwap();
	void toggleScoreboardVisible();
	void broadcastCurrentState();
	void toggleTimerRunning(int index);
	void refreshActiveTemplateLabel();

public slots:
	void openHotkeysDialog();
	void onSelectTemplateFolder();

private slots:
	void onClearTeamsAndReset();

private:
	void updateWebSocketStatus();
	void handleRemoteCommand(const QJsonObject &command);
	void clearAllShortcuts();
	QList<FlyHotkeyBinding> buildDefaultHotkeyBindings() const;
	QList<FlyHotkeyBinding> buildMergedHotkeyBindings() const;
	void applyHotkeyBindings(const QList<FlyHotkeyBinding> &bindings);
	void onOpenDialog(QDialog *dlg);

private:
	QList<FlyHotkeyBinding> hotkeyBindings_;

protected:
	void init() override;
	void buildUI() override;

	QVBoxLayout *configTabLayout_ = nullptr;
	// UI Elements
	QPushButton *matchInfoBtn_ = nullptr;
	QPushButton *teamsBtn_ = nullptr;
	QPushButton *playersBtn_ = nullptr;
	QPushButton *editFieldsBtn_ = nullptr;
	QPushButton *editTimersBtn_ = nullptr;
	QCheckBox *swapSides_ = nullptr;
	QCheckBox *showScoreboard_ = nullptr;
	QPushButton *selectTemplateFolderBtn_ = nullptr;
	QLabel *activeTemplateLabel_ = nullptr;
	QLabel *webSocketStatus_ = nullptr;

	//
	QList<QShortcut *> shortcuts_;
	FlyScoreWebSocketServer *webSocketServer_ = nullptr;
};

#endif
