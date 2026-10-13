#pragma once

#include <QDialog>
#include <QString>

#include "fly_score_state.hpp"
#include "fly_score_base_dialog.hpp"

class QLineEdit;
class QToolButton;
class QPushButton;
class QTabWidget;
class QLabel;
class QSpinBox;

struct FlyTeam;

class FlyPlayersDialog : public FlyBaseDialog {
    Q_OBJECT
public:
    explicit FlyPlayersDialog(const QString &dataDir,
                            FlyScoreState::FlyState &state,
                            QWidget *parent = nullptr);
    ~FlyPlayersDialog() override;

private slots:
    void onAddHomePlayer();
    void onAddGuestPlayer();

private:
    struct FlyPlayerUIRow {
        bool canRemove = true;
        QWidget *row = nullptr;
        QSpinBox *playerTshirtNumberSB = nullptr;
        QLineEdit *playerTshirtNameEdit = nullptr;
        QLineEdit *playerNameEdit = nullptr;
        QPushButton *remove = nullptr;
    };
    struct FlyPlayersUI {
        QVBoxLayout *playersLayout = nullptr;
        QPushButton *addPlayersBtn = nullptr;
        QVector<FlyPlayerUIRow> *playersRows = nullptr;
    };

    void syncUiFromState() override;
    void syncStateFromUi() override;

    void onAddPlayer(FlyPlayersUI *playersUI);
    void refreshRemoveButtons(QVector<FlyPlayerUIRow> *rows);
    FlyPlayerUIRow addPlayerUI(const FlyPlayersUI *players, const FlyScoreState::FlyPlayer &fp, bool canRemove);
    void addPlayersUI(QTabWidget *tabs, FlyPlayersUI *players, const bool isHome);
    void syncTeamUiFromState(FlyPlayersUI *playersUI, FlyScoreState::FlyTeam *team);
    void syncStateFromTeamUi(FlyPlayersUI *playersUI, FlyScoreState::FlyTeam *team);

private:
    FlyPlayersUI *home_ = nullptr;
    FlyPlayersUI *guest_ = nullptr;
};
