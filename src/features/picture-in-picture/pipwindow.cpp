#include "pipwindow.h"
#include <QApplication>
#include <QDebug>
#include <QPainter>
#include <QScreen>
#include <QStyleOption>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineSettings>

#ifdef Q_OS_MACOS
#include <QWindow>
#endif

PiPWindow::PiPWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint), mainLayout(nullptr), titleBarLayout(nullptr), controlsLayout(nullptr), titleBar(nullptr), titleLabel(nullptr), minimizeButton(nullptr), closeButton(nullptr), contentWidget(nullptr), contentLayout(nullptr), imageLabel(nullptr), webView(nullptr), videoView(nullptr), currentMediaType(IMAGE), isDragging(false), isMinimized(false), normalSize(400, 300), showAnimation(nullptr), hideAnimation(nullptr), controlsTimer(nullptr), shadowEffect(nullptr) {
  setupUI();
  setupStyles();
  setupAnimations();

  // macOS固有の設定でSpaces間での表示を有効化
  setupMacOSWindowBehavior();

  // 初期位置を画面右上に設定
  QScreen *screen = QApplication::primaryScreen();
  if (screen) {
    QRect screenGeometry = screen->availableGeometry();
    int x = screenGeometry.width() - 420; // ウィンドウ幅 + マージン
    int y = 50;
    move(x, y);
  }

  // 初期サイズ設定
  resize(normalSize);

  qDebug() << "PiPWindow created";
}

PiPWindow::~PiPWindow() {
  qDebug() << "PiPWindow::~PiPWindow() - Starting cleanup";

  // アニメーションを停止
  if (showAnimation) {
    showAnimation->stop();
  }
  if (hideAnimation) {
    hideAnimation->stop();
  }

  // タイマーを停止
  if (controlsTimer) {
    controlsTimer->stop();
  }

  // WebViewのクリーンアップ
  if (webView) {
    webView->stop();
    webView->page()->deleteLater();
    webView->deleteLater();
    webView = nullptr;
  }

  qDebug() << "PiPWindow::~PiPWindow() - Cleanup completed";
}

void PiPWindow::setupUI() {
  // メインレイアウト
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  createTitleBar();
  createContentArea();

  // コントロール表示タイマー
  controlsTimer = new QTimer(this);
  controlsTimer->setSingleShot(true);
  controlsTimer->setInterval(3000); // 3秒後に非表示
  connect(controlsTimer, &QTimer::timeout, this, &PiPWindow::hideControls);

  setLayout(mainLayout);
}

void PiPWindow::createTitleBar() {
  titleBar = new QWidget(this);
  titleBar->setFixedHeight(32);
  titleBar->setObjectName("titleBar");

  titleBarLayout = new QHBoxLayout(titleBar);
  titleBarLayout->setContentsMargins(12, 4, 4, 4);
  titleBarLayout->setSpacing(8);

  // タイトルラベル
  titleLabel = new QLabel("Picture-in-Picture", titleBar);
  titleLabel->setObjectName("titleLabel");
  titleLabel->setStyleSheet("font-weight: bold; color: white; font-size: 12px;");

  // コントロールボタン
  controlsLayout = new QHBoxLayout();
  controlsLayout->setSpacing(4);

  minimizeButton = new QPushButton("−", titleBar);
  minimizeButton->setObjectName("minimizeButton");
  minimizeButton->setFixedSize(20, 20);
  connect(minimizeButton, &QPushButton::clicked, this, &PiPWindow::onMinimizeButtonClicked);

  closeButton = new QPushButton("×", titleBar);
  closeButton->setObjectName("closeButton");
  closeButton->setFixedSize(20, 20);
  connect(closeButton, &QPushButton::clicked, this, &PiPWindow::onCloseButtonClicked);

  controlsLayout->addWidget(minimizeButton);
  controlsLayout->addWidget(closeButton);

  titleBarLayout->addWidget(titleLabel);
  titleBarLayout->addStretch();
  titleBarLayout->addLayout(controlsLayout);

  mainLayout->addWidget(titleBar);
}

