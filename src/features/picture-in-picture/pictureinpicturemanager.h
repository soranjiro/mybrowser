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
 * @brief Simple Picture-in-Picture functionality manager
 *
 * Provides macOS Spaces compatible custom PiP functionality
 */
class PictureInPictureManager : public QObject {
  Q_OBJECT

public:
  explicit PictureInPictureManager(MainWindow *parent = nullptr);
  ~PictureInPictureManager();

  // Setup actions and menus
  void setupActions();
  void addToMenu(QMenu *viewMenu);

  // Execute PiP functionality
  void createImagePiP(WebView *webView = nullptr);
  void createVideoPiP(WebView *webView = nullptr);
  void closeAllPiP();

  // Get actions
  QAction *getImagePiPAction() const { return imagePiPAction; }
  QAction *getVideoPiPAction() const { return videoPiPAction; }

private slots:
  void onImagePiPTriggered();
  void onVideoPiPTriggered();

private:
  MainWindow *mainWindow;
  QAction *imagePiPAction;
  QAction *videoPiPAction;

  // PiPウィンドウ管理
  QList<MacOSPiPWindow *> activePiPWindows;

  // JavaScript生成メソッド
  QString generateImageExtractionScript() const;
  QString generateVideoExtractionScript() const;

  // WebViewでのJavaScript実行
  void executeJavaScript(WebView *webView, const QString &script);
  void executeVideoJavaScript(WebView *webView, const QString &script);

  // PiP用ヘルパーメソッド
  void createPiPFromImageData(const QString &imageData, const QString &title);
  void createPiPFromVideoData(const QString &videoData, const QString &title);
  void cleanupClosedPiPWindows();
};

#endif // PICTUREINPICTUREMANAGER_H
