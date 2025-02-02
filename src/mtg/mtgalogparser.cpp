#include "mtgalogparser.h"
#include "../lotustracker.h"
#include "../transformations.h"
#include "../macros.h"

#include <QList>
#include <QRegularExpression>
#include <utility>

#define REGEXP_RAW_MSG "\\s(==>|to\\sMatch|<==|Incoming|Match\\sto).+(\\s|\\n)(\\{|\\[)(\\n\\s+.*)+\\n+(\\}|\\])\\n"
#define REGEXP_MSG_RESPONSE_NUMBER "((?<=\\s)\\d+(?=\\:\\s)|(?<=\\()\\d+(?=\\)))"
#define REGEXP_MSG_ID "[\\w\\.]+(?=(\\(|((\\s|\\n)(\\{|\\[))))"
#define REGEXP_MSG_JSON "(\\{|\\[)(\\n\\s+.*)+\\n*(\\}||\\])"

MtgaLogParser::MtgaLogParser(QObject* parent, MtgCards* mtgCards) : QObject(parent), mtgCards(mtgCards)
{
  reRawMsg = QRegularExpression(REGEXP_RAW_MSG);
  reMsgNumber = QRegularExpression(REGEXP_MSG_RESPONSE_NUMBER);
  reMsgId = QRegularExpression(REGEXP_MSG_ID);
  reMsgJson = QRegularExpression(REGEXP_MSG_JSON);
}

MtgaLogParser::~MtgaLogParser()
= default;

Deck MtgaLogParser::jsonObject2Deck(QJsonObject jsonDeck)
{
  QString id = jsonDeck["id"].toString();
  QString name = jsonDeck["name"].toString();
  QJsonArray jsonCards = jsonDeck["mainDeck"].toArray();
  QMap<Card*, int> cards;
  int cardId = 0;
  for (QJsonValueRef jsonCardRef : jsonCards)
  {
    int value = jsonCardRef.toInt();
    if (cardId == 0)
    {
      cardId = value;
    }
    else
    {
      Card* card = mtgCards->findCard(cardId);
      int qtd = value;
      if (card && qtd > 0)
      {
        cards[card] = qtd;
      }
      cardId = 0;
    }
  }
  QJsonArray jsonSideboard = jsonDeck["sideboard"].toArray();
  QMap<Card*, int> sideboard;
  for (QJsonValueRef jsonCardRef : jsonSideboard)
  {
    int value = jsonCardRef.toInt();
    if (cardId == 0)
    {
      cardId = value;
    }
    else
    {
      Card* card = mtgCards->findCard(cardId);
      int qtd = value;
      if (card && qtd > 0)
      {
        sideboard[card] = qtd;
      }
      cardId = 0;
    }
  }
  return Deck(id, name, cards, sideboard);
}

void MtgaLogParser::parse(const QString& logNewContent)
{
  // Extract raw msgs
  QRegularExpressionMatchIterator iterator = reRawMsg.globalMatch(logNewContent + "\n");
  QList<QString> rawMsgs;
  while (iterator.hasNext())
  {
    rawMsgs << iterator.next().captured(0);
  }
  // Extract msg id and json
  // List of msgs in format (msgId, msgJson)
  QList<QPair<QString, QString>> incomingMsgs;
  QList<QPair<QString, QString>> outcomingMsgs;
  for (const QString& msg : rawMsgs)
  {
    QRegularExpressionMatch numberMatch = reMsgNumber.match(msg);
    if (numberMatch.hasMatch())
    {
      int msgNumber = numberMatch.captured(0).toInt();
      // Avoid process duplicated responses
      if (msg.contains("<=="))
      {
        if (msgResponseNumbers.contains(msgNumber))
        {
          LOGD("Duplicated msg")
          continue;
        }
        else
        {
          msgResponseNumbers << msgNumber;
          // Store only last 4 msg resposne number
          if (msgResponseNumbers.size() >= 5)
          {
            msgResponseNumbers.removeFirst();
          }
        }
      }
    }
    QString msgId = "";
    QRegularExpressionMatch idMatch = reMsgId.match(msg);
    if (idMatch.hasMatch())
    {
      msgId = idMatch.captured(0);
      if (msgId.contains("("))
      {
        msgId = msgId.left(msgId.indexOf("("));
      }
    }
    QString msgJson = "";
    QRegularExpressionMatch jsonMatch = reMsgJson.match(msg);
    if (jsonMatch.hasMatch())
    {
      msgJson = jsonMatch.captured(0);
      if (msgJson.at(0) == '[' && msgJson.right(1) != ']')
      {
        msgJson += "]";
      }
    }
    if (msg.contains("==>") || msg.contains("to Match"))
    {
      outcomingMsgs << QPair<QString, QString>(msgId, msgJson);
    }
    else
    {
      incomingMsgs << QPair<QString, QString>(msgId, msgJson);
    }
  }
  // Log msgs
  for (const QPair<QString, QString>& msg : outcomingMsgs)
  {
    parseOutcomingMsg(msg);
  }
  for (const QPair<QString, QString>& msg : incomingMsgs)
  {
    parseIncomingMsg(msg);
  }
}

