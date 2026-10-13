#include "fly_score_base_dialog.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][base-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"

#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QSpinBox>
#include <QDateTimeEdit>
#include <QToolButton>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStyle>

FlyBaseDialog::FlyBaseDialog(const QString &title, 
	const QString &hints, 
	const QString &subTitle,
	const QString &dataDir, 
	FlyScoreState::FlyState &state, 
	QWidget *parent)
	: QDialog(parent),
	  dataDir_(dataDir),
	  state_(state)
{
	setObjectName(QString(typeid(this).name()));
	setWindowTitle(title);
	setModal(false);

	setMinimumWidth(480);
	resize(520, 420);
	setSizeGripEnabled(false);

	root_ = new QVBoxLayout(this);
	root_->setContentsMargins(14, 14, 14, 14);
	root_->setSpacing(10);

	auto *hint = new QLabel(hints, this);
	hint->setObjectName(QString(typeid(this).name()) + QString("Hint"));
	hint->setWordWrap(true);
	root_->addWidget(hint);

	gb_ = new QGroupBox(subTitle, this);
	gb_->setObjectName(subTitle + QString("Group"));

	gl_ = new QGridLayout(gb_);
	gl_->setContentsMargins(10, 10, 10, 10);
	gl_->setHorizontalSpacing(8);
	gl_->setVerticalSpacing(6);
}

FlyBaseDialog::~FlyBaseDialog() = default;

void FlyBaseDialog::addButtons ()
{
	gb_->setLayout(gl_);
	root_->addWidget(gb_);

	auto *buttonsRow = new QHBoxLayout();
	buttonsRow->setContentsMargins(0, 0, 0, 0);
	buttonsRow->setSpacing(8);

	buttonsRow->addStretch(1);

	applyBtn_ = new QPushButton(fly_i18n("Common.SaveAndClose"), this);
	auto *closeBtn = new QPushButton(fly_i18n("Common.Close"), this);

	applyBtn_->setCursor(Qt::PointingHandCursor);
	closeBtn->setCursor(Qt::PointingHandCursor);

	buttonsRow->addWidget(applyBtn_);
	buttonsRow->addWidget(closeBtn);

	root_->addLayout(buttonsRow);
	setLayout(root_);

	this->syncUiFromState();

	connect(applyBtn_, &QPushButton::clicked, this, &FlyBaseDialog::onApply);
	connect(closeBtn, &QPushButton::clicked, this, &FlyBaseDialog::close);
}

void FlyBaseDialog::onApply()
{
	this->syncStateFromUi();
	LOGI("onApply:");
	FlyScoreState::fly_state_save(dataDir_, state_);
	LOGI("saved.");
	accept();
}
