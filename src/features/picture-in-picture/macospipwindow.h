#ifndef MACOSPIPWINDOW_H
#define MACOSPIPWINDOW_H

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>
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

protected:
  // Enable window dragging with mouse events
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
  void onCloseButtonClicked();

private:
  void setupUI();
  void setupMacOSBehavior();                      // macOS specific settings
  void showPlaceholderImage(const QString &text); // Display placeholder image
  void applyMacOSSpacesSettings();                // Apply Spaces settings

  QVBoxLayout *layout;
  QLabel *imageLabel;
  QPushButton *closeButton;
  QNetworkAccessManager *networkManager; // For image loading

  // Window dragging
  QPoint dragStartPosition;
  bool isDragging;

#ifdef Q_OS_MACOS
  NSPanel *pipPanel; // NSPanel for fullscreen support
#endif
};

#endif // MACOSPIPWINDOW_H