void MtgaLogParser::parseOutcomingMsg(QPair<QString, QString> msg)
{
  if (msg.first == "ClientToMatchServiceMessageType_ClientToGREMessage")
  {
    parseClientToGreMessages(msg.second);
  }
  else if (msg.first == "DirectGame.Challenge")
  {
    parseDirectGameChallenge(msg.second);
  }
  else if (msg.first == "Log.Info")
  {
    parseLogInfo(msg.second);
  }
  else if (msg.first == "Draft.MakePick")
  {
    parseDraftPick(msg.second);
  }
}

void MtgaLogParser::parseIncomingMsg(QPair<QString, QString> msg)
{
#ifdef QT_DEBUG
  if (msg.first != "GreToClientEvent")
  {
    LOGD(msg.first);
  }
#endif
  if (msg.first == "PlayerInventory.GetPlayerInventory")
  {
    parsePlayerInventory(msg.second);
  }
  else if (msg.first == "Inventory.Updated")
  {
    parsePlayerInventoryUpdate(msg.second);
  }
  else if (msg.first == "PlayerInventory.GetPlayerCardsV3")
  {
    parsePlayerCollection(msg.second);
  }
  else if (msg.first == "Deck.GetDeckListsV3")
  {
    parsePlayerDecks(msg.second);
  }
  else if (msg.first == "Deck.CreateDeckV3")
  {
    parsePlayerDeckCreate(msg.second);
  }
  else if (msg.first == "Deck.UpdateDeckV3")
  {
    parsePlayerDeckUpdate(msg.second);
  }
  else if (msg.first == "Event.GetPlayerCourseV2")
  {
    parseEventPlayerCourse(msg.second);
  }
  else if (msg.first == "Event.GetPlayerCourses")
  {
    parseEventPlayerCourses(msg.second);
  }
  else if (msg.first == "Event.MatchCreated")
  {
    parseMatchCreated(msg.second);
  }
  else if (msg.first == "MatchGameRoomStateChangedEvent")
  {
    parseMatchInfo(msg.second);
  }
  else if (msg.first == "Event.GetCombinedRankInfo")
  {
    parsePlayerRankInfo(msg.second);
  }
  else if (msg.first == "Rank.Updated")
  {
    parsePlayerRankUpdated(msg.second);
  }
  else if (msg.first == "Event.DeckSubmitV3")
  {
    parsePlayerDeckSubmited(msg.second);
  }
  else if (msg.first == "GreToClientEvent")
  {
    parseGreToClientMessages(msg.second);
  }
  else if (msg.first == "Event.ClaimPrize")
  {
    parseEventFinish(msg.second);
  }
  else if (msg.first == "Draft.DraftStatus" || msg.first == "Draft.MakePick")
  {
    parseDraftStatus(msg.second);
  }
}

void MtgaLogParser::parsePlayerInventory(QString json)
{
  QJsonObject jsonPlayerIventory = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerIventory.empty())
  {
    return;
  }
  int gold = jsonPlayerIventory["gold"].toInt();
  int gems = jsonPlayerIventory["gems"].toInt();
  int wcCommon = jsonPlayerIventory["wcCommon"].toInt();
  int wcUncommon = jsonPlayerIventory["wcUncommon"].toInt();
  int wcRare = jsonPlayerIventory["wcRare"].toInt();
  int wcMythic = jsonPlayerIventory["wcMythic"].toInt();
  double vaultProgress = jsonPlayerIventory["vaultProgress"].toDouble();
  PlayerInventory playerInventory(gold, gems, wcCommon, wcUncommon, wcRare, wcMythic, vaultProgress);
  LOGD(QString("PlayerInventory: %1 wcC, %2 wcI, %3 wcR, %4 wcM")
           .arg(wcCommon)
           .arg(wcUncommon)
           .arg(wcRare)
           .arg(wcMythic));
  emit sgnPlayerInventory(playerInventory);
}

