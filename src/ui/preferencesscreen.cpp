#include "preferencesscreen.h"
#include "ui_preferences.h"
#include "../macros.h"

#if defined Q_OS_MAC
#include "../utils/macautostart.h"
#elif defined Q_OS_WIN
#include "../utils/winautostart.h"
#endif

#include <QToolTip>

PreferencesScreen::PreferencesScreen(QWidget *parent) : QMainWindow(parent),
    ui(new Ui::Preferences())
{
    ui->setupUi(this);
    setWindowFlags(Qt::Dialog | Qt::WindowCloseButtonHint);
    applyCurrentSettings();
    connect(ui->btCheckUpdate, &QPushButton::clicked, this, [] {
        ARENA_TRACKER->sparkleUpdater->CheckForUpdatesNow();
    });
    connect(ui->cbStartAtLogin, &QCheckBox::clicked,
            this, &PreferencesScreen::onStartAtLoginChanged);
    connect(ui->cbAutoUpdate, &QCheckBox::clicked,
            this, &PreferencesScreen::onAutoUpdateChanged);
    connect(ui->rbMTG, &QRadioButton::clicked,
            this, &PreferencesScreen::onCardLayoutChanged);
    connect(ui->rbMTGA, &QRadioButton::clicked,
            this, &PreferencesScreen::onCardLayoutChanged);
    connect(ui->cbShowCardOnHover, &QCheckBox::clicked,
            this, &PreferencesScreen::onShowCardOnHoverChanged);
    connect(ui->cbShowOnlyRemainingCard, &QCheckBox::clicked,
            this, &PreferencesScreen::onShowOnlyRemainingCardsChanged);
    connect(ui->hsAlpha, &QSlider::valueChanged,
            this, &PreferencesScreen::onTrackerAlphaChanged);
    connect(ui->hsUnhideDelay, &QSlider::valueChanged,
            this, &PreferencesScreen::onUnhideDelayChanged);
    connect(ui->cbPTEnabled, &QCheckBox::clicked,
            this, &PreferencesScreen::onPTEnabledChanged);
    connect(ui->cbPTStatistics, &QCheckBox::clicked,
            this, &PreferencesScreen::onPTStatisticsChanged);
    connect(ui->cbOTEnabled, &QCheckBox::clicked,
            this, &PreferencesScreen::onOTEnabledChanged);
    connect(ui->btReset, &QPushButton::clicked,
            this, &PreferencesScreen::onRestoreDefaultsSettingsClicked);
}

PreferencesScreen::~PreferencesScreen()
{
    delete ui;
}

void PreferencesScreen::closeEvent(QCloseEvent *event)
{
    hide();
    event->ignore();
}

void PreferencesScreen::applyCurrentSettings()
{
    ui->cbStartAtLogin->setChecked(APP_SETTINGS->isAutoStartEnabled());
    ui->btCheckUpdate->setChecked(ARENA_TRACKER->sparkleUpdater->AutomaticallyChecksForUpdates());
    if (APP_SETTINGS->getCardLayout() == "mtg") {
        ui->rbMTG->setChecked(true);
    } else {
        ui->rbMTGA->setChecked(true);
    }
    ui->hsAlpha->setValue(APP_SETTINGS->getDeckTrackerAlpha());
    ui->hsUnhideDelay->setValue(APP_SETTINGS->getUnhiddenDelay());
    ui->cbShowCardOnHover->setChecked(APP_SETTINGS->isShowCardOnHoverEnabled());
    ui->cbShowOnlyRemainingCard->setChecked(APP_SETTINGS->isShowOnlyRemainingCardsEnabled());
    ui->cbPTEnabled->setChecked(APP_SETTINGS->isDeckTrackerPlayerEnabled());
    ui->cbPTStatistics->setChecked(APP_SETTINGS->isDeckTrackerPlayerStatisticsEnabled());
    ui->cbOTEnabled->setChecked(APP_SETTINGS->isDeckTrackerOpponentEnabled());
}

