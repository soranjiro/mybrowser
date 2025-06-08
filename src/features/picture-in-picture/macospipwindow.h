#ifndef MACOSPIPWINDOW_H
#define MACOSPIPWINDOW_H

#include <QLabel>
#include <QMouseEvent>
#include <QPushButton>
#include <QVBoxLayout>
#include <QWidget>

// 前方宣言
class QNetworkAccessManager;

#ifdef Q_OS_MACOS
#ifdef __OBJC__
@class NSPanel;
#else
typedef struct objc_object NSPanel;
#endif
#endif

/**
 * @brief macOS Spaces対応のPicture-in-Picture ウィンドウ
 *
 * NSWindowのcollectionBehaviorを使用して、
 * すべての仮想ワークスペース（Spaces）で表示される独自のPiPウィンドウ
 */
class MacOSPiPWindow : public QWidget {
  Q_OBJECT

public:
  explicit MacOSPiPWindow(QWidget *parent = nullptr);
  ~MacOSPiPWindow();

  // 画像を表示
  void showImage(const QPixmap &pixmap, const QString &title = "");
  void showImageFromUrl(const QString &imageUrl, const QString &title = "");

protected:
  // マウスイベントでウィンドウを移動可能にする
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseDoubleClickEvent(QMouseEvent *event) override;

private slots:
  void onCloseButtonClicked();

private:
  void setupUI();
  void setupMacOSBehavior();                      // macOS固有の設定
  void showPlaceholderImage(const QString &text); // プレースホルダー画像表示
  void applyMacOSSpacesSettings();                // Spaces設定を適用

  QVBoxLayout *layout;
  QLabel *imageLabel;
  QPushButton *closeButton;
  QNetworkAccessManager *networkManager; // 画像読み込み用

  // ウィンドウドラッグ用
  QPoint dragStartPosition;
  bool isDragging;

#ifdef Q_OS_MACOS
  NSPanel *pipPanel; // フルスクリーン対応用NSPanel
#endif
};

#endif // MACOSPIPWINDOW_H