void PiPWindow::createContentArea() {
  contentWidget = new QWidget(this);
  contentWidget->setObjectName("contentWidget");

  contentLayout = new QVBoxLayout(contentWidget);
  contentLayout->setContentsMargins(0, 0, 0, 0);
  contentLayout->setSpacing(0);

  // 画像表示用ラベル
  imageLabel = new QLabel(contentWidget);
  imageLabel->setAlignment(Qt::AlignCenter);
  imageLabel->setScaledContents(true);
  imageLabel->setMinimumSize(200, 150);
  imageLabel->hide();

  // WebView（動画・Webコンテンツ用）
  webView = new QWebEngineView(contentWidget);
  webView->setMinimumSize(200, 150);
  webView->hide();

  // 動画専用WebView
  videoView = new QWebEngineView(contentWidget);
  videoView->setMinimumSize(200, 150);
  videoView->hide();

  contentLayout->addWidget(imageLabel);
  contentLayout->addWidget(webView);
  contentLayout->addWidget(videoView);

  mainLayout->addWidget(contentWidget);
}

void PiPWindow::setupStyles() {
  // ウィンドウ全体のスタイル
  setStyleSheet(R"(
        PiPWindow {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #2c3e50, stop: 1 #34495e);
            border: 1px solid #2c3e50;
            border-radius: 12px;
        }

        #titleBar {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #3498db, stop: 1 #2980b9);
            border-radius: 12px 12px 0 0;
            border-bottom: 1px solid #2471a3;
        }

        #titleLabel {
            color: white;
            font-weight: bold;
            font-size: 12px;
        }

        #minimizeButton, #closeButton {
            background: rgba(255, 255, 255, 0.2);
            border: 1px solid rgba(255, 255, 255, 0.3);
            border-radius: 10px;
            color: white;
            font-weight: bold;
            font-size: 14px;
        }

        #minimizeButton:hover {
            background: rgba(255, 193, 7, 0.8);
            color: #000;
        }

        #closeButton:hover {
            background: rgba(220, 53, 69, 0.8);
            color: white;
        }

        #contentWidget {
            background: #1a1a1a;
            border-radius: 0 0 12px 12px;
        }
    )");

  // ドロップシャドウ効果
  shadowEffect = new QGraphicsDropShadowEffect(this);
  shadowEffect->setBlurRadius(20);
  shadowEffect->setColor(QColor(0, 0, 0, 150));
  shadowEffect->setOffset(0, 5);
  setGraphicsEffect(shadowEffect);
}

void PiPWindow::setupAnimations() {
  // 表示アニメーション
  showAnimation = new QPropertyAnimation(this, "windowOpacity", this);
  showAnimation->setDuration(300);
  showAnimation->setStartValue(0.0);
  showAnimation->setEndValue(1.0);

  // 非表示アニメーション
  hideAnimation = new QPropertyAnimation(this, "windowOpacity", this);
  hideAnimation->setDuration(300);
  hideAnimation->setStartValue(1.0);
  hideAnimation->setEndValue(0.0);

  connect(hideAnimation, &QPropertyAnimation::finished, this, &QWidget::hide);
}

void PiPWindow::displayImage(const QPixmap &pixmap, const QString &title) {
  currentMediaType = IMAGE;
  titleLabel->setText(title);

  clearContent();
  imageLabel->setPixmap(pixmap);
  imageLabel->show();

  updateWindowForMediaType();
  showWithAnimation();

  qDebug() << "Displaying image in PiP window:" << title;
}

void PiPWindow::displayVideo(const QString &videoUrl, const QString &title) {
  currentMediaType = VIDEO;
  titleLabel->setText(title);

  clearContent();

  // 動画を埋め込んだHTMLを作成
  QString videoHtml = QString(R"(
        <!DOCTYPE html>
        <html>
        <head>
            <style>
                body {
                    margin: 0;
                    padding: 0;
                    background: black;
                    display: flex;
                    justify-content: center;
                    align-items: center;
                    min-height: 100vh;
                }
                video {
                    width: 100%;
                    height: 100%;
                    object-fit: contain;
                }
            </style>
        </head>
        <body>
            <video controls autoplay>
                <source src="%1" type="video/mp4">
                Your browser does not support the video tag.
            </video>
        </body>
        </html>
    )")
                          .arg(videoUrl);

  videoView->setHtml(videoHtml);
  videoView->show();

  updateWindowForMediaType();
  showWithAnimation();

  qDebug() << "Displaying video in PiP window:" << title << "URL:" << videoUrl;
}

