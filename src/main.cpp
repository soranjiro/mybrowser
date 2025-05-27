#include "mainwindow.h"
#include <QApplication>
#include <QDebug>
#include <QEvent>
#include <QMouseEvent>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

// Global event filter to debug mouse events
class GlobalEventFilter : public QObject {
public:
  bool eventFilter(QObject *obj, QEvent *event) override {
#ifdef DEBUG_MODE
    if (event->type() == QEvent::MouseButtonPress ||
        event->type() == QEvent::MouseButtonRelease ||
        event->type() == QEvent::MouseButtonDblClick) {

      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      qDebug() << "GlobalEventFilter - Event type:" << event->type()
               << "Button:" << mouseEvent->button()
               << "Position:" << mouseEvent->pos()
               << "Global:" << mouseEvent->globalPosition()
               << "Object:" << obj->objectName()
               << "Class:" << obj->metaObject()->className();
    }
#endif
    return false; // Don't filter out the event
  }
};

int main(int argc, char *argv[]) {
  // Enable developer tools remote debugging before QApplication creation
  qputenv("QTWEBENGINE_REMOTE_DEBUGGING", "9222");

  // Enable Picture-in-Picture API and related features with enhanced flags
  qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
          "--enable-features=PictureInPictureAPI,MediaSession,MediaSessionService,OverlayScrollbar "
          "--enable-picture-in-picture-api "
          "--enable-blink-features=PictureInPictureAPI "
          "--force-device-scale-factor=1 "
          "--autoplay-policy=no-user-gesture-required "
          "--disable-features=VizDisplayCompositor "
          "--disable-web-security "
          "--allow-running-insecure-content "
          "--disable-extensions-except "
          "--disable-extensions "
          "--no-sandbox");

  // macOS specific settings for better trackpad/mouse handling
#ifdef Q_OS_MACOS
  qputenv("QT_MAC_WANTS_LAYER", "1");
  qputenv("QT_ENABLE_HIGHDPI_SCALING", "1");
  qputenv("QT_AUTO_SCREEN_SCALE_FACTOR", "1");
  qputenv("QTWEBENGINE_DISABLE_SANDBOX", "1"); // Improve compatibility
#endif

  QApplication a(argc, argv);

  // Improve mouse/trackpad responsiveness on macOS
  a.setAttribute(Qt::AA_SynthesizeMouseForUnhandledTouchEvents, false); // Disable touch synthesis
  a.setAttribute(Qt::AA_SynthesizeTouchForUnhandledMouseEvents, false);

#ifdef Q_OS_MACOS
  // macOS specific mouse handling
  a.setAttribute(Qt::AA_DontShowIconsInMenus, false);
  a.setAttribute(Qt::AA_NativeWindows, false);
#endif

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

  // Enable autoplay for media and Picture-in-Picture
  globalSettings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
  globalSettings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
  globalSettings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);

  // Enable focus on navigation
  globalSettings->setAttribute(QWebEngineSettings::FocusOnNavigationEnabled, true);

  // Enable DNS prefetching for better performance
  globalSettings->setAttribute(QWebEngineSettings::DnsPrefetchEnabled, true);

  // Enable touch icons
  globalSettings->setAttribute(QWebEngineSettings::TouchIconsEnabled, true);

  // Enable error page for better debugging
  globalSettings->setAttribute(QWebEngineSettings::ErrorPageEnabled, true);

  // macOS specific WebEngine settings for better mouse handling
#ifdef Q_OS_MACOS
  globalSettings->setAttribute(QWebEngineSettings::ScrollAnimatorEnabled, false);
  globalSettings->setAttribute(QWebEngineSettings::SpatialNavigationEnabled, false);
#endif

  // Install global event filter to debug mouse events
  GlobalEventFilter *globalFilter = new GlobalEventFilter();
  a.installEventFilter(globalFilter);
#ifdef DEBUG_MODE
  qDebug() << "DEBUG_MODE is enabled";
  qDebug() << "Global event filter installed";
#else
  qDebug() << "RELEASE_MODE is enabled";
#endif

  MainWindow w;
  w.show();

  int result = a.exec();

  // Clean shutdown - ensure all web engine processes are properly terminated
  QWebEngineProfile::defaultProfile()->clearAllVisitedLinks();
  QWebEngineProfile::defaultProfile()->clearHttpCache();

  return result;
}
