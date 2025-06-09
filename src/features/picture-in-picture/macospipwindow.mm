#include "macospipwindow.h"
#include <QPushButton>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QTimer>
#include <QDebug>
#include <QPainter>
#include <QFont>
#include <QMediaPlayer>
#include <QVideoWidget>
#include <QHBoxLayout>
#include <QSlider>
#include <QStackedWidget>
#include <QLabel>
#include <QTime>
#include <QAudioOutput>

#ifdef Q_OS_MACOS
#import <Cocoa/Cocoa.h>
#endif

MacOSPiPWindow::MacOSPiPWindow(QWidget *parent)
    : QWidget(parent), isDragging(false), networkManager(nullptr),
      currentMediaType(Image), mediaPlayer(nullptr), videoWidget(nullptr),
      contentStack(nullptr), videoControlsWidget(nullptr) {
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
    if (mediaPlayer) {
        mediaPlayer->stop();
        delete mediaPlayer;
    }
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

    // Create main layout
    layout = new QVBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);
    layout->setSpacing(5);

    // Close button
    closeButton = new QPushButton("×", this);
    closeButton->setFixedSize(25, 25);
    closeButton->setStyleSheet(
        "QPushButton { background-color: #ff5f57; border: none; border-radius: 12px; "
        "color: white; font-weight: bold; font-size: 12px; } "
        "QPushButton:hover { background-color: #ff3b30; }"
    );

    // Create stacked widget for switching between image and video
    contentStack = new QStackedWidget(this);

    // Setup image display
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setStyleSheet(
        "QLabel { background-color: rgba(0, 0, 0, 200); border: 2px solid #007ACC; "
        "border-radius: 8px; padding: 10px; }"
    );
    imageLabel->setMinimumSize(200, 150);
    imageLabel->setScaledContents(true);

    // Setup video display
    videoWidget = new QVideoWidget(this);
    videoWidget->setStyleSheet(
        "QVideoWidget { background-color: rgba(0, 0, 0, 200); border: 2px solid #007ACC; "
        "border-radius: 8px; }"
    );
    videoWidget->setMinimumSize(200, 150);

    // Setup media player
    mediaPlayer = new QMediaPlayer(this);
    mediaPlayer->setVideoOutput(videoWidget);

    // Add widgets to stack
    contentStack->addWidget(imageLabel);
    contentStack->addWidget(videoWidget);
    contentStack->setCurrentIndex(0); // Start with image mode

    // Setup video controls
    setupVideoControls();

    // Add to main layout
    layout->addWidget(closeButton, 0, Qt::AlignRight);
    layout->addWidget(contentStack);
    layout->addWidget(videoControlsWidget);

    // Initially hide video controls
    videoControlsWidget->hide();

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

