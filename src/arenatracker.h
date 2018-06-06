#ifndef ARENATRACKER_H
#define ARENATRACKER_H

#include "entity/matchinfo.h"
#include "mtg/mtgarena.h"
#include "mtg/mtgcards.h"
#include "mtg/mtgamatch.h"
#include "ui/decktrackerplayer.h"
#include "ui/decktrackeropponent.h"
#include "ui/preferencesscreen.h"
#include "ui/startscreen.h"
#include "ui/trayicon.h"
#include "utils/appsettings.h"
#include "utils/logger.h"

#include <QApplication>

class ArenaTracker : public QApplication
{
    Q_OBJECT

private:
    bool isMatchRunning;
    MtgCards *mtgCards;
    DeckTrackerPlayer *deckTrackerPlayer;
    DeckTrackerOpponent *deckTrackerOpponent;
    TrayIcon *trayIcon;
    PreferencesScreen *preferencesScreen;
    StartScreen *startScreen;
    void setupApp();
    void setupPreferencesScreen();
    void setupStartScreen();
    void setupMtgaMatch();

public:
    ArenaTracker(int& argc, char **argv);
    ~ArenaTracker();
    AppSettings *appSettings;
    Logger *logger;
    MtgArena *mtgArena;
    MtgaMatch *mtgaMatch;
    int run();
    void showPreferencesScreen();

signals:

private slots:
    void onDeckSubmited(Deck deck);
    void onNewMatchStart(MatchInfo match);
    void onDeckTrackerPlayerEnabledChange(bool enabled);
    void onDeckTrackerOpponentEnabledChange(bool enabled);
};

#endif // ARENATRACKER_H
