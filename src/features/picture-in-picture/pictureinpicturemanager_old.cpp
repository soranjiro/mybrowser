#include "pictureinpicturemanager.h"
#include "../main-window/mainwindow.h"
#include "../webview/webview.h"
#include "pipwindow.h"
#include <QAction>
#include <QBuffer>
#include <QDebug>
#include <QFile>
#include <QKeySequence>
#include <QMenu>
#include <QMessageBox>
#include <QPixmap>
#include <QTextStream>
#include <QWebEngineView>

PictureInPictureManager::PictureInPictureManager(MainWindow *parent)
    : QObject(parent), mainWindow(parent), pictureInPictureAction(nullptr),
      contextMenuAction(nullptr), standaloneProcess(nullptr), ipcServer(nullptr) {
  setupActions();
}

PictureInPictureManager::~PictureInPictureManager() {
  // すべてのネイティブPiPウィンドウを閉じる
  closeAllNativePiP();

  // スタンドアロンプロセスを終了
  closeStandalonePiP();

  // IPCサーバーを停止
  if (ipcServer) {
    ipcServer->close();
  }

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
  QString cssContent = loadResourceFile(":/src/features/picture-in-picture/pip.css");
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
  QString jsContent = loadResourceFile(":/src/features/picture-in-picture/pip.js");
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
  QString cssContent = loadResourceFile(":/src/features/picture-in-picture/pip.css");
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
  QString jsContent = loadResourceFile(":/src/features/picture-in-picture/pip.js");
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

// 独自PiP機能の実装
void PictureInPictureManager::createElementPictureInPicture(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "Element Picture-in-Picture",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  QString script = generateElementPiPScript();
  executeJavaScript(webView, script);
}

void PictureInPictureManager::createPagePictureInPicture(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "Page Picture-in-Picture",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  QString script = generatePagePiPScript();
  executeJavaScript(webView, script);
}

void PictureInPictureManager::createScreenshotPictureInPicture(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "Screenshot Picture-in-Picture",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  QString script = generateScreenshotPiPScript();
  executeJavaScript(webView, script);
}

void PictureInPictureManager::exitAllPictureInPicture(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    return;
  }

  QString script = generateExitAllPiPScript();
  executeJavaScript(webView, script);
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
  QString cssContent = loadResourceFile(":/src/features/picture-in-picture/pip.css");
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
  QString jsContent = loadResourceFile(":/src/features/picture-in-picture/pip.js");
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

QString PictureInPictureManager::generateElementPiPScript() const {
  // CSS注入
  QString cssContent = loadResourceFile(":/src/features/picture-in-picture/pip.css");
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
  QString jsContent = loadResourceFile(":/src/features/picture-in-picture/pip.js");
  return cssInjection + jsContent + R"(
        (function() {
            if (window.pipHandler) {
                window.pipHandler.createElementPiP();
            } else {
                console.error('PiP Handler not available for element PiP');
            }
        })();
    )";
}

QString PictureInPictureManager::generatePagePiPScript() const {
  // CSS注入
  QString cssContent = loadResourceFile(":/src/features/picture-in-picture/pip.css");
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
  QString jsContent = loadResourceFile(":/src/features/picture-in-picture/pip.js");
  return cssInjection + jsContent + R"(
        (function() {
            if (window.pipHandler) {
                window.pipHandler.createPagePiP();
            } else {
                console.error('PiP Handler not available for page PiP');
            }
        })();
    )";
}

QString PictureInPictureManager::generateScreenshotPiPScript() const {
  // CSS注入
  QString cssContent = loadResourceFile(":/src/features/picture-in-picture/pip.css");
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
  QString jsContent = loadResourceFile(":/src/features/picture-in-picture/pip.js");
  return cssInjection + jsContent + R"(
        (function() {
            if (window.pipHandler) {
                window.pipHandler.createScreenshotPiP();
            } else {
                console.error('PiP Handler not available for screenshot PiP');
            }
        })();
    )";
}

