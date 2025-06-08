#include "pip_standalone.h"
#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>

int main(int argc, char *argv[]) {
  QApplication app(argc, argv);
  app.setApplicationName("MyBrowser PiP");
  app.setApplicationVersion("1.0");

  // macOSでDockに表示されないようにする
#ifdef Q_OS_MACOS
  app.setAttribute(Qt::AA_DontShowIconsInMenus);
#endif

  QCommandLineParser parser;
  parser.setApplicationDescription("MyBrowser独立PiPウィンドウ");
  parser.addHelpOption();
  parser.addVersionOption();

  parser.process(app);

  qDebug() << "PiP独立プロセス開始";

  PiPStandaloneWindow window;
  window.show();

  return app.exec();
}
