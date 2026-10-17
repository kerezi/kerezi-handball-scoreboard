#include "fly_score_handball_tab.hpp"
#include "fly_score_log.hpp"
#include "fly_score_const.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][handball-tab]"

#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"
#include "fly_score_dock.hpp"

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
}

void FlyHandballTab::buildUI_matchInfo()
{
	{
		QLabel *htln = new QLabel(fly_i18n("Common.Home"));
		htln->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htln->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 4;
		handballTabLayout_->addWidget(htln, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *htf2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		htf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htf2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(htf2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *hts2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		hts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		hts2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(hts2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *htt2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		htt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htt2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(htt2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *mt = new QLabel(fly_i18n("Common.MatchTimer"));
		mt->setStyleSheet(QStringLiteral("font-size: 24pt; font-weight:bold;"));
		mt->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 6;
		handballTabLayout_->addWidget(mt, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *gtt2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		gtt2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtt2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(gtt2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *gts2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		gts2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gts2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(gts2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *gtf2s = new QLabel(fly_i18n("Common.F2MinSusp"));
		gtf2s->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtf2s->setAlignment(Qt::AlignCenter);
		curHei = 2;
		curWid = 1;
		handballTabLayout_->addWidget(gtf2s, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *gtln = new QLabel(fly_i18n("Common.Guest"));
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
		QLabel *htsn = new QLabel(fly_i18n("Common.Home") + " " + fly_i18n("Dock.Players"));
		htsn->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htsn->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 3;
		handballTabLayout_->addWidget(htsn, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *htny = new QLabel(fly_i18n("Common.NumberOfYellow"));
		htny->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htny->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htny, posRow, posCol, curHei, curWid);
		posCol += curWid;

		posCol += 3;

		QLabel *htnd = new QLabel(fly_i18n("Common.NumberOfDisq"));
		htnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htnd->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htnd, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *htnb = new QLabel(fly_i18n("Common.NumberOfBlue"));
		htnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
		htnb->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *mr = new QLabel(fly_i18n("Common.MatchResult"));
		mr->setStyleSheet(QStringLiteral("font-weight:bold;"));
		mr->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 2;
		handballTabLayout_->addWidget(htnb, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *gtnb = new QLabel(fly_i18n("Common.NumberOfBlue"));
		gtnb->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtnb->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtnb, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *gtnd = new QLabel(fly_i18n("Common.NumberOfDisq"));
		gtnd->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtnd->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtnd, posRow, posCol, curHei, curWid);
		posCol += curWid;

		posCol += 3;

		QLabel *gtny = new QLabel(fly_i18n("Common.NumberOfYellow"));
		gtny->setStyleSheet(QStringLiteral("font-weight:bold;"));
		gtny->setAlignment(Qt::AlignCenter);
		curHei = 1;
		curWid = 1;
		handballTabLayout_->addWidget(gtny, posRow, posCol, curHei, curWid);
		posCol += curWid;

		QLabel *gtsn = new QLabel(fly_i18n("Common.Guest") + " " + fly_i18n("Dock.Players"));
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

void FlyHandballTab::refreshUiFromState(bool onlyTimeIfRunning) {}
void FlyHandballTab::syncUiFromState() {}
void FlyHandballTab::syncStateFromUi() {}