void PreferencesScreen::onStartAtLoginChanged()
{
    bool enabled = ui->cbStartAtLogin->isChecked();
#if defined Q_OS_MAC
    MacAutoStart::setEnabled(enabled);
#elif defined Q_OS_WIN
    WinAutoStart::setEnabled(enabled);
#endif
    LOGD(QString("StartAtLogin: %1").arg(enabled ? "true" : "false"));
    APP_SETTINGS->enableAutoStart(enabled);
}

void PreferencesScreen::onAutoUpdateChanged()
{
    bool enabled = ui->cbAutoUpdate->isChecked();
    ARENA_TRACKER->sparkleUpdater->SetAutomaticallyChecksForUpdates(enabled);
    LOGD(QString("AutoUpdate: %1").arg(enabled ? "true" : "false"));
    APP_SETTINGS->enableAutoUpdate(enabled);
}

void PreferencesScreen::onTrackerAlphaChanged()
{
    int alpha = ui->hsAlpha->value();
    emit sgnTrackerAlpha(alpha);
    LOGD(QString("Alpha: %1").arg(alpha));
    APP_SETTINGS->setDeckTrackerAlpha(alpha);
}

void PreferencesScreen::onUnhideDelayChanged()
{
    int delay = ui->hsUnhideDelay->value();
    QString unhideText = delay == 0 ? tr("(disabled)")
                                    : QString(tr("(%1 seconds)")).arg(delay);
    ui->lbUnhideDelay->setText(unhideText);
    emit sgnUnhideDelay(delay);
    LOGD(QString("UnhideDelay: %1").arg(delay));
    APP_SETTINGS->setUnhiddenDelay(delay);
}

void PreferencesScreen::onCardLayoutChanged()
{
    QString cardLayout = "mtga";
    if (ui->rbMTG->isChecked()) {
        cardLayout = "mtg";
    }
    emit sgnTrackerCardLayout(cardLayout);
    LOGD(QString("CardLayout: %1").arg(cardLayout));
    APP_SETTINGS->setCardLayout(cardLayout);
}

void PreferencesScreen::onShowCardOnHoverChanged()
{
    bool enabled = ui->cbShowCardOnHover->isChecked();
    emit sgnShowCardOnHoverEnabled(enabled);
    LOGD(QString("ShowCardOnHoverEnabled: %1").arg(enabled ? "true" : "false"));
    APP_SETTINGS->enableShowCardOnHover(enabled);
}

void PreferencesScreen::onShowOnlyRemainingCardsChanged()
{
    bool enabled = ui->cbShowOnlyRemainingCard->isChecked();
    emit sgnShowOnlyRemainingCardsEnabled(enabled);
    LOGD(QString("ShowOnlyRemainingCardsEnabled: %1").arg(enabled ? "true" : "false"));
    APP_SETTINGS->enableShowOnlyRemainingCards(enabled);
}

void PreferencesScreen::onPTEnabledChanged()
{
    bool enabled = ui->cbPTEnabled->isChecked();
    emit sgnPlayerTrackerEnabled(enabled);
    LOGD(QString("PlayerTrackerEnabled: %1").arg(enabled ? "true" : "false"));
    APP_SETTINGS->enableDeckTrackerPlayer(enabled);
}

void PreferencesScreen::onPTStatisticsChanged()
{
    bool enabled = ui->cbPTStatistics->isChecked();
    emit sgnPlayerTrackerStatistics(enabled);
    LOGD(QString("PlayerTrackerStatistics: %1").arg(enabled ? "true" : "false"));
    APP_SETTINGS->enableDeckTrackerPlayerStatistics(enabled);
}

void PreferencesScreen::onOTEnabledChanged()
{
    bool enabled = ui->cbOTEnabled->isChecked();
    emit sgnOpponentTrackerEnabled(enabled);
    LOGD(QString("DeckTrackerOpponent(: %1").arg(enabled ? "true" : "false"));
    APP_SETTINGS->enableDeckTrackerOpponent(enabled);
}

void PreferencesScreen::onRestoreDefaultsSettingsClicked()
{
    APP_SETTINGS->restoreDefaults();
    applyCurrentSettings();
    onStartAtLoginChanged();
    onCardLayoutChanged();
    onTrackerAlphaChanged();
    onPTEnabledChanged();
    onPTStatisticsChanged();
    onPTEnabledChanged();
    onOTEnabledChanged();
    emit sgnRestoreDefaults();
}
