#include "pictureinpicturemanager.h"
#include "../main-window/mainwindow.h"
#include "../webview/webview.h"
#include "macospipwindow.h"
#include <QAction>
#include <QDebug>
#include <QKeySequence>
#include <QMenu>
#include <QPixmap>

PictureInPictureManager::PictureInPictureManager(MainWindow *parent)
    : QObject(parent), mainWindow(parent), imagePiPAction(nullptr) {
  qDebug() << "PictureInPictureManager initialized (simplified version)";
}

PictureInPictureManager::~PictureInPictureManager() {
  closeAllPiP();
}

void PictureInPictureManager::setupActions() {
  // シンプルな画像PiPアクションのみ作成
  imagePiPAction = new QAction("Image Picture-in-Picture", this);
  imagePiPAction->setShortcut(QKeySequence("Ctrl+Alt+I"));
  imagePiPAction->setStatusTip("Open selected image in Picture-in-Picture window (macOS Spaces compatible)");
  imagePiPAction->setToolTip("Image PiP (Ctrl+Alt+I)");

  connect(imagePiPAction, &QAction::triggered, this, &PictureInPictureManager::onImagePiPTriggered);

  qDebug() << "PiP actions setup completed";
}

void PictureInPictureManager::addToMenu(QMenu *viewMenu) {
  if (!viewMenu || !imagePiPAction) {
    return;
  }

  viewMenu->addSeparator();
  QMenu *pipMenu = viewMenu->addMenu("Picture-in-Picture");
  pipMenu->addAction(imagePiPAction);

  QAction *closeAllAction = new QAction("Close All PiP Windows", this);
  closeAllAction->setShortcut(QKeySequence("Ctrl+Alt+X"));
  connect(closeAllAction, &QAction::triggered, this, &PictureInPictureManager::closeAllPiP);
  pipMenu->addAction(closeAllAction);

  qDebug() << "PiP menu items added";
}

void PictureInPictureManager::createImagePiP(WebView *webView) {
  if (!webView) {
    qDebug() << "No WebView provided for image PiP";
    return;
  }

  // WebViewとの接続を設定
  connect(webView, &WebView::pipImageRequested, this, &PictureInPictureManager::createPiPFromImageData, Qt::UniqueConnection);

  QString script = generateImageExtractionScript();
  executeJavaScript(webView, script);
}

void PictureInPictureManager::closeAllPiP() {
  for (MacOSPiPWindow *window : activePiPWindows) {
    if (window) {
      window->close();
      window->deleteLater();
    }
  }
  activePiPWindows.clear();
  qDebug() << "All PiP windows closed";
}

void PictureInPictureManager::onImagePiPTriggered() {
  // メインウィンドウから現在のWebViewを取得
  if (mainWindow) {
    WebView *currentView = mainWindow->currentWebView();
    if (currentView) {
      createImagePiP(currentView);
    } else {
      qDebug() << "No active WebView found for PiP";
    }
  }
}

QString PictureInPictureManager::generateImageExtractionScript() const {
  return R"(
(function() {
    // クリックされた画像、フォーカスされた画像、またはホバーされた画像を優先して検索
    let targetImage = null;

    // 1. 最近クリックされた画像を探す
    const clickedImages = document.querySelectorAll('img:hover, img:focus, img:active');
    if (clickedImages.length > 0) {
        targetImage = clickedImages[0];
    }

    // 2. 画面中央付近の最大の画像を探す
    if (!targetImage) {
        const images = document.querySelectorAll('img');
        let bestImage = null;
        let bestScore = 0;
        const viewport = {
            width: window.innerWidth,
            height: window.innerHeight,
            centerX: window.innerWidth / 2,
            centerY: window.innerHeight / 2
        };

        images.forEach(img => {
            const rect = img.getBoundingClientRect();

            // 画像が有効で表示されているかチェック
            if (rect.width > 50 && rect.height > 50 && img.complete && img.naturalWidth > 0) {
                // 画面中央からの距離を計算
                const centerX = rect.left + rect.width / 2;
                const centerY = rect.top + rect.height / 2;
                const distanceFromCenter = Math.sqrt(
                    Math.pow(centerX - viewport.centerX, 2) +
                    Math.pow(centerY - viewport.centerY, 2)
                );

                // スコア = 画像サイズ ÷ 中央からの距離（中央に近くて大きい画像が高スコア）
                const imageSize = rect.width * rect.height;
                const score = imageSize / (distanceFromCenter + 1);

                if (score > bestScore) {
                    bestScore = score;
                    bestImage = img;
                }
            }
        });

        targetImage = bestImage;
    }

    // 3. 選択された画像の情報を返す
    if (targetImage) {
        const rect = targetImage.getBoundingClientRect();
        const result = {
            success: true,
            imageUrl: targetImage.src,
            title: targetImage.alt || targetImage.title || `Selected Image`,
            width: rect.width,
            height: rect.height,
            naturalWidth: targetImage.naturalWidth,
            naturalHeight: targetImage.naturalHeight
        };

        console.log('PiP: Selected image for PiP', result);

        // 選択された画像を一時的にハイライト
        const originalBorder = targetImage.style.border;
        targetImage.style.border = '3px solid #007ACC';
        setTimeout(() => {
            targetImage.style.border = originalBorder;
        }, 1000);

        return result;
    } else {
        console.log('PiP: No suitable images found');
        alert('No suitable images found for Picture-in-Picture');
        return {
            success: false,
            message: 'No suitable images found'
        };
    }
})();
)";
}

void PictureInPictureManager::executeJavaScript(WebView *webView, const QString &script) {
  if (!webView) {
    qDebug() << "Cannot execute JavaScript: WebView is null";
    return;
  }

  webView->page()->runJavaScript(script, [this](const QVariant &result) {
    qDebug() << "JavaScript execution result:" << result;

    // JavaScript結果をマップとして解析
    QVariantMap resultMap = result.toMap();

    if (resultMap.contains("success") && resultMap["success"].toBool()) {
      QString imageUrl = resultMap["imageUrl"].toString();
      QString title = resultMap["title"].toString();

      qDebug() << "PiP: Processing image URL:" << imageUrl << "with title:" << title;

      // 実際の画像URLでPiPウィンドウを作成
      createPiPFromImageData(imageUrl, title);
    } else {
      qDebug() << "PiP: JavaScript execution failed or no images found";
      // フォールバック：デモ画像を表示
      createPiPFromImageData("demo://test-image", "Test Image - No Real Images Found");
    }
  });
}

void PictureInPictureManager::createPiPFromImageData(const QString &imageUrl, const QString &title) {
  MacOSPiPWindow *pipWindow = new MacOSPiPWindow();

  // 画像URLから画像をロード（簡略化のため、今はプレースホルダー）
  pipWindow->showImageFromUrl(imageUrl, title);

  activePiPWindows.append(pipWindow);

  // ウィンドウが閉じられたときのクリーンアップ
  connect(pipWindow, &MacOSPiPWindow::destroyed, this, [this, pipWindow]() {
    activePiPWindows.removeAll(pipWindow);
  });

  qDebug() << "PiP window created for image:" << title;
}

void PictureInPictureManager::cleanupClosedPiPWindows() {
  auto it = activePiPWindows.begin();
  while (it != activePiPWindows.end()) {
    if (!*it || (*it)->isHidden()) {
      it = activePiPWindows.erase(it);
    } else {
      ++it;
    }
  }
}
