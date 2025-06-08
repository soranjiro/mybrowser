#ifndef PIPWINDOW_H
#define PIPWINDOW_H

#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWebEngineView>
#include <QWidget>

/**
 * @brief ネイティブなPicture-in-Picture専用ウィンドウ
 *
 * このクラスは画像や動画を専用のフローティングウィンドウに表示します：
 * - ネイティブQtウィンドウとして動作
 * - ドラッグ移動可能
 * - リサイズ可能
 * - 画像・動画・WebEngineビューに対応
 * - 最前面表示（Always on Top）
 */
class PiPWindow : public QWidget {
  Q_OBJECT

public:
  enum MediaType {
    IMAGE,
    VIDEO,
    WEBVIEW
  };

  explicit PiPWindow(QWidget *parent = nullptr);
  ~PiPWindow();

  // メディア表示メソッド
  void displayImage(const QPixmap &pixmap, const QString &title = "Picture-in-Picture");
  void displayVideo(const QString &videoUrl, const QString &title = "Video PiP");
  void displayWebView(const QString &html, const QString &title = "Web Content PiP");
  void displayElement(const QString &elementHtml, const QString &title = "Element PiP");

  // ウィンドウ制御
  void setAlwaysOnTop(bool onTop = true);
  void showWithAnimation();
  void hideWithAnimation();

public slots:
  void toggleMinimize();
  void closeWindow();

protected:
  // イベントハンドラー
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void paintEvent(QPaintEvent *event) override;

private slots:
  void onCloseButtonClicked();
  void onMinimizeButtonClicked();
  void updateControlsVisibility();

private:
  // UI要素
  QVBoxLayout *mainLayout;
  QHBoxLayout *titleBarLayout;
  QHBoxLayout *controlsLayout;

  QWidget *titleBar;
  QLabel *titleLabel;
  QPushButton *minimizeButton;
  QPushButton *closeButton;

  QWidget *contentWidget;
  QVBoxLayout *contentLayout;

  // メディア表示要素
  QLabel *imageLabel;
  QWebEngineView *webView;
  QWebEngineView *videoView;

  // ウィンドウ状態
  MediaType currentMediaType;
  bool isDragging;
  bool isMinimized;
  QPoint dragStartPosition;
  QSize normalSize;

  // アニメーション
  QPropertyAnimation *showAnimation;
  QPropertyAnimation *hideAnimation;
  QTimer *controlsTimer;

  // スタイリング
  QGraphicsDropShadowEffect *shadowEffect;

  // プライベートメソッド
  void setupUI();
  void setupStyles();
  void setupAnimations();
  void setupMacOSWindowBehavior();
  void createTitleBar();
  void createContentArea();
  void applyModernStyling();
  void showControls();
  void hideControls();
  void clearContent();
  void updateWindowForMediaType();
};

#endif // PIPWINDOW_H
