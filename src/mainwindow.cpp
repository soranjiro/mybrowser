#include "mainwindow.h"
#include "managers/bookmarkmanager.h"
#include "managers/commandpalettemanager.h"
#include "managers/pictureinpicturemanager.h"
#include "managers/workspacemanager.h"
#include "ui/verticaltabwidget.h"
#include "ui/webview.h"
#include <QCoreApplication>
#include <QCursor>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTabBar>
#include <QTextStream>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineHistory>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), workspaceManager(nullptr), bookmarkManager(nullptr),
      pictureInPictureManager(nullptr), commandPaletteManager(nullptr) {
  // Debug output for homepage URL setting
#ifdef DEBUG_MODE
  qDebug() << "DEBUG_MODE active - Homepage URL:" << homePageUrl;
#else
  qDebug() << "RELEASE_MODE active - Homepage URL:" << homePageUrl;
#endif

  // Initialize managers FIRST before setupUI
  pictureInPictureManager = new PictureInPictureManager(this);
  commandPaletteManager = new CommandPaletteManager(this);

  loadStyleSheet();
  setupUI();
  setupConnections();
  newTab(); // Open a default tab
}

MainWindow::~MainWindow() {
  // Cleanup is handled by Qt's parent-child relationship
}

void MainWindow::setupUI() {
  // Create vertical tab widget
  tabWidget = new VerticalTabWidget(this);
  tabWidget->setTabsClosable(true); // Enable close buttons on tabs

  // Create managers
  workspaceManager = new WorkspaceManager(this);
  workspaceManager->setTabWidget(tabWidget);

  bookmarkManager = new BookmarkManager(this);

  addressBar = new QLineEdit(this);
  addressBar->setPlaceholderText("Enter URL or search query");
  addressBar->setStyleSheet(
      "QLineEdit { "
      "  padding: 10px 15px; "
      "  margin: 5px; "
      "  background-color: white; "
      "  border: 2px solid #d1d5db; "
      "  border-radius: 8px; "
      "  font-size: 14px; "
      "} "
      "QLineEdit:focus { "
      "  border-color: #007ACC; "
      "  outline: none; "
      "}");

  createActions();
  createToolbars();
  createMenus();

  // Style the menu bar
  menuBar()->setStyleSheet(
      "QMenuBar { "
      "  background-color: #2d2d30; "
      "  color: white; "
      "  padding: 4px; "
      "  font-size: 13px; "
      "} "
      "QMenuBar::item { "
      "  background-color: transparent; "
      "  padding: 8px 12px; "
      "  border-radius: 4px; "
      "} "
      "QMenuBar::item:selected { "
      "  background-color: #007ACC; "
      "} "
      "QMenu { "
      "  background-color: #2d2d30; "
      "  color: white; "
      "  border: 1px solid #3e3e42; "
      "  padding: 4px; "
      "} "
      "QMenu::item { "
      "  padding: 8px 20px; "
      "  border-radius: 4px; "
      "} "
      "QMenu::item:selected { "
      "  background-color: #007ACC; "
      "} "
      "QMenu::separator { "
      "  height: 1px; "
      "  background-color: #3e3e42; "
      "  margin: 4px 8px; "
      "}");

  // Configure tab widget for overlay mode
  tabWidget->setWorkspaceManager(workspaceManager);
  tabWidget->setBookmarkManager(bookmarkManager);
  tabWidget->setAddressBar(addressBar);

  // Set tab widget as central widget for full-screen display
  setCentralWidget(tabWidget);

  // Hide traditional UI elements for overlay mode
  navigationToolBar->hide();
  addressBar->hide();

  // Keep bookmark dock hidden by default (will be integrated into sidebar)
  bookmarkDock = bookmarkManager->createBookmarkDock(this);
  addDockWidget(Qt::RightDockWidgetArea, bookmarkDock);
  bookmarkDock->hide();

  // Create status widgets directly as children of MainWindow
  progressBar = new QProgressBar(this);
  progressBar->setStyleSheet(
      "QProgressBar { "
      "  border: 1px solid black; "
      "  border-radius: 3px; "
      "  text-align: center; "
      "  font-size: 10px; "
      "  background-color: white; "
      "  color: black; "
      "} "
      "QProgressBar::chunk { "
      "  background-color: black; "
      "  width: 10px; " // Adjust chunk appearance if needed
      "}");
  progressBar->setVisible(false);
  progressBar->setMaximumHeight(15);
  progressBar->setTextVisible(false);

  // Remove QStatusBar related code
  // statusBar()->setStyleSheet(...);
  // statusBar()->addWidget(statusLabel); // 削除
  // statusBar()->addPermanentWidget(progressBar);

  setWindowTitle("MyBrowser");
  resize(1024, 768);
}

