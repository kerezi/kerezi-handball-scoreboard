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
class QCheckBox;

struct FlyTeam;
struct FlyScoreState::FlyPlayer;

class FlyPlayerStatDialog : public FlyBaseDialog {
	Q_OBJECT
public:
	explicit FlyPlayerStatDialog(const QString &dataDir, FlyScoreState::FlyPlayer *player, bool isHome,
				     FlyScoreState::FlyState &state, QWidget *parent = nullptr);
	~FlyPlayerStatDialog() override;

protected:
	void syncUiFromState() override;
	void syncStateFromUi() override;

private:
	struct FlyPlayerStatUIRow {
		QWidget *row;
		bool canRemove = true;
		QLabel *moment = nullptr;
		QCheckBox *statRes = nullptr;
		QLabel *plStatType = nullptr;
		QLabel *goalPos = nullptr;
		QLabel *pitchPos = nullptr;
		QLabel *assistPlTSnum = nullptr;
		QLabel *blockPlTSnum = nullptr;
		QPushButton *remove = nullptr;
	};

	struct FlyPlayerStatTabUI {
		QString title = "";
		QVBoxLayout *playerStatLayout = nullptr;
		QPushButton *addPlayerStatBtn = nullptr;
		QVector<FlyPlayerStatUIRow> *playerStatRows = nullptr;
	};

	void syncPlayerStatsUiFromState();
	void syncStateFromPlayerStatsUi();

	bool isHome_ = true;
	FlyScoreState::FlyPlayer *player_ = nullptr;
	FlyPlayerStatTabUI *stat_ = nullptr;
	FlyPlayerStatTabUI *pitch_ = nullptr;
	FlyPlayerStatTabUI *assist_ = nullptr;
	FlyPlayerStatTabUI *block_ = nullptr;
	FlyPlayerStatTabUI *stats_ = nullptr;
	FlyScoreState::FlyPlayerStat *nStat_ = nullptr;

	FlyPlayerStatTabUI *playerStatWidget(QWidget *playerStatTabWidget);
	FlyPlayerStatTabUI *playerPitchWidget(QWidget *playerPitchTabWidget);
	FlyPlayerStatTabUI *playerListWidget(QWidget *playerAssistTabWidget, bool isHome);
	FlyPlayerStatTabUI *playerStatsWidget(QWidget *playerStatsTabWidget);

	void setProp(QWidget *w, const char *prop, QVariant &&val);
	void setPropPlStatType(QWidget *w, bool val);
	FlyPlayerStatUIRow addPlayerStatUI(const FlyScoreState::FlyPlayerStat &ps, bool canRemove);
};