void PiPWindow::displayWebView(const QString &html, const QString &title) {
  currentMediaType = WEBVIEW;
  titleLabel->setText(title);

  clearContent();
  webView->setHtml(html);
  webView->show();

  updateWindowForMediaType();
  showWithAnimation();

  qDebug() << "Displaying web content in PiP window:" << title;
}

void PiPWindow::displayElement(const QString &elementHtml, const QString &title) {
  currentMediaType = WEBVIEW;
  titleLabel->setText(title);

  clearContent();

  // 要素を中央表示するHTMLを作成
  QString wrappedHtml = QString(R"(
        <!DOCTYPE html>
        <html>
        <head>
            <style>
                body {
                    margin: 0;
                    padding: 15px;
                    background: #f5f5f5;
                    display: flex;
                    justify-content: center;
                    align-items: center;
                    min-height: calc(100vh - 30px);
                    font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
                }
                .content {
                    max-width: 100%;
                    max-height: 100%;
                    overflow: auto;
                }
                img, video {
                    max-width: 100%;
                    height: auto;
                }
            </style>
        </head>
        <body>
            <div class="content">
                %1
            </div>
        </body>
        </html>
    )")
                            .arg(elementHtml);

  webView->setHtml(wrappedHtml);
  webView->show();

  updateWindowForMediaType();
  showWithAnimation();

  qDebug() << "Displaying element in PiP window:" << title;
}

void PiPWindow::clearContent() {
  imageLabel->hide();
  webView->hide();
  videoView->hide();
}

void PiPWindow::updateWindowForMediaType() {
  // メディアタイプに応じてウィンドウサイズを調整
  switch (currentMediaType) {
  case IMAGE:
    if (!imageLabel->pixmap().isNull()) {
      QSize pixmapSize = imageLabel->pixmap().size();
      QSize windowSize = pixmapSize.scaled(600, 400, Qt::KeepAspectRatio);
      windowSize.setHeight(windowSize.height() + 32); // タイトルバー分を追加
      resize(windowSize);
    }
    break;
  case VIDEO:
  case WEBVIEW:
    if (normalSize.isValid()) {
      resize(normalSize);
    }
    break;
  }
}

void PiPWindow::setAlwaysOnTop(bool onTop) {
  Qt::WindowFlags flags = windowFlags();
  if (onTop) {
    flags |= Qt::WindowStaysOnTopHint;
  } else {
    flags &= ~Qt::WindowStaysOnTopHint;
  }
  setWindowFlags(flags);
  show(); // フラグ変更後は再表示が必要
}

void PiPWindow::showWithAnimation() {
  if (!isVisible()) {
    show();
    showAnimation->start();
  }
  showControls();
}

void PiPWindow::hideWithAnimation() {
  if (isVisible()) {
    hideAnimation->start();
  }
}

void PiPWindow::toggleMinimize() {
  if (isMinimized) {
    // 復元
    resize(normalSize);
    isMinimized = false;
    minimizeButton->setText("−");
  } else {
    // 最小化
    normalSize = size();
    resize(200, 32); // タイトルバーのみ表示
    isMinimized = true;
    minimizeButton->setText("□");
  }
}

void PiPWindow::closeWindow() {
  hideWithAnimation();
  // 少し遅延してからウィンドウを削除
  QTimer::singleShot(300, this, &QWidget::deleteLater);
}

void PiPWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && titleBar->geometry().contains(event->pos())) {
    isDragging = true;
    dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
    showControls();
    event->accept();
  }
  QWidget::mousePressEvent(event);
}

