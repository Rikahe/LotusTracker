#include "DraftOverlay.h"
#include "../macros.h"

#include <QApplication>
#include <QPoint>
#include <QToolTip>

DraftOverlay::DraftOverlay(QWidget *parent) : DeckOverlayBase(parent)
{
    applyCurrentSettings();
}

DraftOverlay::~DraftOverlay()
{

}

void DraftOverlay::applyCurrentSettings()
{
    move(APP_SETTINGS->getDraftOverlayPos(uiWidth));
    uiScale = APP_SETTINGS->getDraftOverlayScale();
    DeckOverlayBase::onScaleChanged();
}

int DraftOverlay::getDeckNameYPosition()
{
    return uiPos.y() - titleHeight - 7;
}

int DraftOverlay::getHoverCardXPosition()
{
    return uiPos.x() + uiWidth + 10;
}

QString DraftOverlay::getDeckColorIdentity()
{
    return deck.colorIdentity(false, true);
}

bool DraftOverlay::useGrayscaleForZeroQtd()
{
    return false;
}

void DraftOverlay::onPositionChanged()
{
    APP_SETTINGS->setDraftOverlayPos(pos());
}

void DraftOverlay::onScaleChanged()
{
    DeckOverlayBase::onScaleChanged();
    APP_SETTINGS->setDraftOverlayScale(uiScale);
}

void DraftOverlay::afterPaintEvent(QPainter &painter)
{
    // Preferences button
    int buttonSize = 16 + static_cast<int> (uiScale * 1);
    int buttonMarginX = 3;
    int buttonMarginY = 2;
    int preferencesButtonY = uiPos.y() + buttonMarginY;
    QImage settings(":res/preferences.png");
    QImage settingsScaled = settings.scaled(buttonSize, buttonSize,
                                            Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    int settingsPlusX = uiPos.x() + uiWidth - buttonSize - buttonMarginX;
    painter.drawImage(settingsPlusX, preferencesButtonY, settingsScaled);
    preferencesButton = QRect(settingsPlusX, preferencesButtonY, buttonSize, buttonSize);
}

void DraftOverlay::reset()
{
    deck.clear();
    deck.updateTitle("");
    update();
}
void DraftOverlay::onDraftStatus(QList<Card *> availablePicks, QList<Card *> pickedCards)
{
    UNUSED(pickedCards);
    deck.clear();
    for (Card* card : availablePicks) {
        int qtdOwned = playerCollection[card->mtgaId];
        deck.setCardQtd(card, qtdOwned);
    }
    update();
}

void DraftOverlay::onHoverMove(QHoverEvent *event)
{
    showingTooltip = false;
    if (preferencesButton.contains(event->pos())) {
        showingTooltip = true;
        QToolTip::showText(event->pos(), tr("Settings"));
    }
    DeckOverlayBase::onHoverMove(event);
}

void DraftOverlay::mousePressEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton) {
        return;
    }
    if (preferencesButton.contains(event->pos())) {
        return;
    }
    DeckOverlayBase::mousePressEvent(event);
}

void DraftOverlay::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() != Qt::LeftButton && event->button() != Qt::RightButton) {
        return;
    }
    if (preferencesButton.contains(event->pos())) {
        hideCardOnHover();
        LOTUS_TRACKER->showPreferencesScreen();
        LOTUS_TRACKER->gaTracker->sendEvent("Overlay", "Preferences");
        return;
    }
    DeckOverlayBase::mouseReleaseEvent(event);
}
