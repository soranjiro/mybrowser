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
  qDebug() << "PictureInPictureManager initialized";
}

PictureInPictureManager::~PictureInPictureManager() {
  closeAllPiP();
}

void PictureInPictureManager::setupActions() {
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
    let targetImage = null;

    // Find hovered or focused images first
    const activeImages = document.querySelectorAll('img:hover, img:focus, img:active');
    if (activeImages.length > 0) {
        targetImage = activeImages[0];
    }

    // Otherwise find largest visible image
    if (!targetImage) {
        const images = document.querySelectorAll('img');
        let bestImage = null;
        let maxArea = 0;

        for (let img of images) {
            const rect = img.getBoundingClientRect();
            if (rect.width >= 50 && rect.height >= 50 && img.complete) {
                const area = rect.width * rect.height;
                if (area > maxArea) {
                    maxArea = area;
                    bestImage = img;
                }
            }
        }
        targetImage = bestImage;
    }

    if (targetImage) {
        const result = {
            success: true,
            imageUrl: targetImage.src,
            title: targetImage.alt || targetImage.title || 'Selected Image',
            width: targetImage.offsetWidth,
            height: targetImage.offsetHeight,
            naturalWidth: targetImage.naturalWidth,
            naturalHeight: targetImage.naturalHeight
        };

        // Brief highlight
        const border = targetImage.style.border;
        targetImage.style.border = '3px solid #007ACC';
        setTimeout(() => targetImage.style.border = border, 800);

        return result;
    }

    return { success: false, message: 'No suitable images found' };
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

    QVariantMap resultMap = result.toMap();

    if (resultMap.contains("success") && resultMap["success"].toBool()) {
      QString imageUrl = resultMap["imageUrl"].toString();
      QString title = resultMap["title"].toString();

      qDebug() << "PiP: Processing image URL:" << imageUrl << "with title:" << title;
      createPiPFromImageData(imageUrl, title);
    } else {
      qDebug() << "PiP: JavaScript execution failed or no images found";
      createPiPFromImageData("demo://test-image", "Test Image - No Real Images Found");
    }
  });
}

void PictureInPictureManager::createPiPFromImageData(const QString &imageUrl, const QString &title) {
  MacOSPiPWindow *pipWindow = new MacOSPiPWindow();
  pipWindow->showImageFromUrl(imageUrl, title);
  activePiPWindows.append(pipWindow);

  // Clean up when window is destroyed
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