void PiPWindow::mouseMoveEvent(QMouseEvent *event) {
  if (isDragging && (event->buttons() & Qt::LeftButton)) {
    QPoint newPos = event->globalPosition().toPoint() - dragStartPosition;

    // 画面境界チェック
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
      QRect screenGeometry = screen->availableGeometry();
      QRect windowGeometry = QRect(newPos, size());

      // 画面外に出ないように制限
      if (newPos.x() < screenGeometry.left()) {
        newPos.setX(screenGeometry.left());
      }
      if (newPos.y() < screenGeometry.top()) {
        newPos.setY(screenGeometry.top());
      }
      if (windowGeometry.right() > screenGeometry.right()) {
        newPos.setX(screenGeometry.right() - width());
      }
      if (windowGeometry.bottom() > screenGeometry.bottom()) {
        newPos.setY(screenGeometry.bottom() - height());
      }
    }

    move(newPos);
    event->accept();
  }
  QWidget::mouseMoveEvent(event);
}

void PiPWindow::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    isDragging = false;
    controlsTimer->start(); // コントロール非表示タイマー開始
  }
  QWidget::mouseReleaseEvent(event);
}

void PiPWindow::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);
  if (!isMinimized) {
    normalSize = event->size();
  }
}

void PiPWindow::paintEvent(QPaintEvent *event) {
  // カスタム描画でボーダーラジアスを実現
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QStyleOption opt;
  opt.initFrom(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

  QWidget::paintEvent(event);
}

void PiPWindow::setupMacOSWindowBehavior() {
#ifdef Q_OS_MACOS
  // macOSで全てのSpacesに表示されるようにウィンドウレベルを設定
  setAttribute(Qt::WA_ShowWithoutActivating);

  // より強力なウィンドウフラグを設定
  Qt::WindowFlags flags = Qt::Window |
                          Qt::FramelessWindowHint |
                          Qt::WindowStaysOnTopHint |
                          Qt::WindowDoesNotAcceptFocus;
  setWindowFlags(flags);

  qDebug() << "Initial window flags set:" << flags;

  // Cocoa APIを直接使用せずに、Qtの機能で実現
  if (QWindow *qWindow = windowHandle()) {
    qDebug() << "QWindow obtained successfully";

    // ウィンドウのプロパティを設定
    qWindow->setFlag(Qt::WindowStaysOnTopHint, true);
    qWindow->setFlag(Qt::WindowDoesNotAcceptFocus, true);

    // macOS固有の設定をQtの範囲内で行う
    setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
    setAttribute(Qt::WA_MacShowFocusRect, false);

    qDebug() << "macOS window attributes configured:";
    qDebug() << "  - WA_MacAlwaysShowToolWindow: true";
    qDebug() << "  - WA_MacShowFocusRect: false";
    qDebug() << "  - WindowStaysOnTopHint: true";
    qDebug() << "  - WindowDoesNotAcceptFocus: true";
  } else {
    qDebug() << "Warning: Could not obtain QWindow handle";
  }

  qDebug() << "macOS window behavior configured with Qt APIs for PiP across Spaces";
#else
  // macOS以外では標準的な設定
  qDebug() << "Non-macOS platform, using standard window behavior";
#endif
}

void PiPWindow::onCloseButtonClicked() {
  closeWindow();
}

void PiPWindow::onMinimizeButtonClicked() {
  toggleMinimize();
}

void PiPWindow::showControls() {
  titleBar->show();
  controlsTimer->stop();
  controlsTimer->start();
}

void PiPWindow::hideControls() {
  // マウスがタイトルバー上にある場合は非表示にしない
  QPoint mousePos = mapFromGlobal(QCursor::pos());
  if (!titleBar->geometry().contains(mousePos)) {
    // titleBar->hide(); // 常に表示にする場合はコメントアウト
  }
}

void PiPWindow::updateControlsVisibility() {
  QPoint mousePos = mapFromGlobal(QCursor::pos());
  QRect windowRect = rect();

  if (windowRect.contains(mousePos)) {
    showControls();
  } else {
    controlsTimer->start();
  }
}
