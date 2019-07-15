#ifndef LOGGER_H
#define LOGGER_H

#include <QFile>
#include <QObject>

typedef enum
{
  DEBUG,
  INFO,
  WARNING
} LogType;

const char LOG_TYPE_NAMES[][128] = { "DEBUG", "INFO", "ERROR" };

class Logger : public QObject
{
  Q_OBJECT

private:
  QFile* logFile;
  void log(LogType type, const QString& source, const int line, const QString& msg);

public:
  Logger(QObject* parent);
  ~Logger();
  void logI(const QString& source, const int line, const QString& msg);
  void logD(const QString& source, const int line, const QString& msg);
  void logW(const QString& source, const int line, const QString& msg);

signals:
  void sgnLog(LogType type, const QString& log);

public slots:
};

#endif  // LOGGER_H
