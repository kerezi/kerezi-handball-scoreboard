#include "fly_score_teams_dialog.hpp"
#include "config.hpp"

#define LOG_TAG "[" PLUGIN_NAME "][teams-dialog]"
#include "fly_score_log.hpp"

#include "fly_score_logo_helpers.hpp"
#include "fly_score_i18n.hpp"
#include "fly_score_qt_helpers.hpp"
#include "fly_score_state.hpp"

#include <QGroupBox>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QToolButton>
#include <QPushButton>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QStyle>
#include <QColorDialog>
#include <QTabWidget>

static QColor colorFromU32(uint32_t c)
{
	int r = (c >> 16) & 0xFF;
	int g = (c >> 8) & 0xFF;
	int b = (c >> 0) & 0xFF;
	return QColor(r, g, b);
}

static uint32_t u32FromColor(const QColor &qc)
{
	uint32_t r = (uint32_t)qc.red() & 0xFF;
	uint32_t g = (uint32_t)qc.green() & 0xFF;
	uint32_t b = (uint32_t)qc.blue() & 0xFF;
	return (r << 16) | (g << 8) | b;
}

FlyTeamsDialog::FlyTeamsDialog(const QString &dataDir, FlyState &state, QWidget *parent)
	: FlyBaseDialog(fly_i18n("Teams.Title"), fly_i18n("Teams.Hint"), fly_i18n("Dock.Teams"), dataDir, state, parent)
{
	auto *tabs = new QTabWidget(this);
	tabs->setObjectName(fly_i18n("Teams.Title"));
	tabs->setDocumentMode(true);
	tabs->setMovable(false);
	tabs->setUsesScrollButtons(true);

	home_ = new FlyTeamUI{};
	home_->officialRows = new QVector<FlyOfficialUIRow>();
	addTeamUI(tabs, home_, "Teams.HomeTeam");

	guest_ = new FlyTeamUI{};
	guest_->officialRows = new QVector<FlyOfficialUIRow>();
	addTeamUI(tabs, guest_, "Teams.GuestsTeam");

	gl_->addWidget(tabs);

	connect(home_->addOfficialBtn, &QPushButton::clicked, this, &FlyTeamsDialog::onAddHomeOfficial);
	connect(guest_->addOfficialBtn, &QPushButton::clicked, this, &FlyTeamsDialog::onAddGuestOfficial);

	connect(home_->browse, &QToolButton::clicked, this, &FlyTeamsDialog::onBrowseHomeLogo);
	connect(guest_->browse, &QToolButton::clicked, this, &FlyTeamsDialog::onBrowseGuestLogo);

	connect(home_->color, &QToolButton::clicked, this, &FlyTeamsDialog::onPickHomeColor);
	connect(guest_->color, &QToolButton::clicked, this, &FlyTeamsDialog::onPickGuestColor);

	addButtons();
}

FlyTeamsDialog::~FlyTeamsDialog() = default;