void MtgaLogParser::parsePlayerInventoryUpdate(QString json)
{
  QJsonObject jsonPlayerIventoryUpdate = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerIventoryUpdate.empty())
  {
    return;
  }
  QJsonObject delta = jsonPlayerIventoryUpdate["delta"].toObject();
  QJsonArray jsonCards = delta["cardsAdded"].toArray();

  QList<int> newCards;
  for (QJsonValueRef jsonCardRef : jsonCards)
  {
    int newCardId = jsonCardRef.toInt();
    newCards << newCardId;
  }
  LOGD(QString("PlayerInventoryUpdate: %1 new cards").arg(newCards.size()));
  emit sgnPlayerInventoryUpdate(newCards);
}

void MtgaLogParser::parsePlayerCollection(QString json)
{
  QJsonObject jsonPlayerCollection = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerCollection.empty())
  {
    return;
  }
  QMap<int, int> ownedCards;
  for (const QString& ownedCardId : jsonPlayerCollection.keys())
  {
    int ownedCardQtd = jsonPlayerCollection[ownedCardId].toInt();
    ownedCards[ownedCardId.toInt()] = ownedCardQtd;
  }
  LOGD(QString("PlayerCollection: %1 unique cards").arg(ownedCards.size()));
  emit sgnPlayerCollection(ownedCards);
}

void MtgaLogParser::parsePlayerDecks(QString json)
{
  QJsonArray jsonPlayerDecks = Transformations::stringToJsonArray(std::move(json));
  if (jsonPlayerDecks.empty())
  {
    return;
  }
  QList<Deck> playerDecks;
  for (QJsonValueRef jsonDeckRef : jsonPlayerDecks)
  {
    QJsonObject jsonDeck = jsonDeckRef.toObject();
    playerDecks << jsonObject2Deck(jsonDeck);
  }
  LOGD(QString("PlayerDecks: %1 decks").arg(playerDecks.size()));
  emit sgnPlayerDecks(playerDecks);
}

void MtgaLogParser::parseEventPlayerCourse(QString json)
{
  QJsonObject jsonEventPlayerCourse = Transformations::stringToJsonObject(std::move(json));
  if (jsonEventPlayerCourse.empty())
  {
    return;
  }
  QString eventId = jsonEventPlayerCourse["InternalEventName"].toString();
  if (!jsonEventPlayerCourse["CourseDeck"].isNull())
  {
    QString currentModule = jsonEventPlayerCourse["CurrentModule"].toString();
    QJsonObject jsonEventPlayerCourseDeck = jsonEventPlayerCourse["CourseDeck"].toObject();
    Deck deck = jsonObject2Deck(jsonEventPlayerCourseDeck);
    bool isFinished = currentModule == "ClaimPrize";
    LOGD(QString("EventPlayerCourse: %1 with %2. Finished: %3").arg(eventId).arg(deck.name).arg(isFinished));
    emit sgnEventPlayerCourse(eventId, deck, isFinished);
  }
}

void MtgaLogParser::parseEventPlayerCourses(QString json)
{
  QJsonArray jsonEventCourses = Transformations::stringToJsonArray(std::move(json));
  if (jsonEventCourses.empty())
  {
    return;
  }
  QList<QString> events;
  for (QJsonValueRef jsonEventRef : jsonEventCourses)
  {
    QJsonObject event = jsonEventRef.toObject();
    events << event["InternalEventName"].toString();
  }
  LOGD(QString("EventPlayerCourses: %1 events").arg(events.size()));
  emit sgnEventPlayerCourses(events);
}

void MtgaLogParser::parseMatchCreated(QString json)
{
  QJsonObject jsonMatchCreated = Transformations::stringToJsonObject(std::move(json));
  if (jsonMatchCreated.empty())
  {
    return;
  }
  QString opponentName = jsonMatchCreated["opponentScreenName"].toString();
  QString opponentRankClass = jsonMatchCreated["opponentRankingClass"].toString();
  int opponentRankTier = jsonMatchCreated["opponentRankingTier"].toInt();
  QString eventId = jsonMatchCreated["eventId"].toString();
  OpponentInfo opponentInfo(opponentName, opponentRankClass, opponentRankTier);
  LOGD(QString("MatchCreated: Opponent %1, rank: %2(%3)")
           .arg(opponentName)
           .arg(opponentRankClass)
           .arg(opponentRankTier));
  emit sgnMatchCreated(eventId, opponentInfo);
}

