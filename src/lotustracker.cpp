#include "lotustracker.h"
#include "macros.h"
#include "urls.h"
#include "mtg/mtgalogparser.h"
#include "utils/cocoainitializer.h"
#include "utils/metrics.h"

#if defined Q_OS_MAC
#include "utils/macautostart.h"
#include "updater/macsparkleupdater.h"
#elif defined Q_OS_WIN
#include "utils/winautostart.h"
#include "updater/winsparkleupdater.h"
#endif

#include <iostream>
#include <QtNetwork/QLocalSocket>
#include <QMessageBox>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <utility>

#define LOGS_QUEUE_MAX_SIZE 100

LotusTracker::LotusTracker(int& argc, char** argv) : QApplication(argc, argv)
{
  setupApp();
  setupUpdater();
  isOnDraftScreen = false;
  logger = new Logger(this);
  appSettings = new AppSettings(this);
  mtgCards = new MtgCards(this);
  mtgDecksArch = new MtgDecksArch(this);
  mtgArena = new MtgArena(this);
  deckOverlayPlayer = new DeckOverlayPlayer();
  deckOverlayOpponent = new DeckOverlayOpponent();
  deckOverlayDraft = new DeckOverlayDraft();
  trayIcon = new TrayIcon(this);
  connect(trayIcon, &TrayIcon::sgnShowDeckOverlay, this, [this] {
    deckOverlayPlayer->show();
    deckOverlayOpponent->show();
  });
  lotusAPI = new LotusTrackerAPI(this);
  untappedAPI = new UntappedAPI(this);
  startScreen = new StartScreen(nullptr, lotusAPI);
  hideTrackerTimer = new QTimer(this);
  mtgaMatch = new MtgaMatch(this, mtgCards);
  gaTracker = new GAnalytics(CREDENTIALS::GA_ID());
  connect(mtgArena, &MtgArena::sgnMTGAStarted, this, &LotusTracker::onGameStarted);
  connect(mtgArena, &MtgArena::sgnMTGAFocusChanged, this, &LotusTracker::onGameFocusChanged);
  connect(mtgArena, &MtgArena::sgnMTGAStopped, this, &LotusTracker::onGameStopped);
  connect(deckOverlayDraft, &DeckOverlayDraft::sgnRequestPlayerCollection, lotusAPI,
          &LotusTrackerAPI::onRequestPlayerCollection);
  connect(hideTrackerTimer, &QTimer::timeout, this, [this] {
    hideTrackerTimer->stop();
    deckOverlayPlayer->reset();
    deckOverlayPlayer->hide();
    deckOverlayOpponent->reset();
    deckOverlayOpponent->hide();
  });
  // setupMatch should be called before setupLogParser because sgnMatchInfoResult order
  setupLotusAPIConnections();
  setupUntappedAPIConnections();
  setupLogParserConnections();
  setupMtgaMatchConnections();
  setupPreferencesScreen();
  LOGI("Lotus Tracker started");
#if defined Q_OS_MAC
  MacAutoStart::setEnabled(APP_SETTINGS->isAutoStartEnabled());
#elif defined Q_OS_WIN
  WinAutoStart::setEnabled(APP_SETTINGS->isAutoStartEnabled());
#endif
  // store in a variable, as reading this will clear the first run flag
  bool isFirstRun = APP_SETTINGS->isFirstRun();
  if (isFirstRun)
  {
    startScreen->show();
    startScreen->raise();
    showMessage(tr("Lotus Tracker is running in background, you can click on tray icon for preferences."));
  }
  else
  {
    checkConnection = new QTimer();
    connect(checkConnection, &QTimer::timeout, this, [this]() {
      LOGD("Checking internet connection..");
      if (isOnline())
      {
        LOGD("Internet connection OK");
        checkConnection->stop();
        checkForAutoLogin();
        checkForUntappedUploadToken();
      }
    });
    checkConnection->start(3000);
  }

  influx_metric(influxdb_cpp::builder()
                    .meas("lt_app_start")
                    .tag("autostart", APP_SETTINGS->isAutoStartEnabled() ? "true" : "false")
                    .tag("new", isFirstRun ? "true" : "false")
                    .field("count", 1));
}

