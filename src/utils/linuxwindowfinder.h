#pragma once

#include <QObject>

class LinuxWindowFinder : public QObject
{
  Q_OBJECT
public:
  LinuxWindowFinder();

  static int findWindowId(const QString& title);
  static bool isWindowFocused(int wId);
};
