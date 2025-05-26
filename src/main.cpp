#include "mainwindow.h"
#include <QApplication>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings> // Add this line

int main(int argc, char *argv[]) {
  // Enable developer tools remote debugging before QApplication creation
  qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");

  // macOS specific settings for better trackpad/mouse handling
#ifdef Q_OS_MACOS
  qputenv("QT_MAC_WANTS_LAYER", "1");
  qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
#endif

  QApplication a(argc, argv);

  // Improve mouse/trackpad responsiveness on macOS
  a.setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, true);
  a.setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

  // Configure WebEngine settings for full JavaScript support
  QWebEngineProfile *defaultProfile = QWebEngineProfile::defaultProfile();
  QWebEngineSettings *globalSettings = defaultProfile->settings();

  // Set a modern user agent to ensure compatibility with modern websites
  QString userAgent = "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/120.0.0.0 Safari/537.36";
  defaultProfile->setHttpUserAgent(userAgent);

  // Enable JavaScript and related features
  globalSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  globalSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
  globalSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);

  // Enable plugins and media
  globalSettings->setAttribute(QWebEngineSettings::PluginsEnabled, true);
  globalSettings->setAttribute(QWebEngineSettings::PdfViewerEnabled, true);

  // Enable local storage and other web features
  globalSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
  globalSettings->setAttribute(QWebEngineSettings::LocalContentCanAccessRemoteUrls, true);
  globalSettings->setAttribute(QWebEngineSettings::LocalContentCanAccessFileUrls, true);

  // Enable HTML5 features
  globalSettings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  globalSettings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);

  // Enable autoplay for media
  globalSettings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

  // Enable focus on navigation
  globalSettings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);

  // Enable DNS prefetching for better performance
  globalSettings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);

  // Enable touch icons
  globalSettings->setAttribute(QWebEngineSettings::TouchIconsEnabled, true);

  // Enable error page for better debugging
  globalSettings->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);

  MainWindow w;
  w.show();
  return a.exec();
}
