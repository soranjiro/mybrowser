#include "macospipwindow.h"
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QPushButton>

#ifdef Q_OS_MACOS
#include <AppKit/AppKit.h>
#endif

MacOSPiPWindow::MacOSPiPWindow(QWidget *parent)
    : QWidget(parent), isDragging(false) {
  setupUI();
  setupMacOSBehavior();
}

MacOSPiPWindow::~MacOSPiPWindow() {
}

void MacOSPiPWindow::setupUI() {
  // ウィンドウフラグを設定（常に最前面、フレームレス）
  setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Tool);
  setAttribute(Qt::WA_TranslucentBackground);

  // レイアウト作成
  layout = new QVBoxLayout(this);
  layout->setContentsMargins(5, 5, 5, 5);
  layout->setSpacing(5);

  // 画像表示用ラベル
  imageLabel = new QLabel(this);
  imageLabel->setAlignment(Qt::AlignCenter);
  imageLabel->setStyleSheet(
      "QLabel {"
      "  background-color: rgba(0, 0, 0, 200);"
      "  border: 2px solid #007ACC;"
      "  border-radius: 8px;"
      "  padding: 10px;"
      "}");
  imageLabel->setMinimumSize(200, 150);
  imageLabel->setScaledContents(true);

  // 閉じるボタン
  closeButton = new QPushButton("×", this);
  closeButton->setFixedSize(25, 25);
  closeButton->setStyleSheet(
      "QPushButton {"
      "  background-color: #ff5f57;"
      "  border: none;"
      "  border-radius: 12px;"
      "  color: white;"
      "  font-weight: bold;"
      "  font-size: 12px;"
      "}"
      "QPushButton:hover {"
      "  background-color: #ff3b30;"
      "}");

  // レイアウトに追加
  layout->addWidget(closeButton, 0, Qt::AlignRight);
  layout->addWidget(imageLabel);

  // シグナル接続
  connect(closeButton, &QPushButton::clicked, this, &MacOSPiPWindow::onCloseButtonClicked);

  // 初期サイズ設定
  resize(300, 250);
}

void MacOSPiPWindow::setupMacOSBehavior() {
#ifdef Q_OS_MACOS
  // macOS固有のウィンドウ動作を設定
  NSView *view = reinterpret_cast<NSView *>(winId());
  NSWindow *window = [view window];

  if (window) {
    // すべてのSpacesで表示されるように設定
    [window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                  NSWindowCollectionBehaviorStationary |
                                  NSWindowCollectionBehaviorFullScreenAuxiliary];

    // レベル設定（Picture-in-Picture用の高い優先度）
    [window setLevel:NSPopUpMenuWindowLevel];

    // ウィンドウのタイトルバーを隠す
    [window setTitlebarAppearsTransparent:YES];
    [window setTitleVisibility:NSWindowTitleHidden];
  }

  qDebug() << "macOS PiP window behavior configured";
#endif
}

void MacOSPiPWindow::showImage(const QPixmap &pixmap, const QString &title) {
  if (pixmap.isNull()) {
    qDebug() << "Invalid pixmap provided to PiP window";
    return;
  }

  imageLabel->setPixmap(pixmap);

  if (!title.isEmpty()) {
    setWindowTitle(title);
  }

  show();
  raise();
  activateWindow();

  qDebug() << "PiP window showing image:" << title;
}

void MacOSPiPWindow::showImageFromUrl(const QString &imageUrl, const QString &title) {
  // 後で実装：ネットワークからの画像取得
  qDebug() << "Loading image from URL:" << imageUrl;

  // とりあえずプレースホルダー画像を表示
  QPixmap placeholder(200, 150);
  placeholder.fill(Qt::gray);
  showImage(placeholder, title.isEmpty() ? "Loading..." : title);
}

void MacOSPiPWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    isDragging = true;
    dragStartPosition = event->globalPos() - frameGeometry().topLeft();
    event->accept();
  }
}

void MacOSPiPWindow::mouseMoveEvent(QMouseEvent *event) {
  if (isDragging && (event->buttons() & Qt::LeftButton)) {
    move(event->globalPos() - dragStartPosition);
    event->accept();
  }
}

void MacOSPiPWindow::mouseDoubleClickEvent(QMouseEvent *event) {
  Q_UNUSED(event)
  // ダブルクリックで閉じる
  close();
}

void MacOSPiPWindow::onCloseButtonClicked() {
  close();
}
