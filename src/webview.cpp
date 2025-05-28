#include "webview.h"
#include "mainwindow.h" // To potentially access MainWindow for new tab creation logic
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFile>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
#include <QTextStream>
#include <QTimer>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

// Custom page implementation
CustomWebEnginePage::CustomWebEnginePage(QWebEngineProfile *profile, QObject *parent)
    : QWebEnginePage(profile, parent) {
}

void CustomWebEnginePage::javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) {
#ifdef DEBUG_MODE
  // In debug mode, log all console messages
  QString levelStr;
  switch (level) {
  case JavaScriptConsoleMessageLevel::InfoMessageLevel:
    levelStr = "INFO";
    break;
  case JavaScriptConsoleMessageLevel::WarningMessageLevel:
    levelStr = "WARN";
    break;
  case JavaScriptConsoleMessageLevel::ErrorMessageLevel:
    levelStr = "ERROR";
    break;
  }
  qDebug() << "JS Console [" << levelStr << "]:" << message << "(line" << lineNumber << "in" << sourceID << ")";
#else
  // In release mode, only log errors
  if (level == JavaScriptConsoleMessageLevel::ErrorMessageLevel) {
    qDebug() << "JS Error:" << message;
  }
  Q_UNUSED(lineNumber)
  Q_UNUSED(sourceID)
#endif
}

void CustomWebEnginePage::javaScriptAlert(const QUrl &securityOrigin, const QString &msg) {
#ifdef DEBUG_MODE
  qDebug() << "JavaScript Alert:" << msg << "from" << securityOrigin.toString();
#endif

  // Create a properly configured message box
  QMessageBox msgBox;
  msgBox.setIcon(QMessageBox::Information);
  msgBox.setText(msg);
  msgBox.setWindowTitle("JavaScript Alert");
  msgBox.setStandardButtons(QMessageBox::Ok);
  msgBox.setDefaultButton(QMessageBox::Ok);

  // Ensure proper focus and modality
  msgBox.setWindowModality(Qt::ApplicationModal);
  msgBox.setAttribute(Qt::WA_ShowWithoutActivating, false);
  msgBox.activateWindow();
  msgBox.raise();

  msgBox.exec();
}

bool CustomWebEnginePage::acceptNavigationRequest(const QUrl &url, NavigationType type, bool isMainFrame) {
  QString typeString;
  switch (type) {
  case NavigationTypeLinkClicked:
    typeString = "LinkClicked";
    break;
  case NavigationTypeTyped:
    typeString = "Typed";
    break;
  case NavigationTypeFormSubmitted:
    typeString = "FormSubmitted";
    break;
  case NavigationTypeBackForward:
    typeString = "BackForward";
    break;
  case NavigationTypeReload:
    typeString = "Reload";
    break;
  case NavigationTypeRedirect:
    typeString = "Redirect";
    break;
  default:
    typeString = "Other";
  }
#ifdef DEBUG_MODE
  qDebug() << "Navigation Request:" << typeString << "URL:" << url.toString() << "MainFrame:" << isMainFrame;
#endif

  // For anchor links (fragment navigation), force the navigation
  if (url.hasFragment() && url.path() == this->url().path()) {
#ifdef DEBUG_MODE
    qDebug() << "Fragment navigation detected, allowing:" << url.fragment();
#endif
    return true;
  }

  // Allow all navigation for now
  return QWebEnginePage::acceptNavigationRequest(url, type, isMainFrame);
}

