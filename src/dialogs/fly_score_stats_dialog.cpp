#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][stats-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_horizontal_tab_bar.hpp"
#include "fly_score_stats_dialog.hpp"
#include "fly_score_i18n.hpp"
#include "fly_score_state.hpp"
#include "fly_score_qt_helpers.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QPushButton>
#include <QStyle>
#include <QTabWidget>
#include <QCheckBox>
#include <QMessageBox>
#include <limits>

FlyStatsDialog::FlyStatsDialog(const QString &dataDir, FlyScoreState::FlyState &state, QWidget *parent)
	: FlyBaseDialog(fly_i18n("Stats.Title"),
					fly_i18n("Stats.Hint"),
					fly_i18n("Stats.Stats"),
					dataDir, state, parent)
{
	auto *tabs = new QTabWidget();
	tabs->setObjectName(fly_i18n("Common.Stat"));
	tabs->setDocumentMode(true);
	tabs->setMovable(false);
	tabs->setUsesScrollButtons(false);
	tabs->tabBar()->setStyleSheet("QTabBar {font-size:16pt;}");
	tabs->tabBar()->setAutoHide(true);

	teamStats_ = new FlyStatsUI{};
	teamStats_->statsRows = new QVector<FlyStatUIRow>();
	addStatsUI(tabs, teamStats_, false);

	singleStats_ = new FlyStatsUI{};
	singleStats_->statsRows = new QVector<FlyStatUIRow>();
	addStatsUI(tabs, singleStats_, true);

	gl_->addWidget(tabs);

	addButtons();

	syncUiFromState();
}
FlyStatsDialog::~FlyStatsDialog() = default;

void FlyStatsDialog::addStatsUI(QTabWidget *tabs, FlyStatsUI *statsUI, bool isMatchStat)
{
	auto *statsTab = new QWidget(tabs);

	statsUI->statsLayout = new QVBoxLayout(tabs);
	statsUI->statsLayout->setContentsMargins(0, 4, 0, 0);
	statsUI->statsLayout->setSpacing(6);
	statsUI->statsLayout->setAlignment(Qt::AlignTop|Qt::AlignLeft);

	statsUI->addStatBtn = new QPushButton(fly_i18n(isMatchStat ? "Stats.AddMatchStat" : "Stats.AddTeamStat"), statsTab);
	statsUI->addStatBtn->setCursor(Qt::PointingHandCursor);
	statsUI->addStatBtn->setStyleSheet("QPushButton {font-size: 16pt; min-width: 60px;min-width: 65px;}");
	statsUI->statsLayout->addWidget(statsUI->addStatBtn, Qt::AlignCenter);

	statsTab->setLayout(statsUI->statsLayout);

	tabs->addTab(statsTab, fly_i18n(isMatchStat ? "Common.MatchStats" : "Common.TeamStats"));

	connect(statsUI->addStatBtn, &QPushButton::clicked, this, [this, isMatchStat, statsUI] () {
		addStatRow(isMatchStat, statsUI, FlyScoreState::FlyStat {});
	});
}

void FlyStatsDialog::addStatRow(const bool isMatchStat, FlyStatsUI *statsUI, FlyScoreState::FlyStat fs)
{

	FlyStatUIRow r;
	r.canRemove = false;

	auto *row = new QWidget();
	auto *lay = new QHBoxLayout(row);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(8);

	QCheckBox *visible = nullptr;
	if (isMatchStat) {
		visible = new QCheckBox(row);
		visible->setChecked(true);
		visible->setToolTip(fly_i18n("Stats.VisibleInOverlay"));
		visible->setChecked(fs.visible);
	}

	auto *labelEdit = new QLineEdit(row);
	labelEdit->setPlaceholderText(fly_i18n(isMatchStat ? "Stats.MatchLabelPlaceholder" : "Stats.TeamLabelPlaceholder"));
	labelEdit->setText(fs.label);
	labelEdit->setMinimumWidth(220);
	labelEdit->setMaximumWidth(320);

	QSpinBox *firstSpin = new QSpinBox(row);
	firstSpin->setRange(-9999, 9999);
	firstSpin->setValue(fs.first);
	firstSpin->setMinimumWidth(70);
	firstSpin->setMaximumWidth(90);
	QSpinBox *secondSpin = nullptr;
	if (!isMatchStat) {
		secondSpin = new QSpinBox(row);
		secondSpin->setRange(-9999, 9999);
		secondSpin->setValue(fs.second);
		secondSpin->setMinimumWidth(70);
		secondSpin->setMaximumWidth(90);
	}

	auto *removeBtn = new QPushButton(row);
	removeBtn->setText(QStringLiteral("❌"));
	removeBtn->setToolTip(fly_i18n(isMatchStat ? "Stats.RemoveMatchStat" : "Stats.RemoveTeamStat"));
	removeBtn->setCursor(Qt::PointingHandCursor);

	const int h = firstSpin->sizeHint().height();
	removeBtn->setFixedSize(h, h);
	removeBtn->setStyleSheet("QPushButton {"
			 "  font-family:'Segoe UI Emoji','Noto Color Emoji','Apple Color Emoji',sans-serif;"
			 "  font-size:12px;"
			 "  padding:0;"
			 "}");

	if (isMatchStat) lay->addWidget(visible, 0, Qt::AlignVCenter);
	lay->addWidget(labelEdit, 1, Qt::AlignVCenter);
	lay->addWidget(new QLabel(fly_i18n(isMatchStat ? "Common.ValueColon" : "Common.HomeColon"), row), 0, Qt::AlignRight | Qt::AlignVCenter);
	lay->addWidget(firstSpin, 0, Qt::AlignVCenter);
	if (!isMatchStat) lay->addWidget(new QLabel(fly_i18n("Common.GuestColon"), row), 0,  Qt::AlignRight | Qt::AlignVCenter);
	if (!isMatchStat) 	lay->addWidget(secondSpin, 0, Qt::AlignVCenter);

	lay->addWidget(removeBtn, 0, Qt::AlignVCenter);

	row->setLayout(lay);
	statsUI->statsLayout->addWidget(row);

	r.row = row;
	r.visible = visible;
	r.labelEdit = labelEdit;
	r.firstSpin = firstSpin;
	r.secondSpin = secondSpin;
	r.remove = removeBtn;
	statsUI->statsRows->push_back(r);
	
	connect(removeBtn, &QPushButton::clicked, this, [this, row, statsUI, isMatchStat]() {
		if (statsUI->statsRows->size() <= 1) {
			QMessageBox::warning(this, fly_i18n("Common.CannotRemove"),
					     fly_i18n(isMatchStat ? "Stats.OneMatchStatRequired" : "Stats.OneTeamStatRequired"));
			return;
		}

		for (int i = 0; i < statsUI->statsRows->size(); ++i) {
			if ((*statsUI->statsRows)[i].row == row) {
				statsUI->statsLayout->removeWidget(row);
				statsUI->statsRows->removeAt(i);
				row->deleteLater();
				break;
			}
		}

	});

	refreshRemoveButtons(statsUI->statsRows, isMatchStat);
}

