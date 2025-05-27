#include "pictureinpicturemanager.h"
#include "mainwindow.h"
#include "webview.h"
#include <QAction>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
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

  QString exitScript = R"(
        (async function() {
            try {
                if (document.pictureInPictureElement) {
                    await document.exitPictureInPicture();
                    console.log('Picture-in-Picture モードを終了しました');
                } else {
                    console.log('現在Picture-in-Pictureモードではありません');
                }
            } catch (error) {
                console.error('PiP終了エラー:', error);
            }
        })();
    )";

  executeJavaScript(webView, exitScript);
}

QString PictureInPictureManager::generatePiPJavaScript() const {
  return R"(
        (async function() {
            console.log('Picture-in-Picture リクエストを開始します...');

            // Picture-in-Picture APIサポートチェック
            if (!('pictureInPictureEnabled' in document)) {
                alert('このブラウザはPicture-in-Picture APIをサポートしていません。');
                return;
            }

            if (!document.pictureInPictureEnabled) {
                alert('Picture-in-Picture機能が無効になっています。');
                return;
            }

            // 動画要素を検索
            const videos = document.querySelectorAll('video');
            console.log('見つかった動画要素の数:', videos.length);

            if (videos.length === 0) {
                alert('このページには動画が見つかりませんでした。');
                return;
            }

            // 再生中の動画を優先して選択
            let targetVideo = null;
            for (const video of videos) {
                if (!video.paused && video.readyState >= 2) {
                    targetVideo = video;
                    break;
                }
            }

            // 再生中の動画がない場合は最初の動画を選択
            if (!targetVideo) {
                targetVideo = videos[0];
            }

            console.log('対象動画:', targetVideo);
            console.log('動画のdisablePictureInPicture:', targetVideo.disablePictureInPicture);

            try {
                // disablePictureInPicture属性を削除
                if (targetVideo.hasAttribute('disablepictureinpicture')) {
                    targetVideo.removeAttribute('disablepictureinpicture');
                    console.log('disablepictureinpicture属性を削除しました');
                }

                // disablePictureInPictureプロパティを強制的にfalseに設定
                targetVideo.disablePictureInPicture = false;

                // 動画が一時停止中の場合は再生を開始
                if (targetVideo.paused) {
                    console.log('動画を再生開始します...');
                    await targetVideo.play();
                }

                // Picture-in-Pictureモードをリクエスト
                console.log('Picture-in-Pictureモードをリクエストしています...');
                const pipWindow = await targetVideo.requestPictureInPicture();
                console.log('Picture-in-Pictureモードが開始されました:', pipWindow);
                alert('Picture-in-Pictureモードが開始されました！');

            } catch (error) {
                console.error('Picture-in-Pictureエラー:', error);

                if (error.name === 'NotSupportedError') {
                    alert('この動画はPicture-in-Pictureをサポートしていません。');
                } else if (error.name === 'NotAllowedError') {
                    alert('Picture-in-Picture機能の使用が許可されていません。ユーザーアクションが必要です。');
                } else if (error.name === 'InvalidStateError') {
                    alert('動画の状態が無効です。動画を再生してから再試行してください。');
                } else {
                    alert('Picture-in-Pictureの開始に失敗しました: ' + error.message);
                }
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
