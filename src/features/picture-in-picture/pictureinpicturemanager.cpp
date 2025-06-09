#include "pictureinpicturemanager.h"
#include "../main-window/mainwindow.h"
#include "../webview/webview.h"
#include "macospipwindow.h"
#include <QAction>
#include <QDebug>
#include <QKeySequence>
#include <QMenu>
#include <QPixmap>
#include <QTimer>

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

  // Add separator and info action
  pipMenu->addSeparator();
  QAction *infoAction = new QAction("✓ Captures actual screen content", this);
  infoAction->setEnabled(false);
  pipMenu->addAction(infoAction);

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
      qDebug() << "Image PiP triggered via keyboard shortcut";

      // Add visual feedback script
      QString feedbackScript = R"(
        (function() {
          // Create temporary notification
          const notification = document.createElement('div');
          notification.innerHTML = '🔍 PiP画像検索中...';
          notification.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            background: linear-gradient(135deg, #007ACC, #0096FF);
            color: white;
            padding: 12px 20px;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
            z-index: 999999;
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            box-shadow: 0 4px 20px rgba(0, 122, 204, 0.3);
            animation: slideInRight 0.3s ease-out;
          `;

          // Add animation styles if not present
          if (!document.querySelector('#pip-notification-styles')) {
            const style = document.createElement('style');
            style.id = 'pip-notification-styles';
            style.textContent = `
              @keyframes slideInRight {
                from { transform: translateX(100%); opacity: 0; }
                to { transform: translateX(0); opacity: 1; }
              }
              @keyframes slideOutRight {
                from { transform: translateX(0); opacity: 1; }
                to { transform: translateX(100%); opacity: 0; }
              }
            `;
            document.head.appendChild(style);
          }

          document.body.appendChild(notification);

          // Remove notification after 2 seconds
          setTimeout(() => {
            notification.style.animation = 'slideOutRight 0.3s ease-in';
            setTimeout(() => {
              if (notification.parentElement) {
                notification.remove();
              }
            }, 300);
          }, 2000);
        })();
      )";

      // Execute feedback script first
      currentView->page()->runJavaScript(feedbackScript);

      // Then execute PiP creation after a short delay
      QTimer::singleShot(200, this, [this, currentView]() {
        createImagePiP(currentView);
      });
    } else {
      qDebug() << "No active WebView found for PiP";
    }
  }
}

QString PictureInPictureManager::generateImageExtractionScript() const {
  return R"(
(function() {
    let targetImage = null;
    let targetContainer = null;

    // Find hovered or focused images first
    const activeImages = document.querySelectorAll('img:hover, img:focus, img:active');
    if (activeImages.length > 0) {
        targetImage = activeImages[0];
        targetContainer = targetImage.closest('.image-item') || targetImage.parentElement;
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
        if (targetImage) {
            targetContainer = targetImage.closest('.image-item') || targetImage.parentElement;
        }
    }

    if (targetImage) {
        try {
            // Create canvas to capture the actual displayed image
            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');

            // Set canvas size to match the displayed image size
            const rect = targetImage.getBoundingClientRect();
            canvas.width = rect.width;
            canvas.height = rect.height;

            // Draw the image as it appears on screen
            ctx.drawImage(targetImage, 0, 0, canvas.width, canvas.height);

            // Get the image data as base64
            const imageData = canvas.toDataURL('image/png');

            // Add PiP overlay to the original image
            addPiPOverlay(targetImage, targetContainer);

            const result = {
                success: true,
                imageData: imageData,
                imageUrl: targetImage.src, // Keep original URL for reference
                title: targetImage.alt || targetImage.title || 'Selected Image',
                width: rect.width,
                height: rect.height,
                naturalWidth: targetImage.naturalWidth,
                naturalHeight: targetImage.naturalHeight,
                containerCapture: targetContainer ? captureContainer(targetContainer) : null
            };

            return result;
        } catch (error) {
            console.error('Error capturing image:', error);
            // Fallback to URL method
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
    }

    return { success: false, message: 'No suitable images found' };

    // Helper function to add PiP overlay
    function addPiPOverlay(image, container) {
        // Remove any existing overlay
        const existingOverlay = image.parentElement.querySelector('.pip-overlay');
        if (existingOverlay) {
            existingOverlay.remove();
        }

        // Create PiP overlay
        const overlay = document.createElement('div');
        overlay.className = 'pip-overlay';
        overlay.innerHTML = '📺 PiP中';
        overlay.style.cssText = `
            position: absolute;
            top: 5px;
            right: 5px;
            background: linear-gradient(135deg, #007ACC, #0096FF);
            color: white;
            padding: 6px 10px;
            border-radius: 6px;
            font-size: 12px;
            font-weight: bold;
            z-index: 1000;
            pointer-events: none;
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            box-shadow: 0 2px 8px rgba(0, 122, 204, 0.3);
            animation: pipPulse 2s ease-in-out infinite;
        `;

        // Add CSS animation for the pulse effect
        if (!document.querySelector('#pip-animation-styles')) {
            const style = document.createElement('style');
            style.id = 'pip-animation-styles';
            style.textContent = `
                @keyframes pipPulse {
                    0%, 100% {
                        opacity: 1;
                        transform: scale(1);
                    }
                    50% {
                        opacity: 0.8;
                        transform: scale(1.05);
                    }
                }
                .pip-overlay {
                    backdrop-filter: blur(5px);
                }
                .pip-border-effect {
                    position: relative;
                    overflow: hidden;
                }
                .pip-border-effect::after {
                    content: '';
                    position: absolute;
                    top: -2px;
                    left: -2px;
                    right: -2px;
                    bottom: -2px;
                    background: linear-gradient(45deg, #007ACC, #0096FF, #007ACC);
                    border-radius: 8px;
                    z-index: -1;
                    animation: pipBorderRotate 3s linear infinite;
                }
                @keyframes pipBorderRotate {
                    0% { transform: rotate(0deg); }
                    100% { transform: rotate(360deg); }
                }
            `;
            document.head.appendChild(style);
        }

        // Make sure parent has relative positioning
        const parent = image.parentElement;
        if (getComputedStyle(parent).position === 'static') {
            parent.style.position = 'relative';
        }

        parent.appendChild(overlay);

        // Add border effect to the image
        image.classList.add('pip-border-effect');
        image.style.outline = '3px solid rgba(0, 122, 204, 0.8)';
        image.style.borderRadius = '6px';
        image.style.boxShadow = '0 0 20px rgba(0, 122, 204, 0.4)';

        // Remove overlay and effects after a longer duration
        const removeEffects = () => {
            if (overlay.parentElement) {
                overlay.remove();
            }
            image.classList.remove('pip-border-effect');
            image.style.outline = '';
            image.style.borderRadius = '';
            image.style.boxShadow = '';
        };

        // Remove after 15 seconds or when image is clicked
        setTimeout(removeEffects, 15000);

        // Also remove on click
        image.addEventListener('click', removeEffects, { once: true });
    }

    // Helper function to capture container with dependencies
    function captureContainer(container) {
        if (!container) return null;

        try {
            const canvas = document.createElement('canvas');
            const ctx = canvas.getContext('2d');
            const rect = container.getBoundingClientRect();

            canvas.width = rect.width;
            canvas.height = rect.height;

            // Fill background
            ctx.fillStyle = getComputedStyle(container).backgroundColor || '#ffffff';
            ctx.fillRect(0, 0, canvas.width, canvas.height);

            // Draw container content (simplified)
            const containerTitle = container.querySelector('.image-overlay, .title, h1, h2, h3');
            if (containerTitle) {
                ctx.fillStyle = '#333';
                ctx.font = '14px -apple-system, BlinkMacSystemFont, sans-serif';
                ctx.fillText(containerTitle.textContent, 10, rect.height - 10);
            }

            return {
                success: true,
                containerData: canvas.toDataURL('image/png'),
                title: containerTitle ? containerTitle.textContent : 'Container',
                width: rect.width,
                height: rect.height
            };
        } catch (error) {
            console.error('Error capturing container:', error);
            return null;
        }
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
    qDebug() << "JavaScript execution result received";

    QVariantMap resultMap = result.toMap();

    if (resultMap.contains("success") && resultMap["success"].toBool()) {
      QString title = resultMap["title"].toString();

      // Check if we have captured image data (Base64)
      if (resultMap.contains("imageData")) {
        QString imageData = resultMap["imageData"].toString();
        qDebug() << "PiP: Processing captured image data for:" << title;
        createPiPFromImageData(imageData, title);

        // Also handle container capture if available
        QVariantMap containerCapture = resultMap["containerCapture"].toMap();
        if (!containerCapture.isEmpty() && containerCapture["success"].toBool()) {
          QString containerData = containerCapture["containerData"].toString();
          QString containerTitle = containerCapture["title"].toString();
          qDebug() << "PiP: Also creating container PiP for:" << containerTitle;
          // Create a secondary PiP window for the container after a short delay
          QTimer::singleShot(1000, this, [this, containerData, containerTitle]() {
            createPiPFromImageData(containerData, containerTitle + " (Container)");
          });
        }
      }
      // Fallback to URL method
      else if (resultMap.contains("imageUrl")) {
        QString imageUrl = resultMap["imageUrl"].toString();
        qDebug() << "PiP: Processing image URL (fallback):" << imageUrl << "with title:" << title;
        createPiPFromImageData(imageUrl, title);
      }
    } else {
      qDebug() << "PiP: JavaScript execution failed or no images found";
      createPiPFromImageData("demo://test-image", "Test Image - No Real Images Found");
    }
  });
}

void PictureInPictureManager::createPiPFromImageData(const QString &imageData, const QString &title) {
  MacOSPiPWindow *pipWindow = new MacOSPiPWindow();

  // Check if imageData is Base64 or URL
  if (imageData.startsWith("data:image/")) {
    // Base64 image data
    pipWindow->showImageFromBase64(imageData, title);
  } else {
    // URL or other format
    pipWindow->showImageFromUrl(imageData, title);
  }

  activePiPWindows.append(pipWindow);

  // Clean up when window is destroyed
  connect(pipWindow, &MacOSPiPWindow::destroyed, this, [this, pipWindow]() {
    activePiPWindows.removeAll(pipWindow);
  });

  qDebug() << "PiP window created for:" << title;
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
