#ifndef PICTUREINPICTUREMANAGER_H
#define PICTUREINPICTUREMANAGER_H

#include <QAction>
#include <QMenu>
#include <QObject>

class MainWindow;
class WebView;

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

  // アクションの取得
  QAction *getPictureInPictureAction() const { return pictureInPictureAction; }

private slots:
  void onTogglePictureInPicture();

private:
  MainWindow *mainWindow;
  QAction *pictureInPictureAction;
  QAction *contextMenuAction;

  // JavaScript生成メソッド
  QString generatePiPJavaScript() const;
  QString generatePiPDetectionScript() const;
  QString loadResourceFile(const QString &resourcePath) const;
  QString generatePiPToggleScript() const;

  // WebViewでのJavaScript実行
  void executeJavaScript(WebView *webView, const QString &script);
};

#endif // PICTUREINPICTUREMANAGER_H
