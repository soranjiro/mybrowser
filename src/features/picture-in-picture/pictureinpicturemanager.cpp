#include "pictureinpicturemanager.h"
#include "../main-window/mainwindow.h"
#include "../webview/webview.h"
#include "macospipwindow.h"
#include <QAction>
#include <QDebug>
#include <QFile>
#include <QKeySequence>
#include <QMenu>
#include <QPixmap>
#include <QTextStream>
#include <QTimer>

PictureInPictureManager::PictureInPictureManager(MainWindow *parent)
    : QObject(parent), mainWindow(parent), imagePiPAction(nullptr), videoPiPAction(nullptr) {
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

  // Add video PiP action
  videoPiPAction = new QAction("Video Picture-in-Picture", this);
  videoPiPAction->setShortcut(QKeySequence("Ctrl+Alt+V"));
  videoPiPAction->setStatusTip("Open selected video in Picture-in-Picture window");
  videoPiPAction->setToolTip("Video PiP (Ctrl+Alt+V)");

  connect(imagePiPAction, &QAction::triggered, this, &PictureInPictureManager::onImagePiPTriggered);
  connect(videoPiPAction, &QAction::triggered, this, &PictureInPictureManager::onVideoPiPTriggered);
  qDebug() << "PiP actions setup completed";
}

