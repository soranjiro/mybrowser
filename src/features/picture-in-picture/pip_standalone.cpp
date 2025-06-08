#include "pip_standalone.h"
#include "pip_standalone_macos.h"
#include <QApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMouseEvent>
#include <QPainter>
#include <QPixmap>
#include <QScreen>
#include <QStyleOption>
#include <QTimer>

const QString PiPStandaloneWindow::SERVER_NAME = "MyBrowser_PiP_IPC";

PiPStandaloneWindow::PiPStandaloneWindow(QWidget *parent)
    : QWidget(parent, Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint), mainLayout(nullptr), titleBarLayout(nullptr), titleBar(nullptr), titleLabel(nullptr), closeButton(nullptr), imageLabel(nullptr), server(nullptr), clientSocket(nullptr), isDragging(false), shadowEffect(nullptr) {
  setupUI();
  setupStyles();
  setupMacOSBehavior();
  setupServer();

  // 初期位置を画面右上に設定
  QScreen *screen = QApplication::primaryScreen();
  if (screen) {
    QRect screenGeometry = screen->availableGeometry();
    int x = screenGeometry.width() - 420;
    int y = 50;
    move(x, y);
  }

  resize(400, 300);

  qDebug() << "PiP Standalone Window created as separate process";
}

PiPStandaloneWindow::~PiPStandaloneWindow() {
  if (server) {
    server->close();
    delete server;
  }
  if (clientSocket) {
    clientSocket->disconnectFromServer();
    delete clientSocket;
  }
  qDebug() << "PiP Standalone Window destroyed";
}

void PiPStandaloneWindow::setupUI() {
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // タイトルバー
  titleBar = new QWidget(this);
  titleBar->setFixedHeight(32);
  titleBar->setObjectName("titleBar");

  titleBarLayout = new QHBoxLayout(titleBar);
  titleBarLayout->setContentsMargins(12, 4, 4, 4);
  titleBarLayout->setSpacing(8);

  titleLabel = new QLabel("Picture-in-Picture (独立プロセス)", titleBar);
  titleLabel->setObjectName("titleLabel");

  closeButton = new QPushButton("×", titleBar);
  closeButton->setObjectName("closeButton");
  closeButton->setFixedSize(20, 20);
  connect(closeButton, &QPushButton::clicked, this, &PiPStandaloneWindow::onCloseButtonClicked);

  titleBarLayout->addWidget(titleLabel);
  titleBarLayout->addStretch();
  titleBarLayout->addWidget(closeButton);

  // 画像表示エリア
  imageLabel = new QLabel(this);
  imageLabel->setAlignment(Qt::AlignCenter);
  imageLabel->setScaledContents(true);
  imageLabel->setMinimumSize(200, 150);
  imageLabel->setText("画像を読み込み中...");
  imageLabel->setStyleSheet("color: white; font-size: 14px;");

  mainLayout->addWidget(titleBar);
  mainLayout->addWidget(imageLabel);

  setLayout(mainLayout);
}

void PiPStandaloneWindow::setupStyles() {
  setStyleSheet(R"(
        PiPStandaloneWindow {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #2c3e50, stop: 1 #34495e);
            border: 1px solid #2c3e50;
            border-radius: 12px;
        }

        #titleBar {
            background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1,
                                      stop: 0 #e74c3c, stop: 1 #c0392b);
            border-radius: 12px 12px 0 0;
            border-bottom: 1px solid #a93226;
        }

        #titleLabel {
            color: white;
            font-weight: bold;
            font-size: 12px;
        }

        #closeButton {
            background: rgba(255, 255, 255, 0.2);
            border: 1px solid rgba(255, 255, 255, 0.3);
            border-radius: 10px;
            color: white;
            font-weight: bold;
            font-size: 14px;
        }

        #closeButton:hover {
            background: rgba(220, 53, 69, 0.8);
            color: white;
        }
    )");

  // ドロップシャドウ効果
  shadowEffect = new QGraphicsDropShadowEffect(this);
  shadowEffect->setBlurRadius(20);
  shadowEffect->setColor(QColor(0, 0, 0, 150));
  shadowEffect->setOffset(0, 5);
  setGraphicsEffect(shadowEffect);
}

void PiPStandaloneWindow::setupMacOSBehavior() {
#ifdef Q_OS_MACOS
  setAttribute(Qt::WA_ShowWithoutActivating);

  // より強力なフラグを設定
  Qt::WindowFlags flags = Qt::Window |
                          Qt::FramelessWindowHint |
                          Qt::WindowStaysOnTopHint |
                          Qt::WindowDoesNotAcceptFocus;
  setWindowFlags(flags);

  // macOS固有の属性
  setAttribute(Qt::WA_MacAlwaysShowToolWindow, true);
  setAttribute(Qt::WA_MacShowFocusRect, false);

  qDebug() << "独立プロセスPiP: macOS固有設定適用（Qt APIベース）";

  // ウィンドウを表示してからObjective-C++でSpaces設定を適用
  QMetaObject::invokeMethod(this, [this]() {
        // ウィンドウが作成されてからObjective-C++関数を呼び出し
        configureWindowForAllSpaces(this);
        makeWindowFloatingAndSticky(this);
        debugWindowState(this);
        qDebug() << "独立プロセスPiP: Objective-C++によるSpaces設定完了"; }, Qt::QueuedConnection);
#endif
}

