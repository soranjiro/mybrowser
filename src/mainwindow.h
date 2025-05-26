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

class WebView;
class VerticalTabWidget;
class WorkspaceManager;
class BookmarkManager;

class MainWindow : public QMainWindow {
  Q_OBJECT

public:
  MainWindow(QWidget *parent = nullptr);
  ~MainWindow();

  void newTab();                   // Make this public so WebView can access it
  WebView *currentWebView() const; // Make this public too

protected:
  void closeEvent(QCloseEvent *event) override;

private slots:
  void closeCurrentTab();
  void goToUrl();
  void goBack();
  void goForward();
  void reloadPage();
  void stopLoading();
  void updateAddressBar(const QUrl &url);
  void updateWindowTitle(const QString &title);
  void handleLoadProgress(int progress);
  void handleContextMenuRequested(const QPoint &pos);
  void openLinkInNewTab();
  void addBookmark();
  void showBookmarks();
  void showHistory();
  void showSettings();
  void showDevTools();

private:
  void setupUI();
  void setupConnections();
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

  // Status bar components
  QProgressBar *progressBar;
  QLabel *statusLabel;

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

  // For context menu
  QAction *openLinkInNewTabAction;

  // Placeholder for settings
  QString homePageUrl = "https://www.google.com";
  QString defaultSearchEngineUrl = "https://www.google.com/search?q=%1";

  // Placeholder for bookmarks and history
  QList<QPair<QString, QUrl>> bookmarks;
  QList<QPair<QString, QUrl>> history;
};

#endif // MAINWINDOW_H