void MtgaLogParser::parseMatchInfo(QString json)
{
  QJsonObject jsonMatchInfo = Transformations::stringToJsonObject(std::move(json));
  if (jsonMatchInfo.empty())
  {
    return;
  }
  QJsonObject jsonMatchState = jsonMatchInfo["matchGameRoomStateChangedEvent"].toObject();
  QJsonObject jsonRoomInfo = jsonMatchState["gameRoomInfo"].toObject();
  QString roomState = jsonRoomInfo["stateType"].toString();
  if (roomState == "MatchGameRoomStateType_Playing")
  {
    QJsonObject jsonRoomConfig = jsonRoomInfo["gameRoomConfig"].toObject();
    QJsonArray jsonPlayers = jsonRoomConfig["reservedPlayers"].toArray();
    QList<MatchPlayer> matchPlayers;
    for (QJsonValueRef jsonPlayerRef : jsonPlayers)
    {
      QJsonObject jsonPlayer = jsonPlayerRef.toObject();
      QString playerName = jsonPlayer["playerName"].toString();
      int playerSeat = jsonPlayer["systemSeatId"].toInt();
      int playerTeamId = jsonPlayer["teamId"].toInt();
      matchPlayers << MatchPlayer(playerName, playerSeat, playerTeamId);
    }
    LOGD("MatchInfoSeats");
    emit sgnMatchInfoSeats(matchPlayers);
  }
  if (roomState == "MatchGameRoomStateType_MatchCompleted")
  {
    QJsonObject jsonFinalMatchResult = jsonRoomInfo["finalMatchResult"].toObject();
    QJsonArray jsonResults = jsonFinalMatchResult["resultList"].toArray();
    int matchWinningTeamId = -1;
    for (QJsonValueRef jsonPlayerRef : jsonResults)
    {
      QJsonObject jsonResult = jsonPlayerRef.toObject();
      QString resultScope = jsonResult["scope"].toString();
      int winningTeamId = jsonResult["winningTeamId"].toInt();
      if (resultScope == "MatchScope_Match")
      {
        matchWinningTeamId = winningTeamId;
      }
    }
    LOGD("MatchInfoResult");
    emit sgnMatchResult(matchWinningTeamId);
  }
}

void MtgaLogParser::parsePlayerRankInfo(QString json)
{
  QJsonObject jsonPlayerRankInfo = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerRankInfo.empty())
  {
    return;
  }
  QString rankClass = jsonPlayerRankInfo["constructedClass"].toString();
  int rankTier = jsonPlayerRankInfo["constructedLevel"].toInt();
  LOGD(QString("PlayerRankInfo: %1 - %2").arg(rankClass).arg(rankTier));
  emit sgnPlayerRankInfo(qMakePair(rankClass, rankTier));
}

void MtgaLogParser::parsePlayerRankUpdated(QString json)
{
  QJsonObject jsonPlayerRankUpdate = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerRankUpdate.empty())
  {
    return;
  }
  int oldRankTier = jsonPlayerRankUpdate["oldTier"].toInt();
  int newRankTier = jsonPlayerRankUpdate["newTier"].toInt();
  if (newRankTier != oldRankTier)
  {
    QString rankClass = jsonPlayerRankUpdate["newClass"].toString();
    LOGD(QString("RankUpdate: new rank %1").arg(rankClass));
    emit sgnPlayerRankUpdated(qMakePair(rankClass, newRankTier));
  }
}

void MtgaLogParser::parsePlayerDeckCreate(QString json)
{
  QJsonObject jsonPlayerCreateDeck = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerCreateDeck.empty())
  {
    return;
  }
  Deck deck = jsonObject2Deck(jsonPlayerCreateDeck);
  LOGD(QString("PlayerCreateDeck: %1").arg(deck.name));
  emit sgnPlayerDeckCreated(deck);
}

void MtgaLogParser::parsePlayerDeckUpdate(QString json)
{
  QJsonObject jsonPlayerUpdateDeck = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerUpdateDeck.empty())
  {
    return;
  }
  Deck deck = jsonObject2Deck(jsonPlayerUpdateDeck);
  LOGD(QString("PlayerUpdateDeck: %1").arg(deck.name));
  emit sgnPlayerDeckUpdated(deck);
}

void MtgaLogParser::parsePlayerDeckSubmited(QString json)
{
  QJsonObject jsonPlayerDeckSubmited = Transformations::stringToJsonObject(std::move(json));
  if (jsonPlayerDeckSubmited.empty())
  {
    return;
  }
  QString eventId = jsonPlayerDeckSubmited["InternalEventName"].toString();
  QJsonObject jsonDeck = jsonPlayerDeckSubmited["CourseDeck"].toObject();
  Deck deckSubmited = jsonObject2Deck(jsonDeck);
  LOGD(QString("Deck submited: %1").arg(deckSubmited.name));
  emit sgnPlayerDeckSubmited(eventId, deckSubmited);
}