void MainWindow::createActions() {
  newTabAction = new QAction(QIcon::fromTheme("tab-new"), "New Tab", this);
  // ショートカットを削除 - コマンドパレット経由でのみ新しいタブを作成

  closeTabAction = new QAction(QIcon::fromTheme("window-close"), "Close Tab", this);
  closeTabAction->setShortcut(QKeySequence::Close);

  backAction = new QAction(QIcon::fromTheme("go-previous"), "Back", this);
  backAction->setShortcut(QKeySequence::Back);
  backAction->setEnabled(false);

  forwardAction = new QAction(QIcon::fromTheme("go-next"), "Forward", this);
  forwardAction->setShortcut(QKeySequence::Forward);
  forwardAction->setEnabled(false);

  reloadAction = new QAction(QIcon::fromTheme("view-refresh"), "Reload", this);
  reloadAction->setShortcut(QKeySequence::Refresh);

  stopAction = new QAction(QIcon::fromTheme("process-stop"), "Stop", this);
  stopAction->setShortcut(Qt::Key_Escape);
  stopAction->setEnabled(false);

  addBookmarkAction = new QAction(QIcon::fromTheme("bookmark-new"), "Add Bookmark", this);
  viewBookmarksAction = new QAction("View Bookmarks", this);
  viewHistoryAction = new QAction("View History", this);
  settingsAction = new QAction(QIcon::fromTheme("preferences-system"), "Settings", this);
  devToolsAction = new QAction("Developer Tools", this);
  devToolsAction->setShortcut(QKeySequence("Ctrl+Shift+I"));

  // Toggle panel actions - only keep tab bar toggle with Cmd+S
  toggleTabBarAction = new QAction("Toggle Sidebar", this);
  toggleTabBarAction->setShortcut(QKeySequence("Ctrl+S"));
  toggleTabBarAction->setCheckable(true);
  toggleTabBarAction->setChecked(false); // Default to hidden

  openLinkInNewTabAction = new QAction("Open Link in New Tab", this);

  // Setup manager actions
  if (pictureInPictureManager) {
    pictureInPictureManager->setupActions();
  }
  if (commandPaletteManager) {
    qDebug() << "Setting up command palette manager actions...";
    commandPaletteManager->setupActions();

    QAction *cmdAction = commandPaletteManager->getCommandPaletteAction();
    if (cmdAction) {
      qDebug() << "Adding command palette action to MainWindow with shortcuts:";
      for (const QKeySequence &shortcut : cmdAction->shortcuts()) {
        qDebug() << "  -" << shortcut.toString();
      }
      this->addAction(cmdAction);
      qDebug() << "Action added successfully. MainWindow actions count:" << this->actions().size();
    } else {
      qDebug() << "ERROR: commandPaletteAction is null!";
    }

#ifdef QT_DEBUG
    QAction *testAction = commandPaletteManager->getOpenTestPageAction();
    if (testAction) {
      qDebug() << "Adding test page action with shortcut:" << testAction->shortcut().toString();
      this->addAction(testAction);
    }
#endif
  }
}

void MainWindow::createToolbars() {
  navigationToolBar = addToolBar("Navigation");
  navigationToolBar->setStyleSheet(
      "QToolBar { "
      "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
      "    stop: 0 #f8f9fa, stop: 1 #e9ecef); "
      "  border-bottom: 1px solid #d1d5db; "
      "  spacing: 8px; "
      "  padding: 5px; "
      "} "
      "QToolButton { "
      "  background-color: #007ACC; "
      "  border: none; "
      "  border-radius: 6px; "
      "  padding: 8px; "
      "  margin: 2px; "
      "  color: white; "
      "} "
      "QToolButton:hover { "
      "  background-color: #005a9e; "
      "} "
      "QToolButton:pressed { "
      "  background-color: #004578; "
      "} "
      "QToolButton:disabled { "
      "  background-color: #6c757d; "
      "  color: #adb5bd; "
      "}");
  navigationToolBar->addAction(backAction);
  navigationToolBar->addAction(forwardAction);
  navigationToolBar->addAction(reloadAction);
  navigationToolBar->addAction(stopAction);
  navigationToolBar->addAction(newTabAction);
}