QString PictureInPictureManager::generateExitAllPiPScript() const {
  // JavaScript読み込み＆実行
  QString jsContent = loadResourceFile(":/src/features/picture-in-picture/pip.js");
  return jsContent + R"(
        (function() {
            if (window.pipHandler) {
                window.pipHandler.exitAllPiP();
            } else {
                console.error('PiP Handler not available for exit all PiP');
            }
        })();
    )";
}

void PictureInPictureManager::executeJavaScript(WebView *webView, const QString &script) {
  if (!webView) {
    return;
  }

  webView->page()->runJavaScript(script, [](const QVariant &result) {
    Q_UNUSED(result)
    // JavaScript実行完了時の処理（必要に応じて）
    qDebug() << "JavaScript execution completed";
  });
}

// ネイティブPiP機能の実装
void PictureInPictureManager::createNativeImagePiP(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "画像PiP",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  qDebug() << "Creating native image PiP...";

  // 画像抽出用のJavaScriptを実行
  QString script = generateImageExtractionScript();

  qDebug() << "Executing image extraction script...";

  webView->page()->runJavaScript(script, [this, webView](const QVariant &result) {
    // WebViewの存在確認
    if (!webView) {
      qDebug() << "WebView is null in image PiP callback";
      return;
    }

    qDebug() << "Image extraction callback received, result type:" << result.typeName();
    qDebug() << "Result value:" << result;

    QVariantMap resultMap = result.toMap();

    if (resultMap.contains("success") && resultMap["success"].toBool()) {
      QString imageData = resultMap["imageData"].toString();
      QString title = resultMap["title"].toString();
      QString imageType = resultMap["imageType"].toString();

      qDebug() << "Image extracted successfully:" << title << "Type:" << imageType;
      qDebug() << "Image data length:" << imageData.length();

      if (!imageData.isEmpty()) {
        createPiPFromImageData(imageData, title);
      } else {
        qDebug() << "Image data is empty";
        if (mainWindow) {
          QMessageBox::information(mainWindow, "画像PiP",
                                   "このページには表示可能な画像が見つかりませんでした。");
        }
      }
    } else {
      QString error = resultMap["error"].toString();
      qDebug() << "Image extraction failed:" << error;
      if (mainWindow) {
        QMessageBox::warning(mainWindow, "画像PiP",
                             "画像の抽出に失敗しました: " + error);
      }
    }
  });
}

void PictureInPictureManager::createNativeVideoPiP(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "動画PiP",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  qDebug() << "Creating native video PiP...";

  // 動画抽出用のJavaScriptを実行
  QString script = generateVideoExtractionScript();

  webView->page()->runJavaScript(script, [this, webView](const QVariant &result) {
    // WebViewの存在確認
    if (!webView) {
      return;
    }

    QVariantMap resultMap = result.toMap();

    if (resultMap.contains("success") && resultMap["success"].toBool()) {
      QString videoUrl = resultMap["videoUrl"].toString();
      QString title = resultMap["title"].toString();
      QString videoType = resultMap["videoType"].toString();

      qDebug() << "Video extracted successfully:" << title << "URL:" << videoUrl << "Type:" << videoType;

      if (!videoUrl.isEmpty()) {
        createPiPFromVideoUrl(videoUrl, title);
      } else {
        if (mainWindow) {
          QMessageBox::information(mainWindow, "動画PiP",
                                   "このページには再生可能な動画が見つかりませんでした。");
        }
      }
    } else {
      QString error = resultMap["error"].toString();
      if (mainWindow) {
        QMessageBox::warning(mainWindow, "動画PiP",
                             "動画の抽出に失敗しました: " + error);
      }
    }
  });
}

