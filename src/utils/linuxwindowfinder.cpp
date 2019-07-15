#include "linuxwindowfinder.h"

#include <QApplication>
#include <QProcess>

LinuxWindowFinder::LinuxWindowFinder()
{
}

int LinuxWindowFinder::findWindowId(const QString& name)
{
  QProcess process;
  process.setProgram("xdotool");
  process.setArguments(QStringList() << "search" << name);
  process.start();
  while (process.state() != QProcess::NotRunning)
    qApp->processEvents();
  return process.readAll().toInt();
}

bool LinuxWindowFinder::isWindowFocused(int wId)
{
  QProcess process;
  process.setProgram("xdotool");
  process.setArguments(QStringList() << "getwindowfocus");
  process.start();
  while (process.state() != QProcess::NotRunning)
    qApp->processEvents();
  return process.readAll().toInt() == wId;
}