void MainWindow::createMenus() {
  QMenu *fileMenu = menuBar()->addMenu("&File");
  fileMenu->addAction(newTabAction);
  fileMenu->addAction(closeTabAction);
  fileMenu->addSeparator();
  fileMenu->addAction("E&xit", this, &QWidget::close);

  QMenu *editMenu = menuBar()->addMenu("&Edit");
  QAction *copyAction = editMenu->addAction("&Copy");
  copyAction->setShortcut(QKeySequence::Copy);
  connect(copyAction, &QAction::triggered, this, [this]() {
    if (WebView *view = currentWebView()) {
      view->page()->triggerAction(QWebEnginePage::Copy);
    }
  });

  QAction *pasteAction = editMenu->addAction("&Paste");
  pasteAction->setShortcut(QKeySequence::Paste);
  connect(pasteAction, &QAction::triggered, this, [this]() {
    if (WebView *view = currentWebView()) {
      view->page()->triggerAction(QWebEnginePage::Paste);
    }
  });

  QAction *selectAllAction = editMenu->addAction("Select &All");
  selectAllAction->setShortcut(QKeySequence::SelectAll);
  connect(selectAllAction, &QAction::triggered, this, [this]() {
    if (WebView *view = currentWebView()) {
      view->page()->triggerAction(QWebEnginePage::SelectAll);
    }
  });

  QMenu *viewMenu = menuBar()->addMenu("&View");

  // Add toggle actions to view menu
  viewMenu->addAction(toggleTabBarAction);
  viewMenu->addSeparator();

  QAction *zoomInAction = viewMenu->addAction("Zoom &In");
  zoomInAction->setShortcut(QKeySequence::ZoomIn);
  connect(zoomInAction, &QAction::triggered, this, [this]() {
    if (WebView *view = currentWebView()) {
      view->setZoomFactor(view->zoomFactor() * 1.1);
    }
  });

  QAction *zoomOutAction = viewMenu->addAction("Zoom &Out");
  zoomOutAction->setShortcut(QKeySequence::ZoomOut);
  connect(zoomOutAction, &QAction::triggered, this, [this]() {
    if (WebView *view = currentWebView()) {
      view->setZoomFactor(view->zoomFactor() * 0.9);
    }
  });

  QAction *resetZoomAction = viewMenu->addAction("Reset Zoom");
  connect(resetZoomAction, &QAction::triggered, this, [this]() {
    if (WebView *view = currentWebView()) {
      view->setZoomFactor(1.0);
    }
  });

  viewMenu->addSeparator();
  viewMenu->addAction(toggleTabBarAction);
  viewMenu->addSeparator();

  // Add manager actions to menus
  if (pictureInPictureManager) {
    pictureInPictureManager->addToMenu(viewMenu);
  }

  QMenu *historyMenu = menuBar()->addMenu("&History");
  historyMenu->addAction(viewHistoryAction);
  // Dynamically populate history items or show a dialog

  QMenu *bookmarksMenu = menuBar()->addMenu("&Bookmarks");
  bookmarksMenu->addAction(addBookmarkAction);
  bookmarksMenu->addAction(viewBookmarksAction);
  // Dynamically populate bookmark items or show a dialog

  QMenu *toolsMenu = menuBar()->addMenu("&Tools");
  if (commandPaletteManager && commandPaletteManager->getCommandPaletteAction()) {
    toolsMenu->addAction(commandPaletteManager->getCommandPaletteAction());
  }
#ifdef QT_DEBUG
  if (commandPaletteManager && commandPaletteManager->getOpenTestPageAction()) {
    toolsMenu->addAction(commandPaletteManager->getOpenTestPageAction());
  }
#endif
  toolsMenu->addSeparator();
  toolsMenu->addAction(settingsAction);
  toolsMenu->addSeparator();
  toolsMenu->addAction(devToolsAction);

#ifdef DEBUG_MODE
  // Add debug menu for easy access to test pages
  QMenu *debugMenu = menuBar()->addMenu("&Debug");

  QAction *openVideoTestAction = debugMenu->addAction("Video Test Page");
  connect(openVideoTestAction, &QAction::triggered, this, [this]() {
    openTestPage("video_test.html");
  });

  QAction *openPiPTestAction = debugMenu->addAction("PiP Test Page");
  connect(openPiPTestAction, &QAction::triggered, this, [this]() {
    openTestPage("pip_test.html");
  });

  QAction *openPiPIntegrationTestAction = debugMenu->addAction("PiP Integration Test");
  connect(openPiPIntegrationTestAction, &QAction::triggered, this, [this]() {
    openTestPage("pip_integration_test.html");
  });

  QAction *openDebugTestAction = debugMenu->addAction("Debug Test Page");
  connect(openDebugTestAction, &QAction::triggered, this, [this]() {
    openTestPage("debug_test.html");
  });

  debugMenu->addSeparator();

  QAction *openTestsDirectoryAction = debugMenu->addAction("Open Tests Directory");
  connect(openTestsDirectoryAction, &QAction::triggered, this, [this]() {
    if (WebView *view = currentWebView()) {
      QString testsPath = QDir::currentPath() + "/tests/";
      view->load(QUrl::fromLocalFile(testsPath));
    }
  });
#endif

  // Add keyboard shortcuts for command palette actions
  // (Already added in createActions())
}

