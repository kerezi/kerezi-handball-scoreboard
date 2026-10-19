#include "fly_score_handball_tab.hpp"
#include "fly_score_log.hpp"
#include "fly_score_const.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][handball-tab]"

#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"
#include "fly_score_clickable_label.hpp"

#include <Qt>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCheckBox>
#include <QSpinBox>
#include <QLabel>
#include <QWidget>
#include <QSpacerItem>
#include <QSizePolicy>
#include <QIcon>
#include <QString>

FlyHandballTab::~FlyHandballTab() = default;

void FlyHandballTab::init() {}

void FlyHandballTab::buildUI()
{
	auto *handballTab = new QWidget(this);
	auto *handballGL = new QGridLayout(handballTab);
	handballGL->setContentsMargins(0, 0, 0, 0);
	handballGL->setHorizontalSpacing(6);
	handballGL->setVerticalSpacing(0);
	handballTabLayout_ = handballGL;

	buildUI_matchInfo();
	// buildUI_players();
	// buildUI_matchOfficialInfo();
	// buildUI_matchOfficials();

    connectUi();
}
void FlyHandballTab::connectUi()
{
    connect(mt, &FlyClickableLabel::clicked, this, [this]() { dock_->toggleTimerRunning(0); });
}
void FlyHandballTab::buildUI_matchInfo()
{
	{
		htln = new QLabel(fly_i18n("Common.Home"));
		htln->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htln->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 4;
		handballTabLayout_->addWidget(htln, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htf2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		htf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htf2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(htf2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		hts2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		hts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		hts2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(hts2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htt2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		htt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htt2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(htt2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		mt = new FlyClickableLabel(this->state_, fly_i18n("Common.MatchTimer"));
		FlyScoreState::FlyTimer tm = state_.timers[0];
		mt->setText(FlyQtHelper::fly_format_ms_mmss(tm.initial_ms > 0 ? tm.initial_ms : tm.remaining_ms));
		mt->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight:bold;"));
		mt->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 6;
		handballTabLayout_->addWidget(mt, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtt2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		gtt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtt2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(gtt2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gts2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		gts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gts2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(gts2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtf2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		gtf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtf2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(gtf2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtln = new QLabel(fly_i18n("Common.Guest"));
		gtln->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtln->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 4;
		handballTabLayout_->addWidget(gtln, posRow, posCol, curHei, curWid);
		posCol += curWid;
	}

	posRow += 1;
	posCol = 0;
	// 2nd row
	{
		htsn = new QLabel(fly_i18n("Common.Home") + " " + fly_i18n("Dock.Players"));
		htsn->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htsn->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 3;
		handballTabLayout_->addWidget(htsn, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htny = new QLabel(fly_i18n("Common.NumberOfYellow"));
		htny->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htny->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htny, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htnf2ms = new QLabel(fly_i18n("Common.NumberOfFirst2minSusp"));
		htnf2ms->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htnf2ms->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htnf2ms, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htns2ms = new QLabel(fly_i18n("Common.NumberOfSecond2minSusp"));
		htns2ms->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htns2ms->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htns2ms, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htnt2ms = new QLabel(fly_i18n("Common.NumberOfThird2minSusp"));
		htnt2ms->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htnt2ms->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htnt2ms, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htnd = new QLabel(fly_i18n("Common.NumberOfDisq"));
		htnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htnd->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htnd, posRow, posCol, curHei, curWid);
		posCol += curWid;

		htnb = new QLabel(fly_i18n("Common.NumberOfBlue"));
		htnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htnb->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
		posCol += curWid;

		mr = new QLabel(fly_i18n("Common.MatchResult"));
		mr->setStyleSheet(QStringLiteral("font-weight:bold;"));
		mr->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 2;
		handballTabLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtnb = new QLabel(fly_i18n("Common.NumberOfBlue"));
		gtnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtnb->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtnb, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtnd = new QLabel(fly_i18n("Common.NumberOfDisq"));
		gtnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtnd->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtnd, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtnt2ms = new QLabel(fly_i18n("Common.NumberOfThird2minSusp"));
		gtnt2ms->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtnt2ms->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtnt2ms, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtns2ms = new QLabel(fly_i18n("Common.NumberOfSecond2minSusp"));
		gtns2ms->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtns2ms->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtns2ms, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtnf2ms = new QLabel(fly_i18n("Common.NumberOfFirst2minSusp"));
		gtnf2ms->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtnf2ms->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtnf2ms, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtny = new QLabel(fly_i18n("Common.NumberOfYellow"));
		gtny->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtny->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtny, posRow, posCol, curHei, curWid);
		posCol += curWid;

		gtsn = new QLabel(fly_i18n("Common.Guest") + " " + fly_i18n("Dock.Players"));
		gtsn->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtsn->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 3;
		handballTabLayout_->addWidget(gtsn, posRow, posCol, curHei, curWid);
		posCol += curWid;
	}
}

//void FlyHandballTab::buildUI_players()
// {}
//
//void FlyHandballTab::buildUI_matchOfficialInfo()
// {}
//
//void FlyHandballTab::buildUI_matchOfficials()
//{}

void FlyHandballTab::refreshUiFromState(bool onlyTimeIfRunning) {
    syncUiFromState();
}

void FlyHandballTab::syncUiFromState()
{
    FlyScoreState::FlyTimer tm = state_.timers[0];
 	mt->setText(FlyQtHelper::fly_format_ms_mmss(tm.initial_ms > 0 ? tm.initial_ms : tm.remaining_ms));
	mt->refreshUI();
}

void FlyHandballTab::syncStateFromUi() {}
