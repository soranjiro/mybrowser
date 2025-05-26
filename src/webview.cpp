#include "webview.h"
#include "mainwindow.h" // To potentially access MainWindow for new tab creation logic
#include <QApplication>
#include <QDebug>
#include <QWebEngineSettings>

// Custom page implementation
CustomWebEnginePage::CustomWebEnginePage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent) {
}

void CustomWebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) {
  QString levelString;
  switch (level) {
  case InfoMessageLevel:
    levelString = "INFO";
    break;
  case WarningMessageLevel:
    levelString = "WARNING";
    break;
  case ErrorMessageLevel:
    levelString = "ERROR";
    break;
  }
  qDebug() << QString("JS Console [%1]: %2 (line %3 in %4)").arg(levelString, message).arg(lineNumber).arg(sourceID);
}

WebView::WebView(QWidget *parent) : QWebEngineView(parent) {
  // Use custom page to capture JavaScript console messages
  CustomWebEnginePage *customPage = new CustomWebEnginePage(QWebEngineProfile::defaultProfile(), this);
  setPage(customPage);

  // Ensure JavaScript is enabled for this page
  QWebEngineSettings *pageSettings = customPage->settings();
  pageSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
  pageSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
  pageSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

  // Forward signals that MainWindow might be interested in
  connect(page(), &QWebEnginePage::titleChanged, this, &WebView::titleChanged);
  connect(page(), &QWebEnginePage::urlChanged, this, &WebView::urlChanged);
  connect(page(), &QWebEnginePage::loadProgress, this, &WebView::loadProgress);
  connect(page(), &QWebEnginePage::loadFinished, this, &WebView::loadFinished);
  connect(page(), &QWebEnginePage::loadStarted, this, &WebView::loadStarted);

  // Enable gesture recognition for swipe gestures
  grabGesture(Qt::SwipeGesture);

  // Enable right-click context menu with "Inspect Element" option
  setContextMenuPolicy(Qt::DefaultContextMenu);
}

void WebView::setPage(QWebEnginePage *page) {
  QWebEngineView::setPage(page);

  // Ensure JavaScript is enabled for any new page
  if (page) {
    QWebEngineSettings *pageSettings = page->settings();
    pageSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
    pageSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
    pageSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
    pageSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
    pageSettings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
    pageSettings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
    pageSettings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
  }
}

// This function is called when a link requests to be opened in a new window
// (e.g., target="_blank" or JavaScript window.open())
QWebEngineView *WebView::createWindow(QWebEnginePage::WebWindowType type) {
  // Find the MainWindow instance to call its newTab method
  MainWindow *mainWindow = nullptr;
  QWidget *parent = this->parentWidget();
  while (parent) {
    mainWindow = qobject_cast<MainWindow *>(parent);
    if (mainWindow)
      break;
    parent = parent->parentWidget();
  }

  if (mainWindow) {
    // Create a new tab in the main window
    mainWindow->newTab(); // This creates a new WebView and sets it as current
    WebView *newView = mainWindow->currentWebView();

    // For pop-ups, we might want to handle them differently,
    // but for now, just open them in a new tab.
    // if (type == QWebEnginePage::WebBrowserWindow || type == QWebEnginePage::WebDialog) {
    // }
    return newView;
  }
  return nullptr; // Should not happen if embedded in MainWindow
}

bool WebView::event(QEvent *event) {
  if (event->type() == QEvent::Gesture) {
    return gestureEvent(static_cast<QGestureEvent *>(event));
  }
  return QWebEngineView::event(event);
}

bool WebView::gestureEvent(QGestureEvent *event) {
  if (QGesture *swipe = event->gesture(Qt::SwipeGesture)) {
    swipeTriggered(static_cast<QSwipeGesture *>(swipe));
    return true;
  }
  return false;
}

void WebView::swipeTriggered(QSwipeGesture *gesture) {
  if (gesture->state() == Qt::GestureFinished) {
    if (gesture->horizontalDirection() == QSwipeGesture::Left) {
      // Left swipe - go forward in history
      if (page()->history()->canGoForward()) {
        page()->history()->forward();
      }
    } else if (gesture->horizontalDirection() == QSwipeGesture::Right) {
      // Right swipe - go back in history
      if (page()->history()->canGoBack()) {
        page()->history()->back();
      }
    }
  }
}