void MainWindow::setupConnections() {
  // Connect vertical tab widget signals
  connect(tabWidget, &VerticalTabWidget::currentChanged, this, [this](int index) {
    if (index != -1) {
      WebView *view = qobject_cast<WebView *>(tabWidget->widget(index));
      if (view) {
        updateAddressBar(view->url());
        updateWindowTitle(view->title());
        backAction->setEnabled(view->page()->history()->canGoBack());
        forwardAction->setEnabled(view->page()->history()->canGoForward());
      }
    } else {
      updateAddressBar(QUrl());
      updateWindowTitle("");
      backAction->setEnabled(false);
      forwardAction->setEnabled(false);
    }
  });

  connect(tabWidget, &VerticalTabWidget::tabCloseRequested, this, [this](int index) {
    if (tabWidget->count() > 1) {
      QWidget *widget = tabWidget->widget(index);
      tabWidget->removeTab(index);
      widget->deleteLater();
    }
  });

  connect(tabWidget, &VerticalTabWidget::newTabRequested, this, &MainWindow::newTab);

  // Connect address bar and integrated address bar
  connect(addressBar, &QLineEdit::returnPressed, this, &MainWindow::goToUrl);
  connect(tabWidget, &VerticalTabWidget::addressBarReturnPressed, this, &MainWindow::goToUrl);

  connect(newTabAction, &QAction::triggered, this, &MainWindow::newTab);
  connect(closeTabAction, &QAction::triggered, this, &MainWindow::closeCurrentTab);
  connect(backAction, &QAction::triggered, this, &MainWindow::goBack);
  connect(forwardAction, &QAction::triggered, this, &MainWindow::goForward);
  connect(reloadAction, &QAction::triggered, this, &MainWindow::reloadPage);
  connect(stopAction, &QAction::triggered, this, &MainWindow::stopLoading);

  connect(addBookmarkAction, &QAction::triggered, this, &MainWindow::addBookmark);
  connect(viewBookmarksAction, &QAction::triggered, this, &MainWindow::showBookmarks);
  connect(viewHistoryAction, &QAction::triggered, this, &MainWindow::showHistory);
  connect(settingsAction, &QAction::triggered, this, &MainWindow::showSettings);
  connect(devToolsAction, &QAction::triggered, this, &MainWindow::showDevTools);

  connect(toggleTabBarAction, &QAction::triggered, this, &MainWindow::toggleTabBar);

  connect(openLinkInNewTabAction, &QAction::triggered, this, &MainWindow::openLinkInNewTab);

  // Connect bookmark manager signals
  connect(bookmarkManager, &BookmarkManager::bookmarkActivated, this, [this](const QUrl &url) {
    if (WebView *view = currentWebView()) {
      view->load(url);
    }
  });

  connect(bookmarkManager, &BookmarkManager::openBookmarkInNewTab, this, [this](const QUrl &url) {
    newTab();
    if (WebView *view = currentWebView()) {
      view->load(url);
    }
  });
}

WebView *MainWindow::currentWebView() const {
  return qobject_cast<WebView *>(tabWidget->currentWidget());
}