void FlyTeamsDialog::addTeamUI(QTabWidget *tabs, FlyTeamUI *teamUI, const char *name)
{
	auto *teamTab = new QGroupBox(tabs);
	auto *teamRoot = new QGridLayout(teamTab);
	teamRoot->setContentsMargins(10, 10, 10, 10);
	teamRoot->setSpacing(8);

	teamUI->title = new QLineEdit(teamTab);
	teamUI->sub = new QLineEdit(teamTab);
	teamUI->logo = new QLineEdit(teamTab);
	teamUI->browse = new QToolButton(teamTab);
	teamUI->color = new QToolButton(teamTab);
	{
		const QIcon openIcon = fly_themed_icon(this, "folder-open", QStyle::SP_DirOpenIcon);
		teamUI->browse->setText(QString());
		teamUI->browse->setIcon(openIcon);
		teamUI->browse->setToolTip(fly_i18n("Teams.BrowseLogo"));
		teamUI->browse->setAutoRaise(false);
		teamUI->browse->setCursor(Qt::PointingHandCursor);

		teamUI->color->setText(QStringLiteral("#FFFFFF"));
		teamUI->color->setToolTip(fly_i18n("Teams.PickTeamColor"));
		teamUI->color->setAutoRaise(false);
		teamUI->color->setCursor(Qt::PointingHandCursor);
		teamUI->color->setMinimumWidth(90);
	}

	teamRoot->addWidget(new QLabel(fly_i18n("Common.TitleColon"), gb_), 0, 0, 1, 1);
	teamRoot->addWidget(teamUI->title, 0, 1, 1, 3);

	teamRoot->addWidget(new QLabel(fly_i18n("Common.SubtitleColon"), gb_), 1, 0, 1, 1);
	teamRoot->addWidget(teamUI->sub, 1, 1, 1, 3);

	teamRoot->addWidget(new QLabel(fly_i18n("Common.LogoColon"), gb_), 2, 0, 1, 1);
	teamRoot->addWidget(teamUI->logo, 2, 1, 1, 3);
	teamRoot->addWidget(teamUI->browse, 2, 4, 1, 1);

	teamRoot->addWidget(new QLabel(fly_i18n("Common.ColorColon"), gb_), 3, 0, 1, 1);
	teamRoot->addWidget(teamUI->color, 3, 1, 1, 2);
	teamUI->addOfficialBtn =
		new QPushButton(QString("+ ") + fly_i18n(name) + QString(" ") + fly_i18n("Teams.Officials"), teamTab);
	teamUI->addOfficialBtn->setCursor(Qt::PointingHandCursor);
	// teamRoot->addWidget(teamUI->addOfficialBtn, 3, 3, 1, 2);

	//QLabel *ol = new QLabel();
	//ol->setText(fly_i18n(name) + QString(" ") + fly_i18n("Teams.Officials"));
	//ol->setAlignment(Qt::AlignCenter);
	//teamRoot->addWidget(ol, 4, 0, 1, 5);
	teamRoot->addWidget(teamUI->addOfficialBtn, 4, 0, 1, 5);

	QWidget *tow = new QWidget();
	teamUI->officialsLayout = new QVBoxLayout();
	teamUI->officialsLayout->setContentsMargins(0, 4, 0, 0);
	teamUI->officialsLayout->setSpacing(6);
	teamUI->officialsLayout->setAlignment(Qt::AlignTop);
	tow->setLayout(teamUI->officialsLayout);
	teamRoot->addWidget(tow, 5, 0, 1, 5);

	teamTab->setLayout(teamRoot);

	tabs->addTab(teamTab, fly_i18n(name));
};

void FlyTeamsDialog::updateColorButton(QToolButton *btn, uint32_t color)
{
	if (!btn)
		return;

	QColor qc = colorFromU32(color);
	const QString hex = qc.name(QColor::HexRgb).toUpper();

	btn->setText(hex);

	btn->setStyleSheet(QString("QToolButton {"
				   "  padding: 6px 8px;"
				   "  border: 1px solid rgba(255,255,255,0.15);"
				   "  border-radius: 4px;"
				   "  background: %1;"
				   "  color: %2;"
				   "}")
				   .arg(hex)
				   .arg(qc.lightness() > 140 ? "#000000" : "#FFFFFF"));
}

void FlyTeamsDialog::onAddOfficial(FlyTeamUI *teamUI)
{
	FlyOfficial of;
	char ol = 'A' + teamUI->officialRows->size();
	of.officialLetter = QChar(QString(ol).at(0));
	of.officialName = QString();
	of.officialRole = QString();
	of.warning_ms = 0;
	of.fst2minSusp_ms = 0;
	of.disqualification_ms = 0;
	of.blueCard_ms = 0;

	// of.visible = true;

	addOfficialUI(teamUI, of, true);
	teamUI->addOfficialBtn->setDisabled(teamUI->officialRows->size() > 4);
	refreshRemoveButtons(teamUI->officialRows);
}

void FlyTeamsDialog::onAddHomeOfficial()
{
	onAddOfficial(home_);
}

void FlyTeamsDialog::onAddGuestOfficial()
{
	onAddOfficial(guest_);
}

void FlyTeamsDialog::refreshRemoveButtons(QVector<FlyOfficialUIRow> *rows)
{
	const bool canRemoveOfficial = rows->size() > 1;
	for (auto &r : *rows) {
		r.canRemove = canRemoveOfficial;
		if (r.remove) {
			r.remove->setEnabled(canRemoveOfficial);
			r.remove->setCursor(canRemoveOfficial ? Qt::PointingHandCursor : Qt::ArrowCursor);
			r.remove->setToolTip(canRemoveOfficial ? fly_i18n("Fields.RemoveField")
							   : fly_i18n("Fields.OneTeamStatRequired"));
		}
	}
}

