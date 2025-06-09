#ifndef MACOSPIPWINDOW_H
#define MACOSPIPWINDOW_H

#include <QAudioOutput>
#include <QHBoxLayout>
#include <QLabel>
#include <QMediaPlayer>
#include <QMouseEvent>
#include <QPushButton>
#include <QSlider>
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QVideoWidget>
#include <QWidget>

// Forward declarations
class QNetworkAccessManager;

#ifdef Q_OS_MACOS
#ifdef __OBJC__
@class NSPanel;
#else
typedef struct objc_object NSPanel;
#endif
#endif

/**
 * @brief macOS Spaces compatible Picture-in-Picture window
 *
 * Uses NSWindow collectionBehavior to display on all virtual workspaces (Spaces)
 * as an independent PiP window
 */
class MacOSPiPWindow : public QWidget {
  Q_OBJECT

public:
  explicit MacOSPiPWindow(QWidget *parent = nullptr);
  ~MacOSPiPWindow();

  // Display images
  void showImage(const QPixmap &pixmap, const QString &title = "");
  void showImageFromUrl(const QString &imageUrl, const QString &title = "");
  void showImageFromBase64(const QString &base64Data, const QString &title = "");

  // Display videos
  void showVideo(const QString &videoUrl, const QString &title = "");
  void showVideoFromBase64(const QString &base64Data, const QString &title = "");

  // Video controls
  void playVideo();
  void pauseVideo();
  void stopVideo();
  void setVideoPosition(qint64 position);
  void setVideoVolume(int volume);

  // Current media type
  enum MediaType {
    Image,
    Video
  };
  MediaType getCurrentMediaType() const;

protected:
  // Enable window dragging with mouse events
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
  void onCloseButtonClicked();
  void onPlayPauseClicked();
  void onPositionChanged(qint64 position);
  void onPositionChanged(int position); // Overload for QSlider
  void onDurationChanged(qint64 duration);
  void onVolumeChanged(int volume);

private:
  void setupUI();
  void setupMacOSBehavior();                      // macOS specific settings
  void showPlaceholderImage(const QString &text); // Display placeholder image
  void showPlaceholderVideo(const QString &text); // Display placeholder video
  void setupVideoControls();                      // Setup video player controls
  void updateVideoControls();                     // Update control states
  void switchToImageMode();                       // Switch to image display
  void switchToVideoMode();                       // Switch to video display
  void applyMacOSSpacesSettings();                // Apply Spaces settings

  QVBoxLayout *layout;
  QStackedWidget *contentStack; // Switch between image and video
  QLabel *imageLabel;
  QVideoWidget *videoWidget;
  QMediaPlayer *mediaPlayer;
  QPushButton *closeButton;

  // Video controls
  QWidget *videoControlsWidget;
  QHBoxLayout *controlsLayout;
  QPushButton *playPauseButton;
  QSlider *positionSlider;
  QSlider *volumeSlider;
  QLabel *timeLabel;

  QNetworkAccessManager *networkManager; // For media loading
  MediaType currentMediaType;

  // Window dragging
  QPoint dragStartPosition;
  bool isDragging;

#ifdef Q_OS_MACOS
  NSPanel *pipPanel; // NSPanel for fullscreen support
#endif
};

#endif // MACOSPIPWINDOW_H