void MacOSPiPWindow::showImageFromBase64(const QString &base64Data, const QString &title) {
    qDebug() << "Loading image from Base64 data:" << title;

    if (base64Data.startsWith("data:image/")) {
        // Extract the base64 data part (after the comma)
        int commaIndex = base64Data.indexOf(',');
        if (commaIndex != -1) {
            QString base64String = base64Data.mid(commaIndex + 1);
            QByteArray imageData = QByteArray::fromBase64(base64String.toUtf8());

            QPixmap pixmap;
            if (pixmap.loadFromData(imageData)) {
                // Scale the image to a reasonable size while maintaining aspect ratio
                QPixmap scaledPixmap = pixmap.scaled(600, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);
                showImage(scaledPixmap, title);
                qDebug() << "Successfully loaded image from Base64 data:" << title;
            } else {
                qDebug() << "Failed to create pixmap from Base64 data";
                showPlaceholderImage(title + " (Base64 Load Failed)");
            }
        } else {
            qDebug() << "Invalid Base64 data format";
            showPlaceholderImage(title + " (Invalid Base64)");
        }
    } else {
        qDebug() << "Invalid Base64 data format - missing data:image/ prefix";
        showPlaceholderImage(title + " (Invalid Format)");
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

void MacOSPiPWindow::showPlaceholderVideo(const QString &text) {
    // Switch to video mode to show placeholder in video widget area
    currentMediaType = Video;
    switchToVideoMode();

    // Create a placeholder widget with message
    QPixmap placeholder(400, 300);
    placeholder.fill(QColor(255, 107, 107, 180)); // Red-ish color for video

    QPainter painter(&placeholder);
    painter.setPen(Qt::white);
    painter.setFont(QFont("Arial", 18, QFont::Bold));

    // Draw video icon
    painter.drawText(QRect(0, 80, 400, 50), Qt::AlignCenter, "🎬");
    painter.setFont(QFont("Arial", 14, QFont::Bold));
    painter.drawText(QRect(10, 150, 380, 100), Qt::AlignCenter | Qt::TextWordWrap, text);

    painter.end();

    // Set a simple style for video widget with placeholder appearance
    videoWidget->setStyleSheet(
        "QVideoWidget { "
        "background-color: rgba(255, 107, 107, 180); "
        "border: 2px solid #FF6B6B; "
        "border-radius: 8px; "
        "}"
    );

    if (!text.isEmpty()) {
        setWindowTitle(text);
    }

    show();
    raise();
    activateWindow();

    // Apply macOS settings after window is displayed
    QTimer::singleShot(500, this, [this]() {
        applyMacOSSpacesSettings();
    });

    qDebug() << "PiP window showing video placeholder:" << text;
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

void MacOSPiPWindow::setupVideoControls() {
    videoControlsWidget = new QWidget(this);
    videoControlsWidget->setStyleSheet(
        "QWidget { background-color: rgba(0, 0, 0, 150); border-radius: 5px; padding: 5px; }"
    );

    controlsLayout = new QHBoxLayout(videoControlsWidget);
    controlsLayout->setContentsMargins(5, 5, 5, 5);
    controlsLayout->setSpacing(5);

    // Play/Pause button
    playPauseButton = new QPushButton("▶", videoControlsWidget);
    playPauseButton->setFixedSize(30, 30);
    playPauseButton->setStyleSheet(
        "QPushButton { background-color: #007ACC; border: none; border-radius: 15px; "
        "color: white; font-weight: bold; font-size: 14px; } "
        "QPushButton:hover { background-color: #005a9e; }"
    );

    // Position slider
    positionSlider = new QSlider(Qt::Horizontal, videoControlsWidget);
    positionSlider->setRange(0, 0);
    positionSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #555; height: 8px; border-radius: 4px; } "
        "QSlider::handle:horizontal { background: #007ACC; border: none; width: 18px; "
        "height: 18px; border-radius: 9px; margin: -5px 0; } "
        "QSlider::sub-page:horizontal { background: #007ACC; border-radius: 4px; }"
    );

    // Volume slider
    volumeSlider = new QSlider(Qt::Horizontal, videoControlsWidget);
    volumeSlider->setRange(0, 100);
    volumeSlider->setValue(50);
    volumeSlider->setMaximumWidth(80);
    volumeSlider->setStyleSheet(
        "QSlider::groove:horizontal { background: #555; height: 8px; border-radius: 4px; } "
        "QSlider::handle:horizontal { background: #007ACC; border: none; width: 18px; "
        "height: 18px; border-radius: 9px; margin: -5px 0; } "
        "QSlider::sub-page:horizontal { background: #007ACC; border-radius: 4px; }"
    );

    // Time label
    timeLabel = new QLabel("00:00 / 00:00", videoControlsWidget);
    timeLabel->setStyleSheet("QLabel { color: white; font-size: 12px; }");
    timeLabel->setMinimumWidth(80);

    // Add to layout
    controlsLayout->addWidget(playPauseButton);
    controlsLayout->addWidget(positionSlider);
    controlsLayout->addWidget(timeLabel);
    controlsLayout->addWidget(volumeSlider);

    // Connect signals
    connect(playPauseButton, &QPushButton::clicked, this, &MacOSPiPWindow::onPlayPauseClicked);
    connect(positionSlider, static_cast<void(QSlider::*)(int)>(&QSlider::valueChanged),
            this, static_cast<void(MacOSPiPWindow::*)(int)>(&MacOSPiPWindow::onPositionChanged));
    connect(volumeSlider, static_cast<void(QSlider::*)(int)>(&QSlider::valueChanged), this, &MacOSPiPWindow::onVolumeChanged);
    connect(mediaPlayer, &QMediaPlayer::durationChanged, this, &MacOSPiPWindow::onDurationChanged);
    connect(mediaPlayer, &QMediaPlayer::positionChanged, this, [this](qint64 position) {
        if (!positionSlider->isSliderDown()) {
            positionSlider->setValue(position);
        }
        updateVideoControls();
    });
}

void MacOSPiPWindow::showVideo(const QString &videoUrl, const QString &title) {
    qDebug() << "Loading video from URL:" << videoUrl;

    currentMediaType = Video;
    switchToVideoMode();

    if (!title.isEmpty()) {
        setWindowTitle(title);
    }

    // Handle demo/test URLs specially
    if (videoUrl.startsWith("demo://") || videoUrl.startsWith("test://")) {
        qDebug() << "Showing placeholder for demo video:" << title;
        showPlaceholderVideo(title);
        return;
    }

    // Validate URL format
    if (!videoUrl.startsWith("http://") && !videoUrl.startsWith("https://") && !videoUrl.startsWith("file://")) {
        qDebug() << "Invalid video URL format:" << videoUrl;
        showPlaceholderVideo(title + " (無効なURL)");
        return;
    }

    mediaPlayer->setSource(QUrl(videoUrl));
    mediaPlayer->setVideoOutput(videoWidget);

    show();
    raise();
    activateWindow();

    // Apply macOS settings after window is displayed
    QTimer::singleShot(500, this, [this]() {
        applyMacOSSpacesSettings();
    });

    qDebug() << "PiP window showing video:" << title;
}

void MacOSPiPWindow::showVideoFromBase64(const QString &base64Data, const QString &title) {
    qDebug() << "Loading video from Base64 data:" << title;

    // For now, show placeholder - full Base64 video support would require more complex implementation
    showPlaceholderImage("Base64 Video: " + title);
    qDebug() << "Base64 video display not fully implemented yet";
}

void MacOSPiPWindow::playVideo() {
    if (mediaPlayer && currentMediaType == Video) {
        mediaPlayer->play();
        playPauseButton->setText("⏸");
        qDebug() << "Playing video";
    }
}

void MacOSPiPWindow::pauseVideo() {
    if (mediaPlayer && currentMediaType == Video) {
        mediaPlayer->pause();
        playPauseButton->setText("▶");
        qDebug() << "Pausing video";
    }
}

void MacOSPiPWindow::stopVideo() {
    if (mediaPlayer && currentMediaType == Video) {
        mediaPlayer->stop();
        playPauseButton->setText("▶");
        qDebug() << "Stopping video";
    }
}

void MacOSPiPWindow::setVideoPosition(qint64 position) {
    if (mediaPlayer && currentMediaType == Video) {
        mediaPlayer->setPosition(position);
    }
}

void MacOSPiPWindow::setVideoVolume(int volume) {
    if (mediaPlayer) {
        QAudioOutput *audioOutput = mediaPlayer->audioOutput();
        if (audioOutput) {
            audioOutput->setVolume(volume / 100.0);
        }
    }
}

void MacOSPiPWindow::onPlayPauseClicked() {
    if (mediaPlayer && currentMediaType == Video) {
        if (mediaPlayer->playbackState() == QMediaPlayer::PlayingState) {
            pauseVideo();
        } else {
            playVideo();
        }
    }
}

void MacOSPiPWindow::onPositionChanged(qint64 position) {
    if (mediaPlayer && currentMediaType == Video) {
        mediaPlayer->setPosition(position);
    }
}

void MacOSPiPWindow::onPositionChanged(int position) {
    if (mediaPlayer && currentMediaType == Video) {
        mediaPlayer->setPosition(static_cast<qint64>(position));
    }
}

void MacOSPiPWindow::onDurationChanged(qint64 duration) {
    positionSlider->setRange(0, duration);
    updateVideoControls();
}

void MacOSPiPWindow::onVolumeChanged(int volume) {
    setVideoVolume(volume);
}

void MacOSPiPWindow::updateVideoControls() {
    if (mediaPlayer && currentMediaType == Video) {
        qint64 position = mediaPlayer->position();
        qint64 duration = mediaPlayer->duration();

        QTime currentTime = QTime::fromMSecsSinceStartOfDay(position);
        QTime totalTime = QTime::fromMSecsSinceStartOfDay(duration);

        QString timeText = QString("%1 / %2")
            .arg(currentTime.toString(duration >= 3600000 ? "hh:mm:ss" : "mm:ss"))
            .arg(totalTime.toString(duration >= 3600000 ? "hh:mm:ss" : "mm:ss"));

        timeLabel->setText(timeText);
    }
}

void MacOSPiPWindow::switchToImageMode() {
    currentMediaType = Image;
    contentStack->setCurrentIndex(0);
    videoControlsWidget->hide();
    resize(300, 250);
}

void MacOSPiPWindow::switchToVideoMode() {
    currentMediaType = Video;
    contentStack->setCurrentIndex(1);
    videoControlsWidget->show();
    resize(400, 300);
}
