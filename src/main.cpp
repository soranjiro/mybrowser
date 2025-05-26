#include "mainwindow.h"
#include <QApplication>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings> // Add this line

int main(int argc, char *argv[]) {
  QApplication a(argc, argv);
  QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PluginsEnabled, true);
  QWebEngineProfile::defaultProfile()->settings()->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);
  MainWindow w;
  w.show();
  return a.exec();
}
