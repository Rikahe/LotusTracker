#ifndef MATCHINFO_H
#define MATCHINFO_H

#include "deck.h"
#include "opponentinfo.h"

#include <QMap>
#include <QPair>
#include <QString>
#include <QElapsedTimer>

typedef enum
{
  MatchMode_SINGLE,
  MatchMode_BEST_OF_3,
  MatchMode_UNKNOWN
} MatchMode;

class GameInfo
{
private:
  QElapsedTimer timer;

public:
  bool playerGoFirst, playerMulligan, opponentMulligan, isCompleted, playerWins;
  int duration;
  Deck opponentDeck;

  GameInfo()
    : playerGoFirst(false)
    , playerMulligan(false)
    , opponentMulligan(false)
    , isCompleted(false)
    , playerWins(false)
    , duration(0)
    , opponentDeck(Deck())
  {
    timer.start();
  }

  void finish(bool playerGameWins)
  {
    duration = static_cast<int>(timer.elapsed() / 1000);
    playerWins = playerGameWins;
    isCompleted = true;
  }
};

class MatchInfo
{
public:
  QString eventId;
  OpponentInfo opponentInfo;
  MatchMode mode;
  QList<GameInfo> games;
  bool playerMatchWins;
  int playerGameWins, playerGameLoses;

  explicit MatchInfo(QString eventId = "", OpponentInfo opponentInfo = OpponentInfo());
  void createNewGame();
  GameInfo& currentGame();
  QMap<Card*, int> getOpponentMatchesCards();
  QString getOpponentDeckArch();
  QString getOpponentDeckColorIdentity();

  static QString MatchModeToString(MatchMode matchMode);
};

#endif  // MATCHINFO_H
