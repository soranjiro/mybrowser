#include "pictureinpicturemanager.h"
#include "../main-window/mainwindow.h"
#include "../webview/webview.h"
#include <QAction>
#include <QFile>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QTextStream>
#include <QWebEngineView>

PictureInPictureManager::PictureInPictureManager(MainWindow *parent)
    : QObject(parent), mainWindow(parent), pictureInPictureAction(nullptr), contextMenuAction(nullptr) {
  setupActions();
}

PictureInPictureManager::~PictureInPictureManager() {
  // Qtの親子関係により自動的にクリーンアップされます
}

void PictureInPictureManager::setupActions() {
  // Picture-in-Picture アクション
  pictureInPictureAction = new QAction("Picture-in-Picture", this);
  pictureInPictureAction->setShortcut(QKeySequence(Qt::CTRL | Qt::Key_P));
  pictureInPictureAction->setStatusTip("Toggle Picture-in-Picture mode for video");
  pictureInPictureAction->setToolTip("Toggle Picture-in-Picture mode for video (Ctrl+P)");

  connect(pictureInPictureAction, &QAction::triggered, this, &PictureInPictureManager::onTogglePictureInPicture);

  // コンテキストメニュー用アクション
  contextMenuAction = new QAction("ピクチャーインピクチャー", this);
  contextMenuAction->setStatusTip("動画をピクチャーインピクチャーモードで開く");

  connect(contextMenuAction, &QAction::triggered, this, &PictureInPictureManager::onTogglePictureInPicture);
}

void PictureInPictureManager::addToMenu(QMenu *viewMenu) {
  if (viewMenu && pictureInPictureAction) {
    viewMenu->addAction(pictureInPictureAction);
  }
}

void PictureInPictureManager::addToContextMenu(QMenu *contextMenu) {
  if (contextMenu && contextMenuAction) {
    contextMenu->addAction(contextMenuAction);
  }
}

void PictureInPictureManager::onTogglePictureInPicture() {
  togglePictureInPicture();
}

void PictureInPictureManager::togglePictureInPicture(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "Picture-in-Picture",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  QString script = generatePiPToggleScript();
  executeJavaScript(webView, script);
}

void PictureInPictureManager::requestPictureInPicture(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "Picture-in-Picture",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  QString script = generatePiPJavaScript();
  executeJavaScript(webView, script);
}

void PictureInPictureManager::exitPictureInPicture(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    return;
  }

  // CSS注入
  QString cssContent = loadResourceFile(":/features/picture-in-picture/pip.css");
  QString cssInjection;
  if (!cssContent.isEmpty()) {
    cssInjection = QString(R"(
        if (!document.querySelector('style[data-pip-styles]')) {
          const style = document.createElement('style');
          style.setAttribute('data-pip-styles', 'true');
          style.textContent = `%1`;
          document.head.appendChild(style);
        }
    )")
                       .arg(cssContent);
  }

  // JavaScript読み込み＆実行
  QString jsContent = loadResourceFile(":/features/picture-in-picture/pip.js");
  QString exitScript = cssInjection + jsContent + R"(
        (async function() {
            if (window.pipHandler) {
                await window.pipHandler.exitPictureInPicture();
            } else {
                console.error('PiP Handler not available');
            }
        })();
    )";

  executeJavaScript(webView, exitScript);
}

void PictureInPictureManager::enablePiPForAllVideos(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    return;
  }

  // CSS注入
  QString cssContent = loadResourceFile(":/features/picture-in-picture/pip.css");
  QString cssInjection;
  if (!cssContent.isEmpty()) {
    cssInjection = QString(R"(
        if (!document.querySelector('style[data-pip-styles]')) {
          const style = document.createElement('style');
          style.setAttribute('data-pip-styles', 'true');
          style.textContent = `%1`;
          document.head.appendChild(style);
        }
    )")
                       .arg(cssContent);
  }

  // JavaScript読み込み＆実行
  QString jsContent = loadResourceFile(":/features/picture-in-picture/pip.js");
  QString enableScript = cssInjection + jsContent + R"(
        (function() {
            if (window.pipHandler) {
                return window.pipHandler.enablePiPForAllVideos();
            } else {
                console.error('PiP Handler not available');
                return 0;
            }
        })();
    )";

  executeJavaScript(webView, enableScript);
}