void MtgaLogParser::parseDirectGameChallenge(QString json)
{
  QJsonObject jsonMessage = Transformations::stringToJsonObject(std::move(json));
  QJsonObject jsonParams = jsonMessage["params"].toObject();
  QString jsonDeckString = jsonParams["deck"]
                               .toString()
                               .replace("\\", "")
                               .replace("\"Id\"", "\"id\"")
                               .replace("\"Quantity\"", "\"quantity\"");
  LOGD(jsonDeckString);
  QJsonObject jsonDeck = Transformations::stringToJsonObject(jsonDeckString);
  Deck deckSubmited = jsonObject2Deck(jsonDeck);
  LOGD(QString("Deck submited: %1").arg(deckSubmited.name));
  emit sgnPlayerDeckSubmited("DirectGame", deckSubmited);
}

void MtgaLogParser::parseGreToClientMessages(QString json)
{
  QJsonObject jsonGreToClientMsg = Transformations::stringToJsonObject(std::move(json));
  if (jsonGreToClientMsg.empty())
  {
    return;
  }
  QJsonObject jsonGreToClientEvent = jsonGreToClientMsg["greToClientEvent"].toObject();
  QJsonArray jsonGreToClientMessages = jsonGreToClientEvent["greToClientMessages"].toArray();
  for (QJsonValueRef jsonMessageRef : jsonGreToClientMessages)
  {
    QJsonObject jsonMessage = jsonMessageRef.toObject();
    QString messageType = jsonMessage["type"].toString();
    if (messageType == "GREMessageType_GameStateMessage" || messageType == "GREMessageType_QueuedGameStateMessage")
    {
      int gameStateId = jsonMessage["gameStateId"].toInt();
      QJsonObject jsonGameStateMessage = jsonMessage["gameStateMessage"].toObject();
      QString gameStateType = jsonGameStateMessage["type"].toString();
      if (gameStateType == "GameStateType_Full")
      {
        parseGameStateFull(jsonGameStateMessage);
      }
      else if (gameStateType == "GameStateType_Diff")
      {
        QJsonArray systemSeatIds = jsonMessage["systemSeatIds"].toArray();
        int playerSeatId = systemSeatIds.size() > 0 ? systemSeatIds.first().toInt() : 0;
        parseGameStateDiff(playerSeatId, gameStateId, jsonGameStateMessage);
      }
    }
  }
}

void MtgaLogParser::parseGameStateFull(QJsonObject jsonMessage)
{
  QJsonObject gameInfo = jsonMessage["gameInfo"].toObject();
  QString winCondition = gameInfo["matchWinCondition"].toString();
  MatchMode mode = MatchMode_UNKNOWN;
  if (winCondition == "MatchWinCondition_SingleElimination")
  {
    mode = MatchMode_SINGLE;
  }
  if (winCondition == "MatchWinCondition_Best2of3")
  {
    mode = MatchMode_BEST_OF_3;
  }
  QList<MatchZone> zones = getMatchZones(jsonMessage);
  int seatId = jsonMessage["turnInfo"].toObject()["decisionPlayer"].toInt();
  LOGD(QString("GameStart Mode: %1, Zones: %2, DecisionPlayer: %3")
           .arg(MatchInfo::MatchModeToString(mode))
           .arg(zones.size())
           .arg(seatId));
  emit sgnGameStart(mode, zones, seatId);
}