WebView::WebView(QWidget *parent) : QWebEngineView(parent), devToolsView(nullptr) {
  // Use custom page to capture JavaScript console messages
  CustomWebEnginePage *customPage = new CustomWebEnginePage(QWebEngineProfile::defaultProfile(), this);
  setPage(customPage);

  // Improve mouse/click responsiveness for macOS
  setAttribute(Qt::WA_AcceptTouchEvents, true);
  setAttribute(Qt::WA_MouseTracking, true);
  setAttribute(Qt::WA_Hover, true);
  setFocusPolicy(Qt::StrongFocus);

  // Enable mouse tracking for better responsiveness
  setMouseTracking(true);

  // macOS specific mouse handling improvements
#ifdef Q_OS_MACOS
  // Disable touch events completely to avoid conflicts with mouse events on macOS
  setAttribute(Qt::WA_AcceptTouchEvents, false);
  setAttribute(Qt::WA_MouseNoMask, true);
  setAttribute(Qt::WA_Hover, true);
  setMouseTracking(true);
#ifdef DEBUG_MODE
  qDebug() << "macOS specific mouse handling enabled for WebView";
#endif
#endif

  // Install event filter to catch mouse events at a lower level
  installEventFilter(this);
  // Also install event filter on the page
  customPage->installEventFilter(this);
#ifdef DEBUG_MODE
  qDebug() << "Event filter installed for WebView and page";
#endif

  // Ensure JavaScript is enabled for this page
  QWebEngineSettings *pageSettings = customPage->settings();
  pageSettings->setAttribute(QWebEngineSettings::JavascriptEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::JavascriptCanOpenWindows, true);
  pageSettings->setAttribute(QWebEngineSettings::JavascriptCanAccessClipboard, true);
  pageSettings->setAttribute(QWebEngineSettings::LocalStorageEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::WebGLEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::Accelerated2dCanvasEnabled, true);
  pageSettings->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

  // Configure profile for media features
  QWebEngineProfile *profile = customPage->profile();
  if (profile) {
    // Enable media features that support Picture-in-Picture
    profile->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);

    // Add user agent for enhanced media support
    static bool profileConfigured = false;
    if (!profileConfigured) {
      profile->setHttpUserAgent(profile->httpUserAgent() + " PictureInPictureSupported");
      profileConfigured = true;
    }
  }

  // Forward signals that MainWindow might be interested in
  connect(page(), &QWebEnginePage::titleChanged, this, &WebView::titleChanged);
  connect(page(), &QWebEnginePage::urlChanged, this, &WebView::urlChanged);
  connect(page(), &QWebEnginePage::loadProgress, this, &WebView::loadProgress);
  connect(page(), &QWebEnginePage::loadFinished, this, &WebView::loadFinished);
  // Inject JavaScript to debug click events on page load
  connect(page(), &QWebEnginePage::loadFinished, this, [this](bool success) {
    if (success) {
      // Load and inject CSS
      QString cssContent = loadResourceFile(":/resources/css/pip.css");
      QString cssInjection;
      if (!cssContent.isEmpty()) {
        cssInjection = QString(R"(
          if (!document.querySelector('style[data-webview-styles]')) {
            const style = document.createElement('style');
            style.setAttribute('data-webview-styles', 'true');
            style.textContent = `%1`;
            document.head.appendChild(style);
          }
        )")
                           .arg(cssContent);
      }

      // Load and inject WebView enhancement JavaScript
      QString jsContent = loadResourceFile(":/resources/js/webview-enhancement.js");

      // Set debug mode based on compile-time flag
      QString debugMode =
#ifdef DEBUG_MODE
          "true"
#else
        "false"
#endif
          ;

      QString enhancementScript = cssInjection + jsContent + QString(R"(
        // Initialize WebView enhancement with debug mode
        if (window.webViewEnhancer) {
          window.webViewEnhancer.debugMode = %1;
          window.webViewEnhancer.init();
        } else {
          window.webViewEnhancer = new WebViewEnhancer(%1);
        }

        // Enable Picture-in-Picture for all videos
        if (window.webViewEnhancer) {
          window.webViewEnhancer.removeDisablePiPAttributes();
        }
      )")
                                                                 .arg(debugMode);

      page()->runJavaScript(enhancementScript);
    }
  });

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
    // Safely disconnect from current page
    if (this->page()) {
      this->page()->setDevToolsPage(nullptr);
    }
    devToolsView->close();
    devToolsView->deleteLater();
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
  // Log all mouse-related events for debugging
  switch (event->type()) {
  case QEvent::MouseButtonPress:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - MouseButtonPress";
#endif
    setFocus(Qt::MouseFocusReason);
    break;
  case QEvent::MouseButtonRelease:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - MouseButtonRelease";
#endif
    break;
  case QEvent::MouseButtonDblClick:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - MouseButtonDblClick";
#endif
    setFocus(Qt::MouseFocusReason);
    break;
  case QEvent::Gesture:
    return gestureEvent(static_cast<QGestureEvent *>(event));
  case QEvent::TouchBegin:
  case QEvent::TouchUpdate:
  case QEvent::TouchEnd:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - Touch event detected (may interfere with mouse):" << event->type();
#endif
    // Return false to let the event propagate normally
    return false;
  default:
    break;
  }

  // Always call parent implementation to ensure proper event handling
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
  if (!devToolsView) {
    devToolsView = new QWebEngineView();
    devToolsView->setWindowTitle("Developer Tools");
    devToolsView->resize(800, 600);
  }

  page()->setDevToolsPage(devToolsView->page());
  devToolsView->show();
  devToolsView->raise();
  devToolsView->activateWindow();
}