void PictureInPictureManager::createNativeElementPiP(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "要素PiP",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  qDebug() << "Creating native element PiP...";

  // 要素選択用のJavaScriptを実行
  QString script = generateElementSelectionScript();

  webView->page()->runJavaScript(script, [this, webView](const QVariant &result) {
    // WebViewの存在確認
    if (!webView) {
      return;
    }

    QVariantMap resultMap = result.toMap();

    if (resultMap.contains("success") && resultMap["success"].toBool()) {
      QString elementHtml = resultMap["elementHtml"].toString();
      QString title = resultMap["title"].toString();
      QString elementType = resultMap["elementType"].toString();

      qDebug() << "Element extracted successfully:" << title << "Type:" << elementType;

      if (!elementHtml.isEmpty()) {
        createPiPFromElementHtml(elementHtml, title);
      } else {
        if (mainWindow) {
          QMessageBox::information(mainWindow, "要素PiP",
                                   "要素の選択がキャンセルされました。");
        }
      }
    } else {
      QString error = resultMap["error"].toString();
      if (error != "cancelled" && mainWindow) { // ユーザーキャンセルの場合はエラーメッセージを表示しない
        QMessageBox::warning(mainWindow, "要素PiP",
                             "要素の抽出に失敗しました: " + error);
      }
    }
  });
}

void PictureInPictureManager::closeAllNativePiP() {
  qDebug() << "Closing all native PiP windows...";

  // リストのコピーを作成してからクリア（削除処理中にリストが変更されるのを防ぐ）
  QList<PiPWindow *> windowsToClose = activePiPWindows;
  activePiPWindows.clear();

  for (PiPWindow *window : windowsToClose) {
    if (window) {
      window->closeWindow();
      // window->deleteLater(); // closeWindow内で呼び出されるため不要
    }
  }

  qDebug() << "All native PiP windows closed";
}

// ネイティブPiP用ヘルパーメソッド
void PictureInPictureManager::createPiPFromImageData(const QString &imageData, const QString &title) {
  qDebug() << "Creating PiP window from image data...";

  // Base64データからQPixmapを作成
  QByteArray imageBytes = QByteArray::fromBase64(imageData.split(',').last().toUtf8());
  QPixmap pixmap;

  if (pixmap.loadFromData(imageBytes)) {
    PiPWindow *pipWindow = new PiPWindow(mainWindow);
    pipWindow->displayImage(pixmap, title.isEmpty() ? "画像 - Picture-in-Picture" : title);

    activePiPWindows.append(pipWindow);

    // ウィンドウが閉じられた時の処理
    connect(pipWindow, &QObject::destroyed, this, [this, pipWindow]() {
      activePiPWindows.removeAll(pipWindow);
      qDebug() << "Image PiP window removed from active list";
    });

    qDebug() << "Image PiP window created successfully";
  } else {
    QMessageBox::warning(mainWindow, "画像PiP",
                         "画像データの読み込みに失敗しました。");
  }
}

void PictureInPictureManager::createPiPFromVideoUrl(const QString &videoUrl, const QString &title) {
  qDebug() << "Creating PiP window from video URL...";

  PiPWindow *pipWindow = new PiPWindow(mainWindow);
  pipWindow->displayVideo(videoUrl, title.isEmpty() ? "動画 - Picture-in-Picture" : title);

  activePiPWindows.append(pipWindow);

  // ウィンドウが閉じられた時の処理
  connect(pipWindow, &QObject::destroyed, this, [this, pipWindow]() {
    activePiPWindows.removeAll(pipWindow);
    qDebug() << "Video PiP window removed from active list";
  });

  qDebug() << "Video PiP window created successfully";
}

void PictureInPictureManager::createPiPFromElementHtml(const QString &elementHtml, const QString &title) {
  qDebug() << "Creating PiP window from element HTML...";

  PiPWindow *pipWindow = new PiPWindow(mainWindow);
  pipWindow->displayElement(elementHtml, title.isEmpty() ? "要素 - Picture-in-Picture" : title);

  activePiPWindows.append(pipWindow);

  // ウィンドウが閉じられた時の処理
  connect(pipWindow, &QObject::destroyed, this, [this, pipWindow]() {
    activePiPWindows.removeAll(pipWindow);
    qDebug() << "Element PiP window removed from active list";
  });

  qDebug() << "Element PiP window created successfully";
}