LotusTracker::~LotusTracker()
{
  DEL(logger)
  DEL(deckOverlayPlayer)
  DEL(deckOverlayOpponent)
  DEL(preferencesScreen)
  DEL(trayIcon)
  DEL(mtgArena)
  DEL(mtgaMatch)
  DEL(lotusAPI)
  DEL(gaTracker)
}

int LotusTracker::run()
{
  try
  {
    return isAlreadyRunning() ? 1 : exec();
  }
  catch (const std::exception& ex)
  {
    trackException(LotusException(ex.what()));
    return -1;
  }
}

void LotusTracker::setupApp()
{
#if defined Q_OS_MAC
  setAttribute(Qt::AA_UseHighDpiPixmaps);
  QIcon icon(":/icon_black.png");
  icon.addFile(":/icon_black@2x.png");
#elif defined Q_OS_WIN
  QIcon icon(":/icon.ico");
#else
  QIcon icon(":/icon.ico");
#endif
  setAttribute(Qt::AA_Use96Dpi);
  setApplicationName(APP_NAME);
  setApplicationDisplayName(APP_NAME);
  setApplicationVersion(VERSION);
  setOrganizationName("Mtg Lotus Valley");
  setOrganizationDomain(URLs::SITE());
  setWindowIcon(icon);
}

void LotusTracker::setupUpdater()
{
#if defined Q_OS_MAC
  CocoaInitializer cocoaInitializer;
  QString updateUrl = QString("%1/%2").arg(URLs::SITE()).arg("appcast-osx.xml");
  sparkleUpdater = new MacSparkleUpdater(updateUrl);
#elif defined Q_OS_WIN
  QString updateUrl = QString("%1/%2").arg(URLs::SITE()).arg("appcast-win.xml");
  sparkleUpdater = new WinSparkleUpdater(updateUrl);
#endif
}

bool LotusTracker::isAlreadyRunning()
{
  QString serverName = "ArenaTracker";
  QLocalSocket socket;
  socket.connectToServer(serverName);
  if (socket.waitForConnected(500))
  {
    QMessageBox::information(preferencesScreen, "Lotus Tracker", "Lotus Tracker already running in background.",
                             QMessageBox::Ok);
    return true;
  }
  QLocalServer::removeServer(serverName);
  localServer = new QLocalServer(nullptr);
  if (!localServer->listen(serverName))
  {
    return true;
  }
  return false;
}

bool LotusTracker::isOnline()
{
  QNetworkAccessManager nam;
  QNetworkRequest req(QUrl("http://www.google.com"));
  QNetworkReply* reply = nam.get(req);
  QEventLoop loop;
  connect(reply, SIGNAL(finished()), &loop, SLOT(quit()));
  loop.exec();
  return reply->bytesAvailable();
}