void WebView::requestPictureInPicture() {
  // Use the same JavaScript as in MainWindow but with direct execution
  QString script = R"(
    (function() {
      var videos = document.querySelectorAll('video');
      if (videos.length === 0) {
        alert('このページに動画が見つかりません');
        return;
      }

      // Find the first video that is playing or can be played
      var targetVideo = null;
      for (var i = 0; i < videos.length; i++) {
        var video = videos[i];
        if (!video.paused || video.readyState >= 2) {
          targetVideo = video;
          break;
        }
      }

      if (!targetVideo && videos.length > 0) {
        targetVideo = videos[0]; // Fallback to first video
      }

      if (!targetVideo) {
        alert('ピクチャーインピクチャーに適した動画が見つかりません');
        return;
      }

      // Check if Picture-in-Picture is supported
      if (!document.pictureInPictureEnabled || !targetVideo.requestPictureInPicture) {
        alert('ピクチャーインピクチャーはこのページまたはブラウザでサポートされていません');
        return;
      }

      // Check if already in Picture-in-Picture
      if (document.pictureInPictureElement) {
        document.exitPictureInPicture().then(function() {
          console.log('ピクチャーインピクチャーモードを終了しました');
        }).catch(function(error) {
          console.error('ピクチャーインピクチャー終了エラー:', error);
          alert('ピクチャーインピクチャーの終了に失敗しました: ' + error.message);
        });
      } else {
        // Enter Picture-in-Picture
        targetVideo.requestPictureInPicture().then(function() {
          console.log('ピクチャーインピクチャーモードに入りました');
        }).catch(function(error) {
          console.error('ピクチャーインピクチャー開始エラー:', error);
          alert('ピクチャーインピクチャーの開始に失敗しました: ' + error.message);
        });
      }
    })();
  )";

  page()->runJavaScript(script);
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

  // Add Picture-in-Picture action
  QAction *pipAction = menu->addAction("ピクチャーインピクチャー");
  connect(pipAction, &QAction::triggered, this, &WebView::requestPictureInPicture);

  // Add developer tools action
  QAction *inspectAction = menu->addAction("要素を検証");
  connect(inspectAction, &QAction::triggered, this, &WebView::showDevTools);

  menu->popup(event->globalPos());
}

