#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][timers-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_timers_dialog.hpp"
#include "fly_score_i18n.hpp"
#include "fly_score_state.hpp"
#include "fly_score_qt_helpers.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QPushButton>
#include <QStyle>

FlyTimersDialog::FlyTimersDialog(const QString &dataDir, FlyScoreState::FlyState &state, QWidget *parent)
	: FlyBaseDialog(fly_i18n("Timers.Title"),
					fly_i18n("Timers.Hint"),
					fly_i18n("Common.Timers"),
					dataDir, state)
{
	setModal(true);
	resize(560, 420);
	setSizeGripEnabled(false);

	buildUi();
	syncUiFromState();
}

void FlyTimersDialog::buildUi()
{
	auto *rowsBox = new QWidget();
	rowsBox->setObjectName(QStringLiteral("timersRowsGroup"));

	auto *rowsLayout = new QVBoxLayout(rowsBox);
	rowsLayout->setContentsMargins(10, 10, 10, 10);
	rowsLayout->setSpacing(8);

	timersLayout_ = new QVBoxLayout();
	timersLayout_->setContentsMargins(0, 4, 0, 0);
	timersLayout_->setSpacing(6);
	timersLayout_->setAlignment(Qt::AlignTop);

	rowsLayout->addLayout(timersLayout_);

	addTimerBtn_ = new QPushButton(fly_i18n("Timers.AddTimer"), rowsBox);
	addTimerBtn_->setCursor(Qt::PointingHandCursor);
	rowsLayout->addWidget(addTimerBtn_, 0, Qt::AlignLeft);

	rowsBox->setLayout(rowsLayout);
	gl_->addWidget(rowsBox);

	connect(addTimerBtn_, &QPushButton::clicked, this, &FlyTimersDialog::onAddTimer);

	addButtons();
}

FlyTimersDialog::Row FlyTimersDialog::addRow(const FlyScoreState::FlyScoreState::FlyTimer &tm, bool canRemove)
{
	Row r;

	auto *row = new QWidget();
	auto *lay = new QHBoxLayout(row);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(8);

	auto *labelEdit = new QLineEdit(row);
	labelEdit->setPlaceholderText(fly_i18n("Timers.LabelPlaceholder"));
	labelEdit->setText(tm.label);
	labelEdit->setMinimumWidth(160);
	labelEdit->setMaximumWidth(260);

	auto *timeEdit = new QLineEdit(row);
	timeEdit->setPlaceholderText(QStringLiteral("mm:ss"));
	timeEdit->setClearButtonEnabled(true);
	timeEdit->setMaxLength(8);
	timeEdit->setMinimumWidth(70);
	timeEdit->setMaximumWidth(90);
	timeEdit->setText(FlyQtHelper::fly_format_ms_mmss(tm.initial_ms > 0 ? tm.initial_ms : tm.remaining_ms));

	auto *modeCombo = new QComboBox(row);
	modeCombo->addItems({QStringLiteral("countdown"), QStringLiteral("countup")});
	if (tm.mode.isEmpty())
		modeCombo->setCurrentText(QStringLiteral("countdown"));
	else
		modeCombo->setCurrentText(tm.mode);
	modeCombo->setMinimumWidth(110);
	modeCombo->setMaximumWidth(140);

	auto *removeBtn = new QPushButton(row);
	removeBtn->setText(QStringLiteral("❌"));
	removeBtn->setToolTip(canRemove ? fly_i18n("Timers.RemoveTimer")
					: fly_i18n("Timers.TimerCannotBeRemoved"));
	removeBtn->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);

	const int h = timeEdit->sizeHint().height();
	const int btnSize = h;
	removeBtn->setFixedSize(btnSize, btnSize);
	removeBtn->setEnabled(canRemove);
	removeBtn->setStyleSheet("QPushButton {"
				 "  font-family:'Segoe UI Emoji','Noto Color Emoji','Apple Color Emoji',sans-serif;"
				 "  font-size:12px;"
				 "  padding:0;"
				 "}");

	lay->addWidget(labelEdit, 1, Qt::AlignVCenter);
	lay->addWidget(new QLabel(fly_i18n("Common.TimeColon"), row), 0, Qt::AlignVCenter);
	lay->addWidget(timeEdit, 0, Qt::AlignVCenter);
	lay->addWidget(new QLabel(fly_i18n("Common.TypeColon"), row), 0, Qt::AlignVCenter);
	lay->addWidget(modeCombo, 0, Qt::AlignVCenter);
	lay->addWidget(removeBtn, 0, Qt::AlignVCenter);

	row->setLayout(lay);
	timersLayout_->addWidget(row);

	r.row = row;
	r.labelEdit = labelEdit;
	r.timeEdit = timeEdit;
	r.modeCombo = modeCombo;
	r.remove = removeBtn;

	if (canRemove) {
		connect(removeBtn, &QPushButton::clicked, this, [this, row]() {
			for (int i = 0; i < rows_.size(); ++i) {
				if (rows_[i].row == row) {
					auto rr = rows_[i];
					rows_.removeAt(i);
					if (timersLayout_)
						timersLayout_->removeWidget(rr.row);
					rr.row->setHidden(true);
					rr.row->deleteLater();
					break;
				}
			}
		});
	}

	return r;
}