FlyTeamsDialog::FlyOfficialUIRow FlyTeamsDialog::addOfficialUI(const FlyTeamUI *teamUI, const FlyOfficial &of,
							       bool canRemove)
{
	FlyOfficialUIRow r;
	r.canRemove = canRemove;

	auto *row = new QWidget(this);
	auto *lay = new QHBoxLayout(row);
	lay->setContentsMargins(0, 0, 0, 0);
	lay->setSpacing(8);

	auto *officialLetterLabel = new QLabel(row);
	officialLetterLabel->setText(of.officialLetter);
	officialLetterLabel->setMinimumWidth(18);
	officialLetterLabel->setMaximumWidth(26);

	auto *officialNameEdit = new QLineEdit(row);
	officialNameEdit->setPlaceholderText(fly_i18n("Fields.TeamLabelPlaceholder"));
	officialNameEdit->setText(of.officialName);
	officialNameEdit->setMinimumWidth(180);
	officialNameEdit->setMaximumWidth(260);

	auto *officialRoleEdit = new QLineEdit(row);
	officialRoleEdit->setPlaceholderText(fly_i18n("Fields.TeamLabelPlaceholder"));
	officialRoleEdit->setText(of.officialRole);
	officialRoleEdit->setMinimumWidth(180);
	officialRoleEdit->setMaximumWidth(260);

	auto *removeBtn = new QPushButton(row);
	removeBtn->setText(QStringLiteral("❌"));
	removeBtn->setToolTip(canRemove ? fly_i18n("Fields.RemoveField") : fly_i18n("Fields.FieldCannotBeRemoved"));
	removeBtn->setCursor(canRemove ? Qt::PointingHandCursor : Qt::ArrowCursor);

	const int btnSize = officialNameEdit->sizeHint().height();
	removeBtn->setFixedSize(btnSize, btnSize);
	removeBtn->setEnabled(canRemove);
	removeBtn->setStyleSheet("QPushButton {"
				 "  font-family:'Segoe UI Emoji','Noto Color Emoji','Apple Color Emoji',sans-serif;"
				 "  font-size:12px;"
				 "  padding:0;"
				 "}");

	lay->addWidget(officialLetterLabel, 2, Qt::AlignVCenter);
	lay->addWidget(officialNameEdit, 2, Qt::AlignVCenter);
	lay->addWidget(officialRoleEdit, 2, Qt::AlignVCenter);
	lay->addWidget(removeBtn, 1, Qt::AlignVCenter);

	row->setLayout(lay);
	teamUI->officialsLayout->addWidget(row);

	r.row = row;
	r.officialLetterLabel = officialLetterLabel;
	r.officialNameEdit = officialNameEdit;
	r.officialRoleEdit = officialRoleEdit;
	r.remove = removeBtn;
	teamUI->officialRows->push_back(r);

	if (canRemove) {
		connect(removeBtn, &QPushButton::clicked, this, [this, teamUI, row]() {
			if (teamUI->officialRows->size() <= 1) {
				QMessageBox::warning(this, fly_i18n("Common.CannotRemove"),
						     fly_i18n("Fields.OneTeamStatRequired"));
				return;
			}

			for (int i = 0; i < teamUI->officialRows->size(); ++i) {
				if (teamUI->officialRows->at(i).row == row) {
					auto rr = teamUI->officialRows->at(i);
					teamUI->officialRows->removeAt(i);
					if (teamUI->officialsLayout)
						teamUI->officialsLayout->removeWidget(rr.row);
					rr.row->deleteLater();
					break;
				}
			}

			teamUI->addOfficialBtn->setDisabled(teamUI->officialRows->size() > 4);
			refreshRemoveButtons(teamUI->officialRows);
		});
	}

	return r;
}

