#include "../src/mtg/mtgalogparser.h"
#include "macros_test.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QIODevice>

#include <QSignalSpy>
#include <QtTest/QtTest>

Q_DECLARE_METATYPE(Deck)
Q_DECLARE_METATYPE(OpponentInfo)
Q_DECLARE_METATYPE(MatchPlayer)
Q_DECLARE_METATYPE(MatchStateDiff)
Q_DECLARE_METATYPE(MatchZone)
Q_DECLARE_METATYPE(MatchMode)
Q_DECLARE_METATYPE(PlayerInventory)

class TestMtgaLogParser : public QObject
{
  Q_OBJECT
private:
  MtgCards* mtgCards;
  MtgaLogParser* mtgaLogParser;

public:
  TestMtgaLogParser()
  {
    mtgCards = new MtgCards(this);
    mtgaLogParser = new MtgaLogParser(this, mtgCards);
  }

private slots:
  void testParsePlayerInventory()
  {
    qRegisterMetaType<PlayerInventory>();
    QString log;
    READ_LOG("PlayerInventory.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerInventory);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    PlayerInventory playerInventory = args.first().value<PlayerInventory>();
    QCOMPARE(playerInventory.getWcMythic(), 2);
  }

  void testParsePlayerInventoryUpdate()
  {
    QString log;
    READ_LOG("PlayerInventoryUpdate.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerInventoryUpdate);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<int> newCards = args.first().value<QList<int>>();
    QCOMPARE(newCards.first(), 65963);
  }

  void testParsePlayerCollection()
  {
    qRegisterMetaType<QMap<int, int>>();
    QString log;
    READ_LOG("PlayerCollection.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerCollection);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QMap<int, int> playerCollection = args.first().value<QMap<int, int>>();
    QCOMPARE(playerCollection.size(), 1001);
    QCOMPARE(playerCollection[66781], 3);
  }

  void testParsePlayerDecks()
  {
    qRegisterMetaType<QList<Deck>>();
    QString log;
    READ_LOG("PlayerDecks.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerDecks);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<Deck> playerDecks = args.first().value<QList<Deck>>();
    QCOMPARE(playerDecks.size(), 12);
    Card* vraskasContemptCard = mtgCards->findCard(66223);
    QCOMPARE(playerDecks.first().currentCards()[vraskasContemptCard], 2);
  }

  void testParseMatchCreated()
  {
    qRegisterMetaType<OpponentInfo>();
    QString log;
    READ_LOG("MatchCreated.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnMatchCreated);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    OpponentInfo match = args[1].value<OpponentInfo>();
    QCOMPARE(match.opponentRankClass(), "Bronze");
    QCOMPARE(match.opponentRankTier(), 4);
  }

  void testParseMatchInfoSeats()
  {
    qRegisterMetaType<QList<MatchPlayer>>();
    QString log;
    READ_LOG("MatchInfoSeats.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnMatchInfoSeats);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<MatchPlayer> matchPlayers = args.first().value<QList<MatchPlayer>>();
    MatchPlayer matchPlayer = matchPlayers[0];
    QCOMPARE(matchPlayer.name(), "Edipo2s");
    QCOMPARE(matchPlayer.seatId(), 1);
  }

  void testParseMatchInfoMatchResult()
  {
    QString log;
    READ_LOG("MatchInfoResult.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnMatchResult);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    int matchWinningTeamId = args.first().toInt();
    QCOMPARE(matchWinningTeamId, 2);
  }

  void testParsePlayerRankInfo()
  {
    qRegisterMetaType<QPair<QString, int>>();
    QString log;
    READ_LOG("PlayerRankInfo.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerRankInfo);
    mtgaLogParser->parse(log);

    // Rank
    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QPair<QString, int> playerRankInfo = args.first().value<QPair<QString, int>>();
    QCOMPARE(playerRankInfo.first, "Beginner");
    QCOMPARE(playerRankInfo.second, 1);
  }

  void testParsePlayerRankUpdated()
  {
    qRegisterMetaType<QPair<QString, int>>();
    QString log;
    READ_LOG("PlayerRankUpdated.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerRankUpdated);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QPair<QString, int> playerRankInfo = args.first().value<QPair<QString, int>>();
    QCOMPARE(playerRankInfo.first, "Bronze");
    QCOMPARE(playerRankInfo.second, 4);
  }

  void testParsePlayerDeckCreate()
  {
    qRegisterMetaType<Deck>();
    QString log;
    READ_LOG("PlayerDeckCreate.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerDeckCreated);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    Deck playerDeckCreated = args.first().value<Deck>();
    Card* theScarabGod = mtgCards->findCard(65769);
    QCOMPARE(playerDeckCreated.currentCards()[theScarabGod], 3);
  }

  void testParsePlayerDeckUpdate()
  {
    qRegisterMetaType<Deck>();
    QString log;
    READ_LOG("PlayerDeckUpdate.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerDeckUpdated);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    Deck playerDeckUpdated = args.first().value<Deck>();
    Card* theScarabGod = mtgCards->findCard(65769);
    QCOMPARE(playerDeckUpdated.currentCards()[theScarabGod], 3);
  }

  void testParsePlayerDeckSubmit()
  {
    qRegisterMetaType<Deck>();
    QString log;
    READ_LOG("PlayerDeckSubmit.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerDeckSubmited);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    Deck playerDeckSubmited = args.first().value<Deck>();
    Card* duress = mtgCards->findCard(66175);
    QCOMPARE(playerDeckSubmited.currentCards()[duress], 2);
  }

  void testParseEventPlayerCourse()
  {
    qRegisterMetaType<Deck>();
    QString log;
    READ_LOG("EventGetPlayerCourse.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnEventPlayerCourse);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QString eventID = args.first().value<QString>();
    Deck playerCurrentDeck = args[1].value<Deck>();
    Card* theScarabGod = mtgCards->findCard(65769);
    QCOMPARE(eventID, "Quick_constructed_april_26");
    QCOMPARE(playerCurrentDeck.currentCards()[theScarabGod], 2);
  }

  void testParsePlayerTakesMulligan()
  {
    QString log;
    READ_LOG("PlayerTakesMulligan.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnPlayerTakesMulligan);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
  }

  void testParseGameStart()
  {
    qRegisterMetaType<QList<MatchZone>>();
    qRegisterMetaType<MatchMode>();
    QString log;
    READ_LOG("GameStateFull.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnGameStart);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    QList<MatchZone> zones = args[1].value<QList<MatchZone>>();
    QCOMPARE(zones.size(), 10);
    for (MatchZone zone : zones)
    {
      if (zone.type() == ZoneType_LIBRARY)
      {
        QCOMPARE(zone.objectIds.size(), 60);
      }
    }
  }

  void testParseMatchStateDiffObjectIds()
  {
    qRegisterMetaType<MatchStateDiff>();
    QString log;
    READ_LOG("GameStateDiffObjectIds.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnMatchStateDiff);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    MatchStateDiff matchStateDiff = args.first().value<MatchStateDiff>();
    QCOMPARE(matchStateDiff.zones().size(), 4);
    MatchZone zonePlayerHand;
    for (MatchZone zone : matchStateDiff.zones())
    {
      if (zone.id() == 31)
      {
        zonePlayerHand = zone;
        break;
      }
    }
    QCOMPARE(zonePlayerHand.objectIds[103], 65889);
    QCOMPARE(zonePlayerHand.objectIds[104], 65575);
    QCOMPARE(zonePlayerHand.objectIds[105], 65769);
    QCOMPARE(zonePlayerHand.objectIds[106], 66707);
    QCOMPARE(zonePlayerHand.objectIds[107], 64913);
    QCOMPARE(zonePlayerHand.objectIds[108], 65889);
    QCOMPARE(zonePlayerHand.objectIds[109], 66223);
  }

  void testParseMatchStateDiffZoneTransfer()
  {
    qRegisterMetaType<MatchStateDiff>();
    QString log;
    READ_LOG("GameStateDiffZoneTransfer.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnMatchStateDiff);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    MatchStateDiff matchStateDiff = args.first().value<MatchStateDiff>();
    QCOMPARE(matchStateDiff.idsChanged()[287], 343);
    MatchZoneTransfer matchZoneTransfer = matchStateDiff.idsZoneChanged()[343];
    QCOMPARE(matchZoneTransfer.zoneSrcId(), 35);
    QCOMPARE(matchZoneTransfer.zoneDstId(), 28);
  }

  void testParseMatchStateDiffNewTurn()
  {
    QString log;
    READ_LOG("GameStateDiffNewTurn.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnNewTurnStarted);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    int turnNumber = args.first().toInt();
    QCOMPARE(turnNumber, 2);
  }

  void testParseOpponentTakesMulligan()
  {
    QString log;
    READ_LOG("OpponentTakesMulligan.txt", log);
    QSignalSpy spy(mtgaLogParser, &MtgaLogParser::sgnOpponentTakesMulligan);
    mtgaLogParser->parse(log);

    QCOMPARE(spy.count(), 1);
    QList<QVariant> args = spy.takeFirst();
    int opponentSeatId = args.first().toInt();
    QCOMPARE(opponentSeatId, 2);
  }
};