WebView::~WebView() {
#ifdef DEBUG_MODE
  qDebug() << "WebView::~WebView() - Starting cleanup";
#endif

  try {
    // Remove event filters to prevent crashes
    removeEventFilter(this);
    if (page()) {
      page()->removeEventFilter(this);

      // Disconnect all signals to prevent callbacks during destruction
      disconnect(page(), nullptr, this, nullptr);
    }

    // Safely close developer tools if open
    if (devToolsView) {
#ifdef DEBUG_MODE
      qDebug() << "WebView::~WebView() - Closing developer tools";
#endif

      // Disconnect from page before closing
      if (page()) {
        page()->setDevToolsPage(nullptr);
      }

      // Safely delete the developer tools view
      devToolsView->close();
      devToolsView->deleteLater();
      devToolsView = nullptr;
    }

#ifdef DEBUG_MODE
    qDebug() << "WebView::~WebView() - Cleanup completed";
#endif
  } catch (...) {
#ifdef DEBUG_MODE
    qDebug() << "WebView::~WebView() - Exception during cleanup";
#endif
  }
}

void WebView::mousePressEvent(QMouseEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::mousePressEvent - Button:" << event->button()
           << "Position:" << event->pos()
           << "Global:" << event->globalPosition()
           << "Modifiers:" << event->modifiers()
           << "Accepted:" << event->isAccepted();
#endif

  // Ensure focus and proper event handling
  setFocus(Qt::MouseFocusReason);

  // Accept the event to ensure it's processed
  event->accept();

  // Call parent implementation to ensure proper WebEngine handling
  QWebEngineView::mousePressEvent(event);

#ifdef DEBUG_MODE
  qDebug() << "WebView::mousePressEvent - After parent call, Accepted:" << event->isAccepted();
#endif
}

void WebView::mouseReleaseEvent(QMouseEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::mouseReleaseEvent - Button:" << event->button()
           << "Position:" << event->pos()
           << "Global:" << event->globalPosition()
           << "Accepted:" << event->isAccepted();
#endif

  // Accept the event
  event->accept();

  // Call parent implementation
  QWebEngineView::mouseReleaseEvent(event);

#ifdef DEBUG_MODE
  qDebug() << "WebView::mouseReleaseEvent - After parent call, Accepted:" << event->isAccepted();
#endif
}

void WebView::mouseMoveEvent(QMouseEvent *event) {
  // Only log if a button is pressed to avoid spam
#ifdef DEBUG_MODE
  if (event->buttons() != Qt::NoButton) {
    qDebug() << "WebView::mouseMoveEvent - Buttons:" << event->buttons()
             << "Position:" << event->pos();
  }
#endif

  // Call parent implementation
  QWebEngineView::mouseMoveEvent(event);
}

bool WebView::eventFilter(QObject *obj, QEvent *event) {
  // Log all mouse events that come through the event filter
#ifdef DEBUG_MODE
  if (event->type() == QEvent::MouseButtonPress ||
      event->type() == QEvent::MouseButtonRelease ||
      event->type() == QEvent::MouseButtonDblClick) {

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    qDebug() << "WebView::eventFilter - Event type:" << event->type()
             << "Button:" << mouseEvent->button()
             << "Position:" << mouseEvent->pos()
             << "Global:" << mouseEvent->globalPosition()
             << "Object:" << obj->objectName()
             << "Class:" << obj->metaObject()->className();

    // Let the event continue processing
    return false;
  }
#endif

  // For all other events, pass to parent
  return QWebEngineView::eventFilter(obj, event);
}

void WebView::focusInEvent(QFocusEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::focusInEvent - Reason:" << event->reason();
#endif
  QWebEngineView::focusInEvent(event);

  // Ensure the WebView has focus for proper event handling
  setFocus(Qt::OtherFocusReason);
}

void WebView::focusOutEvent(QFocusEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::focusOutEvent - Reason:" << event->reason();
#endif
  QWebEngineView::focusOutEvent(event);
}

QString WebView::loadResourceFile(const QString &resourcePath) const {
  QFile file(resourcePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Failed to load resource file:" << resourcePath;
    return QString();
  }

  QTextStream in(&file);
  return in.readAll();
}