void PictureInPictureManager::cleanupClosedPiPWindows() {
  // 既に削除されたウィンドウをリストから除去
  for (int i = activePiPWindows.size() - 1; i >= 0; --i) {
    if (!activePiPWindows[i] || activePiPWindows[i]->isVisible() == false) {
      activePiPWindows.removeAt(i);
    }
  }
}

// ネイティブPiP用JavaScript生成メソッド
QString PictureInPictureManager::generateImageExtractionScript() const {
  return R"(
    (function() {
      try {
        console.log('=== 画像抽出開始 ===');

        // ページ内のすべての画像要素を取得
        const images = document.querySelectorAll('img');
        console.log('見つかった画像の数:', images.length);

        if (images.length === 0) {
          return {
            success: false,
            error: 'このページには画像が見つかりませんでした'
          };
        }

        // 最大の画像を見つける（面積で比較）
        let largestImage = null;
        let largestArea = 0;

        for (const img of images) {
          // 表示されていて、サイズが有効な画像のみを対象
          if (img.offsetWidth > 0 && img.offsetHeight > 0 && img.src) {
            const area = img.offsetWidth * img.offsetHeight;
            if (area > largestArea && area > 10000) { // 最小サイズ制限（100x100）
              largestArea = area;
              largestImage = img;
            }
          }
        }

        if (!largestImage) {
          // サイズ制限なしで再検索
          for (const img of images) {
            if (img.src && img.complete) {
              largestImage = img;
              break;
            }
          }
        }

        if (!largestImage) {
          return {
            success: false,
            error: '表示可能な画像が見つかりませんでした'
          };
        }

        console.log('選択された画像:', largestImage.src, 'サイズ:', largestImage.offsetWidth + 'x' + largestImage.offsetHeight);

        // Canvasに画像を描画してBase64データを取得
        const canvas = document.createElement('canvas');
        const ctx = canvas.getContext('2d');

        // 画像の自然なサイズを使用
        canvas.width = largestImage.naturalWidth || largestImage.offsetWidth || 400;
        canvas.height = largestImage.naturalHeight || largestImage.offsetHeight || 300;

        // 画像が読み込まれていない場合は元のsrcを返す
        if (!largestImage.complete || largestImage.naturalWidth === 0) {
          return {
            success: true,
            imageData: largestImage.src,
            title: largestImage.alt || largestImage.title || document.title || '画像',
            imageType: 'url'
          };
        }

        try {
          ctx.drawImage(largestImage, 0, 0);
          const imageData = canvas.toDataURL('image/png');

          return {
            success: true,
            imageData: imageData,
            title: largestImage.alt || largestImage.title || document.title || '画像',
            imageType: 'base64'
          };
        } catch (canvasError) {
          console.log('Canvas描画エラー、元のURLを使用:', canvasError);
          return {
            success: true,
            imageData: largestImage.src,
            title: largestImage.alt || largestImage.title || document.title || '画像',
            imageType: 'url'
          };
        }

      } catch (error) {
        console.error('画像抽出エラー:', error);
        return {
          success: false,
          error: error.message
        };
      }
    })();
  )";
}