void LotusTracker::setupPreferencesScreen()
{
  preferencesScreen = new PreferencesScreen();
  // Tab General
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnLogFilePathChanged, mtgArena,
          &MtgArena::onLogFilePathChanged);
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnRestoreDefaults, preferencesScreen->getTabOverlay(),
          &TabOverlay::onRestoreDefaultsSettings);
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnDeckOverlayDraftEnabled, this,
          &LotusTracker::onDeckOverlayDraftEnabledChange);
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnDraftOverlaySource, deckOverlayDraft,
          &DeckOverlayDraft::onSourceChanged);
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnPlayerOverlayEnabled, this,
          &LotusTracker::onDeckOverlayPlayerEnabledChange);
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnRestoreDefaults, deckOverlayPlayer,
          &DeckOverlayPlayer::applyCurrentSettings);
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnOpponentOverlayEnabled, this,
          &LotusTracker::onDeckOverlayOpponentEnabledChange);
  connect(preferencesScreen->getTabGeneral(), &TabGeneral::sgnRestoreDefaults, deckOverlayOpponent,
          &DeckOverlayOpponent::applyCurrentSettings);
  // Tab Overlay
  // --- Player Overlay
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnTrackerAlpha, deckOverlayPlayer,
          &DeckOverlayBase::changeAlpha);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnUnhideDelay, deckOverlayPlayer,
          &DeckOverlayBase::changeUnhiddenTimeout);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnTrackerCardLayout, deckOverlayPlayer,
          &DeckOverlayBase::changeCardLayout);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnShowCardManaCostEnabled, deckOverlayPlayer,
          &DeckOverlayBase::onShowCardManaCostEnabled);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnShowCardOnHoverEnabled, deckOverlayPlayer,
          &DeckOverlayBase::onShowCardOnHoverEnabled);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnShowOnlyRemainingCardsEnabled, deckOverlayPlayer,
          &DeckOverlayPlayer::onShowOnlyRemainingCardsEnabled);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnPlayerTrackerStatistics, deckOverlayPlayer,
          &DeckOverlayPlayer::onStatisticsEnabled);
  // --- Opponent Overlay
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnTrackerAlpha, deckOverlayOpponent,
          &DeckOverlayBase::changeAlpha);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnUnhideDelay, deckOverlayOpponent,
          &DeckOverlayBase::changeUnhiddenTimeout);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnTrackerCardLayout, deckOverlayOpponent,
          &DeckOverlayBase::changeCardLayout);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnShowCardManaCostEnabled, deckOverlayOpponent,
          &DeckOverlayBase::onShowCardManaCostEnabled);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnShowCardOnHoverEnabled, deckOverlayOpponent,
          &DeckOverlayBase::onShowCardOnHoverEnabled);
  // --- Draft Overlay
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnTrackerAlpha, deckOverlayDraft,
          &DeckOverlayBase::changeAlpha);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnUnhideDelay, deckOverlayDraft,
          &DeckOverlayBase::changeUnhiddenTimeout);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnTrackerCardLayout, deckOverlayDraft,
          &DeckOverlayBase::changeCardLayout);
  connect(preferencesScreen->getTabOverlay(), &TabOverlay::sgnShowCardManaCostEnabled, deckOverlayDraft,
          &DeckOverlayBase::onShowCardManaCostEnabled);
  // Tab Logs
  connect(logger, &Logger::sgnLog, preferencesScreen->getTabLogs(), &TabLogs::onNewLog);
  connect(logger, &Logger::sgnLog, this, [this](LogType type, const QString& log) {
    if (type == DEBUG)
    {
      return;
    }
    QString msg = QString("%1 - %2").arg(LOG_TYPE_NAMES[type]).arg(log);
    logsQueue.enqueue(msg);
    if (logsQueue.size() > LOGS_QUEUE_MAX_SIZE)
    {
      logsQueue.dequeue();
    }
  });
}

