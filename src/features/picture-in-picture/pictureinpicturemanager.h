#ifndef PICTUREINPICTUREMANAGER_H
#define PICTUREINPICTUREMANAGER_H

#include <QAction>
#include <QList>
#include <QLocalServer>
#include <QLocalSocket>
#include <QMenu>
#include <QObject>
#include <QProcess>

class MainWindow;
class WebView;
class PiPWindow;

/**
 * @brief Picture-in-Picture機能を管理するクラス
 *
 * このクラスは以下の機能を提供します：
 * - PiPアクションの作成と管理
 * - キーボードショートカットの設定
 * - メニュー項目の追加
 * - WebViewでのPiP機能実行
 */
class PictureInPictureManager : public QObject {
  Q_OBJECT

public:
  explicit PictureInPictureManager(MainWindow *parent = nullptr);
  ~PictureInPictureManager();

  // アクションとメニューの設定
  void setupActions();
  void addToMenu(QMenu *viewMenu);
  void addToContextMenu(QMenu *contextMenu);

  // PiP機能の実行
  void togglePictureInPicture(WebView *webView = nullptr);
  void requestPictureInPicture(WebView *webView = nullptr);
  void exitPictureInPicture(WebView *webView = nullptr);
  void enablePiPForAllVideos(WebView *webView = nullptr);

  // ネイティブPiP機能（新機能）
  void createNativeImagePiP(WebView *webView = nullptr);
  void createNativeVideoPiP(WebView *webView = nullptr);
  void createNativeElementPiP(WebView *webView = nullptr);
  void closeAllNativePiP();

  // 独立プロセスPiP機能（macOS Spaces対応）
  void createStandaloneImagePiP(WebView *webView = nullptr);
  void closeStandalonePiP();

  // 独自PiP機能（Webベース - 従来の機能）
  void createElementPictureInPicture(WebView *webView = nullptr);
  void createPagePictureInPicture(WebView *webView = nullptr);
  void createScreenshotPictureInPicture(WebView *webView = nullptr);
  void exitAllPictureInPicture(WebView *webView = nullptr);

  // アクションの取得
  QAction *getPictureInPictureAction() const { return pictureInPictureAction; }

signals:
  void standalonePiPClosed();

private slots:
  void onTogglePictureInPicture();
  void onStandaloneProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
  void onStandaloneProcessError(QProcess::ProcessError error);
  void onIpcConnection();

private:
  MainWindow *mainWindow;
  QAction *pictureInPictureAction;
  QAction *contextMenuAction;

  // ネイティブPiPウィンドウ管理
  QList<PiPWindow *> activePiPWindows;

  // 独立プロセスPiP管理
  QProcess *standaloneProcess;
  QLocalServer *ipcServer;

  // JavaScript生成メソッド
  QString generatePiPJavaScript() const;
  QString generatePiPDetectionScript() const;
  QString loadResourceFile(const QString &resourcePath) const;
  QString generatePiPToggleScript() const;

  // ネイティブPiP用JavaScript生成メソッド
  QString generateImageExtractionScript() const;
  QString generateVideoExtractionScript() const;
  QString generateElementSelectionScript() const;

  // 独自PiP用JavaScript生成メソッド（Webベース）
  QString generateElementPiPScript() const;
  QString generatePagePiPScript() const;
  QString generateScreenshotPiPScript() const;
  QString generateExitAllPiPScript() const;

  // WebViewでのJavaScript実行
  void executeJavaScript(WebView *webView, const QString &script);

  // ネイティブPiP用ヘルパーメソッド
  void createPiPFromImageData(const QString &imageData, const QString &title);
  void createPiPFromVideoUrl(const QString &videoUrl, const QString &title);
  void createPiPFromElementHtml(const QString &elementHtml, const QString &title);
  void cleanupClosedPiPWindows();

  // 独立プロセスPiP用ヘルパーメソッド
  void createStandalonePiP(const QString &imageUrl, const QString &title);
  void setupIpcServer();
  QByteArray convertImageDataToByteArray(const QString &imageData);
};

#endif // PICTUREINPICTUREMANAGER_H
