#include "mainwindow.h"
#include "bookmarkmanager.h"
#include "quicksearchdialog.h"
#include "verticaltabwidget.h"
#include "webview.h"
#include "workspacemanager.h"
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
    : QMainWindow(parent), workspaceManager(nullptr), bookmarkManager(nullptr), quickSearchDialog(nullptr) {
  loadStyleSheet();
  setupUI();
  setupConnections();
  newTab(); // Open a default tab

  // Load search history from file
  loadSearchHistory();

  // Initialize command palette (quick search dialog)
  quickSearchDialog = new QuickSearchDialog(this);
  quickSearchDialog->setSearchHistory(searchHistory);
  connect(quickSearchDialog, &QuickSearchDialog::searchRequested, this, [this](const QString &query) {
    handleQuickSearch(query);
  });
  connect(quickSearchDialog, &QuickSearchDialog::commandRequested, this, [this](const QString &command) {
    handleCommand(command);
  });
}

MainWindow::~MainWindow() {
  // Save search history before destroying
  saveSearchHistory();
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
  newTabAction->setShortcut(QKeySequence::AddTab);

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

  // Command palette action
  quickSearchAction = new QAction("Command Palette", this);
  quickSearchAction->setShortcut(QKeySequence("Ctrl+T"));

  // Toggle panel actions - only keep tab bar toggle with Cmd+S
  toggleTabBarAction = new QAction("Toggle Sidebar", this);
  toggleTabBarAction->setShortcut(QKeySequence("Ctrl+S"));
  toggleTabBarAction->setCheckable(true);
  toggleTabBarAction->setChecked(false); // Default to hidden

  openLinkInNewTabAction = new QAction("Open Link in New Tab", this);
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

  QMenu *historyMenu = menuBar()->addMenu("&History");
  historyMenu->addAction(viewHistoryAction);
  // Dynamically populate history items or show a dialog

  QMenu *bookmarksMenu = menuBar()->addMenu("&Bookmarks");
  bookmarksMenu->addAction(addBookmarkAction);
  bookmarksMenu->addAction(viewBookmarksAction);
  // Dynamically populate bookmark items or show a dialog

  QMenu *toolsMenu = menuBar()->addMenu("&Tools");
  toolsMenu->addAction(quickSearchAction);
  toolsMenu->addSeparator();
  toolsMenu->addAction(settingsAction);
  toolsMenu->addSeparator();
  toolsMenu->addAction(devToolsAction);
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

  connect(quickSearchAction, &QAction::triggered, this, &MainWindow::quickSearch);
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
    // For Qt WebEngine, developer tools can be shown in a separate window
    QWebEnginePage *page = view->page();
    page->setDevToolsPage(new QWebEnginePage(this));
    // Note: In a full implementation, you'd want to show this in a separate window or dock widget
    QMessageBox::information(this, "Developer Tools", "Developer tools functionality requires additional implementation for Qt WebEngine.");
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

void MainWindow::quickSearch() {
  if (quickSearchDialog) {
    quickSearchDialog->setSearchHistory(searchHistory);
    quickSearchDialog->exec();
  }
}

void MainWindow::handleQuickSearch(const QString &query) {
  if (query.isEmpty())
    return;

  // Add to search history (avoid duplicates)
  searchHistory.removeAll(query);
  searchHistory.prepend(query);

  // Keep only last 10 searches
  if (searchHistory.size() > 10) {
    searchHistory.removeLast();
  }

  // Update quick search dialog with new history
  if (quickSearchDialog) {
    quickSearchDialog->setSearchHistory(searchHistory);
  }

  QString urlString;

  // Check if it's a URL
  if (query.startsWith("http://") || query.startsWith("https://")) {
    urlString = query;
  } else if (query.contains(".") && !query.contains(" ") && query.indexOf(".") > 0) {
    // Looks like a domain, add https://
    urlString = "https://" + query;
  } else {
    // Create Google search URL
    urlString = QString("https://www.google.com/search?q=%1")
                    .arg(QUrl::toPercentEncoding(query).constData());
  }

  // Load in current tab or create new tab if none exists
  WebView *view = currentWebView();
  if (!view) {
    newTab();
    view = currentWebView();
  }

  if (view) {
    view->setUrl(QUrl(urlString));
  }
}

void MainWindow::saveSearchHistory() {
  QDir appDir = QDir::home();
  if (!appDir.exists(".mybrowser")) {
    appDir.mkdir(".mybrowser");
  }

  QString historyPath = appDir.absoluteFilePath(".mybrowser/search_history.txt");
  QFile file(historyPath);

  if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
    QTextStream out(&file);
    for (const QString &entry : searchHistory) {
      out << entry << "\n";
    }
  }
}

void MainWindow::loadSearchHistory() {
  QDir appDir = QDir::home();
  QString historyPath = appDir.absoluteFilePath(".mybrowser/search_history.txt");
  QFile file(historyPath);

  searchHistory.clear();

  if (file.open(QIODevice::ReadOnly | QIODevice::Text)) {
    QTextStream in(&file);
    while (!in.atEnd()) {
      QString line = in.readLine().trimmed();
      if (!line.isEmpty()) {
        searchHistory.append(line);
      }
    }
  }
}

