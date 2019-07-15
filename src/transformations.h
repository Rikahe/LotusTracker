#ifndef EXTENSIONS_H
#define EXTENSIONS_H

#include <QImage>
#include <QJsonArray>
#include <QJsonDocument>
#include <QString>

class Transformations final
{
public:
  static QString colorIdentityListToString(const QList<QChar>& distinctManaSymbols);
  static QJsonArray stringToJsonArray(const QString& json);
  static QJsonObject stringToJsonObject(const QString& json);
  static QImage applyRoundedCorners2Image(const QImage& image, int cornerRadius);
  static QImage toGrayscale(const QImage& image);
};

#endif  // EXTENSIONS_H
