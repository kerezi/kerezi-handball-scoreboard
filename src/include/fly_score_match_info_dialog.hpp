#pragma once

#include <QDialog>
#include <QString>

#include "fly_score_state.hpp"

class QLineEdit;
class QSpinBox;
class QDateTimeEdit;
class QToolButton;
class QPushButton;

class FlyMatchInfoDialog : public QDialog {
    Q_OBJECT
public:
    explicit FlyMatchInfoDialog(const QString &dataDir,
                            FlyState &state,
                            QWidget *parent = nullptr);
    ~FlyMatchInfoDialog() override;

private slots:
    void onApply();

private:
    void syncUiFromState();
    void syncStateFromUi();

private:
    QString  dataDir_;
    FlyState &state_;

    QLineEdit   *competition_  = nullptr;
    QLineEdit   *competitionRound_ = nullptr;
	QSpinBox *matchNumber_ = nullptr;
	QDateTimeEdit *datetime_ = nullptr;
    QLineEdit   *city_ = nullptr;
	QLineEdit *venue_ = nullptr;
	QSpinBox *spectators_ = nullptr;
    QPushButton *applyBtn_ = nullptr;
};