void PictureInPictureManager::addToMenu(QMenu *viewMenu) {
  if (!viewMenu || !imagePiPAction) {
    return;
  }

  viewMenu->addSeparator();
  QMenu *pipMenu = viewMenu->addMenu("Picture-in-Picture");
  pipMenu->addAction(imagePiPAction);
  pipMenu->addAction(videoPiPAction);

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
      createPiPFromImageData("demo://test-image", "Test Image - 画像が見つかりませんでした");
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

void PictureInPictureManager::createVideoPiP(WebView *webView) {
  if (!webView) {
    qDebug() << "No WebView provided for video PiP";
    return;
  }

  connect(webView, &WebView::pipVideoRequested, this, &PictureInPictureManager::createPiPFromVideoData, Qt::UniqueConnection);
  QString script = generateVideoExtractionScript();
  executeVideoJavaScript(webView, script);
}

void PictureInPictureManager::onVideoPiPTriggered() {
  if (mainWindow) {
    WebView *currentView = mainWindow->currentWebView();
    if (currentView) {
      qDebug() << "Video PiP triggered via keyboard shortcut";

      // Add visual feedback script for video
      QString feedbackScript = R"(
        (function() {
          // Create temporary notification
          const notification = document.createElement('div');
          notification.innerHTML = '🎬 PiP動画検索中...';
          notification.style.cssText = `
            position: fixed;
            top: 20px;
            right: 20px;
            background: linear-gradient(135deg, #FF6B6B, #FF8E53);
            color: white;
            padding: 12px 20px;
            border-radius: 8px;
            font-size: 14px;
            font-weight: bold;
            z-index: 999999;
            font-family: -apple-system, BlinkMacSystemFont, sans-serif;
            box-shadow: 0 4px 20px rgba(255, 107, 107, 0.3);
            animation: slideInRight 0.3s ease-out;
          `;

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

      // Then execute video PiP creation after a short delay
      QTimer::singleShot(200, this, [this, currentView]() {
        createVideoPiP(currentView);
      });
    } else {
      qDebug() << "No active WebView found for video PiP";
    }
  }
}

QString PictureInPictureManager::generateVideoExtractionScript() const {
  return R"(
// Load enhanced PiP functionality
(function() {
    // Check if PiP handler already exists
    if (window.pictureInPictureHandler) {
        console.log('PiP handler already exists, attempting video PiP...');
        return window.pictureInPictureHandler.forceVideoStreamingPiP();
    }

    // Load the enhanced PiP script if not loaded
    if (!window.PIP_CONFIG) {
        // Insert the enhanced PiP script
        const script = document.createElement('script');
        script.textContent = `)" +
         getEnhancedPiPScript() + R"(`;
        document.head.appendChild(script);

        // Wait for initialization
        setTimeout(function() {
            if (window.pictureInPictureHandler) {
                console.log('Enhanced PiP handler loaded, attempting video PiP...');
                return window.pictureInPictureHandler.forceVideoStreamingPiP();
            } else {
                console.error('Failed to load enhanced PiP handler');
                return { success: false, message: 'Failed to load enhanced PiP handler' };
            }
        }, 500);
    } else {
        // PiP script is loaded, use it
        if (window.pictureInPictureHandler) {
            console.log('Using existing PiP handler for video PiP...');
            return window.pictureInPictureHandler.forceVideoStreamingPiP();
        }
    }

    // Fallback to basic video detection if enhanced script fails
    console.log('Falling back to basic video detection...');
    let targetVideo = null;

    // Find videos
    const videos = document.querySelectorAll('video');
    let bestVideo = null;
    let maxArea = 0;

    for (let video of videos) {
        const rect = video.getBoundingClientRect();
        if (rect.width >= 100 && rect.height >= 50 && video.readyState >= 2) {
            const area = rect.width * rect.height;
            if (area > maxArea) {
                maxArea = area;
                bestVideo = video;
            }
        }
    }
    targetVideo = bestVideo;

    if (targetVideo) {
        const rect = targetVideo.getBoundingClientRect();
        let videoUrl = targetVideo.src || targetVideo.currentSrc;
        let isVideoWithNativePiPDisabled = targetVideo.hasAttribute('disablepictureinpicture');

        const result = {
            success: true,
            videoUrl: videoUrl,
            isDisabledPiP: isVideoWithNativePiPDisabled,
            title: targetVideo.title || 'Selected Video',
            width: rect.width,
            height: rect.height,
            duration: targetVideo.duration,
            currentTime: targetVideo.currentTime,
            paused: targetVideo.paused,
            readyState: targetVideo.readyState,
            videoWidth: targetVideo.videoWidth,
            videoHeight: targetVideo.videoHeight
        };

        console.log('Basic video PiP extraction result:', result);
        return result;
    }

    return { success: false, message: 'No suitable videos found' };
})();
)";
}

void PictureInPictureManager::executeVideoJavaScript(WebView *webView, const QString &script) {
  if (!webView) {
    qDebug() << "Cannot execute video JavaScript: WebView is null";
    return;
  }

  webView->page()->runJavaScript(script, [this](const QVariant &result) {
    qDebug() << "Video JavaScript execution result received";

    QVariantMap resultMap = result.toMap();
    qDebug() << "JavaScript result map:" << resultMap;

    if (resultMap.contains("success") && resultMap["success"].toBool()) {
      QString title = resultMap["title"].toString();
      QString videoUrl = resultMap["videoUrl"].toString();
      QString videoData = resultMap["videoData"].toString();
      bool isDisabledPiP = resultMap["isDisabledPiP"].toBool();

      qDebug() << "Video PiP extraction successful:";
      qDebug() << "  Title:" << title;
      qDebug() << "  URL:" << videoUrl;
      qDebug() << "  Has VideoData:" << !videoData.isEmpty();
      qDebug() << "  Is Disabled PiP:" << isDisabledPiP;
      qDebug() << "  Width:" << resultMap["width"].toInt();
      qDebug() << "  Height:" << resultMap["height"].toInt();
      qDebug() << "  Duration:" << resultMap["duration"].toDouble();
      qDebug() << "  Ready State:" << resultMap["readyState"].toInt();

      // Handle different types of video data
      if (!videoData.isEmpty()) {
        // We have additional video data (Base64, Blob data, or frame capture)
        qDebug() << "PiP: Processing video with additional data for disablepictureinpicture video";
        if (videoData.startsWith("data:image/")) {
          // This is a captured frame - show as image PiP
          createPiPFromImageData(videoData, title + " (フレームキャプチャ)");
        } else if (videoData.startsWith("data:video/")) {
          // This is Base64 video data
          createPiPFromVideoData(videoData, title);
        } else {
          // Fallback to URL-based approach
          createPiPFromVideoData(videoUrl, title);
        }
      } else if (!videoUrl.isEmpty()) {
        qDebug() << "PiP: Processing video URL:" << videoUrl << "with title:" << title;
        createPiPFromVideoData(videoUrl, title);
      } else {
        qDebug() << "PiP: No video URL or data found in successful result";
        createPiPFromVideoData("demo://test-video", title + " - 動画データが見つかりませんでした");
      }
    } else {
      QString errorMsg = resultMap["message"].toString();
      qDebug() << "PiP: Video JavaScript execution failed or no videos found. Error:" << errorMsg;
      createPiPFromVideoData("demo://test-video", "Test Video - 動画が見つかりませんでした");
    }
  });
}

void PictureInPictureManager::createPiPFromVideoData(const QString &videoData, const QString &title) {
  MacOSPiPWindow *pipWindow = new MacOSPiPWindow();

  // Check if videoData is Base64 or URL
  if (videoData.startsWith("data:video/")) {
    // Base64 video data
    pipWindow->showVideoFromBase64(videoData, title);
  } else {
    // URL or other format
    pipWindow->showVideo(videoData, title);
  }

  activePiPWindows.append(pipWindow);

  // Clean up when window is destroyed
  connect(pipWindow, &MacOSPiPWindow::destroyed, this, [this, pipWindow]() {
    activePiPWindows.removeAll(pipWindow);
  });

  qDebug() << "Video PiP window created for:" << title;
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

QString PictureInPictureManager::getEnhancedPiPScript() const {
  // Read and return the enhanced PiP script
  QString scriptPath = ":/pip_enhanced.js"; // Using Qt resource system
  QFile file(scriptPath);

  if (file.open(QIODevice::ReadOnly)) {
    QTextStream stream(&file);
    return stream.readAll();
  }

  // Fallback: return inline enhanced PiP script if resource not found
  return R"(
// Enhanced Picture-in-Picture JavaScript Functionality
window.PIP_CONFIG = {
  DETECTION_TIMEOUT: 30000,
  RETRY_DELAY: 1000,
  MAX_RETRIES: 5,
  THROTTLE_DELAY: 100,
  DEBOUNCE_DELAY: 300,
  FRAME_CAPTURE_FPS: 15,
  FRAME_CAPTURE_MAX_DURATION: 600000,

  SITE_CONFIGS: {
    'youtube.com': {
      selectors: [
        'ytd-app video',
        'video[src*="googlevideo"]',
        '.ytp-video-container video',
        '#movie_player video',
        '.html5-video-player video',
        'video.video-stream',
        'video[class*="video"]',
        '.ytp-html5-video',
        'video[autoplay]',
        'video:not([width="0"]):not([height="0"])',
        'ytd-player video',
        '.ytd-player-container video',
        'video[poster]',
        'video[controls]',
        'video[preload]',
        '.player-container video',
        'video.html5-main-video',
        'div[id*="player"] video'
      ],
      waitTime: 8000,
      attributes: ['disablepictureinpicture', 'controlslist'],
      customLogic: function(videos) {
        return videos.filter(function(v) { return v.readyState >= 2 && !v.paused; });
      }
    }
  }
};

// Utility functions
window.PiPUtils = class {
  static safeExecute(fn, context, ...args) {
    try {
      return fn.apply(context, args);
    } catch (error) {
      console.error('PiP Safe Execute Error:', error);
      return null;
    }
  }

  static throttle(func, delay) {
    let timeoutId;
    let lastExecTime = 0;
    return function (...args) {
      const currentTime = Date.now();
      if (currentTime - lastExecTime > delay) {
        func.apply(this, args);
        lastExecTime = currentTime;
      } else {
        clearTimeout(timeoutId);
        timeoutId = setTimeout(() => func.apply(this, args), delay);
      }
    };
  }

  static debounce(func, delay) {
    let timeoutId;
    return function (...args) {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => func.apply(this, args), delay);
    };
  }
};

// Main PiP Handler
window.PictureInPictureHandler = class {
  constructor() {
    this.initializeHandler();
  }

  initializeHandler() {
    console.log('Enhanced PiP Handler initialized');
    this.setupPerformanceMonitoring();
  }

  setupPerformanceMonitoring() {
    this.stats = {
      detectionAttempts: 0,
      successfulDetections: 0,
      errors: 0
    };
  }

  forceVideoStreamingPiP() {
    console.log('Starting enhanced video streaming PiP...');
    this.stats.detectionAttempts++;

    const hostname = window.location.hostname;
    const siteConfig = window.PIP_CONFIG.SITE_CONFIGS[hostname] || {};

    // Enhanced video detection with site-specific logic
    const videos = this.detectVideos(siteConfig);

    if (videos.length === 0) {
      console.log('No videos found, trying fallback detection...');
      return this.fallbackVideoDetection();
    }

    // Score and select best video
    const bestVideo = this.selectBestVideo(videos, siteConfig);

    if (bestVideo) {
      this.stats.successfulDetections++;
      return this.processVideoForPiP(bestVideo);
    }

    this.stats.errors++;
    return { success: false, message: 'No suitable video found for PiP' };
  }

  detectVideos(siteConfig) {
    const selectors = siteConfig.selectors || ['video'];
    const videos = [];

    for (const selector of selectors) {
      try {
        const elements = document.querySelectorAll(selector);
        for (const element of elements) {
          if (this.isValidVideo(element)) {
            videos.push(element);
          }
        }
      } catch (error) {
        console.warn('Selector failed:', selector, error);
      }
    }

    return [...new Set(videos)]; // Remove duplicates
  }

  isValidVideo(video) {
    if (!video || video.tagName !== 'VIDEO') return false;

    const rect = video.getBoundingClientRect();
    return rect.width >= 50 && rect.height >= 50 && video.readyState >= 1;
  }

  selectBestVideo(videos, siteConfig) {
    let bestVideo = null;
    let bestScore = 0;

    for (const video of videos) {
      const score = this.scoreVideo(video, siteConfig);
      if (score > bestScore) {
        bestScore = score;
        bestVideo = video;
      }
    }

    return bestVideo;
  }

  scoreVideo(video, siteConfig) {
    let score = 0;
    const rect = video.getBoundingClientRect();

    // Size scoring
    score += Math.min(rect.width * rect.height / 10000, 100);

    // Ready state scoring
    score += video.readyState * 10;

    // Playing state scoring
    if (!video.paused) score += 50;

    // Visibility scoring
    if (rect.top >= 0 && rect.left >= 0) score += 20;

    // Custom logic from site config
    if (siteConfig.customLogic) {
      try {
        const customResult = siteConfig.customLogic([video]);
        if (customResult.length > 0) score += 30;
      } catch (error) {
        console.warn('Custom logic failed:', error);
      }
    }

    return score;
  }

  processVideoForPiP(video) {
    const rect = video.getBoundingClientRect();
    const videoUrl = video.src || video.currentSrc;
    const isDisabledPiP = video.hasAttribute('disablepictureinpicture');

    // Force remove PiP restrictions
    this.forceRemoveDisablePiP(video);

    const result = {
      success: true,
      videoUrl: videoUrl,
      isDisabledPiP: isDisabledPiP,
      title: this.getVideoTitle(video),
      width: rect.width,
      height: rect.height,
      duration: video.duration,
      currentTime: video.currentTime,
      paused: video.paused,
      readyState: video.readyState,
      videoWidth: video.videoWidth,
      videoHeight: video.videoHeight,
      enhancedPiP: true
    };

    console.log('Enhanced PiP processing result:', result);
    return result;
  }

  forceRemoveDisablePiP(video) {
    try {
      // Remove disablepictureinpicture attribute
      video.removeAttribute('disablepictureinpicture');

      // Remove controlslist restrictions
      const controlsList = video.getAttribute('controlslist');
      if (controlsList) {
        const newControlsList = controlsList.replace(/nopip|no-pip/gi, '').trim();
        if (newControlsList) {
          video.setAttribute('controlslist', newControlsList);
        } else {
          video.removeAttribute('controlslist');
        }
      }

      console.log('Successfully removed PiP restrictions from video');
    } catch (error) {
      console.warn('Failed to remove PiP restrictions:', error);
    }
  }

  getVideoTitle(video) {
    return video.title ||
           video.getAttribute('aria-label') ||
           document.title ||
           'Enhanced PiP Video';
  }

  fallbackVideoDetection() {
    console.log('Using fallback video detection...');
    const videos = document.querySelectorAll('video');

    for (const video of videos) {
      if (this.isValidVideo(video)) {
        return this.processVideoForPiP(video);
      }
    }

    return { success: false, message: 'No videos found with fallback detection' };
  }
};

// Initialize the enhanced PiP handler
if (!window.pictureInPictureHandler) {
  window.pictureInPictureHandler = new window.PictureInPictureHandler();
  console.log('Enhanced PiP handler initialized and ready');
}
)";
}
