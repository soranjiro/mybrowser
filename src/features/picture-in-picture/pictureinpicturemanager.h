#ifndef PICTUREINPICTUREMANAGER_H
#define PICTUREINPICTUREMANAGER_H

#include <QAction>
#include <QList>
#include <QMenu>
#include <QObject>

class MainWindow;
class WebView;
class MacOSPiPWindow;

/**
 * @brief シンプルなPicture-in-Picture機能を管理するクラス
 *
 * macOS Spaces対応の独自PiP機能のみを提供
 */
class PictureInPictureManager : public QObject {
  Q_OBJECT

public:
  explicit PictureInPictureManager(MainWindow *parent = nullptr);
  ~PictureInPictureManager();

  // アクションとメニューの設定
  void setupActions();
  void addToMenu(QMenu *viewMenu);

  // PiP機能の実行（画像のみ）
  void createImagePiP(WebView *webView = nullptr);
  void closeAllPiP();

  // アクションの取得
  QAction *getImagePiPAction() const { return imagePiPAction; }

private slots:
  void onImagePiPTriggered();

private:
  MainWindow *mainWindow;
  QAction *imagePiPAction;

  // PiPウィンドウ管理
  QList<MacOSPiPWindow *> activePiPWindows;

  // JavaScript生成メソッド
  QString generateImageExtractionScript() const;

  // WebViewでのJavaScript実行
  void executeJavaScript(WebView *webView, const QString &script);

  // PiP用ヘルパーメソッド
  void createPiPFromImageData(const QString &imageData, const QString &title);
  void cleanupClosedPiPWindows();
};

#endif // PICTUREINPICTUREMANAGER_H
