#ifndef APPSECURE_H
#define APPSECURE_H

#include <QObject>
#include <qt5keychain/keychain.h>

using namespace QKeychain;

class AppSecure : public QObject
{
  Q_OBJECT
public:
  explicit AppSecure(QObject* parent = nullptr);

  void store(const QString& key, const QString& value);
  QString restore(const QString& key);
  void remove(const QString& key);

private:
  WritePasswordJob wpj;
  ReadPasswordJob rpj;
  DeletePasswordJob dpj;

signals:

public slots:
};

#endif  // APPSECURE_H
