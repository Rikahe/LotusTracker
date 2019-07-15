#ifndef DATABASE_H
#define DATABASE_H

#include "requestdata.h"
#include "rqtuploadplayermatch.h"
#include "../entity/deck.h"
#include "../entity/user.h"
#include "../mtg/mtgamatch.h"

#include <QBuffer>
#include <QObject>
#include <QtNetwork/QNetworkAccessManager>
#include <QMap>

class LotusTrackerAPI : public QObject
{
  Q_OBJECT
private:
  typedef void (LotusTrackerAPI::*LotusTrackerAPIMethod)();

  QString userName;
  bool isRefreshTokenInProgress;
  RqtRegisterPlayerMatch rqtRegisterPlayerMatch;
  QNetworkAccessManager networkManager;
  QDateTime lastUpdatePlayerCollectionDate, lastUpdatePlayerInventoryDate;
  // Params for recall after refresh token
  QMap<QString, LotusTrackerAPIMethod> requestToRecallOnFinish;
  QMap<QString, QPair<QString, RequestData>> requestsToRecall;  // url, <method, request>
  Deck paramDeck;

  UserSettings createUserSettingsFromSign(QJsonObject jsonRsp);
  UserSettings createUserSettingsFromRefreshedToken(QJsonObject jsonRsp);
  qlonglong getExpiresEpoch(const QString& expiresIn);
  void getPlayerDeckToUpdate(const QString& deckID);
  void getPlayerDeckToUpdateRequestOnFinish();
  void getMatchInfoRequestOnFinish();
  Deck jsonToDeck(QJsonObject deckJson);
  void registerPlayerMatch(QString matchID);
  void uploadMatchRequestOnFinish();
  QNetworkRequest prepareGetRequest(RequestData requestData, bool checkUserAuth, LotusTrackerAPIMethod onRequestFinish);
  QNetworkRequest prepareRequest(RequestData requestData, bool checkUserAuth, const QString& method = "");
  QBuffer* prepareBody(RequestData requestData);
  void sendGet(RequestData requestData, LotusTrackerAPIMethod onRequestFinish);
  void sendPatch(const RequestData& requestData);
  void sendPost(const RequestData& requestData, LotusTrackerAPIMethod onRequestFinish = nullptr);
  void requestOnFinish();

public:
  explicit LotusTrackerAPI(QObject* parent = nullptr);
  ~LotusTrackerAPI();
  // Auth
  void signInUser(const QString& email, const QString& password);
  void registerUser(const QString& email, const QString& password);
  void recoverPassword(const QString& email);
  void refreshToken(const QString& refreshToken);
  //
  void updatePlayerCollection(QMap<int, int> ownedCards);
  void setPlayerUserName(QString userName);
  void updatePlayerInventory(PlayerInventory playerInventory);
  void createPlayerDeck(Deck deck);
  void updatePlayerDeck(Deck deck);
  void publishOrUpdatePlayerDeck(QString playerName, Deck deck);
  void getMatchInfo(const QString& eventId, const QString& deckId);
  void uploadMatch(const MatchInfo& matchInfo, const Deck& playerDeck, QString playerRankClass);
  void uploadEventResult(QString eventId, QString deckId, QString deckColors, int maxWins, int wins, int losses);

signals:
  void sgnUserLogged(bool fromSignUp);
  void sgnRequestFinished();
  void sgnRequestFinishedWithSuccess();
  void sgnRequestFinishedWithError();
  void sgnPasswordRecovered();
  void sgnTokenRefreshed();
  void sgnTokenRefreshError();
  void sgnDeckWinRate(int wins, int losses, double winRate);
  void sgnEventInfo(QString eventName, QString eventType);
  void sgnPlayerCollection(QMap<int, int> ownedCards);
  void sgnParseDeckPosSideboardJson(QJsonObject json);

public slots:
  void onRequestPlayerCollection();
  void onDecodeDeckPosSideboardPayload(QString type, QString payload);

private slots:
  void authRequestOnFinish();
  void recoverPasswordRequestOnFinish();
  void tokenRefreshRequestOnFinish();
  void onTokenRefreshed();
  void getPlayerCollectionRequestOnFinish();
  void onParseDeckPosSideboardPayloadRequestFinish();
};

#endif  // DATABASE_H
