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
class FlyHorizontalTabWidget;

struct FlyTeam;
struct FlyScoreState::FlyPlayer;

class FlyPlayerStatDialog : public FlyBaseDialog {
	Q_OBJECT
public:
	explicit FlyPlayerStatDialog(const QString &dataDir, FlyScoreState::FlyPlayer *player, bool isHome,
				     FlyScoreState::FlyState &state, QWidget *parent = nullptr);
	~FlyPlayerStatDialog() override;

// private slots:
// 	void onAddHomePlayer();
// 	void onAddGuestPlayer();

private:
	struct FlyPlayerStatUIRow {
		bool canRemove = true;
		QWidget *row = nullptr;
		QSpinBox *playerTshirtNumberSB = nullptr;
		QLineEdit *playerTshirtNameEdit = nullptr;
		QLineEdit *playerNameEdit = nullptr;
		QPushButton *remove = nullptr;
	};
	struct FlyPlayerStatTabUI {
		QString title = "";
		QVBoxLayout *playerStatLayout = nullptr;
		QPushButton *addPlayerStatBtn = nullptr;
		QVector<FlyPlayerStatUIRow> *playerStatRows = nullptr;
	};

	void syncUiFromState() override;
	void syncStateFromUi() override;

	// void onAddPlayerStat(FlyPlayerStatUI *playerStatUI);
	void refreshRemoveButtons(QVector<FlyPlayerStatUIRow> *rows);
	// FlyPlayerStatUIRow addPlayerStatUI(const FlyPlayerStatUI *playerStats, const FlyScoreState::FlyPlayerStat &ps, bool canRemove);
	void addPlayerStatTabUI(FlyHorizontalTabWidget *tabs, FlyPlayerStatDialog::FlyPlayerStatTabUI *statTab,
				QWidget *playerStatTab);
	// void syncTeamUiFromState(FlyPlayerStatUI *playerStatUI, FlyScoreState::FlyTeam *team);
	// void syncStateFromTeamUi(FlyPlayerStatUI *playerStatUI, FlyScoreState::FlyTeam *team);

private:
	bool isHome_ = true;
	FlyScoreState::FlyPlayer *player_ = nullptr;
	FlyPlayerStatTabUI *shot_ = nullptr;
	FlyPlayerStatTabUI *assist_ = nullptr;
	FlyPlayerStatTabUI *block_ = nullptr;
	FlyPlayerStatTabUI *steal_ = nullptr;

	FlyPlayerStatTabUI *playerShotStatWidget(QWidget *playerShotTabWidget);
	FlyPlayerStatTabUI *playerAssistStatWidget(QWidget *playerAssistTabWidget);

};