void LotusTracker::setupLotusAPIConnections()
{
  connect(lotusAPI, &LotusTrackerAPI::sgnDeckWinRate, deckOverlayPlayer, &DeckOverlayPlayer::onPlayerDeckStatus);
  connect(lotusAPI, &LotusTrackerAPI::sgnEventInfo, deckOverlayPlayer, &DeckOverlayPlayer::onReceiveEventInfo);
  connect(lotusAPI, &LotusTrackerAPI::sgnRequestFinishedWithSuccess, deckOverlayPlayer,
          &DeckOverlayPlayer::onLotusAPIRequestFinishedWithSuccess);
  connect(lotusAPI, &LotusTrackerAPI::sgnRequestFinishedWithError, deckOverlayPlayer,
          &DeckOverlayPlayer::onLotusAPIRequestFinishedWithError);
  connect(lotusAPI, &LotusTrackerAPI::sgnEventInfo, deckOverlayOpponent, &DeckOverlayOpponent::onReceiveEventInfo);
  connect(lotusAPI, &LotusTrackerAPI::sgnUserLogged, this, &LotusTracker::onUserSigned);
  connect(lotusAPI, &LotusTrackerAPI::sgnTokenRefreshed, this, &LotusTracker::onUserTokenRefreshed);
  connect(lotusAPI, &LotusTrackerAPI::sgnTokenRefreshError, this, &LotusTracker::onUserTokenRefreshError);
  connect(lotusAPI, &LotusTrackerAPI::sgnPlayerCollection, deckOverlayDraft, &DeckOverlayDraft::setPlayerCollection);
  connect(lotusAPI, &LotusTrackerAPI::sgnParseDeckPosSideboardJson, mtgArena->getLogParser(),
          &MtgaLogParser::onParseDeckPosSideboardJson);
}

void LotusTracker::setupUntappedAPIConnections()
{
  connect(untappedAPI, &UntappedAPI::sgnNewAnonymousUploadToken, this,
          [this](QString uploadToken) { appSettings->setUntappedAnonymousUploadToken(uploadToken); });
}

void LotusTracker::setupLogParserConnections()
{
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerCollection, this,
          &LotusTracker::onPlayerCollectionUpdated);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerDecks, this, &LotusTracker::onPlayerDecks);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerInventory, lotusAPI,
          &LotusTrackerAPI::updatePlayerInventory);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerDeckCreated, lotusAPI, &LotusTrackerAPI::createPlayerDeck);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerDeckUpdated, lotusAPI, &LotusTrackerAPI::updatePlayerDeck);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerRankInfo, mtgaMatch, &MtgaMatch::onPlayerRankInfo);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerDeckSubmited, this, &LotusTracker::onDeckSubmited);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerDeckWithSideboardSubmited, deckOverlayPlayer,
          &DeckOverlayPlayer::loadDeckWithSideboard);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnEventPlayerCourse, this, &LotusTracker::onEventPlayerCourse);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnMatchCreated, this, &LotusTracker::onMatchStart);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnGameStart, this, &LotusTracker::onGameStart);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnGameCompleted, this, &LotusTracker::onGameCompleted);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnMatchResult, this, &LotusTracker::onMatchEnds);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnEventFinish, this, &LotusTracker::onEventFinish);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnDraftPick, this, &LotusTracker::onDraftPick);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnDraftStatus, this, &LotusTracker::onDraftStatus);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnDecodeDeckPosSideboardPayload, lotusAPI,
          &LotusTrackerAPI::onDecodeDeckPosSideboardPayload);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnGoingToHome, this, [this]() {
    deckOverlayPlayer->reset();
    deckOverlayPlayer->hide();
  });
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnLeavingDraft, this, [this]() {
    isOnDraftScreen = false;
    deckOverlayDraft->hide();
  });
}