void MtgaLogParser::parseGameStateDiff(int playerSeatId, int gameStateId, QJsonObject jsonMessage)
{
  QJsonObject jsonInfoObject = jsonMessage["gameInfo"].toObject();
  if (jsonInfoObject["matchState"].toString() == "MatchState_GameComplete")
  {
    QJsonArray jsonResults = jsonInfoObject["results"].toArray();
    emit sgnGameCompleted(getGameResults(jsonResults));
    return;
  }
  QList<MatchZone> zones = getMatchZones(jsonMessage);
  QJsonArray jsonGameObjects = jsonMessage["gameObjects"].toArray();
  for (QJsonValueRef jsonGameObjectRef : jsonGameObjects)
  {
    QJsonObject jsonGameObject = jsonGameObjectRef.toObject();
    int instanceId = jsonGameObject["instanceId"].toInt();
    int grpId = jsonGameObject["grpId"].toInt();
    int zoneId = jsonGameObject["zoneId"].toInt();
    for (MatchZone& zone : zones)
    {
      if (zone.id() == zoneId)
      {
        zone.objectIds[instanceId] = grpId;
        break;
      }
    }
  }
  int turnNumber = 0;
  bool hasShuffleAnnotation = false;
  QJsonArray jsonGSMAnnotations = jsonMessage["annotations"].toArray();
  for (QJsonValueRef jsonAnnotationRef : jsonGSMAnnotations)
  {
    QJsonObject jsonAnnotation = jsonAnnotationRef.toObject();
    QString type = jsonAnnotation["type"].toArray().first().toString();
    if (type == "AnnotationType_NewTurnStarted")
    {
      QJsonObject jsonTurnInfo = jsonMessage["turnInfo"].toObject();
      if (jsonTurnInfo.contains("turnNumber"))
      {
        turnNumber = jsonTurnInfo["turnNumber"].toInt();
        LOGD(QString("NewTurn: %1").arg(turnNumber));
        emit sgnNewTurnStarted(turnNumber);
      }
    }
    if (type == "AnnotationType_Shuffle")
    {
      hasShuffleAnnotation = true;
    }
  }
  QJsonArray jsonDiffDeletedInstanceIds = jsonMessage["diffDeletedInstanceIds"].toArray();
  if (jsonDiffDeletedInstanceIds.size() >= 40 && !hasShuffleAnnotation)
  {
    QList<int> diffDeletedInstanceIds;
    for (QJsonValueRef jsonDiffDeletedInstanceIdsRef : jsonDiffDeletedInstanceIds)
    {
      diffDeletedInstanceIds << jsonDiffDeletedInstanceIdsRef.toInt();
    }
    checkMulligans(playerSeatId, diffDeletedInstanceIds, zones);
  }
  QMap<int, int> idsChanged = getIdsChanged(jsonGSMAnnotations);
  QMap<int, MatchZoneTransfer> idsZoneChanged = getIdsZoneChanged(jsonGSMAnnotations);
  MatchStateDiff matchStateDiff(gameStateId, zones, idsChanged, idsZoneChanged);
  emit sgnMatchStateDiff(matchStateDiff);
  return;
}

QMap<int, int> MtgaLogParser::getGameResults(QJsonArray jsonResults)
{
  QMap<int, int> teamIdWins;
  for (QJsonValueRef jsonPlayerRef : jsonResults)
  {
    QJsonObject jsonResult = jsonPlayerRef.toObject();
    QString resultScope = jsonResult["scope"].toString();
    int winningTeamId = jsonResult["winningTeamId"].toInt();
    if (resultScope == "MatchScope_Game")
    {
      if (teamIdWins.keys().contains(winningTeamId))
      {
        teamIdWins[winningTeamId] += 1;
      }
      else
      {
        teamIdWins[winningTeamId] = 1;
      }
    }
  }
  return teamIdWins;
}

QList<MatchZone> MtgaLogParser::getMatchZones(QJsonObject jsonGameStateMessage)
{
  QJsonArray jsonZones = jsonGameStateMessage["zones"].toArray();
  QList<MatchZone> zones;
  for (QJsonValueRef jsonZoneRef : jsonZones)
  {
    QJsonObject jsonZone = jsonZoneRef.toObject();
    QString zoneTypeName = jsonZone["type"].toString();
    ZoneType zoneType = MatchZone::zoneTypeFromName(zoneTypeName);
    if (zoneType != ZoneType_UNKNOWN)
    {
      int id = jsonZone["zoneId"].toInt();
      int ownerSeatId = jsonZone["ownerSeatId"].toInt();
      QMap<int, int> objectIds;
      QJsonArray jsonObjectInstanceIds = jsonZone["objectInstanceIds"].toArray();
      for (QJsonValueRef jsonObjectIdRef : jsonObjectInstanceIds)
      {
        int objectId = jsonObjectIdRef.toInt();
        objectIds[objectId] = 0;
      }
      zones << MatchZone(id, ownerSeatId, zoneType, objectIds);
    }
  }
  return zones;
}

void MtgaLogParser::checkMulligans(int playerSeatId, const QList<int>& diffDeletedInstanceIds, QList<MatchZone> zones)
{
  for (MatchZone zone : zones)
  {
    if (zone.type() == ZoneType_LIBRARY)
    {
      QList<int> zoneIds = zone.objectIds.keys();
      if (listContainsSublist(diffDeletedInstanceIds, zoneIds))
      {
        int seatId = zone.ownerSeatId();
        if (seatId == playerSeatId)
        {
          QMap<int, int> newHandDrawed;
          for (MatchZone zone2 : zones)
          {
            if (zone2.type() == ZoneType_HAND && zone2.ownerSeatId() == seatId)
            {
              newHandDrawed = zone2.objectIds;
              break;
            }
          }
          LOGD("Player mulligan");
          emit sgnPlayerTakesMulligan(newHandDrawed);
        }
        else
        {
          LOGD("OpponentTakesMulligan");
          emit sgnOpponentTakesMulligan(seatId);
        }
      }
    }
  }
}