QString PictureInPictureManager::generateVideoExtractionScript() const {
  return R"(
    (function() {
      try {
        console.log('=== 動画抽出開始 ===');

        // ページ内のすべての動画要素を取得
        const videos = document.querySelectorAll('video');
        console.log('見つかった動画の数:', videos.length);

        if (videos.length === 0) {
          return {
            success: false,
            error: 'このページには動画が見つかりませんでした'
          };
        }

        // 再生中または再生可能な動画を優先的に選択
        let selectedVideo = null;

        // 1. 再生中の動画を探す
        for (const video of videos) {
          if (!video.paused && video.readyState >= 2) {
            selectedVideo = video;
            break;
          }
        }

        // 2. 再生可能な動画を探す
        if (!selectedVideo) {
          for (const video of videos) {
            if (video.readyState >= 2 && video.src) {
              selectedVideo = video;
              break;
            }
          }
        }

        // 3. 最初の動画を選択
        if (!selectedVideo) {
          selectedVideo = videos[0];
        }

        console.log('選択された動画:', selectedVideo.src || selectedVideo.currentSrc);

        // 動画のURLを取得
        let videoUrl = selectedVideo.src || selectedVideo.currentSrc;

        // srcがない場合はsource要素から取得
        if (!videoUrl) {
          const sources = selectedVideo.querySelectorAll('source');
          if (sources.length > 0) {
            videoUrl = sources[0].src;
          }
        }

        if (!videoUrl) {
          return {
            success: false,
            error: '動画のURLが取得できませんでした'
          };
        }

        // 相対URLを絶対URLに変換
        if (videoUrl.startsWith('/') || videoUrl.startsWith('./')) {
          const baseUrl = window.location.origin;
          videoUrl = new URL(videoUrl, baseUrl).href;
        }

        const title = selectedVideo.title ||
                     selectedVideo.getAttribute('data-title') ||
                     document.title ||
                     '動画';

        return {
          success: true,
          videoUrl: videoUrl,
          title: title,
          videoType: selectedVideo.tagName.toLowerCase(),
          duration: selectedVideo.duration || 0,
          currentTime: selectedVideo.currentTime || 0,
          paused: selectedVideo.paused
        };

      } catch (error) {
        console.error('動画抽出エラー:', error);
        return {
          success: false,
          error: error.message
        };
      }
    })();
  )";
}