void LotusTracker::setupMtgaMatchConnections()
{
  // Player
  connect(mtgaMatch, &MtgaMatch::sgnPlayerPutOnLibraryCard, deckOverlayPlayer,
          &DeckOverlayPlayer::onPlayerPutOnLibraryCard);
  connect(mtgaMatch, &MtgaMatch::sgnPlayerPutOnHandCard, deckOverlayPlayer, &DeckOverlayPlayer::onPlayerPutOnHandCard);
  connect(mtgaMatch, &MtgaMatch::sgnPlayerDrawCard, deckOverlayPlayer, &DeckOverlayPlayer::onPlayerDrawCard);
  connect(mtgaMatch, &MtgaMatch::sgnPlayerDiscardCard, deckOverlayPlayer, &DeckOverlayPlayer::onPlayerDiscardCard);
  connect(mtgaMatch, &MtgaMatch::sgnPlayerDiscardFromLibraryCard, deckOverlayPlayer,
          &DeckOverlayPlayer::onPlayerDiscardFromLibraryCard);
  connect(mtgaMatch, &MtgaMatch::sgnPlayerPutOnBattlefieldCard, deckOverlayPlayer,
          &DeckOverlayPlayer::onPlayerPutOnBattlefieldCard);
  // Opponent
  connect(mtgaMatch, &MtgaMatch::sgnOpponentPutOnLibraryCard, deckOverlayOpponent,
          &DeckOverlayOpponent::onOpponentPutOnLibraryCard);
  connect(mtgaMatch, &MtgaMatch::sgnOpponentPutOnHandCard, deckOverlayOpponent,
          &DeckOverlayOpponent::onOpponentPutOnHandCard);
  connect(mtgaMatch, &MtgaMatch::sgnOpponentPlayCard, deckOverlayOpponent, &DeckOverlayOpponent::onOpponentPlayCard);
  connect(mtgaMatch, &MtgaMatch::sgnOpponentDiscardCard, deckOverlayOpponent,
          &DeckOverlayOpponent::onOpponentDiscardCard);
  connect(mtgaMatch, &MtgaMatch::sgnOpponentDiscardFromLibraryCard, deckOverlayOpponent,
          &DeckOverlayOpponent::onOpponentDiscardFromLibraryCard);
  connect(mtgaMatch, &MtgaMatch::sgnOpponentPutOnBattlefieldCard, deckOverlayOpponent,
          &DeckOverlayOpponent::onOpponentPutOnBattlefieldCard);
  // Match
  connect(mtgaMatch, &MtgaMatch::sgnPlayerUserName, lotusAPI, &LotusTrackerAPI::setPlayerUserName);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnMatchInfoSeats, mtgaMatch, &MtgaMatch::onMatchInfoSeats);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnMatchStateDiff, mtgaMatch, &MtgaMatch::onMatchStateDiff);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnNewTurnStarted, mtgaMatch, &MtgaMatch::onNewTurnStarted);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnPlayerTakesMulligan, mtgaMatch,
          &MtgaMatch::onPlayerTakesMulligan);
  connect(mtgArena->getLogParser(), &MtgaLogParser::sgnOpponentTakesMulligan, mtgaMatch,
          &MtgaMatch::onOpponentTakesMulligan);
}

void LotusTracker::avoidAppClose()
{
  preferencesScreen->show();
  preferencesScreen->hide();
}

void LotusTracker::showStartScreen()
{
  startScreen->show();
  startScreen->raise();
}

void LotusTracker::showPreferencesScreen()
{
  preferencesScreen->show();
  preferencesScreen->raise();
}

void LotusTracker::showMessage(QString msg, QString title)
{
  trayIcon->showMessage(std::move(title), std::move(msg));
}

void LotusTracker::publishOrUpdatePlayerDeck(Deck deck)
{
  QString playerName = mtgaMatch->getPlayerName();
  lotusAPI->publishOrUpdatePlayerDeck(playerName, std::move(deck));
}

void LotusTracker::trackException(const LotusException& ex)
{
  qDebug() << ex.what();
  gaTracker->sendException(ex.what());
}

void LotusTracker::onPlayerCollectionUpdated(const QMap<int, int>& ownedCards)
{
  deckOverlayDraft->setPlayerCollection(ownedCards);
  lotusAPI->updatePlayerCollection(ownedCards);
}

void LotusTracker::onPlayerDecks(QList<Deck> playerDecks)
{
  UNUSED(playerDecks);
  hideTrackerTimer->start(2000);
}

void LotusTracker::onDeckSubmited(QString eventId, Deck deck)
{
  deckOverlayPlayer->loadDeck(deck);
  lotusAPI->updatePlayerDeck(deck);
  lotusAPI->getMatchInfo(std::move(eventId), deck.id);
}