void MainWindow::newTab() {
  WebView *webView = new WebView(this);
  int index = tabWidget->addTab(webView, "New Tab");
  tabWidget->setCurrentIndex(index);

  connect(webView, &WebView::urlChanged, this, &MainWindow::updateAddressBar);
  connect(webView, &WebView::titleChanged, this, &MainWindow::updateWindowTitle);
  connect(webView, &WebView::loadProgress, this, &MainWindow::handleLoadProgress);
  connect(webView, &WebView::loadFinished, this, [this, webView](bool ok) {
    stopAction->setEnabled(false);
    reloadAction->setEnabled(true);
    if (ok) {
      history.prepend({webView->title(), webView->url()});
      // Limit history size if needed

      // 自動的にすべての動画をPicture-in-Picture対応にする
      if (pictureInPictureManager) {
        pictureInPictureManager->enablePiPForAllVideos(webView);
      }
    }
    backAction->setEnabled(webView->page()->history()->canGoBack());
    forwardAction->setEnabled(webView->page()->history()->canGoForward());
  });
  connect(webView, &WebView::loadStarted, this, [this]() {
    stopAction->setEnabled(true);
    reloadAction->setEnabled(false);
  });

  webView->setContextMenuPolicy(Qt::CustomContextMenu);
  connect(webView, &WebView::customContextMenuRequested, this, &MainWindow::handleContextMenuRequested);

  webView->load(homePageUrl); // Load home page in new tab
  addressBar->setFocus();
}

void MainWindow::closeCurrentTab() {
  if (tabWidget->count() <= 1) { // Don't close the last tab, or close window
    close();
    return;
  }
  int index = tabWidget->currentIndex();
  if (index != -1) {
    WebView *view = qobject_cast<WebView *>(tabWidget->widget(index));
    tabWidget->removeTab(index);
    delete view;
  }
}

void MainWindow::goToUrl() {
  if (WebView *view = currentWebView()) {
    // Get URL text from appropriate address bar
    QString urlString;

    // Check if the signal is coming from the integrated address bar
    QObject *signalSender = sender();
    if (signalSender == tabWidget) {
      // Get text from integrated address bar in the sidebar
      QLineEdit *integratedBar = tabWidget->getIntegratedAddressBar();
      if (integratedBar) {
        urlString = integratedBar->text();
      }
    } else {
      // Get text from main address bar
      urlString = addressBar->text();
    }

    if (urlString.isEmpty())
      return;

    QUrl url = QUrl::fromUserInput(urlString);
    if (url.scheme().isEmpty()) {                                // If no scheme, assume it's a search query or needs http
      if (urlString.contains(".") && !urlString.contains(" ")) { // Basic check for domain-like string
        url = QUrl("http://" + urlString);
      } else {
        url = QUrl(defaultSearchEngineUrl.arg(urlString));
      }
    }
    view->load(url);
  }
}

void MainWindow::goBack() {
  if (WebView *view = currentWebView()) {
    view->back();
  }
}

void MainWindow::goForward() {
  if (WebView *view = currentWebView()) {
    view->forward();
  }
}

void MainWindow::reloadPage() {
  if (WebView *view = currentWebView()) {
    view->reload();
  }
}

void MainWindow::stopLoading() {
  if (WebView *view = currentWebView()) {
    view->stop();
  }
}

void MainWindow::updateAddressBar(const QUrl &url) {
  if (tabWidget->currentWidget() == sender() || (currentWebView() && currentWebView()->url() == url)) {
    // Update both address bars
    addressBar->setText(url.toString());

    QLineEdit *integratedBar = tabWidget->getIntegratedAddressBar();
    if (integratedBar) {
      integratedBar->setText(url.toString());
    }
  }
  if (WebView *view = currentWebView()) { // Update history navigation buttons
    backAction->setEnabled(view->page()->history()->canGoBack());
    forwardAction->setEnabled(view->page()->history()->canGoForward());
  }
}

void MainWindow::updateWindowTitle(const QString &title) {
  if (tabWidget->currentWidget() == sender()) {
    int index = tabWidget->currentIndex();
    if (index != -1) {
      if (title.isEmpty()) {
        tabWidget->setTabText(index, "Loading...");
      } else {
        tabWidget->setTabText(index, title.left(20)); // Truncate for tab
      }
    }
  }
  // Optionally set main window title to active tab title
  // setWindowTitle(title + " - MyBrowser");
}

