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
struct FlyTeam;

class FlyTeamsDialog : public FlyBaseDialog {
    Q_OBJECT
public:
    explicit FlyTeamsDialog(const QString &dataDir,
                            FlyState &state,
                            QWidget *parent = nullptr);
    ~FlyTeamsDialog() override;

private slots:
    void onBrowseHomeLogo();
    void onBrowseGuestLogo();
    void onPickHomeColor();
    void onPickGuestColor();
    void onAddHomeOfficial();
    void onAddGuestOfficial();

private:
    struct FlyOfficialUIRow {
        bool canRemove = true;
        QWidget *row = nullptr;
        QLabel *officialLetterLabel = nullptr;
        QLineEdit *officialNameEdit = nullptr;
        QLineEdit *officialRoleEdit = nullptr;
        QPushButton *remove = nullptr;
    };
    struct FlyTeamUI {
        QLineEdit   *title  = nullptr;
        QLineEdit   *sub    = nullptr;
        QLineEdit   *logo   = nullptr;
        QToolButton *browse = nullptr;
        QToolButton *color  = nullptr;

        QVBoxLayout *officialsLayout = nullptr;
        QPushButton *addOfficialBtn = nullptr;
        QVector<FlyOfficialUIRow> *officialRows = nullptr;
    };

    void syncUiFromState() override;
    void syncStateFromUi() override;

    void onAddOfficial(FlyTeamUI *teamUI);
    void updateColorButton(QToolButton *btn, uint32_t color);
    void refreshRemoveButtons(QVector<FlyOfficialUIRow> *rows);
    FlyOfficialUIRow addOfficialUI(const FlyTeamUI *team, const FlyOfficial &of, bool canRemove);
    void addTeamUI(QTabWidget *tabs, FlyTeamUI *team, const char *name);
    void syncTeamUiFromState(FlyTeamUI *teamUI, FlyTeam *team);
    void syncStateFromTeamUi(FlyTeamUI *teamUI, FlyTeam *team);
    void onBrowseLogo(FlyTeamUI *teamUI, FlyTeam *team, const char *name);
    void onPickColor(FlyTeamUI *teamUI, FlyTeam *team, const char *name);

private:
    FlyTeamUI *home_ = nullptr;
    FlyTeamUI *guest_ = nullptr;
};