void WebView::setPage(QWebEnginePage *page) {
  // Close existing developer tools if open
  if (devToolsView) {
    // Safely disconnect from current page
    if (this->page()) {
      this->page()->setDevToolsPage(nullptr);
    }
    devToolsView->close();
    devToolsView->deleteLater();
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
  // Log all mouse-related events for debugging
  switch (event->type()) {
  case QEvent::MouseButtonPress:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - MouseButtonPress";
#endif
    setFocus(Qt::MouseFocusReason);
    break;
  case QEvent::MouseButtonRelease:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - MouseButtonRelease";
#endif
    break;
  case QEvent::MouseButtonDblClick:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - MouseButtonDblClick";
#endif
    setFocus(Qt::MouseFocusReason);
    break;
  case QEvent::Gesture:
    return gestureEvent(static_cast<QGestureEvent *>(event));
  case QEvent::TouchBegin:
  case QEvent::TouchUpdate:
  case QEvent::TouchEnd:
#ifdef DEBUG_MODE
    qDebug() << "WebView::event - Touch event detected (may interfere with mouse):" << event->type();
#endif
    // Return false to let the event propagate normally
    return false;
  default:
    break;
  }

  // Always call parent implementation to ensure proper event handling
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
  if (!devToolsView) {
    devToolsView = new QWebEngineView();
    devToolsView->setWindowTitle("Developer Tools");
    devToolsView->resize(800, 600);
  }

  page()->setDevToolsPage(devToolsView->page());
  devToolsView->show();
  devToolsView->raise();
  devToolsView->activateWindow();
}

void WebView::requestPictureInPicture() {
  // Use the same JavaScript as in MainWindow but with direct execution
  QString script = R"(
    (function() {
      var videos = document.querySelectorAll('video');
      if (videos.length === 0) {
        alert('このページに動画が見つかりません');
        return;
      }

      // Find the first video that is playing or can be played
      var targetVideo = null;
      for (var i = 0; i < videos.length; i++) {
        var video = videos[i];
        if (!video.paused || video.readyState >= 2) {
          targetVideo = video;
          break;
        }
      }

      if (!targetVideo && videos.length > 0) {
        targetVideo = videos[0]; // Fallback to first video
      }

      if (!targetVideo) {
        alert('ピクチャーインピクチャーに適した動画が見つかりません');
        return;
      }

      // Check if Picture-in-Picture is supported
      if (!document.pictureInPictureEnabled || !targetVideo.requestPictureInPicture) {
        alert('ピクチャーインピクチャーはこのページまたはブラウザでサポートされていません');
        return;
      }

      // Check if already in Picture-in-Picture
      if (document.pictureInPictureElement) {
        document.exitPictureInPicture().then(function() {
          console.log('ピクチャーインピクチャーモードを終了しました');
        }).catch(function(error) {
          console.error('ピクチャーインピクチャー終了エラー:', error);
          alert('ピクチャーインピクチャーの終了に失敗しました: ' + error.message);
        });
      } else {
        // Enter Picture-in-Picture
        targetVideo.requestPictureInPicture().then(function() {
          console.log('ピクチャーインピクチャーモードに入りました');
        }).catch(function(error) {
          console.error('ピクチャーインピクチャー開始エラー:', error);
          alert('ピクチャーインピクチャーの開始に失敗しました: ' + error.message);
        });
      }
    })();
  )";

  page()->runJavaScript(script);
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

  // Add Picture-in-Picture action
  QAction *pipAction = menu->addAction("ピクチャーインピクチャー");
  connect(pipAction, &QAction::triggered, this, &WebView::requestPictureInPicture);

  // Add developer tools action
  QAction *inspectAction = menu->addAction("要素を検証");
  connect(inspectAction, &QAction::triggered, this, &WebView::showDevTools);

  menu->popup(event->globalPos());
}

