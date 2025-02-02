#include "untappedapi.h"
#include "../macros.h"
#include "../urls.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QUrl>

#define API_BASE_URL "https://api.mtga.untapped.gg/api/v1"
#define CLIENT_ID "7bIfoHez8t1tKPVurVpVbT6QfA9muN8PVgcctp02"
#define HEADER_AUTHORIZATION "Authorization"

UntappedAPI::UntappedAPI(QObject*  /*parent*/)
{
}

UntappedAPI::~UntappedAPI()
= default;

void UntappedAPI::fetchAnonymousUploadToken()
{
  QJsonObject jsonObj;
  jsonObj.insert("client_id", QJsonValue(CLIENT_ID));
  QByteArray body = QJsonDocument(jsonObj).toJson();

  QUrl url(QString("%1/users").arg(API_BASE_URL));
  if (LOG_REQUEST_ENABLED)
  {
    LOGD(QString("Request: %1").arg(url.toString()));
  }
  QNetworkRequest request(url);
  request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
  QNetworkReply* reply = networkManager.post(request, body);
  connect(reply, &QNetworkReply::finished, this, &UntappedAPI::anonymousUploadTokenOnFinish);
}

void UntappedAPI::anonymousUploadTokenOnFinish()
{
  auto* reply = static_cast<QNetworkReply*>(sender());
  QString rsp = reply->readAll();
  if (LOG_REQUEST_ENABLED)
  {
    LOGD(rsp);
  }

  int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
  if (statusCode < 200 || statusCode > 299)
  {
    QString reason = reply->attribute(QNetworkRequest::HttpReasonPhraseAttribute).toString();
    LOGW(QString("Error: %1 - %2").arg(reply->errorString()).arg(reason));
    return;
  }

  emit sgnNewAnonymousUploadToken(rsp.replace("\"", ""));
}
