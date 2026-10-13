#pragma once

#include <QDialog>
#include <QString>

#include "fly_score_state.hpp"

class QPushButton;
class QVBoxLayout;
class QGroupBox;
class QGridLayout;


class FlyBaseDialog : public QDialog {
    Q_OBJECT
public:
    explicit FlyBaseDialog(const QString &title,
                            const QString &hints,
                            const QString &subTitle,
                            const QString &dataDir,
                            FlyScoreState::FlyState &state,
                            QWidget *parent = nullptr);
    ~FlyBaseDialog() override;

protected slots:
    void onApply();

protected:
    virtual void syncUiFromState() = 0;
    virtual void syncStateFromUi() = 0;
    void addButtons();

protected:
    QString  dataDir_;
    FlyScoreState::FlyState &state_;

    QVBoxLayout *root_ = nullptr;
    QGroupBox *gb_ = nullptr;
    QGridLayout *gl_ = nullptr;
    QPushButton *applyBtn_ = nullptr;
};