void MainWindow::handleCommand(const QString &command) {
  QString cmd = command.toLower().trimmed();

  // Navigation commands
  if (cmd == "new tab" || cmd == "new") {
    newTab();
  } else if (cmd == "close tab" || cmd == "close") {
    closeCurrentTab();
  } else if (cmd == "reload page" || cmd == "reload" || cmd == "refresh") {
    reloadPage();
  } else if (cmd == "hard reload" || cmd == "force reload") {
    if (WebView *view = currentWebView()) {
      view->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);
    }
  } else if (cmd == "stop loading" || cmd == "stop") {
    stopLoading();
  } else if (cmd == "go back" || cmd == "back") {
    goBack();
  } else if (cmd == "go forward" || cmd == "forward") {
    goForward();
  }

  // Zoom commands
  else if (cmd == "zoom in" || cmd == "zoom+") {
    if (WebView *view = currentWebView()) {
      qreal factor = view->zoomFactor();
      view->setZoomFactor(factor * 1.25);
    }
  } else if (cmd == "zoom out" || cmd == "zoom-") {
    if (WebView *view = currentWebView()) {
      qreal factor = view->zoomFactor();
      view->setZoomFactor(factor * 0.8);
    }
  } else if (cmd == "reset zoom" || cmd == "zoom reset") {
    if (WebView *view = currentWebView()) {
      view->setZoomFactor(1.0);
    }
  }

  // Bookmark commands
  else if (cmd == "add bookmark" || cmd == "bookmark") {
    addBookmark();
  } else if (cmd == "show bookmarks" || cmd == "bookmarks") {
    showBookmarks();
  }

  // Workspace commands
  else if (cmd == "new workspace") {
    if (workspaceManager) {
      workspaceManager->createNewWorkspace("New Workspace");
    }
  } else if (cmd == "switch workspace") {
    // Show workspace switcher - this could be enhanced with a list
    QMessageBox::information(this, "Command Palette", "Workspace switching UI not implemented yet");
  } else if (cmd == "rename workspace") {
    if (workspaceManager) {
      workspaceManager->renameWorkspace(workspaceManager->getCurrentWorkspaceId(), "");
    }
  } else if (cmd == "delete workspace") {
    if (workspaceManager) {
      workspaceManager->deleteWorkspace(workspaceManager->getCurrentWorkspaceId());
    }
  }

  // History and other commands
  else if (cmd == "show history" || cmd == "history") {
    showHistory();
  } else if (cmd == "clear history") {
    // Clear search history
    searchHistory.clear();
    saveSearchHistory();
    if (quickSearchDialog) {
      quickSearchDialog->setSearchHistory(searchHistory);
    }
    QMessageBox::information(this, "Command Palette", "Search history cleared");
  } else if (cmd == "show downloads" || cmd == "downloads") {
    QMessageBox::information(this, "Command Palette", "Downloads view not implemented yet");
  }

  // Developer tools
  else if (cmd == "developer tools" || cmd == "devtools" || cmd == "dev tools") {
    showDevTools();
  } else if (cmd == "view source" || cmd == "source") {
    if (WebView *view = currentWebView()) {
      view->triggerPageAction(QWebEnginePage::ViewSource);
    }
  }

  // Page actions
  else if (cmd == "print page" || cmd == "print") {
    if (WebView *view = currentWebView()) {
      // Qt 6 WebEngine doesn't have PrintToPdf action
      QMessageBox::information(this, "Command Palette", "Print functionality not implemented yet");
    }
  } else if (cmd == "save page" || cmd == "save") {
    if (WebView *view = currentWebView()) {
      view->triggerPageAction(QWebEnginePage::SavePage);
    }
  } else if (cmd == "find in page" || cmd == "find") {
    // This would need a find bar implementation
    QMessageBox::information(this, "Command Palette", "Find in page not implemented yet");
  }

  // Window commands
  else if (cmd == "toggle fullscreen" || cmd == "fullscreen") {
    if (isFullScreen()) {
      showNormal();
    } else {
      showFullScreen();
    }
  } else if (cmd == "show sidebar" || cmd == "sidebar") {
    if (tabWidget) {
      tabWidget->showSidebar();
    }
  } else if (cmd == "hide sidebar") {
    if (tabWidget) {
      tabWidget->hideSidebar();
    }
  }

  // Settings
  else if (cmd == "settings" || cmd == "preferences") {
    showSettings();
  }

  // Unknown command
  else {
    QMessageBox::information(this, "Command Palette",
                             QString("Unknown command: %1").arg(command));
  }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
  QMainWindow::resizeEvent(event);
  adjustStatusWidgetsGeometry();
}

void MainWindow::adjustStatusWidgetsGeometry() {
  if (!progressBar)
    return; // statusLabelのチェックを削除

  int barWidth = 150;
  int barHeight = 5; // より細くする
  // int labelWidth = 150; // 削除
  // int labelHeight = 20; // 削除
  // int spacing = 5; // 削除

  // int totalWidth = labelWidth + spacing + barWidth; // 修正
  // if (!progressBar->isVisible()) { // 修正
  //   totalWidth = labelWidth; // 修正
  // }
  // if (!statusLabel->isVisible()){ // 削除
  //     totalWidth = barWidth; // 削除
  // }
  if (!progressBar->isVisible()) { // statusLabelのチェックを削除
    return;
  }

  // int xPos = (width() - totalWidth) / 2; // 修正
  int xPos = (width() - barWidth) / 2; // progressBarのみなので修正
  int yPos = 10;                       // Small margin from the top

  // if (statusLabel->isVisible() && progressBar->isVisible()){ // 削除
  //   statusLabel->setGeometry(xPos, yPos, labelWidth, labelHeight); // 削除
  //   progressBar->setGeometry(xPos + labelWidth + spacing, yPos + (labelHeight - barHeight)/2, barWidth, barHeight); // 削除
  // } else if (statusLabel->isVisible()){ // 削除
  //   statusLabel->setGeometry((width() - labelWidth) / 2, yPos, labelWidth, labelHeight); // 削除
  // } else if (progressBar->isVisible()){ // 修正
  progressBar->setGeometry(xPos, yPos, barWidth, barHeight); // progressBarのみなので修正
  // }
}