void MainWindow::handleLoadProgress(int progress) {
  if (tabWidget->currentWidget() == sender()) {
    if (progress > 0 && progress < 100) {
      // statusLabel->setText(QString("Loading... %1%").arg(progress)); // 削除
      progressBar->setValue(progress);
      // statusLabel->setVisible(true); // 削除
      progressBar->setVisible(true);
    } else if (progress == 100) {
      // statusLabel->setText("Ready"); // 削除
      progressBar->setVisible(false);
      // statusLabel->setVisible(false); // 削除
    } else { // progress == 0 or other cases
      // statusLabel->setVisible(false); // 削除
      progressBar->setVisible(false);
    }
    adjustStatusWidgetsGeometry(); // Adjust geometry when visibility changes
  }
}

void MainWindow::handleContextMenuRequested(const QPoint &pos) {
  WebView *view = qobject_cast<WebView *>(sender());
  if (!view)
    return;

  QMenu contextMenu(this);
  QWebEnginePage::WebAction webAction;

  webAction = QWebEnginePage::Back;
  if (view->page()->action(webAction)->isEnabled()) {
    contextMenu.addAction(view->page()->action(webAction));
  }
  webAction = QWebEnginePage::Forward;
  if (view->page()->action(webAction)->isEnabled()) {
    contextMenu.addAction(view->page()->action(webAction));
  }
  webAction = QWebEnginePage::Reload;
  contextMenu.addAction(view->page()->action(webAction));
  contextMenu.addSeparator();

  webAction = QWebEnginePage::CopyLinkToClipboard;
  QAction *copyLinkAction = view->page()->action(webAction);
  if (copyLinkAction->isEnabled()) { // Check if the action is valid and enabled
    contextMenu.addAction(copyLinkAction);
  }

  // Note: QWebEngineHitTestResult is not available in Qt 6
  // For now, we'll add the action but it may not work properly for link detection
  contextMenu.addAction(openLinkInNewTabAction);
  openLinkInNewTabAction->setData(view->url()); // Store current URL as fallback

  webAction = QWebEnginePage::CopyImageToClipboard; // Or CopyImageUrlToClipboard
  QAction *copyImageAction = view->page()->action(webAction);
  if (copyImageAction->isEnabled()) { // Check if the action is valid and enabled
    contextMenu.addAction(copyImageAction);
  }

  webAction = QWebEnginePage::Copy;
  contextMenu.addAction(view->page()->action(webAction));
  webAction = QWebEnginePage::Paste;
  contextMenu.addAction(view->page()->action(webAction));
  contextMenu.addSeparator();
  webAction = QWebEnginePage::SelectAll;
  contextMenu.addAction(view->page()->action(webAction));
  contextMenu.addSeparator();

  // Add manager context menu actions
  if (pictureInPictureManager) {
    pictureInPictureManager->addToContextMenu(&contextMenu);
  }

  contextMenu.exec(view->mapToGlobal(pos));
}

void MainWindow::openLinkInNewTab() {
  QAction *action = qobject_cast<QAction *>(sender());
  if (action) {
    QUrl url = action->data().toUrl();
    if (url.isValid()) {
      newTab(); // Creates a new tab and sets it as current
      if (WebView *newView = currentWebView()) {
        newView->load(url);
      }
    }
  }
}

void MainWindow::addBookmark() {
  if (WebView *view = currentWebView()) {
    if (!view->url().isEmpty()) {
      QString title = view->title();
      if (title.isEmpty())
        title = view->url().host(); // Use host if title is empty

      bookmarkManager->addBookmark(title, view->url());
    }
  }
}

void MainWindow::showBookmarks() {
  // This is a placeholder. Implement a proper bookmark manager dialog/sidebar.
  if (bookmarks.isEmpty()) {
    QMessageBox::information(this, "Bookmarks", "No bookmarks yet.");
    return;
  }
  QMenu bookmarksMenu(this);
  for (const auto &bm : bookmarks) {
    QAction *action = bookmarksMenu.addAction(bm.first); // Use name for display
    action->setData(bm.second);                          // Store URL
  }
  connect(&bookmarksMenu, &QMenu::triggered, this, [this](QAction *action) {
    QUrl url = action->data().toUrl();
    if (url.isValid()) {
      if (WebView *view = currentWebView()) {
        view->load(url);
      } else {
        newTab();
        if (WebView *newView = currentWebView())
          newView->load(url);
      }
    }
  });
  bookmarksMenu.exec(QCursor::pos());
}