void LotusTracker::onEventPlayerCourse(const QString& eventId, const Deck& currentDeck, bool isFinished)
{
  eventPlayerCourse = qMakePair(eventId, currentDeck);
  if (isFinished && appSettings->isShowDeckAfterDraftEnabled())
  {
    deckOverlayPlayer->loadDeck(currentDeck);
    deckOverlayPlayer->show();
  }
}

void LotusTracker::onMatchStart(const QString& eventId, OpponentInfo opponentInfo)
{
  isOnDraftScreen = false;
  mtgaMatch->onStartNewMatch(eventId, std::move(opponentInfo));
  // Load deck from event in course if not loaded yet (event continues without submitDeck)
  if (eventId == eventPlayerCourse.first)
  {
    Deck deck = eventPlayerCourse.second;
    deckOverlayPlayer->loadDeck(deck);
    lotusAPI->getMatchInfo(eventId, deck.id);
  }
  deckOverlayOpponent->reset();
  deckOverlayOpponent->setEventId(eventId);
  gaTracker->sendEvent("Match", "starts", eventId);
}

void LotusTracker::onGameStart(MatchMode mode, QList<MatchZone> zones, int seatId)
{
  if (!mtgaMatch->isRunning)
  {
    return;
  }
  mtgaMatch->onGameStart(mode, std::move(zones), seatId);
  LOGD(QString("mtgArena->isFocused: %1").arg(mtgArena->isFocused));
  if (APP_SETTINGS->isDeckOverlayPlayerEnabled() && mtgArena->isFocused)
  {
    deckOverlayPlayer->show();
  }
  if (APP_SETTINGS->isDeckOverlayOpponentEnabled() && mtgArena->isFocused)
  {
    deckOverlayOpponent->show();
  }
  if (APP_SETTINGS->isFirstMatch())
  {
    showMessage(tr("You can hide/show the overlay with a mouse right click on it."));
  }
}

void LotusTracker::onGameStarted()
{
  gaTracker->startSession();
}

void LotusTracker::onGameFocusChanged(bool hasFocus)
{
  if (isOnDraftScreen && APP_SETTINGS->isDeckOverlayDraftEnabled())
  {
    if (hasFocus)
    {
      deckOverlayDraft->show();
    }
    else if (APP_SETTINGS->isHideOnLoseGameFocusEnabled())
    {
      deckOverlayDraft->hide();
    }
  }
  if (mtgaMatch->isRunning && APP_SETTINGS->isDeckOverlayPlayerEnabled())
  {
    if (hasFocus)
    {
      deckOverlayPlayer->show();
    }
    else if (APP_SETTINGS->isHideOnLoseGameFocusEnabled())
    {
      deckOverlayPlayer->hide();
    }
  }
  if (mtgaMatch->isRunning && APP_SETTINGS->isDeckOverlayOpponentEnabled())
  {
    if (hasFocus)
    {
      deckOverlayOpponent->show();
    }
    else if (APP_SETTINGS->isHideOnLoseGameFocusEnabled())
    {
      deckOverlayOpponent->hide();
    }
  }
  gaTracker->sendEvent("Game", "Focus change");
}

void LotusTracker::onGameStopped()
{
  deckOverlayPlayer->hide();
  deckOverlayOpponent->hide();
  gaTracker->endSession();
}

void LotusTracker::onGameCompleted(QMap<int, int> teamIdWins)
{
  if (!mtgaMatch->isRunning)
  {
    return;
  }
  mtgaMatch->onGameCompleted(deckOverlayOpponent->getDeck(), std::move(teamIdWins));
  hideTrackerTimer->start(5000);
}

void LotusTracker::onMatchEnds(int winningTeamId)
{
  if (!mtgaMatch->isRunning)
  {
    return;
  }
  mtgaMatch->onEndCurrentMatch(winningTeamId);
  if (mtgaMatch->getInfo().eventId != "NPE")
  {
    lotusAPI->uploadMatch(mtgaMatch->getInfo(), deckOverlayPlayer->getDeck(), mtgaMatch->getPlayerRankInfo().first);
  }
  gaTracker->sendEvent("Match", "ends");
}

