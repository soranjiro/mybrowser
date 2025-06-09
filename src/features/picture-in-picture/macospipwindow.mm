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

    // Configure window as independent for Mission Control recognition
    setAttribute(Qt::WA_QuitOnClose, false);
    setAttribute(Qt::WA_DeleteOnClose, true);

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
    // Configure window flags for independent Mission Control display
    setWindowFlags(Qt::WindowStaysOnTopHint | Qt::FramelessWindowHint | Qt::Window);
    setAttribute(Qt::WA_TranslucentBackground);

    setWindowTitle("MyBrowser - Picture-in-Picture");
    setWindowIcon(QIcon(":/icons/pip.png"));

    // Create layout
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // Image display label
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet(
        "QLabel { background-color: rgba(0, 0, 0, 200); border: 2px solid #007ACC; "
        "border-radius: 8px; padding: 10px; }"
    );
    imageLabel->setMinimumSize(200, 150);
    imageLabel->setScaledContents(true);

    // Close button
    closeButton = new QPushButton("×", this);
    closeButton->setFixedSize(25, 25);
    closeButton->setStyleSheet(
        "QPushButton { background-color: #ff5f57; border: none; border-radius: 12px; "
        "color: white; font-weight: bold; font-size: 12px; } "
        "QPushButton:hover { background-color: #ff3b30; }"
    );

    // Add to layout
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    layout->addWidget(imageLabel);

    connect(closeButton, &QPushButton::clicked, this, &MacOSPiPWindow::onCloseButtonClicked);
    resize(300, 250);
}

void MacOSPiPWindow::setupMacOSBehavior() {
    // macOS specific settings applied after showImage()
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

    // Apply macOS settings after window is displayed
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
        NSRect frame = [originalWindow frame];

        // Create NSPanel with nonactivatingPanel style for fullscreen support
        NSWindowStyleMask styleMask = NSWindowStyleMaskNonactivatingPanel |
                                     NSWindowStyleMaskTitled |
                                     NSWindowStyleMaskClosable |
                                     NSWindowStyleMaskResizable;

        pipPanel = [[NSPanel alloc] initWithContentRect:frame
                                               styleMask:styleMask
                                                 backing:NSBackingStoreBuffered
                                                   defer:NO];

        NSView *contentView = [originalWindow contentView];
        [pipPanel setContentView:contentView];

        // Configure for fullscreen and Mission Control support
        [pipPanel setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                                        NSWindowCollectionBehaviorFullScreenAuxiliary |
                                        NSWindowCollectionBehaviorParticipatesInCycle];

        [pipPanel setLevel:NSScreenSaverWindowLevel];
        [pipPanel setHidesOnDeactivate:NO];
        [pipPanel setFloatingPanel:YES];
        [pipPanel setBecomesKeyOnlyIfNeeded:YES];
        [pipPanel setWorksWhenModal:YES];

        // Configure transparency and dragging
        [pipPanel setOpaque:NO];
        [pipPanel setBackgroundColor:[NSColor clearColor]];
        [pipPanel setTitlebarAppearsTransparent:YES];
        [pipPanel setTitleVisibility:NSWindowTitleHidden];
        [pipPanel setMovableByWindowBackground:YES];
        [pipPanel setExcludedFromWindowsMenu:NO];
        [pipPanel setCanHide:NO];
        [pipPanel setIgnoresMouseEvents:NO];

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

    if (!networkManager) {
        networkManager = new QNetworkAccessManager(this);
    }

    if (imageUrl.startsWith("http://") || imageUrl.startsWith("https://")) {
        QNetworkRequest request(imageUrl);
        request.setRawHeader("User-Agent", "Mozilla/5.0 (Macintosh; Intel Mac OS X 10_15_7) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/91.0.4472.124 Safari/537.36");

        QNetworkReply *reply = networkManager->get(request);

        connect(reply, &QNetworkReply::finished, this, [this, reply, title]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray imageData = reply->readAll();
                QPixmap pixmap;

                if (pixmap.loadFromData(imageData)) {
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

        QTimer::singleShot(10000, reply, [reply]() {
            if (reply->isRunning()) {
                reply->abort();
            }
        });

        showPlaceholderImage("Loading " + title + "...");
    } else {
        showPlaceholderImage(title.isEmpty() ? "Invalid URL" : title);
    }
}

void MacOSPiPWindow::showPlaceholderImage(const QString &text) {
    QPixmap placeholder(300, 200);
    placeholder.fill(QColor(64, 128, 255, 180));

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
    close();
}

void MacOSPiPWindow::onCloseButtonClicked() {
    close();
}
