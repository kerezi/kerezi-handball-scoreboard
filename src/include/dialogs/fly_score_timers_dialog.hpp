#pragma once

#include <QDialog>
#include <QVector>
#include <QString>

#include "fly_score_state.hpp"
#include "fly_score_base_dialog.hpp"

class QVBoxLayout;
class QPushButton;
class QLineEdit;
class QComboBox;

class FlyTimersDialog : public FlyBaseDialog {
    Q_OBJECT
public:
    FlyTimersDialog(const QString &dataDir, FlyScoreState::FlyState &state, QWidget *parent = nullptr);

private slots:
    void onAddTimer();

private:
    struct Row {
        QWidget    *row       = nullptr;
        QLineEdit  *labelEdit = nullptr;
        QLineEdit  *timeEdit  = nullptr;
        QLineEdit  *durationEdit  = nullptr;
        QComboBox  *modeCombo = nullptr;
        QPushButton *remove   = nullptr;
    };

    void buildUi();
    void syncUiFromState() override;
    void syncStateFromUi() override;
    Row  addRow(const FlyScoreState::FlyTimer &tm, bool canRemove);

private:
    QVBoxLayout *timersLayout_ = nullptr;
    QPushButton *addTimerBtn_  = nullptr;
    QVector<Row> rows_;
};