QString PictureInPictureManager::generateElementSelectionScript() const {
  return R"(
    (function() {
      try {
        console.log('=== 要素選択開始 ===');

        return new Promise((resolve) => {
          let selectedElement = null;
          let isSelecting = true;

          // オーバーレイを作成
          const overlay = document.createElement('div');
          overlay.id = 'pip-element-selector-overlay';
          overlay.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.3);
            z-index: 999998;
            cursor: crosshair;
          `;

          // 指示メッセージを作成
          const instructions = document.createElement('div');
          instructions.style.cssText = `
            position: fixed;
            top: 20px;
            left: 50%;
            transform: translateX(-50%);
            background: rgba(0, 0, 0, 0.9);
            color: white;
            padding: 15px 25px;
            border-radius: 8px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            font-size: 14px;
            z-index: 999999;
            text-align: center;
            box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
          `;
          instructions.innerHTML = `
            <div style="font-weight: bold; margin-bottom: 8px;">🎯 要素選択モード</div>
            <div>PiPで表示したい要素をクリックしてください</div>
            <div style="font-size: 12px; opacity: 0.8; margin-top: 8px;">ESCキーでキャンセル</div>
          `;

          let highlightedElement = null;
          const originalOutlines = new Map();

          // 要素をハイライト
          const highlightElement = (element) => {
            if (highlightedElement && highlightedElement !== element) {
              const originalOutline = originalOutlines.get(highlightedElement);
              if (originalOutline !== undefined) {
                highlightedElement.style.outline = originalOutline;
              } else {
                highlightedElement.style.removeProperty('outline');
              }
            }

            if (element && element !== overlay && element !== instructions) {
              highlightedElement = element;
              originalOutlines.set(element, element.style.outline || '');
              element.style.outline = '3px solid #007ACC';
              element.style.outlineOffset = '2px';
            }
          };

          // クリーンアップ処理
          const cleanup = () => {
            console.log('要素選択クリーンアップ実行');
            isSelecting = false;
            document.removeEventListener('mouseover', onMouseOver);
            document.removeEventListener('click', onClick);
            document.removeEventListener('keydown', onKeyDown);

            if (highlightedElement) {
              const originalOutline = originalOutlines.get(highlightedElement);
              if (originalOutline !== undefined) {
                highlightedElement.style.outline = originalOutline;
              } else {
                highlightedElement.style.removeProperty('outline');
              }
            }

            if (overlay.parentNode) {
              overlay.remove();
            }
          };

          // イベントハンドラー
          const onMouseOver = (e) => {
            if (!isSelecting) return;
            e.stopPropagation();
            highlightElement(e.target);
          };

          const onClick = (e) => {
            if (!isSelecting) return;
            e.preventDefault();
            e.stopPropagation();

            if (e.target === overlay || e.target === instructions) {
              return;
            }

            console.log('要素が選択されました:', e.target);
            selectedElement = e.target;
            cleanup();

            // 要素のHTMLを取得
            const elementHtml = selectedElement.outerHTML;
            const elementInfo = getElementInfo(selectedElement);

            console.log('要素データ生成完了:', elementInfo);

            resolve({
              success: true,
              elementHtml: elementHtml,
              title: elementInfo,
              elementType: selectedElement.tagName.toLowerCase()
            });
          };

          const onKeyDown = (e) => {
            if (e.key === 'Escape') {
              console.log('要素選択がキャンセルされました');
              cleanup();
              resolve({
                success: false,
                error: 'cancelled'
              });
            }
          };

          // 要素情報を取得
          const getElementInfo = (element) => {
            const tagName = element.tagName.toLowerCase();
            const className = element.className ? '.' + element.className.split(' ').join('.') : '';
            const id = element.id ? '#' + element.id : '';
            const textContent = element.textContent ? element.textContent.substring(0, 30) + '...' : '';

            if (element.tagName === 'IMG') {
              return `画像 (${element.alt || element.src.split('/').pop() || 'image'})`;
            } else if (element.tagName === 'VIDEO') {
              return `動画 (${element.title || 'video'})`;
            } else if (element.tagName === 'IFRAME') {
              return `フレーム (${element.title || element.src || 'iframe'})`;
            } else if (textContent) {
              return `${tagName}${id}${className} - "${textContent}"`;
            } else {
              return `${tagName}${id}${className}`;
            }
          };

          // イベントリスナーを追加
          document.addEventListener('mouseover', onMouseOver);
          document.addEventListener('click', onClick);
          document.addEventListener('keydown', onKeyDown);

          // オーバーレイを表示
          overlay.appendChild(instructions);
          document.body.appendChild(overlay);

          console.log('要素選択モードが開始されました');

          // 30秒後にタイムアウト（時間を延長）
          setTimeout(() => {
            if (isSelecting) {
              console.log('要素選択がタイムアウトしました');
              cleanup();
              resolve({
                success: false,
                error: 'タイムアウト - 30秒以内に要素を選択してください'
              });
            }
          }, 30000);
        });

      } catch (error) {
        console.error('要素選択エラー:', error);
        return {
          success: false,
          error: error.message
        };
      }
    })();
  )";
}

// スタンドアロンPiP機能の実装
void PictureInPictureManager::createStandalonePiP(const QString &imageUrl, const QString &title) {
  qDebug() << "Creating standalone PiP with image:" << imageUrl;

  // 既存のスタンドアロンプロセスがあれば終了
  if (standaloneProcess && standaloneProcess->state() != QProcess::NotRunning) {
    standaloneProcess->terminate();
    if (!standaloneProcess->waitForFinished(3000)) {
      standaloneProcess->kill();
    }
  }

  // 新しいプロセスを作成
  if (!standaloneProcess) {
    standaloneProcess = new QProcess(this);
    connect(standaloneProcess, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
            this, &PictureInPictureManager::onStandaloneProcessFinished);
    connect(standaloneProcess, &QProcess::errorOccurred,
            this, &PictureInPictureManager::onStandaloneProcessError);
  }

  // 実行ファイルのパスを取得
  QString executablePath = QCoreApplication::applicationDirPath() + "/PiPStandalone";

  QStringList arguments;
  arguments << "--image" << imageUrl;
  arguments << "--title" << title;
  arguments << "--server-name" << "pip_ipc_server";

  qDebug() << "Starting standalone PiP process:" << executablePath << arguments;

  standaloneProcess->start(executablePath, arguments);

  if (!standaloneProcess->waitForStarted(5000)) {
    qWarning() << "Failed to start standalone PiP process:" << standaloneProcess->errorString();
    QMessageBox::warning(mainWindow, "Standalone PiP Error",
                         QString("スタンドアロンPiPプロセスの開始に失敗しました: %1")
                             .arg(standaloneProcess->errorString()));
    return;
  }

  qDebug() << "Standalone PiP process started successfully";

  // IPCサーバーを開始
  setupIpcServer();
}

void PictureInPictureManager::setupIpcServer() {
  if (!ipcServer) {
    ipcServer = new QLocalServer(this);
    connect(ipcServer, &QLocalServer::newConnection,
            this, &PictureInPictureManager::onIpcConnection);
  }

  // 既存のサーバーがあれば削除
  QLocalServer::removeServer("pip_ipc_server");

  if (!ipcServer->listen("pip_ipc_server")) {
    qWarning() << "Failed to start IPC server:" << ipcServer->errorString();
    return;
  }

  qDebug() << "IPC server started successfully";
}

void PictureInPictureManager::onIpcConnection() {
  QLocalSocket *socket = ipcServer->nextPendingConnection();
  if (!socket)
    return;

  connect(socket, &QLocalSocket::readyRead, [this, socket]() {
    QByteArray data = socket->readAll();
    QString message = QString::fromUtf8(data);
    qDebug() << "Received IPC message:" << message;

    // メッセージの処理
    if (message == "pip_closed") {
      qDebug() << "Standalone PiP window was closed";
      emit standalonePiPClosed();
    }
  });

  connect(socket, &QLocalSocket::disconnected, socket, &QLocalSocket::deleteLater);

  qDebug() << "IPC connection established";
}

void PictureInPictureManager::onStandaloneProcessFinished(int exitCode, QProcess::ExitStatus exitStatus) {
  qDebug() << "Standalone PiP process finished with code:" << exitCode << "status:" << exitStatus;

  if (exitStatus == QProcess::CrashExit) {
    qWarning() << "Standalone PiP process crashed";
  }

  // プロセスの参照をクリア
  if (standaloneProcess) {
    standaloneProcess->deleteLater();
    standaloneProcess = nullptr;
  }

  // IPCサーバーを停止
  if (ipcServer) {
    ipcServer->close();
  }
}

void PictureInPictureManager::onStandaloneProcessError(QProcess::ProcessError error) {
  QString errorString;
  switch (error) {
  case QProcess::FailedToStart:
    errorString = "プロセスの開始に失敗しました";
    break;
  case QProcess::Crashed:
    errorString = "プロセスがクラッシュしました";
    break;
  case QProcess::Timedout:
    errorString = "プロセスがタイムアウトしました";
    break;
  case QProcess::WriteError:
    errorString = "書き込みエラーが発生しました";
    break;
  case QProcess::ReadError:
    errorString = "読み込みエラーが発生しました";
    break;
  default:
    errorString = "不明なエラーが発生しました";
    break;
  }

  qWarning() << "Standalone PiP process error:" << errorString;
  QMessageBox::warning(mainWindow, "Standalone PiP Error",
                       QString("スタンドアロンPiPプロセスでエラーが発生しました: %1").arg(errorString));
}

void PictureInPictureManager::closeStandalonePiP() {
  if (standaloneProcess && standaloneProcess->state() != QProcess::NotRunning) {
    standaloneProcess->terminate();
    if (!standaloneProcess->waitForFinished(3000)) {
      standaloneProcess->kill();
    }
  }
}

// 画像PiP用のスタンドアロン機能
void PictureInPictureManager::createStandaloneImagePiP(WebView *webView) {
  if (!webView && mainWindow) {
    webView = mainWindow->currentWebView();
  }

  if (!webView) {
    QMessageBox::warning(mainWindow, "Standalone Image PiP",
                         "アクティブなWebビューが見つかりません。");
    return;
  }

  // 画像要素の選択と情報取得のJavaScript
  QString script = R"(
        (async function() {
            console.log('スタンドアロン画像PiP選択開始');

            return new Promise((resolve) => {
                let isSelecting = true;
                let overlay = null;

                const cleanup = () => {
                    isSelecting = false;
                    document.removeEventListener('mouseover', onMouseOver);
                    document.removeEventListener('click', onClick);
                    document.removeEventListener('keydown', onKeyDown);
                    if (overlay && overlay.parentNode) {
                        overlay.parentNode.removeChild(overlay);
                    }
                };

                const onMouseOver = (e) => {
                    if (!isSelecting) return;

                    const element = e.target;
                    if (element.tagName === 'IMG') {
                        element.style.outline = '3px solid #007AFF';
                    }
                };

                const onClick = (e) => {
                    if (!isSelecting) return;

                    e.preventDefault();
                    e.stopPropagation();

                    const element = e.target;
                    if (element.tagName !== 'IMG') {
                        console.log('画像以外の要素が選択されました');
                        return;
                    }

                    cleanup();

                    console.log('画像が選択されました:', element.src);

                    resolve({
                        success: true,
                        imageUrl: element.src,
                        title: element.alt || element.title || 'Image PiP',
                        width: element.naturalWidth || element.width,
                        height: element.naturalHeight || element.height
                    });
                };

                const onKeyDown = (e) => {
                    if (e.key === 'Escape') {
                        cleanup();
                        resolve({
                            success: false,
                            error: 'cancelled'
                        });
                    }
                };

                // オーバーレイの作成
                overlay = document.createElement('div');
                overlay.style.cssText = `
                    position: fixed;
                    top: 0;
                    left: 0;
                    width: 100%;
                    height: 100%;
                    background: rgba(0, 0, 0, 0.5);
                    z-index: 10000;
                    display: flex;
                    align-items: center;
                    justify-content: center;
                    pointer-events: none;
                `;

                const instructions = document.createElement('div');
                instructions.style.cssText = `
                    background: white;
                    padding: 20px;
                    border-radius: 8px;
                    text-align: center;
                    font-family: system-ui;
                    box-shadow: 0 4px 20px rgba(0, 0, 0, 0.3);
                `;
                instructions.innerHTML = `
                    <h3 style="margin: 0 0 10px 0; color: #333;">スタンドアロン画像PiP</h3>
                    <p style="margin: 0; color: #666;">画像をクリックして別プロセスでPiPウィンドウを開きます<br>ESCキーでキャンセル</p>
                `;

                overlay.appendChild(instructions);
                document.body.appendChild(overlay);

                document.addEventListener('mouseover', onMouseOver);
                document.addEventListener('click', onClick);
                document.addEventListener('keydown', onKeyDown);

                setTimeout(() => {
                    if (isSelecting) {
                        cleanup();
                        resolve({
                            success: false,
                            error: 'タイムアウト'
                        });
                    }
                }, 30000);
            });
        })();
    )";

  webView->page()->runJavaScript(script, [this](const QVariant &result) {
    QVariantMap resultMap = result.toMap();

    if (resultMap["success"].toBool()) {
      QString imageUrl = resultMap["imageUrl"].toString();
      QString title = resultMap["title"].toString();

      qDebug() << "Selected image for standalone PiP:" << imageUrl << "title:" << title;

      // スタンドアロンPiPを作成
      createStandalonePiP(imageUrl, title);
    } else {
      QString error = resultMap["error"].toString();
      if (error != "cancelled") {
        QMessageBox::information(mainWindow, "Standalone Image PiP",
                                 QString("画像の選択に失敗しました: %1").arg(error));
      }
    }
  });
}