void MainWindow::showHistory() {
  // This is a placeholder. Implement a proper history manager dialog/sidebar.
  if (history.isEmpty()) {
    QMessageBox::information(this, "History", "Browsing history is empty.");
    return;
  }
  QMenu historyMenu(this);
  for (const auto &item : history) {
    QAction *action = historyMenu.addAction(item.first.left(50) + (item.first.length() > 50 ? "..." : "") + " (" + item.second.host() + ")");
    action->setData(item.second);
  }
  connect(&historyMenu, &QMenu::triggered, this, [this](QAction *action) {
    QUrl url = action->data().toUrl();
    if (url.isValid()) {
      if (WebView *view = currentWebView()) {
        view->load(url);
      } else {
        newTab();
        if (WebView *newView = currentWebView())
          newView->load(url);
      }
    }
  });
  historyMenu.exec(QCursor::pos());
}

void MainWindow::showSettings() {
  // Placeholder for settings dialog
  // Example: Allow changing homepage
  bool ok;
  QString newHomepage = QInputDialog::getText(this, "Settings", "Enter new homepage URL:", QLineEdit::Normal, homePageUrl, &ok);
  if (ok && !newHomepage.isEmpty()) {
    homePageUrl = QUrl::fromUserInput(newHomepage).toString();
    QMessageBox::information(this, "Settings", "Homepage updated.");
    // Persist settings
  }

  // Example: Allow changing default search engine (simplified)
  QString newSearchEngine = QInputDialog::getText(this, "Settings",
                                                  "Enter new default search engine URL (use %s for query):",
                                                  QLineEdit::Normal, defaultSearchEngineUrl, &ok);
  if (ok && !newSearchEngine.isEmpty() && newSearchEngine.contains("%s")) {
    defaultSearchEngineUrl = newSearchEngine.replace("%s", "%1"); // Qt uses %1 for arg
    QMessageBox::information(this, "Settings", "Default search engine updated.");
    // Persist settings
  } else if (ok && !newSearchEngine.contains("%s")) {
    QMessageBox::warning(this, "Settings", "Search engine URL must contain '%s' for the query placeholder.");
  }
}

void MainWindow::showDevTools() {
  if (WebView *view = currentWebView()) {
    view->showDevTools();
  }
}

void MainWindow::closeEvent(QCloseEvent *event) {
  // Save bookmarks, history, settings here before closing
  QMainWindow::closeEvent(event);
}

void MainWindow::loadStyleSheet() {
  QFile file(":/styles/styles.qss");
  if (!file.exists()) {
    // Fallback to file system
    file.setFileName(QDir::currentPath() + "/styles/styles.qss");
  }

  if (file.open(QFile::ReadOnly | QFile::Text)) {
    QTextStream stream(&file);
    QString styleSheet = stream.readAll();
    setStyleSheet(styleSheet);
    file.close();
  }
}

void MainWindow::toggleTabBar() {
  // In overlay mode, this toggles the sidebar since tabs are in the sidebar
  if (tabWidget->isSidebarVisible()) {
    tabWidget->hideSidebar();
  } else {
    tabWidget->showSidebar();
  }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  adjustStatusWidgetsGeometry();
}

void MainWindow::adjustStatusWidgetsGeometry() {
  if (!progressBar)
    return;

  int barWidth = 150;
  int barHeight = 5;

  if (!progressBar->isVisible()) {
    return;
  }

  int xPos = (width() - barWidth) / 2;
  int yPos = 10;

  progressBar->setGeometry(xPos, yPos, barWidth, barHeight);
}

#ifdef DEBUG_MODE
void MainWindow::openTestPage(const QString &fileName) {
  // Get the application directory
  QString appDir = QCoreApplication::applicationDirPath();
  QDir projectDir(appDir);

  // Find the project root directory (where tests folder exists)
  while (!projectDir.exists("tests") && projectDir.cdUp()) {
    // Move to parent directory
  }

  QString testsPath = projectDir.absoluteFilePath("tests/" + fileName);
  QUrl fileUrl = QUrl::fromLocalFile(testsPath);

  if (QFile::exists(testsPath)) {
    newTab();
    if (WebView *view = currentWebView()) {
      view->load(fileUrl);
    }
  } else {
    QMessageBox::warning(this, "Test Page Not Found",
                         QString("Could not find test file: %1").arg(testsPath));
  }
}
#endif
