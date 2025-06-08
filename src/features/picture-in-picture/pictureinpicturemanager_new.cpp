#include "../webview/webview.h"
#include "macospipwindow.h"
#include "pictureinpicturemanager.h"
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
    WebView *currentView = nullptr; // MainWindowから取得する必要がある
    createImagePiP(currentView);
  }
}

QString PictureInPictureManager::generateImageExtractionScript() const {
  return R"(
(function() {
    // ページ内のすべての画像を取得
    const images = document.querySelectorAll('img');
    const hoveredImages = [];

    // マウスが乗っている画像を探す
    images.forEach(img => {
        const rect = img.getBoundingClientRect();
        // 簡単な判定：画像が表示されているかチェック
        if (rect.width > 50 && rect.height > 50 && img.complete && img.naturalWidth > 0) {
            hoveredImages.push({
                src: img.src,
                alt: img.alt || img.title || 'Image',
                width: rect.width,
                height: rect.height
            });
        }
    });

    // 最初の適切な画像を選択
    if (hoveredImages.length > 0) {
        const selectedImage = hoveredImages[0];

        // カスタムイベントでC++側に画像情報を送信
        const event = new CustomEvent('pipImageSelected', {
            detail: {
                imageUrl: selectedImage.src,
                title: selectedImage.alt,
                width: selectedImage.width,
                height: selectedImage.height
            }
        });
        document.dispatchEvent(event);

        console.log('PiP: Image selected for PiP', selectedImage);
        return true;
    } else {
        console.log('PiP: No suitable images found');
        alert('No suitable images found for Picture-in-Picture');
        return false;
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
  });
}

void PictureInPictureManager::createPiPFromImageData(const QString &imageUrl, const QString &title) {
  MacOSPiPWindow *pipWindow = new MacOSPiPWindow();

  // 画像URLから画像をロード（簡略化のため、今はプレースホルダー）
  pipWindow->showImageFromUrl(imageUrl, title);

  activePiPWindows.append(pipWindow);

  // ウィンドウが閉じられたときのクリーンアップ
  connect(pipWindow, &QWidget::destroyed, this, [this, pipWindow]() {
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