QString PictureInPictureManager::loadResourceFile(const QString &resourcePath) const {
  QFile file(resourcePath);
  if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    qWarning() << "Failed to load resource file:" << resourcePath;
    return QString();
  }

  QTextStream in(&file);
  return in.readAll();
}

QString PictureInPictureManager::generatePiPJavaScript() const {
  // CSSファイルを注入
  QString cssContent = loadResourceFile(":/features/picture-in-picture/pip.css");
  QString cssInjection;
  if (!cssContent.isEmpty()) {
    cssInjection = QString(R"(
        // Inject PiP CSS
        if (!document.querySelector('style[data-pip-styles]')) {
          const style = document.createElement('style');
          style.setAttribute('data-pip-styles', 'true');
          style.textContent = `%1`;
          document.head.appendChild(style);
        }
    )")
                       .arg(cssContent);
  }

  // JavaScriptファイルを読み込み
  QString jsContent = loadResourceFile(":/features/picture-in-picture/pip.js");
  if (jsContent.isEmpty()) {
    qWarning() << "Failed to load PiP JavaScript file, falling back to inline implementation";
    // フォールバック用の最小限の実装
    return R"(
        (async function() {
            console.log('=== PiP実装開始（フォールバック） ===');
            alert('Picture-in-Picture機能のJavaScriptファイルの読み込みに失敗しました。');
        })();
    )";
  }

  return cssInjection + R"(
        // Load and execute PiP JavaScript
        )" +
         jsContent + R"(

        // Execute PiP functionality
        (async function() {
            if (window.pipHandler) {
                await window.pipHandler.executePictureInPicture();
            } else {
                console.error('PiP Handler not available');
            }
        })();
    )";
}

QString PictureInPictureManager::generatePiPDetectionScript() const {
  return R"(
        (function() {
            console.log('Picture-in-Picture サポート状況:');
            console.log('- pictureInPictureEnabled in document:', 'pictureInPictureEnabled' in document);
            console.log('- document.pictureInPictureEnabled:', document.pictureInPictureEnabled);
            console.log('- HTMLVideoElement.prototype.requestPictureInPicture:',
                      typeof HTMLVideoElement.prototype.requestPictureInPicture);

            const videos = document.querySelectorAll('video');
            console.log('動画要素の数:', videos.length);

            videos.forEach((video, index) => {
                console.log(`動画 ${index + 1}:`, {
                    src: video.src || video.currentSrc,
                    disablePictureInPicture: video.disablePictureInPicture,
                    hasDisableAttribute: video.hasAttribute('disablepictureinpicture'),
                    readyState: video.readyState,
                    paused: video.paused
                });
            });

            return {
                supported: document.pictureInPictureEnabled,
                videoCount: videos.length
            };
        })();
    )";
}

QString PictureInPictureManager::generatePiPToggleScript() const {
  return R"(
        (async function() {
            console.log('Picture-in-Picture トグル機能を実行します...');

            // 現在PiPモードかチェック
            if (document.pictureInPictureElement) {
                console.log('現在PiPモード中です。終了します...');
                try {
                    await document.exitPictureInPicture();
                    console.log('Picture-in-Pictureモードを終了しました');
                    alert('Picture-in-Pictureモードを終了しました');
                } catch (error) {
                    console.error('PiP終了エラー:', error);
                    alert('Picture-in-Pictureの終了に失敗しました: ' + error.message);
                }
                return;
            }

            // PiPモードでない場合は開始
            console.log('Picture-in-Pictureモードを開始します...');
    )" + generatePiPJavaScript().mid(generatePiPJavaScript().indexOf("// Picture-in-Picture APIサポートチェック"));
}

void PictureInPictureManager::executeJavaScript(WebView *webView, const QString &script) {
  if (!webView) {
    return;
  }

  webView->page()->runJavaScript(script, [this](const QVariant &result) {
    Q_UNUSED(result)
    // JavaScript実行完了時の処理（必要に応じて）
  });
}