void LotusTracker::onEventFinish(QString eventId, QString deckId, QString deckColors, int maxWins, int wins, int losses)
{
  lotusAPI->uploadEventResult(std::move(eventId), std::move(deckId), std::move(deckColors), maxWins, wins, losses);
  if (APP_SETTINGS->isShowDeckAfterDraftEnabled())
  {
    deckOverlayPlayer->reset();
  }
}

void LotusTracker::onDeckOverlayDraftEnabledChange(bool enabled)
{
  if (enabled && isOnDraftScreen)
  {
    deckOverlayDraft->show();
  }
  else
  {
    deckOverlayDraft->hide();
  }
}

void LotusTracker::onDeckOverlayPlayerEnabledChange(bool enabled)
{
  if (enabled && mtgaMatch->isRunning)
  {
    deckOverlayPlayer->show();
  }
  if (!enabled && mtgaMatch->isRunning)
  {
    deckOverlayPlayer->hide();
  }
}

void LotusTracker::onDeckOverlayOpponentEnabledChange(bool enabled)
{
  if (enabled && mtgaMatch->isRunning)
  {
    deckOverlayOpponent->show();
  }
  if (!enabled && mtgaMatch->isRunning)
  {
    deckOverlayOpponent->hide();
  }
}

void LotusTracker::checkForAutoLogin()
{
  UserSettings userSettings = appSettings->getUserSettings();
  switch (userSettings.getAuthStatus())
  {
    case AUTH_VALID:
    {
      emit lotusAPI->sgnUserLogged(false);
      break;
    }
    case AUTH_EXPIRED:
    {
      lotusAPI->refreshToken(userSettings.refreshToken);
      break;
    }
    case AUTH_INVALID:
    {
      break;
    }
  }
}

void LotusTracker::checkForUntappedUploadToken()
{
  if (appSettings->getUntappedAnonymousUploadToken().isEmpty())
  {
    untappedAPI->fetchAnonymousUploadToken();
  }
}

void LotusTracker::onUserSigned(bool fromSignUp)
{
  if (fromSignUp)
  {
    gaTracker->sendEvent("LotusTracker", "SignUp");
  }
  startScreen->hide();
  trayIcon->updateUserSettings();
}

void LotusTracker::onUserTokenRefreshed()
{
  startScreen->hide();
  trayIcon->updateUserSettings();
}

void LotusTracker::onUserTokenRefreshError()
{
  startScreen->show();
  trayIcon->updateUserSettings();
}

void LotusTracker::onDraftPick(int mtgaId, int packNumber, int pickNumber)
{
  QString eventName = deckOverlayDraft->getCurrentDraftName();
  QList<Card*> availablePicks = deckOverlayDraft->getAvailablePicks();
  APP_SETTINGS->setDraftPick(eventName, packNumber, pickNumber, mtgaId, availablePicks);
}

void LotusTracker::onDraftStatus(QString eventName, const QString& status, int packNumber, int pickNumber,
                                 QList<Card*> availablePicks, QList<Card*> pickedCards)
{
  UNUSED(packNumber);
  UNUSED(pickNumber);
  isOnDraftScreen = true;
  deckOverlayDraft->onDraftStatus(std::move(eventName), std::move(availablePicks), std::move(pickedCards));
  if (status == "Draft.PickNext")
  {
    deckOverlayDraft->show();
  }
  else
  {
    deckOverlayDraft->reset();
    deckOverlayDraft->hide();
  }
}

// Just for Tests
void LotusTracker::setEventInfo(QString eventName, QString eventType)
{
  deckOverlayOpponent->onReceiveEventInfo(std::move(eventName), std::move(eventType));
}
