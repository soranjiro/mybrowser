#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QAction>
#include <QCloseEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMainWindow>
#include <QMenu>
#include <QPair>
#include <QProgressBar>
#include <QStatusBar>
#include <QTabWidget>
#include <QToolBar>
#include <QUrl>
#include <QWebChannel>

class WebView;
class VerticalTabWidget;
class WorkspaceManager;
class BookmarkManager;
class PictureInPictureManager;
class CommandPaletteManager;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void newTab();                   // Make this public so WebView can access it
  WebView *currentWebView() const; // Make this public too

  // Manager accessors
  WorkspaceManager *getWorkspaceManager() const { return workspaceManager; }
  VerticalTabWidget *getTabWidget() const { return tabWidget; }
  PictureInPictureManager *getPictureInPictureManager() const { return pictureInPictureManager; }
  CommandPaletteManager *getCommandPaletteManager() const { return commandPaletteManager; }

protected:
  void closeEvent(QCloseEvent *event) override;
  void resizeEvent(QResizeEvent *event) override; // 追加

public slots:
  // Public slots that can be called by managers
  void closeCurrentTab();
  void goToUrl();
  void goBack();
  void goForward();
  void reloadPage();
  void stopLoading();
  void addBookmark();
  void showBookmarks();
  void showHistory();
  void showSettings();
  void showDevTools();

  // WebChannel invokable methods for JavaScript communication
  Q_INVOKABLE void handleSwipeBack();    // スワイプで戻る
  Q_INVOKABLE void handleSwipeForward(); // スワイプで進む

#ifdef DEBUG_MODE
  void openTestPage(const QString &fileName);
#endif

private slots:
  void updateAddressBar(const QUrl &url);
  void updateWindowTitle(const QString &title);
  void handleLoadProgress(int progress);
  void handleContextMenuRequested(const QPoint &pos);
  void openLinkInNewTab();
  void toggleTabBar();
  void adjustStatusWidgetsGeometry(); // 追加

private:
  void setupUI();
  void setupConnections();
  void loadStyleSheet();
  void createActions();
  void createMenus();
  void createToolbars();

  QLineEdit *addressBar;
  VerticalTabWidget *tabWidget;
  QToolBar *navigationToolBar;
  QToolBar *bookmarksToolBar; // Example for future use

  // Managers
  WorkspaceManager *workspaceManager;
  BookmarkManager *bookmarkManager;
  PictureInPictureManager *pictureInPictureManager;
  CommandPaletteManager *commandPaletteManager;

  // Dock widgets for panels
  QDockWidget *bookmarkDock;

  // Status bar components
  QProgressBar *progressBar;

  QAction *newTabAction;
  QAction *closeTabAction;
  QAction *backAction;
  QAction *forwardAction;
  QAction *reloadAction;
  QAction *stopAction;
  QAction *addBookmarkAction;
  QAction *viewBookmarksAction;
  QAction *viewHistoryAction;
  QAction *settingsAction;
  QAction *devToolsAction;

  // Toggle actions for panels
  QAction *toggleTabBarAction;

  // For context menu
  QAction *openLinkInNewTabAction;

  // WebChannel for JavaScript communication
  QWebChannel *webChannel;

  // Placeholder for settings
#ifdef DEBUG_MODE
  QString homePageUrl = "file:///Users/user/Documents/03_app/mybrowser/tests/debug_test.html";
#else
  QString homePageUrl = "https://www.google.com";
#endif
  QString defaultSearchEngineUrl = "https://www.google.com/search?q=%1";

  // Placeholder for bookmarks and history
  QList<QPair<QString, QUrl>> bookmarks;
  QList<QPair<QString, QUrl>> history;
};

#endif // MAINWINDOW_H