void FlyStatsDialog::syncFromState(bool isMatchStat, FlyStatsUI *statsUI, QVector<FlyScoreState::FlyStat> *stats)
{
	for (auto &r : *statsUI->statsRows) {
		if (statsUI->statsLayout && r.row) statsUI->statsLayout->removeWidget(r.row);
		if (r.row) {
			r.row->deleteLater();
			r.row->setHidden(true);
		}
	}
	statsUI->statsRows->clear();

	if (!statsUI->statsLayout)
		return;

	statsUI->statsRows->reserve(stats->size());

	for (int i = 0; i < stats->size(); ++i) {
		const FlyScoreState::FlyStat &fs = (*stats)[i];
		const bool canRemove = (stats->size() > 1);
		addStatRow(isMatchStat, statsUI, fs);
	}

	refreshRemoveButtons(statsUI->statsRows, isMatchStat);
}

void FlyStatsDialog::syncUiFromState()
{
	syncFromState(false, teamStats_, &state_.stTeamsStats);
	syncFromState(true, singleStats_, &state_.stMatchStats);
}

void FlyStatsDialog::refreshRemoveButtons(QVector<FlyStatUIRow> * statsRows, bool isMatchStat)
{
	const bool canRemove = statsRows->size() > 1;
	for (auto &r : *statsRows) {
		r.canRemove = canRemove;
		if (r.remove) {
			r.remove->setDisabled(!canRemove);
			r.remove->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);
			r.remove->setToolTip(canRemove ? fly_i18n(isMatchStat ? "Stats.RemoveStat" : "Stats.RemoveMatchStat")
							   : fly_i18n(isMatchStat ? "Stats.OneTeamStatRequired" : "Stats.OneMatchStatRequired"));
		}
	}
}

void FlyStatsDialog::syncStateFromUi()
{
	syncToState(false, teamStats_, &state_.stTeamsStats);
	syncToState(true, singleStats_, &state_.stMatchStats);
}

void FlyStatsDialog::syncToState(bool isMatchStat, FlyStatsUI *statsUI, QVector<FlyScoreState::FlyStat> *stats)
{
	stats->clear();
	stats->reserve(statsUI->statsRows->size());

	for (int i = 0; i < statsUI->statsRows->size(); ++i) {
		const auto &r = (*statsUI->statsRows)[i];

		FlyScoreState::FlyStat fs;
		fs.label = r.labelEdit ? r.labelEdit->text() : QString();
		fs.first = r.firstSpin ? r.firstSpin->value() : 0;
		fs.second = r.secondSpin ? r.secondSpin->value() : 0;
		fs.visible = r.visible ? r.visible->isChecked() : true;

		stats->push_back(fs);
	}

	FlyScoreState::fly_state_save(dataDir_, state_);
}

void FlyStatsDialog::onAccept()
{
	if (teamStats_->statsRows->isEmpty()) {
		QMessageBox::warning(this, fly_i18n("Stats.MissingTeamStats"),
				     fly_i18n("Stats.MustKeepTeamStat"));
		return;
	}

	if (singleStats_->statsRows->isEmpty()) {
		QMessageBox::warning(this, fly_i18n("Stats.MissingMatchStats"),
				     fly_i18n("Stats.MustKeepMatchStat"));
		return;
	}

	syncStateFromUi();
	accept();
}