bool MtgaLogParser::listContainsSublist(const QList<int>& list, QList<int> subList)
{
  for (int item : subList)
  {
    if (!list.contains(item))
    {
      return false;
    }
  }
  return true;
}

QMap<int, int> MtgaLogParser::getIdsChanged(QJsonArray jsonGSMAnnotations)
{
  QMap<int, int> idsChanged;
  for (QJsonValueRef jsonAnnotationRef : jsonGSMAnnotations)
  {
    QJsonObject jsonAnnotation = jsonAnnotationRef.toObject();
    QString type = jsonAnnotation["type"].toArray().first().toString();
    if (type == "AnnotationType_ObjectIdChanged")
    {
      QJsonArray jsonDetails = jsonAnnotation["details"].toArray();
      int orgId, newId = 0;
      for (QJsonValueRef jsonDetailsRef : jsonDetails)
      {
        QJsonObject details = jsonDetailsRef.toObject();
        QString key = details["key"].toString();
        if (key == "orig_id")
        {
          orgId = details["valueInt32"].toArray().first().toInt();
        }
        else if (key == "new_id")
        {
          newId = details["valueInt32"].toArray().first().toInt();
        }
      }
      idsChanged[orgId] = newId;
    }
  }
  return idsChanged;
}

QMap<int, MatchZoneTransfer> MtgaLogParser::getIdsZoneChanged(QJsonArray jsonGSMAnnotations)
{
  QMap<int, MatchZoneTransfer> idsZoneChanged;
  for (QJsonValueRef jsonAnnotationRef : jsonGSMAnnotations)
  {
    QJsonObject jsonAnnotation = jsonAnnotationRef.toObject();
    QString type = jsonAnnotation["type"].toArray().first().toString();
    if (type == "AnnotationType_ZoneTransfer")
    {
      int transferId = jsonAnnotation["affectedIds"].toArray().first().toInt();
      QJsonArray jsonDetails = jsonAnnotation["details"].toArray();
      int srcZone = 0;
      int dstZone = 0;
      ZoneTransferCategory transferCategory = ZoneTransfer_UNKOWN;
      for (QJsonValueRef jsonDetailsRef : jsonDetails)
      {
        QJsonObject details = jsonDetailsRef.toObject();
        QString key = details["key"].toString();
        if (key == "zone_src")
        {
          srcZone = details["valueInt32"].toArray().first().toInt();
        }
        else if (key == "zone_dest")
        {
          dstZone = details["valueInt32"].toArray().first().toInt();
        }
        else if (key == "category")
        {
          QString category = details["valueString"].toArray().first().toString();
          transferCategory = category == "Resolve" ?
                                 ZoneTransfer_RESOLVED :
                                 category == "Countered" ? ZoneTransfer_COUNTERED : ZoneTransfer_UNKOWN;
        }
      }
      idsZoneChanged[transferId] = MatchZoneTransfer(srcZone, dstZone, transferCategory);
    }
  }
  return idsZoneChanged;
}

void MtgaLogParser::parseClientToGreMessages(QString json)
{
  QJsonObject jsonClientToGreMsg = Transformations::stringToJsonObject(std::move(json));
  if (jsonClientToGreMsg.empty())
  {
    return;
  }
  QString type = jsonClientToGreMsg["clientToMatchServiceMessageType"].toString();
  QString payload = jsonClientToGreMsg["payload"].toString();
  if (type == "ClientToMatchServiceMessageType_ClientToGREMessage" && payload.size() >= 200)
  {
    emit sgnDecodeDeckPosSideboardPayload(type, payload);
  }
}

void MtgaLogParser::onParseDeckPosSideboardJson(QJsonObject jsonMessage)
{
  if (!jsonMessage.contains("submitdeckresp"))
  {
    return;
  }
  QJsonObject jsonDeckResp = jsonMessage["submitdeckresp"].toObject();
  QJsonObject jsonDeck = jsonDeckResp["deck"].toObject();
  QJsonArray jsonMainDeckCards = jsonDeck["deckcardsList"].toArray();
  QMap<Card*, int> mainDeck;
  for (QJsonValueRef jsonCardRef : jsonMainDeckCards)
  {
    int cardId = jsonCardRef.toInt();
    Card* card = mtgCards->findCard(cardId);
    if (mainDeck.keys().contains(card))
    {
      mainDeck[card] += 1;
    }
    else
    {
      mainDeck[card] = 1;
    }
  }
  QJsonArray jsonSideboardCards = jsonDeck["sideboardcardsList"].toArray();
  QMap<Card*, int> sideboard;
  for (QJsonValueRef jsonCardRef : jsonSideboardCards)
  {
    int cardId = jsonCardRef.toInt();
    Card* card = mtgCards->findCard(cardId);
    if (sideboard.keys().contains(card))
    {
      sideboard[card] += 1;
    }
    else
    {
      sideboard[card] = 1;
    }
  }
  emit sgnPlayerDeckWithSideboardSubmited(mainDeck, sideboard);
}

