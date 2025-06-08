#include "webview.h"
#include "../main-window/mainwindow.h" // To potentially access MainWindow for new tab creation logic
#include <QAction>
#include <QApplication>
#include <QContextMenuEvent>
#include <QDebug>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QMessageBox>
#include <QMouseEvent>
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
  pageSettings->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
  pageSettings->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);

  // Configure profile for media features
  QWebEngineProfile *profile = customPage->profile();
  if (profile) {
    // Enable media features that support Picture-in-Picture
    profile->settings()->setAttribute(QWebEngineSettings::PlaybackRequiresUserGesture, false);
    profile->settings()->setAttribute(QWebEngineSettings::AllowRunningInsecureContent, true);
    profile->settings()->setAttribute(QWebEngineSettings::AllowWindowActivationFromJavaScript, true);

    // Add user agent for enhanced media support
    static bool profileConfigured = false;
    if (!profileConfigured) {
      QString enhancedUserAgent = profile->httpUserAgent();
      if (!enhancedUserAgent.contains("PictureInPictureSupported")) {
        enhancedUserAgent += " PictureInPictureSupported";
      }
      profile->setHttpUserAgent(enhancedUserAgent);
      profileConfigured = true;
    }
  }

  // Forward signals that MainWindow might be interested in
  connect(page(), &QWebEnginePage::titleChanged, this, &WebView::titleChanged);
  connect(page(), &QWebEnginePage::urlChanged, this, &WebView::urlChanged);
  connect(page(), &QWebEnginePage::loadProgress, this, &WebView::loadProgress);
  connect(page(), &QWebEnginePage::loadFinished, this, &WebView::loadFinished);
  connect(page(), &QWebEnginePage::loadStarted, this, &WebView::loadStarted); // Inject JavaScript to debug click events on page load
  connect(page(), &QWebEnginePage::loadFinished, this, [this](bool success) {
    if (success) {
      QString clickDebugScript = R"(
        (function() {
          var debugMode = )" + QString(
#ifdef DEBUG_MODE
                                     "true"
#else
          "false"
#endif
                                     ) +
                                 R"(;

          if (debugMode) {
            console.log('WebView: Injecting click debug script');
          }

          // Debug all click events (only in debug mode)
          if (debugMode) {
            document.addEventListener('click', function(e) {
              console.log('JS Click Event:', {
                target: e.target.tagName,
                className: e.target.className,
                href: e.target.href,
                fullUrl: e.target.href ? e.target.href.toString() : 'no href',
                textContent: e.target.textContent,
                x: e.clientX,
                y: e.clientY,
                button: e.button,
                ctrlKey: e.ctrlKey,
                metaKey: e.metaKey,
                defaultPrevented: e.defaultPrevented,
                isTrusted: e.isTrusted
              });
            }, true);

            // Debug mousedown events
            document.addEventListener('mousedown', function(e) {
              console.log('JS MouseDown Event:', {
                target: e.target.tagName,
                className: e.target.className,
                x: e.clientX,
                y: e.clientY,
                button: e.button,
                defaultPrevented: e.defaultPrevented
              });
            }, true);

            // Debug mouseup events
            document.addEventListener('mouseup', function(e) {
              console.log('JS MouseUp Event:', {
                target: e.target.tagName,
                className: e.target.className,
                x: e.clientX,
                y: e.clientY,
                button: e.button,
                defaultPrevented: e.defaultPrevented
              });
            }, true);

            // Additional debug for focus events
            document.addEventListener('focus', function(e) {
              console.log('JS Focus Event:', e.target.tagName);
            }, true);

            document.addEventListener('blur', function(e) {
              console.log('JS Blur Event:', e.target.tagName);
            }, true);
          }

          // Enable Picture-in-Picture for all videos
          function enablePictureInPicture() {
            var videos = document.querySelectorAll('video');
            videos.forEach(function(video) {
              // Remove disablepictureinpicture attribute if present
              if (video.hasAttribute('disablepictureinpicture')) {
                video.removeAttribute('disablepictureinpicture');
                if (debugMode) {
                  console.log('WebView: Removed disablepictureinpicture from video element');
                }
              }

              // Ensure picture-in-picture is not disabled via property
              if (video.disablePictureInPicture === true) {
                video.disablePictureInPicture = false;
                if (debugMode) {
                  console.log('WebView: Enabled picture-in-picture via property');
                }
              }
            });
          }

          // Run immediately
          enablePictureInPicture();

          // Watch for new videos being added to the DOM
          var observer = new MutationObserver(function(mutations) {
            mutations.forEach(function(mutation) {
              mutation.addedNodes.forEach(function(node) {
                if (node.nodeType === 1) { // ELEMENT_NODE
                  // Check if the node itself is a video
                  if (node.tagName === 'VIDEO') {
                    if (node.hasAttribute('disablepictureinpicture')) {
                      node.removeAttribute('disablepictureinpicture');
                      if (debugMode) {
                        console.log('WebView: Removed disablepictureinpicture from newly added video');
                      }
                    }
                    if (node.disablePictureInPicture === true) {
                      node.disablePictureInPicture = false;
                      if (debugMode) {
                        console.log('WebView: Enabled picture-in-picture for newly added video');
                      }
                    }
                  }
                  // Check for videos within the added node
                  var videos = node.querySelectorAll ? node.querySelectorAll('video') : [];
                  videos.forEach(function(video) {
                    if (video.hasAttribute('disablepictureinpicture')) {
                      video.removeAttribute('disablepictureinpicture');
                      if (debugMode) {
                        console.log('WebView: Removed disablepictureinpicture from video in newly added content');
                      }
                    }
                    if (video.disablePictureInPicture === true) {
                      video.disablePictureInPicture = false;
                      if (debugMode) {
                        console.log('WebView: Enabled picture-in-picture for video in newly added content');
                      }
                    }
                  });
                }
              });
            });
          });

          // Start observing
          observer.observe(document.body, {
            childList: true,
            subtree: true
          });

          // Enhanced link handling - FIXED for JavaScript links
          // Add highest priority click handler for JavaScript links
          document.addEventListener('click', function(e) {
            if (debugMode) {
              console.log('WebView High Priority Handler - Click detected:', {
                tagName: e.target.tagName,
                href: e.target.href,
                fullHref: e.target.href ? e.target.href.toString() : 'no href',
                textContent: e.target.textContent,
                target: e.target.target,
                x: e.clientX,
                y: e.clientY,
                defaultPrevented: e.defaultPrevented,
                timeStamp: e.timeStamp
              });
            }

            // Immediately handle JavaScript links with highest priority
            if (e.target.tagName === 'A' && e.target.href && e.target.href.startsWith('javascript:')) {
              if (debugMode) {
                console.log('WebView High Priority: JavaScript link detected, executing immediately...');
                console.log('WebView High Priority: Full href:', e.target.href);
              }

              try {
                var jsCode = decodeURIComponent(e.target.href.substring(11));
                if (debugMode) {
                  console.log('WebView High Priority: Executing JavaScript code:', jsCode);
                }

                // Execute the JavaScript code in global scope
                var result = eval.call(window, jsCode);
                if (debugMode) {
                  console.log('WebView High Priority: JavaScript executed successfully, result:', result);
                }
              } catch (error) {
                console.error('WebView High Priority: Error executing JavaScript link:', error);
                console.error('WebView High Priority: JavaScript code was:', jsCode);
              }

              // Prevent default and stop propagation for JavaScript links
              e.preventDefault();
              e.stopPropagation();
              return false;
            }
          }, true); // Use capture phase for highest priority

          document.addEventListener('click', function(e) {
            if (e.target.tagName === 'A' && e.target.href) {
              if (debugMode) {
                console.log('WebView Enhanced Link Handler - Click detected:', {
                  href: e.target.href,
                  target: e.target.target,
                  hash: e.target.hash,
                  hostname: e.target.hostname,
                  pathname: e.target.pathname,
                  defaultPrevented: e.defaultPrevented
                });
              }

              // Handle JavaScript links explicitly
              if (e.target.href.startsWith('javascript:')) {
                if (debugMode) {
                  console.log('WebView: JavaScript link detected, executing...');
                  console.log('WebView: Full href:', e.target.href);
                }
                try {
                  // Extract and execute the JavaScript code
                  var jsCode = decodeURIComponent(e.target.href.substring(11)); // Remove 'javascript:'
                  if (debugMode) {
                    console.log('WebView: Executing JavaScript code:', jsCode);
                  }

                  // Multiple approaches to ensure execution works
                  // Method 1: Direct eval (global scope)
                  try {
                    eval.call(window, jsCode);
                    if (debugMode) {
                      console.log('WebView: JavaScript executed successfully with eval.call');
                    }
                  } catch (evalError) {
                    if (debugMode) {
                      console.log('WebView: eval.call failed, trying Function constructor');
                    }
                    // Method 2: Function constructor (safer and more reliable)
                    var func = new Function(jsCode);
                    func.call(window);
                    if (debugMode) {
                      console.log('WebView: JavaScript executed successfully with Function constructor');
                    }
                  }
                } catch (error) {
                  console.error('WebView: Error executing JavaScript link:', error);
                  console.error('WebView: JavaScript code was:', jsCode);
                }
                // Prevent default navigation for JavaScript links only
                e.preventDefault();
                e.stopPropagation();
                return;
              }

              // For fragment links (same page navigation)
              if (e.target.hash && (e.target.hostname === window.location.hostname || e.target.hostname === '')) {
                if (debugMode) {
                  console.log('WebView: Fragment link detected, allowing normal navigation');
                }
                // Let the browser handle fragment navigation naturally
                return;
              }

              // For external links
              if (e.target.target === '_blank' || (e.target.hostname && e.target.hostname !== window.location.hostname)) {
                if (debugMode) {
                  console.log('WebView: External link detected, allowing normal navigation');
                }
                // Let the browser handle external links naturally
                return;
              }

              // For regular links (same hostname)
              if (debugMode) {
                console.log('WebView: Regular link detected, allowing normal navigation');
              }
              // Let the browser handle all other links naturally
              return;
            }

            // For non-link elements, don't interfere with their click handling
            if (debugMode && e.target.tagName !== 'A') {
              console.log('WebView: Non-link click detected:', {
                tagName: e.target.tagName,
                type: e.target.type,
                className: e.target.className,
                defaultPrevented: e.defaultPrevented
              });
            }
          }, false); // Use bubbling phase, lower priority

          // Prevent touch events only on macOS and only for specific conflict cases
          var isMacOS = navigator.platform.toUpperCase().indexOf('MAC') >= 0;
          if (isMacOS && debugMode) {
            document.addEventListener('touchstart', function(e) {
              // Only prevent if there are actual conflicts detected
              // Let most touch events through for better compatibility
              if (debugMode) {
                console.log('WebView: TouchStart detected on macOS - monitoring but not preventing');
              }
              // Don't prevent by default - let touch events work normally
              // e.preventDefault(); // Commented out to allow normal touch handling
            }, { passive: true });
          }
        })();
      )";
      page()->runJavaScript(clickDebugScript);
    }
  });

  // Enable gesture recognition for swipe gestures
  grabGesture(Qt::SwipeGesture);
  grabGesture(Qt::PanGesture); // トラックパッドでのスワイプをより感度よく検出

  // JavaScriptでトラックパッドスワイプを検出
  connect(this, &QWebEngineView::loadFinished, this, [this](bool ok) {
    if (ok) {
      QString swipeScript = R"(
        (function() {
          let startX = 0;
          let startY = 0;
          let startTime = 0;

          // Wait for QWebChannel to be available
          function initializeSwipeHandling() {
            if (typeof qt !== 'undefined' && qt.webChannelTransport && typeof mainWindow !== 'undefined') {
              console.log('WebChannel available, setting up swipe handlers');

              // マウスホイールイベントでスワイプを検出（macOSトラックパッド対応）
              document.addEventListener('wheel', function(e) {
                // 水平スクロールの検出
                if (Math.abs(e.deltaX) > Math.abs(e.deltaY) && Math.abs(e.deltaX) > 30) {
                  if (e.deltaX > 0) {
                    // 右スワイプ - 戻る
                    console.log('Trackpad swipe back detected');
                    mainWindow.handleSwipeBack();
                  } else {
                    // 左スワイプ - 進む
                    console.log('Trackpad swipe forward detected');
                    mainWindow.handleSwipeForward();
                  }
                }
              }, { passive: true });

              // タッチイベントでもスワイプを検出
              document.addEventListener('touchstart', function(e) {
                if (e.touches.length === 1) {
                  startX = e.touches[0].clientX;
                  startY = e.touches[0].clientY;
                  startTime = Date.now();
                }
              }, { passive: true });

              document.addEventListener('touchend', function(e) {
                if (e.changedTouches.length === 1 && startTime > 0) {
                  const endX = e.changedTouches[0].clientX;
                  const endY = e.changedTouches[0].clientY;
                  const deltaX = endX - startX;
                  const deltaY = endY - startY;
                  const deltaTime = Date.now() - startTime;

                  // スワイプの条件: 水平方向の移動が垂直方向より大きく、十分な距離で短時間
                  if (Math.abs(deltaX) > Math.abs(deltaY) && Math.abs(deltaX) > 100 && deltaTime < 500) {
                    if (deltaX > 0) {
                      // 右スワイプ - 戻る
                      console.log('Touch swipe back detected');
                      mainWindow.handleSwipeBack();
                    } else {
                      // 左スワイプ - 進む
                      console.log('Touch swipe forward detected');
                      mainWindow.handleSwipeForward();
                    }
                  }
                  startTime = 0;
                }
              }, { passive: true });
            } else {
              // Retry after a short delay
              setTimeout(initializeSwipeHandling, 100);
            }
          }

          // Start initialization
          initializeSwipeHandling();
        })();
      )";
      page()->runJavaScript(swipeScript);
    }
  });

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
  // デフォルトのコンテキストメニューを無効化して、カスタムメニューのみ表示
  QMenu *menu = new QMenu(this);

  // 基本的なナビゲーションアクション（英語で統一）
  QAction *backAction = menu->addAction("Back");
  backAction->setEnabled(page() && page()->history()->canGoBack());
  connect(backAction, &QAction::triggered, [this]() { page()->history()->back(); });

  QAction *forwardAction = menu->addAction("Forward");
  forwardAction->setEnabled(page() && page()->history()->canGoForward());
  connect(forwardAction, &QAction::triggered, [this]() { page()->history()->forward(); });

  QAction *reloadAction = menu->addAction("Reload");
  connect(reloadAction, &QAction::triggered, [this]() { page()->triggerAction(QWebEnginePage::Reload); });

  menu->addSeparator();

  // ページアクションを英語で追加
  QAction *copyAction = menu->addAction("Copy");
  copyAction->setEnabled(page()->action(QWebEnginePage::Copy)->isEnabled());
  connect(copyAction, &QAction::triggered, [this]() { page()->triggerAction(QWebEnginePage::Copy); });

  QAction *pasteAction = menu->addAction("Paste");
  pasteAction->setEnabled(page()->action(QWebEnginePage::Paste)->isEnabled());
  connect(pasteAction, &QAction::triggered, [this]() { page()->triggerAction(QWebEnginePage::Paste); });

  QAction *selectAllAction = menu->addAction("Select All");
  connect(selectAllAction, &QAction::triggered, [this]() { page()->triggerAction(QWebEnginePage::SelectAll); });

  menu->addSeparator();

  // Picture-in-Picture action
  QAction *pipAction = menu->addAction("Picture in Picture");
  connect(pipAction, &QAction::triggered, this, &WebView::requestPictureInPicture);

  // Developer tools action
  QAction *inspectAction = menu->addAction("Inspect Element");
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

void WebView::handleSwipeBack() {
  if (page() && page()->history()->canGoBack()) {
    page()->history()->back();
  }
}

void WebView::handleSwipeForward() {
  if (page() && page()->history()->canGoForward()) {
    page()->history()->forward();
  }
}

void WebView::handlePipImageSelection(const QString &imageUrl, const QString &title) {
  qDebug() << "WebView: PiP image selection handled:" << title << imageUrl;
  emit pipImageRequested(imageUrl, title);
}