void PiPStandaloneWindow::setupServer() {
  server = new QLocalServer(this);

  // 既存のサーバーを削除
  QLocalServer::removeServer(SERVER_NAME);

  if (!server->listen(SERVER_NAME)) {
    qDebug() << "サーバー起動失敗:" << server->errorString();
    return;
  }

  connect(server, &QLocalServer::newConnection, this, &PiPStandaloneWindow::onNewConnection);
  qDebug() << "PiPサーバー起動完了:" << SERVER_NAME;
  qDebug() << "サーバーフルネーム:" << server->fullServerName();
  qDebug() << "サーバーアドレス:" << server->serverName();
}

void PiPStandaloneWindow::onNewConnection() {
  if (clientSocket) {
    clientSocket->deleteLater();
  }

  clientSocket = server->nextPendingConnection();
  connect(clientSocket, &QLocalSocket::readyRead, this, &PiPStandaloneWindow::onDataReceived);
  connect(clientSocket, &QLocalSocket::disconnected, [this]() {
    qDebug() << "クライアント接続切断";
    clientSocket = nullptr;
  });

  qDebug() << "新しいクライアント接続確立";
}

void PiPStandaloneWindow::onDataReceived() {
  if (!clientSocket)
    return;

  QByteArray data = clientSocket->readAll();
  qDebug() << "受信データサイズ:" << data.size() << "bytes";

  // JSON形式かQDataStream形式かを判定
  if (data.startsWith('{')) {
    // JSON形式の処理
    processJsonData(data);
  } else {
    // QDataStream形式の処理
    processQDataStreamData(data);
  }
}

void PiPStandaloneWindow::processJsonData(const QByteArray &data) {
  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(data, &error);

  if (error.error != QJsonParseError::NoError) {
    qDebug() << "JSON解析エラー:" << error.errorString();
    return;
  }

  QJsonObject obj = doc.object();
  QString type = obj["type"].toString();

  if (type == "display_image") {
    QString title = obj["title"].toString();
    QString imageDataBase64 = obj["imageData"].toString();

    // Base64デコード
    QByteArray imageData = QByteArray::fromBase64(imageDataBase64.toUtf8());

    displayImage(imageData, title);
    qDebug() << "JSON画像表示要求受信:" << title << "データサイズ:" << imageData.size();
  }
}

void PiPStandaloneWindow::processQDataStreamData(const QByteArray &data) {
  QDataStream stream(data);
  stream.setVersion(QDataStream::Qt_6_0);

  QString command;
  stream >> command;

  if (command == "DISPLAY_IMAGE") {
    QString title;
    QByteArray imageData;
    stream >> title >> imageData;

    displayImage(imageData, title);
    qDebug() << "QDataStream画像表示要求受信:" << title << "データサイズ:" << imageData.size();
  } else if (command == "CLOSE") {
    close();
  }
}

void PiPStandaloneWindow::displayImage(const QByteArray &imageData, const QString &title) {
  titleLabel->setText(title);

  QPixmap pixmap;
  if (pixmap.loadFromData(imageData)) {
    imageLabel->setPixmap(pixmap);

    // ウィンドウサイズを画像に合わせて調整
    QSize pixmapSize = pixmap.size();
    QSize windowSize = pixmapSize.scaled(600, 400, Qt::KeepAspectRatio);
    windowSize.setHeight(windowSize.height() + 32); // タイトルバー分
    resize(windowSize);

    qDebug() << "画像表示完了:" << title << "サイズ:" << windowSize;

#ifdef Q_OS_MACOS
    // 画像表示後にmacOS Spaces設定を再適用
    QTimer::singleShot(200, this, [this]() {
      configureWindowForAllSpaces(this);
      makeWindowFloatingAndSticky(this);
      qDebug() << "displayImage: macOS Spaces設定を再適用しました";
    });
#endif
  } else {
    imageLabel->setText("画像の読み込みに失敗しました");
    qDebug() << "画像読み込み失敗";
  }
}

void PiPStandaloneWindow::mousePressEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton && titleBar->geometry().contains(event->pos())) {
    isDragging = true;
    dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
    event->accept();
  }
  QWidget::mousePressEvent(event);
}

void PiPStandaloneWindow::mouseMoveEvent(QMouseEvent *event) {
  if (isDragging && (event->buttons() & Qt::LeftButton)) {
    QPoint newPos = event->globalPosition().toPoint() - dragStartPosition;

    // 画面境界チェック
    QScreen *screen = QApplication::primaryScreen();
    if (screen) {
      QRect screenGeometry = screen->availableGeometry();
      QRect windowGeometry = QRect(newPos, size());

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

void PiPStandaloneWindow::mouseReleaseEvent(QMouseEvent *event) {
  if (event->button() == Qt::LeftButton) {
    isDragging = false;
  }
  QWidget::mouseReleaseEvent(event);
}

void PiPStandaloneWindow::paintEvent(QPaintEvent *event) {
  QPainter painter(this);
  painter.setRenderHint(QPainter::Antialiasing);

  QStyleOption opt;
  opt.initFrom(this);
  style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

  QWidget::paintEvent(event);
}

void PiPStandaloneWindow::onCloseButtonClicked() {
  close();
}

void PiPStandaloneWindow::showEvent(QShowEvent *event) {
  QWidget::showEvent(event);

#ifdef Q_OS_MACOS
  // ウィンドウが完全に表示された後にObjective-C++の設定を適用
  QTimer::singleShot(100, this, [this]() {
    configureWindowForAllSpaces(this);
    makeWindowFloatingAndSticky(this);
    debugWindowState(this);
    qDebug() << "showEvent: macOS Spaces設定を再適用しました";
  });
#endif
}