WebView::~WebView() {
#ifdef DEBUG_MODE
  qDebug() << "WebView::~WebView() - Starting cleanup";
#endif

  try {
    // Remove event filters to prevent crashes
    removeEventFilter(this);
    if (page()) {
      page()->removeEventFilter(this);

      // Disconnect all signals to prevent callbacks during destruction
      disconnect(page(), nullptr, this, nullptr);
    }

    // Safely close developer tools if open
    if (devToolsView) {
#ifdef DEBUG_MODE
      qDebug() << "WebView::~WebView() - Closing developer tools";
#endif

      // Disconnect from page before closing
      if (page()) {
        page()->setDevToolsPage(nullptr);
      }

      // Safely delete the developer tools view
      devToolsView->close();
      devToolsView->deleteLater();
      devToolsView = nullptr;
    }

#ifdef DEBUG_MODE
    qDebug() << "WebView::~WebView() - Cleanup completed";
#endif
  } catch (...) {
#ifdef DEBUG_MODE
    qDebug() << "WebView::~WebView() - Exception during cleanup";
#endif
  }
}

void WebView::mousePressEvent(QMouseEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::mousePressEvent - Button:" << event->button()
           << "Position:" << event->pos()
           << "Global:" << event->globalPosition()
           << "Modifiers:" << event->modifiers()
           << "Accepted:" << event->isAccepted();
#endif

  // Ensure focus and proper event handling
  setFocus(Qt::MouseFocusReason);

  // Accept the event to ensure it's processed
  event->accept();

  // Call parent implementation to ensure proper WebEngine handling
  QWebEngineView::mousePressEvent(event);

#ifdef DEBUG_MODE
  qDebug() << "WebView::mousePressEvent - After parent call, Accepted:" << event->isAccepted();
#endif
}

void WebView::mouseReleaseEvent(QMouseEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::mouseReleaseEvent - Button:" << event->button()
           << "Position:" << event->pos()
           << "Global:" << event->globalPosition()
           << "Accepted:" << event->isAccepted();
#endif

  // Accept the event
  event->accept();

  // Call parent implementation
  QWebEngineView::mouseReleaseEvent(event);

#ifdef DEBUG_MODE
  qDebug() << "WebView::mouseReleaseEvent - After parent call, Accepted:" << event->isAccepted();
#endif
}

void WebView::mouseMoveEvent(QMouseEvent *event) {
  // Only log if a button is pressed to avoid spam
#ifdef DEBUG_MODE
  if (event->buttons() != Qt::NoButton) {
    qDebug() << "WebView::mouseMoveEvent - Buttons:" << event->buttons()
             << "Position:" << event->pos();
  }
#endif

  // Call parent implementation
  QWebEngineView::mouseMoveEvent(event);
}

bool WebView::eventFilter(QObject *obj, QEvent *event) {
  // Log all mouse events that come through the event filter
#ifdef DEBUG_MODE
  if (event->type() == QEvent::MouseButtonPress ||
      event->type() == QEvent::MouseButtonRelease ||
      event->type() == QEvent::MouseButtonDblClick) {

    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    qDebug() << "WebView::eventFilter - Event type:" << event->type()
             << "Button:" << mouseEvent->button()
             << "Position:" << mouseEvent->pos()
             << "Global:" << mouseEvent->globalPosition()
             << "Object:" << obj->objectName()
             << "Class:" << obj->metaObject()->className();

    // Let the event continue processing
    return false;
  }
#endif

  // For all other events, pass to parent
  return QWebEngineView::eventFilter(obj, event);
}

void WebView::focusInEvent(QFocusEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::focusInEvent - Reason:" << event->reason();
#endif
  QWebEngineView::focusInEvent(event);

  // Ensure the WebView has focus for proper event handling
  setFocus(Qt::OtherFocusReason);
}

void WebView::focusOutEvent(QFocusEvent *event) {
#ifdef DEBUG_MODE
  qDebug() << "WebView::focusOutEvent - Reason:" << event->reason();
#endif
  QWebEngineView::focusOutEvent(event);
}
