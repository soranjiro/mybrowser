#include "macospipwindow.h"
#include <QPushButton>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QFont>

#ifdef Q_OS_MACOS
#import <Cocoa/Cocoa.h>
#endif

MacOSPiPWindow::MacOSPiPWindow(QWidget *parent)
    : QWidget(parent), isDragging(false), networkManager(nullptr) {
#ifdef Q_OS_MACOS
    pipPanel = nullptr;
#endif

    // Mission Controlで独立したウィンドウとして認識されるように属性を設定
    setAttribute(Qt::WA_QuitOnClose, false); // アプリ終了に影響しない
    setAttribute(Qt::WA_DeleteOnClose, true); // 閉じた時に自動削除

    setupUI();
    setupMacOSBehavior();
}

MacOSPiPWindow::~MacOSPiPWindow() {
#ifdef Q_OS_MACOS
    if (pipPanel) {
        [pipPanel close];
        [pipPanel release];
        pipPanel = nullptr;
    }
#endif
}

void MacOSPiPWindow::setupUI() {
    // Mission Controlで独立したウィンドウとして表示されるようにフラグを変更
    // Qt::Toolを削除し、通常のウィンドウとして設定
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);

    // Mission Controlでの表示を確実にするため、ウィンドウタイトルを設定
    setWindowTitle("MyBrowser - Picture-in-Picture");

    // ウィンドウアイコンを設定（Mission Controlでの識別を向上）
    setWindowIcon(QIcon(":/icons/pip.png")); // リソースが利用できる場合

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
        "}"
    );
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
        "}"
    );

    // レイアウトに追加
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    layout->addWidget(imageLabel);

    // シグナル接続
    connect(closeButton, &QPushButton::clicked, this, &MacOSPiPWindow::onCloseButtonClicked);

    // 初期サイズ設定
    resize(300, 250);
}

void MacOSPiPWindow::setupMacOSBehavior() {
    // macOS特有の設定は showImage() 後に適用するため、ここでは基本設定のみ
    qDebug() << "MacOS PiP window behavior setup completed";
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

    // ウィンドウが表示された後にmacOS設定を適用
    QTimer::singleShot(500, this, [this]() {
        applyMacOSSpacesSettings();
    });

    qDebug() << "PiP window showing image:" << title;
}

void MacOSPiPWindow::applyMacOSSpacesSettings() {
#ifdef Q_OS_MACOS
    NSView *view = reinterpret_cast<NSView *>(winId());
    NSWindow *originalWindow = [view window];

    if (originalWindow && !pipPanel) {
        // 元のウィンドウからNSPanelを作成してフルスクリーンスペース対応
        NSRect frame = [originalWindow frame];

        // nonactivatingPanelスタイルマスクを設定（記事参考）
        NSWindowStyleMask styleMask = NSWindowStyleMaskNonactivatingPanel |
                                     NSWindowStyleMaskTitled |
                                     NSWindowStyleMaskClosable |
                                     NSWindowStyleMaskResizable;

        // NSPanelを作成（フルスクリーンアプリ上でも表示可能）
        pipPanel = [[NSPanel alloc] initWithContentRect:frame
                                               styleMask:styleMask
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];

        // 元のウィンドウのコンテンツビューをパネルに移動
        NSView *contentView = [originalWindow contentView];
        [pipPanel setContentView:contentView];

        // フルスクリーン対応のコレクションビヘイビア設定
        [pipPanel setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                        NSWindowCollectionBehaviorFullScreenAuxiliary |
                                        NSWindowCollectionBehaviorParticipatesInCycle];

        // フルスクリーンアプリ上でも表示されるようにレベルを設定
        [pipPanel setLevel:NSScreenSaverWindowLevel]; // より高いレベルで表示

        // パネルの動作設定
        [pipPanel setHidesOnDeactivate:NO];
        [pipPanel setFloatingPanel:YES];
        [pipPanel setBecomesKeyOnlyIfNeeded:YES];
        [pipPanel setWorksWhenModal:YES]; // モーダルダイアログ上でも動作

        // 透明性とドラッグ設定
        [pipPanel setOpaque:NO];
        [pipPanel setBackgroundColor:[NSColor clearColor]];
        [pipPanel setTitlebarAppearsTransparent:YES];
        [pipPanel setTitleVisibility:NSWindowTitleHidden];
        [pipPanel setMovableByWindowBackground:YES];

        // Mission Controlでの表示設定
        [pipPanel setExcludedFromWindowsMenu:NO];
        [pipPanel setCanHide:NO];
        [pipPanel setIgnoresMouseEvents:NO];

        // 元のウィンドウを隠してパネルを表示
        [originalWindow orderOut:nil];
        [pipPanel makeKeyAndOrderFront:nil];

        qDebug() << "PiP window converted to NSPanel for fullscreen space and Mission Control compatibility";

    } else if (pipPanel) {
        // すでにパネルが存在する場合は前面に表示
        [pipPanel makeKeyAndOrderFront:nil];
        qDebug() << "PiP panel brought to front";

    } else {
        qDebug() << "Warning: Could not get NSWindow handle for PiP panel configuration";
    }
#else
    qDebug() << "macOS-specific settings not available on this platform";
#endif
}

void MacOSPiPWindow::showImageFromUrl(const QString &imageUrl, const QString &title) {
    qDebug() << "Loading image from URL:" << imageUrl;

    // ネットワークマネージャーが存在しない場合は作成
    if (!networkManager) {
        networkManager = new QNetworkAccessManager(this);
    }

    // 実際のURLからの画像読み込み
    if (imageUrl.startsWith("http://") || imageUrl.startsWith("https://")) {
        QNetworkRequest request(imageUrl);
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");

        QNetworkReply *reply = networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, title]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray imageData = reply->readAll();
                QPixmap pixmap;

                if (pixmap.loadFromData(imageData)) {
                    // 画像のサイズを調整（最大サイズ制限）
                    QPixmap scaledPixmap = pixmap.scaled(600, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                    showImage(scaledPixmap, title);
                    qDebug() << "Successfully loaded image from URL:" << title;
                } else {
                    qDebug() << "Failed to create pixmap from image data";
                    showPlaceholderImage(title + " (Load Failed)");
                }
            } else {
                qDebug() << "Network error loading image:" << reply->errorString();
                showPlaceholderImage(title + " (Network Error)");
            }
            reply->deleteLater();
        });

        // タイムアウト設定
        QTimer::singleShot(10000, reply, [reply]() {
            if (reply->isRunning()) {
                reply->abort();
            }
        });

        // 読み込み中のプレースホルダーを表示
        showPlaceholderImage("Loading " + title + "...");

    } else {
        // ローカルファイルまたは無効なURL
        showPlaceholderImage(title.isEmpty() ? "Invalid URL" : title);
    }
}

void MacOSPiPWindow::showPlaceholderImage(const QString &text) {
    // プレースホルダー画像を作成
    QPixmap placeholder(300, 200);
    placeholder.fill(QColor(64, 128, 255, 180)); // 半透明の青色

    // テキストを描画
    QPainter painter(&placeholder);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 16, QFont::Bold));
    painter.drawText(placeholder.rect(), Qt::AlignCenter, text);
    painter.end();

    showImage(placeholder, text);
}

void MacOSPiPWindow::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        isDragging = true;
        dragStartPosition = event->globalPosition().toPoint() - frameGeometry().topLeft();
        event->accept();
    }
}

void MacOSPiPWindow::mouseMoveEvent(QMouseEvent *event) {
    if (isDragging && (event->buttons() & Qt::LeftButton)) {
        move(event->globalPosition().toPoint() - dragStartPosition);
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
