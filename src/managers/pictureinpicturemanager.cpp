#include "pictureinpicturemanager.h"
#include "../mainwindow.h"
#include "../ui/webview.h"
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
  QString cssContent = loadResourceFile(":/resources/css/pip.css");
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
  QString jsContent = loadResourceFile(":/resources/js/pip.js");
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
  QString cssContent = loadResourceFile(":/resources/css/pip.css");
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
  QString jsContent = loadResourceFile(":/resources/js/pip.js");
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
  QString cssContent = loadResourceFile(":/resources/css/pip.css");
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
  QString jsContent = loadResourceFile(":/resources/js/pip.js");
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

  // Step 2: 利用可能なAPIの確認
  const apis = {
    pictureInPictureEnabled : 'pictureInPictureEnabled' in document,
    pictureInPictureElement : 'pictureInPictureElement' in document,
    exitPictureInPicture : 'exitPictureInPicture' in document,
    HTMLVideoElement_requestPictureInPicture : 'requestPictureInPicture' in HTMLVideoElement.prototype,
    HTMLVideoElement_disablePictureInPicture : 'disablePictureInPicture' in HTMLVideoElement.prototype
  };

  console.log('📋 利用可能なAPI:', apis);

  // Step 3: Picture-in-Picture環境を強制的に作成
  function createPiPEnvironment() {
    console.log('🔧 PiP環境を強制作成中...');

    // Document レベルでの設定
    if (!document.pictureInPictureEnabled) {
      try {
        Object.defineProperty(document, 'pictureInPictureEnabled', {
          value : true,
          writable : false,
          configurable : true
        });
        console.log('✅ document.pictureInPictureEnabled を強制的に true に設定');
      } catch (e) {
        console.log('⚠️ document.pictureInPictureEnabled 強制設定失敗:', e);
      }
    }

    // pictureInPictureElement の確保
    if (!('pictureInPictureElement' in document)) {
      try {
        Object.defineProperty(document, 'pictureInPictureElement', {
          value : null,
          writable : true,
          configurable : true
        });
        console.log('✅ document.pictureInPictureElement を設定');
      } catch (e) {
        console.log('⚠️ document.pictureInPictureElement 設定失敗:', e);
      }
    }

    // exitPictureInPicture の確保
    if (!document.exitPictureInPicture) {
      document.exitPictureInPicture = function() {
        console.log('🔧 カスタム exitPictureInPicture 実行');
        return Promise.resolve();
      };
      console.log('✅ document.exitPictureInPicture を設定');
    }

    // HTMLVideoElement の拡張
    if (!HTMLVideoElement.prototype.requestPictureInPicture) {
      HTMLVideoElement.prototype.requestPictureInPicture = function() {
        console.log('🔧 カスタム requestPictureInPicture 実行');
        console.log('⚠️ Qt WebEngineではPicture-in-Pictureがネイティブサポートされていません');

        // シミュレーション用のPiPウィンドウを作成
                        return new Promise((resolve, reject) => {
          try {
            // ビデオ要素のクローンを作成してフローティングウィンドウとして表示
            this.createFloatingVideoWindow();

            // PiP開始イベントを発火
            const pipEvent = new Event('enterpictureinpicture');
            this.dispatchEvent(pipEvent);

            // 疑似PiPウィンドウオブジェクトを返す
            const mockPiPWindow = {
              width : 320,
              height : 180,
              resizeBy : function(x, y){console.log('PiP resize:', x, y);
          }
          ,
              addEventListener : function(type, listener) { console.log('PiP event listener:', type); }
                                };

                                resolve(mockPiPWindow);
      }
      catch (error) {
        reject(new DOMException('Picture-in-Picture シミュレーション失敗', 'NotSupportedError'));
      }
    });
  };
}

// フローティングビデオウィンドウ作成関数
HTMLVideoElement.prototype.createFloatingVideoWindow = function() {
  console.log('🎬 フローティングビデオウィンドウを作成中...');

  // 既存のフローティングウィンドウがあれば削除
  const existingWindow = document.getElementById('pip-floating-window');
  if (existingWindow) {
    existingWindow.remove();
  }

  // フローティングコンテナを作成
  const floatingContainer = document.createElement('div');
  floatingContainer.id = 'pip-floating-window';
  floatingContainer.style.cssText = ` position : fixed;
top:
  20px;
right:
  20px;
width:
  320px;
height:
  180px;
background:
  black;
border:
  2px solid #333;
  border - radius : 8px;
  box - shadow : 0 4px 20px rgba(0, 0, 0, 0.3);
  z - index : 999999;
cursor:
  move;
overflow:
  hidden;
  `;

  // ビデオクローンを作成
  const videoClone = this.cloneNode(true);
  videoClone.style.cssText = ` width : 100 % ;
height:
  100 % ;
  object - fit : contain;
  `;

  // 元の動画と同期
  videoClone.currentTime = this.currentTime;
  if (!this.paused) {
    videoClone.play();
  }

  // 閉じるボタンを追加
  const closeButton = document.createElement('button');
  closeButton.textContent = '×';
  closeButton.style.cssText = ` position : absolute;
top:
  5px;
right:
  5px;
width:
  25px;
height:
  25px;
background:
  rgba(0, 0, 0, 0.7);
color:
  white;
border:
  none;
  border - radius : 50 % ;
cursor:
  pointer;
  font - size : 14px;
  z - index : 1000000;
  `;

  closeButton.onclick = () => {
    floatingContainer.remove();
    // PiP終了イベントを発火
    const pipExitEvent = new Event('leavepictureinpicture');
    this.dispatchEvent(pipExitEvent);
    document.pictureInPictureElement = null;
    console.log('🔚 フローティングPiPウィンドウを閉じました');
  };

  // ドラッグ機能を追加
  let isDragging = false;
  let dragStartX, dragStartY;

  floatingContainer.onmousedown = (e) => {
    if (e.target == = closeButton)
      return;
    isDragging = true;
    dragStartX = e.clientX - floatingContainer.offsetLeft;
    dragStartY = e.clientY - floatingContainer.offsetTop;
  };

  document.onmousemove = (e) => {
    if (isDragging) {
      floatingContainer.style.left = (e.clientX - dragStartX) + 'px';
      floatingContainer.style.top = (e.clientY - dragStartY) + 'px';
    }
  };

  document.onmouseup = () => {
    isDragging = false;
  };

  // 要素を組み立て
  floatingContainer.appendChild(videoClone);
  floatingContainer.appendChild(closeButton);
  document.body.appendChild(floatingContainer);

  // Document の pictureInPictureElement を設定
  document.pictureInPictureElement = this;

  console.log('✅ フローティングPiPウィンドウが作成されました');
};
}

