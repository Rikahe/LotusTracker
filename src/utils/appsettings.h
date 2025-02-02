#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include "appsecure.h"
#include "../entity/card.h"
#include "../entity/user.h"

#include <QSettings>
#include <QString>

class AppSettings : public QObject
{
  Q_OBJECT

private:
  AppSecure* appSecure;
  QSettings settings;
  void migrateUserSettings();

public:
  explicit AppSettings(QObject* parent = nullptr);

  QString getInstallationUuid();
  bool isAutoStartEnabled();
  void enableAutoStart(bool enabled);
  bool isAutoUpdateEnabled();
  void enableAutoUpdate(bool enabled);
  bool isFirstRun();
  bool isFirstMatch();
  bool isFirstDraft();
  bool isHideOnLoseGameFocusEnabled();
  void enableHideOnLoseGameFocus(bool enabled);
  int getDeckTrackerAlpha();
  void setDeckTrackerAlpha(int alpha);
  QString getCardLayout();
  void setCardLayout(const QString& cardLayout);
  QString getLogPath();
  void setLogPath(const QString& logPath);
  int getUnhiddenDelay();
  void setUnhiddenDelay(int unhiddenTimeout);
  bool isShowCardManaCostEnabled();
  void enableShowCardManaCost(bool enabled);
  bool isShowCardOnHoverEnabled();
  void enableShowCardOnHover(bool enabled);
  bool isShowOnlyRemainingCardsEnabled();
  void enableShowOnlyRemainingCards(bool enabled);
  bool isShowDebugLogsEnabled();
  void enableShowDebugLogs(bool enabled);
  // Deck overlay player
  bool isDeckOverlayPlayerEnabled();
  void enableDeckOverlayPlayer(bool enabled);
  bool isDeckOverlayPlayerStatisticsEnabled();
  void enableDeckOverlayPlayerStatistics(bool enabled);
  QPoint getDeckOverlayPlayerPos(int uiWidth);
  void setDeckOverlayPlayerPos(QPoint pos);
  int getDeckOverlayPlayerScale();
  void setDeckOverlayPlayerScale(int scale);
  // Deck overlay opponent
  bool isDeckOverlayOpponentEnabled();
  void enableDeckOverlayOpponent(bool enabled);
  QPoint getDeckOverlayOpponentPos(int uiWidth, int cardHoverWidth);
  void setDeckOverlayOpponentPos(QPoint pos);
  int getDeckOverlayOpponentScale();
  void setDeckOverlayOpponentScale(int scale);
  // Deck overlay Draft
  bool isDeckOverlayDraftEnabled();
  void enableDeckOverlayDraft(bool enabled);
  QPoint getDeckOverlayDraftPos(int uiWidth);
  void setDeckOverlayDraftPos(QPoint pos);
  int getDeckOverlayDraftScale();
  void setDeckOverlayDraftScale(int scale);
  bool isShowDeckAfterDraftEnabled();
  void enableShowDeckAfterDraft(bool enabled);
  bool hasDraftPick(const QString& eventId);
  void clearDraftPick(const QString& eventId);
  QString getDraftPicks(QString eventId, int packNumber, int pickNumber);
  QString getDraftPicked(QString eventId, int packNumber, int pickNumber);
  void setDraftPick(QString eventId, int packNumber, int pickNumber, int pickedCard, QList<Card*> availablePicks);
  QString getDraftPickBaseKey(const QString& eventId, int packNumber, int pickNumber);
  // User settings
  void setUserSettings(UserSettings userSettings, const QString& userName = "");
  UserSettings getUserSettings();
  void clearUserSettings();
  void restoreDefaults();
  // Untapped
  void setUntappedAnonymousUploadToken(QString uploadToken);
  QString getUntappedAnonymousUploadToken();
  void clearUntappedAnonymousUploadToken();
};

#endif  // APPSETTINGS_H