void FlyTimersDialog::syncUiFromState()
{
	for (auto &r : rows_) {
		if (timersLayout_ && r.row)
			timersLayout_->removeWidget(r.row);
		if (r.row) {
			r.row->setHidden(true);
			r.row->deleteLater();
		}
	}
	rows_.clear();

	if (!timersLayout_)
		return;

	if (state_.timers.isEmpty()) {
		FlyScoreState::FlyTimer main;
		main.label = fly_i18n("Default.Timer.FirstHalf");
		main.mode = QStringLiteral("countdown");
		main.running = false;
		main.initial_ms = 0;
		main.remaining_ms = 0;
		main.last_tick_ms = 0;
		main.visible = true;
		state_.timers.push_back(main);
	}

	rows_.reserve(state_.timers.size());
	for (int i = 0; i < state_.timers.size(); ++i) {
		const auto &tm = state_.timers[i];
		const bool canRemove = (i > 0);
		Row r = addRow(tm, canRemove);
		rows_.push_back(r);
	}
}

void FlyTimersDialog::syncStateFromUi()
{
	const auto oldTimers = state_.timers;

	state_.timers.clear();
	state_.timers.reserve(rows_.size());

	for (int i = 0; i < rows_.size(); ++i) {
		const auto &r = rows_[i];

		FlyScoreState::FlyTimer tm;
		tm.label = r.labelEdit ? r.labelEdit->text() : QString();

		QString mode = r.modeCombo ? r.modeCombo->currentText() : QStringLiteral("countdown");
		tm.mode = mode;

		qint64 ms = 0;
		if (r.timeEdit) {
			qint64 parsed = FlyQtHelper::fly_parse_mmss_to_ms(r.timeEdit->text());
			ms = (parsed >= 0) ? parsed : 0;
		}

		tm.initial_ms = ms;
		tm.remaining_ms = ms;
		tm.running = false;
		tm.last_tick_ms = 0;

		if (i < oldTimers.size()) {
			tm.visible = oldTimers[i].visible;
		} else {
			tm.visible = true;
		}

		state_.timers.push_back(tm);
	}

	if (state_.timers.isEmpty()) {
		FlyScoreState::FlyTimer main;
		main.label = fly_i18n("Default.Timer.FirstHalf");
		main.mode = QStringLiteral("countdown");
		main.running = false;
		main.initial_ms = 0;
		main.remaining_ms = 0;
		main.last_tick_ms = 0;
		main.visible = true;
		state_.timers.push_back(main);
	}

	FlyScoreState::fly_state_save(dataDir_, state_);
}

void FlyTimersDialog::onAddTimer()
{
	FlyScoreState::FlyTimer tm;
	tm.label = QString();
	tm.mode = QStringLiteral("countdown");
	tm.running = false;
	tm.initial_ms = 0;
	tm.remaining_ms = 0;
	tm.last_tick_ms = 0;
	tm.visible = true;

	Row r = addRow(tm, true);
	rows_.push_back(r);
}