void FlyTeamsDialog::syncTeamUiFromState(FlyTeamUI *teamUI, FlyTeam *team)
{
	if (teamUI && teamUI->title)
		teamUI->title->setText(team->title);
	if (teamUI && teamUI->sub)
		teamUI->sub->setText(team->subtitle);
	if (teamUI && teamUI->logo)
		teamUI->logo->setText(team->logo);
	updateColorButton(teamUI->color, team->color);

	for (auto &r : *(teamUI->officialRows)) {
		if (teamUI->officialsLayout && r.row)
			teamUI->officialsLayout->removeWidget(r.row);
		if (r.row)
			r.row->deleteLater();
	}

	teamUI->officialRows->clear();
	// vis_.clear();

	if (!teamUI->officialsLayout)
		return;

	for (int i = 0; i < team->officials.size(); ++i) {
		const FlyOfficial &of = team->officials[i];
		const bool canRemove = (team->officials.size() > 1);
		addOfficialUI(teamUI, of, canRemove);
		teamUI->addOfficialBtn->setDisabled(teamUI->officialRows->size() > 4);
	}

	refreshRemoveButtons(teamUI->officialRows);
}

void FlyTeamsDialog::syncStateFromTeamUi(FlyTeamUI *teamUI, FlyTeam *team)
{
	if (team != nullptr)
		team->title = teamUI->title->text();
	if (team != nullptr)
		team->subtitle = teamUI->sub->text();
	if (team != nullptr)
		team->logo = teamUI->logo->text();
	updateColorButton(teamUI->color, team->color);
	
	team->officials.clear();
	for (int i = 0; i < teamUI->officialRows->size(); ++i) {
		const FlyOfficialUIRow &ofr = teamUI->officialRows->at(i);
		const bool canRemove = (teamUI->officialRows->size() > 1);
		FlyOfficial of = FlyOfficial{};
		of.officialLetter = ofr.officialLetterLabel->text().at(0);
		of.officialName = ofr.officialNameEdit->text();
		of.officialRole = ofr.officialRoleEdit->text();
		team->officials.push_back(of);
	}
}

void FlyTeamsDialog::syncUiFromState()
{
	syncTeamUiFromState(home_, &state_.home);
	syncTeamUiFromState(guest_, &state_.guest);
}

void FlyTeamsDialog::syncStateFromUi()
{
	syncStateFromTeamUi(home_, &state_.home);
	syncStateFromTeamUi(guest_, &state_.guest);
}

void FlyTeamsDialog::onPickColor(FlyTeamUI *teamUI, FlyTeam *team, const char *name)
{
	QColor start = colorFromU32(team->color);
	QColor chosen =
		QColorDialog::getColor(start, this, fly_i18n("Teams.PickHomeColor"), QColorDialog::ShowAlphaChannel);

	if (!chosen.isValid())
		return;

	team->color = u32FromColor(chosen);
	updateColorButton(teamUI->color, team->color);
	LOGI("%s color updated: %u", name, team->color);
}

void FlyTeamsDialog::onPickHomeColor()
{
	onPickColor(home_, &state_.home, "Home");
}

void FlyTeamsDialog::onPickGuestColor()
{
	onPickColor(guest_, &state_.guest, "Guest");
}

void FlyTeamsDialog::onBrowseLogo(FlyTeamUI *teamUI, FlyTeam *team, const char *name)
{
	QString m = "Teams.Select" + QString::fromUtf8(name) + "Logo";
	const QString p = QFileDialog::getOpenFileName(this, fly_i18n(m.toUtf8()), {},
						       fly_i18n("Common.ImageFilter"));
	if (p.isEmpty())
		return;

	const QString rel = fly_copy_logo_to_overlay(dataDir_, p, QString::fromUtf8(name));
	if (rel.isEmpty()) {
		QMessageBox::warning(this, fly_i18n("Teams.MessageTitle"), fly_i18n("Teams.CopyLogoFailed"));
		return;
	}

	if (teamUI->logo)
		teamUI->logo->setText(rel);
	team->logo = rel;

	LOGI("%s Logo updated: %s", name, rel.toUtf8().constData());
}

void FlyTeamsDialog::onBrowseHomeLogo()
{
	onBrowseLogo(home_, &state_.home, "Home");
}

void FlyTeamsDialog::onBrowseGuestLogo()
{
	onBrowseLogo(guest_, &state_.guest, "Guest");
}