void MtgaLogParser::parseEventFinish(QString json)
{
  QJsonObject jsonClainPrize = Transformations::stringToJsonObject(std::move(json));
  if (jsonClainPrize.empty())
  {
    return;
  }
  QString eventState = jsonClainPrize["CurrentModule"].toString();
  if (eventState != "Complete")
  {
    return;
  }
  QString eventId = jsonClainPrize["InternalEventName"].toString();
  QJsonObject jsonCourseDeck = jsonClainPrize["CourseDeck"].toObject();
  Deck deck = jsonObject2Deck(jsonCourseDeck);
  QJsonObject jsonWinLossGate = jsonClainPrize["ModuleInstanceData"].toObject()["WinLossGate"].toObject();
  int maxWins = jsonWinLossGate["MaxWins"].toInt();
  int wins = jsonWinLossGate["CurrentWins"].toInt();
  int losses = jsonWinLossGate["CurrentLosses"].toInt();
  emit sgnEventFinish(eventId, deck.id, deck.colorIdentity(), maxWins, wins, losses);
}

void MtgaLogParser::parseDraftPick(QString json)
{
  QJsonObject jsonLogInfo = Transformations::stringToJsonObject(std::move(json));
  if (jsonLogInfo.empty())
  {
    return;
  }
  QJsonObject params = jsonLogInfo["params"].toObject();
  int cardId = params["cardId"].toString().toInt();
  int packNumber = params["packNumber"].toString().toInt();
  int pickNumber = params["pickNumber"].toString().toInt();
  emit sgnDraftPick(cardId, packNumber, pickNumber);
}

void MtgaLogParser::parseDraftStatus(QString json)
{
  QJsonObject jsonDraftStatus = Transformations::stringToJsonObject(std::move(json));
  if (jsonDraftStatus.empty())
  {
    return;
  }
  QString eventId = jsonDraftStatus["eventName"].toString();
  QString status = jsonDraftStatus["draftStatus"].toString();
  int packNumber = jsonDraftStatus["packNumber"].toInt();
  int pickNumber = jsonDraftStatus["pickNumber"].toInt();
  QList<Card*> availablePicks;
  QJsonArray jsonAvailablePicks = jsonDraftStatus["draftPack"].toArray();
  for (QJsonValueRef jsonAvailablePickRef : jsonAvailablePicks)
  {
    int mtgaId = jsonAvailablePickRef.toString().toInt();
    availablePicks << LOTUS_TRACKER->mtgCards->findCard(mtgaId);
  }
  QList<Card*> pickedCards;
  QJsonArray jsonPickedCards = jsonDraftStatus["pickedCards"].toArray();
  for (QJsonValueRef jsonPickedCardRef : jsonPickedCards)
  {
    int mtgaId = jsonPickedCardRef.toString().toInt();
    pickedCards << LOTUS_TRACKER->mtgCards->findCard(mtgaId);
  }
  emit sgnDraftStatus(eventId, status, packNumber, pickNumber, availablePicks, pickedCards);
}

void MtgaLogParser::parseLogInfo(QString json)
{
  QJsonObject jsonLogInfo = Transformations::stringToJsonObject(std::move(json));
  if (jsonLogInfo.empty())
  {
    return;
  }
  try
  {
    QJsonObject params = jsonLogInfo["params"].toObject();
    QString messageName = params["messageName"].toString();
    if (messageName == "Client.SceneChange")
    {
      QJsonObject payloadObject = params["payloadObject"].toObject();
      QString fromSceneName = payloadObject["fromSceneName"].toString();
      QString toSceneName = payloadObject["toSceneName"].toString();
      if (fromSceneName == "Draft")
      {
        emit sgnLeavingDraft();
      }
      if (toSceneName == "Home")
      {
        emit sgnGoingToHome();
      }
    }
  }
  catch (const std::exception& ex)
  {
    LOGW(ex.what())
  }
}