// Step 4: 環境を作成
createPiPEnvironment();

// Step 5: 動画の準備と実行
const videos = document.querySelectorAll('video');
console.log('📹 見つかった動画:', videos.length + '個');

if (videos.length == = 0) {
  alert('このページには動画が見つかりませんでした。');
  return;
}

// 最初のビデオを対象に選択
let targetVideo = videos[0];
for (const video of videos) {
  if (!video.paused && video.readyState >= 2) {
    targetVideo = video;
    break;
  }
}

console.log('🎯 対象動画を選択:', targetVideo);

// Step 6: Picture-in-Picture実行
try {
  console.log('=== PiP実行開始 ===');

  // 動画が一時停止中の場合は再生
  if (targetVideo.paused) {
    console.log('▶️ 動画を再生開始...');
    await targetVideo.play();
  }

  console.log('🔄 Picture-in-Picture をリクエスト中...');
  const pipWindow = await targetVideo.requestPictureInPicture();

  console.log('✅ Picture-in-Picture が開始されました!');
  console.log('📊 PiPウィンドウ:', pipWindow);

  alert('Picture-in-Picture シミュレーションが開始されました！\\n\\n' +
        '右上にフローティングビデオウィンドウが表示されています。\\n' +
        'ドラッグして移動したり、×ボタンで閉じたりできます。');

} catch (error) {
  console.error('❌ Picture-in-Picture エラー:', error);

  let errorMessage = 'Picture-in-Picture の開始に失敗しました。\\n\\n';

  if (error.name == = 'NotSupportedError') {
    errorMessage += 'この環境では Picture-in-Picture がサポートされていません。\\n';
    errorMessage += 'Qt WebEngine の制限により、ネイティブ PiP は利用できませんが、\\n';
    errorMessage += '代替実装としてフローティングウィンドウを提供します。';
  } else if (error.name == = 'NotAllowedError') {
    errorMessage += 'Picture-in-Picture の使用が許可されていません。\\n';
    errorMessage += 'ユーザーの操作が必要です。';
  } else if (error.name == = 'InvalidStateError') {
    errorMessage += '動画の状態が無効です。\\n';
    errorMessage += '動画を再生してから再試行してください。';
  } else {
    errorMessage += 'エラー詳細: ' + error.message;
  }

  alert(errorMessage);
}

console.log('=== PiP実装完了 ===');
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
