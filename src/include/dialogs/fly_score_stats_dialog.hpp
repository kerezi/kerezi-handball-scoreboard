#pragma once

#include <QDialog>
#include <QVector>
#include <QString>

#include "fly_score_state.hpp"
#include "fly_score_base_dialog.hpp"

class QVBoxLayout;
class QPushButton;
class QLineEdit;
class QSpinBox;
class QCheckBox;
class QTabWidget;
class QWidget;

class FlyStatsDialog : public FlyBaseDialog {
	Q_OBJECT
public:
	FlyStatsDialog(const QString &dataDir, FlyScoreState::FlyState &state, QWidget *parent = nullptr);
	~FlyStatsDialog() override;
private slots:
	void onAccept();

private:
    struct FlyStatUIRow {
		QWidget *row = nullptr;
		QLineEdit *labelEdit = nullptr;
		QSpinBox *firstSpin = nullptr;
		QSpinBox *secondSpin = nullptr;
		QPushButton *remove = nullptr;
		QCheckBox *visible = nullptr;
		bool canRemove = true;
    };

	struct FlyStatsUI {
        QVBoxLayout *statsLayout = nullptr;
        QPushButton *addStatBtn = nullptr;
        QVector<FlyStatUIRow> *statsRows = nullptr;
    };

	void addStatsUI(QTabWidget *tabs, FlyStatsUI *statsUI, bool isMatchStat);
	void syncUiFromState() override;
    void syncToState(bool isMatchStat, FlyStatsUI *statsUI, QVector<FlyScoreState::FlyStat> *stats);
	void syncStateFromUi() override;
	void syncFromState(bool isMatchStat, FlyStatsUI *statsUI, QVector<FlyScoreState::FlyStat> *stats);
	void refreshRemoveButtons(QVector<FlyStatUIRow> * statsRows, bool isMatchStat);

	void addStatRow(const bool isMatchStat, FlyStatsUI *statsUIL, FlyScoreState::FlyStat fs);

	private:

	FlyStatsUI *teamStats_;
	FlyStatsUI *singleStats_;
};
