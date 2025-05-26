#include "webview.h"
#include "mainwindow.h" // To potentially access MainWindow for new tab creation logic
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDebug>
#include <QKeyEvent>
#include <QMenu>
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

WebView::WebView(QWidget *parent) : QWebEngineView(parent), devToolsView(nullptr) {
  // Use custom page to capture JavaScript console messages
  CustomWebEnginePage *customPage = new CustomWebEnginePage(QWebEngineProfile::defaultProfile(), this);
  setPage(customPage);

  // Improve mouse/click responsiveness
  setAttribute(Qt::WA_AcceptTouchEvents, true);
  setFocusPolicy(Qt::StrongFocus);

  // Enable mouse tracking for better responsiveness
  setMouseTracking(true);

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
  setContextMenuPolicy(Qt::CustomContextMenu);
  connect(this, &WebView::customContextMenuRequested, this,
          [this](const QPoint &pos) {
            QContextMenuEvent event(QContextMenuEvent::Mouse, pos, mapToGlobal(pos));
            contextMenuEvent(&event);
          });
}

void WebView::setPage(QWebEnginePage *page) {
  // Close existing developer tools if open
  if (devToolsView) {
    devToolsView->close();
    devToolsView = nullptr;
  }

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
  // Handle gesture events
  if (event->type() == QEvent::Gesture) {
    return gestureEvent(static_cast<QGestureEvent *>(event));
  }

  // Force focus on mouse click for better responsiveness
  if (event->type() == QEvent::MouseButtonPress) {
    setFocus(Qt::MouseFocusReason);
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

void WebView::showDevTools() {
  // Show developer tools for the current page
  if (!page()) {
    qDebug() << "No page available for developer tools";
    return;
  }

  // If developer tools are already open, bring to front
  if (devToolsView) {
    devToolsView->raise();
    devToolsView->activateWindow();
    return;
  }

  // Create developer tools page and view
  QWebEnginePage *devToolsPage = new QWebEnginePage(QWebEngineProfile::defaultProfile(), this);
  devToolsView = new QWebEngineView();
  devToolsView->setPage(devToolsPage);

  // Set the dev tools page for the current page
  page()->setDevToolsPage(devToolsPage);

  // Configure the developer tools window
  devToolsView->setWindowTitle("開発者ツール - " + page()->title());
  devToolsView->setWindowIcon(this->window()->windowIcon());
  devToolsView->resize(1200, 800);
  devToolsView->setAttribute(Qt::WA_DeleteOnClose);

  // Clean up when developer tools window is closed
  connect(devToolsView, &QObject::destroyed, this, [this]() {
    devToolsView = nullptr;
    if (page()) {
      page()->setDevToolsPage(nullptr);
    }
  }, Qt::QueuedConnection);

  // Update title when page title changes
  connect(page(), &QWebEnginePage::titleChanged, this, [this](const QString &title) {
    if (devToolsView) {
      devToolsView->setWindowTitle("開発者ツール - " + title);
    }
  }, Qt::QueuedConnection);

  devToolsView->show();
  qDebug() << "Developer tools opened";
}

void WebView::keyPressEvent(QKeyEvent *event) {
  // Handle F12 or Ctrl+Shift+I to open developer tools
  if (event->key() == Qt::Key_F12 ||
      (event->modifiers() == (Qt::ControlModifier | Qt::ShiftModifier) && event->key() == Qt::Key_I)) {
    showDevTools();
    return;
  }

  QWebEngineView::keyPressEvent(event);
}

void WebView::contextMenuEvent(QContextMenuEvent *event) {
  QMenu *menu = new QMenu(this);

  // Add basic navigation actions
  QAction *backAction = menu->addAction("戻る");
  backAction->setEnabled(page() && page()->history()->canGoBack());
  connect(backAction, &QAction::triggered, [this]() { page()->history()->back(); });

  QAction *forwardAction = menu->addAction("進む");
  forwardAction->setEnabled(page() && page()->history()->canGoForward());
  connect(forwardAction, &QAction::triggered, [this]() { page()->history()->forward(); });

  QAction *reloadAction = menu->addAction("再読み込み");
  connect(reloadAction, &QAction::triggered, [this]() { page()->triggerAction(QWebEnginePage::Reload); });

  menu->addSeparator();

  // Add developer tools action
  QAction *inspectAction = menu->addAction("要素を検証");
  connect(inspectAction, &QAction::triggered, this, &WebView::showDevTools);

  menu->popup(event->globalPos());
}
