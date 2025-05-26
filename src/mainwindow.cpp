#include "mainwindow.h"
#include "bookmarkmanager.h"
#include "verticaltabwidget.h"
#include "webview.h"
#include "workspacemanager.h"
#include <QCursor>
#include <QDockWidget>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QTabBar>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWebEngineHistory>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), workspaceManager(nullptr), bookmarkManager(nullptr) {
  setupUI();
  setupConnections();
  newTab(); // Open a default tab
}

MainWindow::~MainWindow() {
}

void MainWindow::setupUI() {
  // Create vertical tab widget
  tabWidget = new VerticalTabWidget(this);

  // Create managers
  workspaceManager = new WorkspaceManager(this);
  workspaceManager->setTabWidget(tabWidget);

  bookmarkManager = new BookmarkManager(this);

  addressBar = new QLineEdit(this);
  addressBar->setPlaceholderText("Enter URL or search query");

  createActions();
  createToolbars();
  createMenus();

  // Create main layout with workspace toolbar and vertical tabs
  QWidget *centralWidget = new QWidget(this);
  QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Add workspace toolbar at the top
  QWidget *workspaceToolbar = workspaceManager->createWorkspaceToolbar(this);
  mainLayout->addWidget(workspaceToolbar);

  // Add navigation toolbar
  mainLayout->addWidget(navigationToolBar);
  mainLayout->addWidget(addressBar);

  // Add vertical tab widget
  mainLayout->addWidget(tabWidget);

  setCentralWidget(centralWidget);

  // Setup bookmark dock
  QDockWidget *bookmarkDock = bookmarkManager->createBookmarkDock(this);
  addDockWidget(Qt::RightDockWidgetArea, bookmarkDock);

  // Create status bar
  statusLabel = new QLabel("Ready");
  progressBar = new QProgressBar();
  progressBar->setVisible(false);
  this->statusBar()->addWidget(statusLabel);
  this->statusBar()->addPermanentWidget(progressBar);

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

  openLinkInNewTabAction = new QAction("Open Link in New Tab", this);
}

void MainWindow::createToolbars() {
  navigationToolBar = addToolBar("Navigation");
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

  QMenu *historyMenu = menuBar()->addMenu("&History");
  historyMenu->addAction(viewHistoryAction);
  // Dynamically populate history items or show a dialog

  QMenu *bookmarksMenu = menuBar()->addMenu("&Bookmarks");
  bookmarksMenu->addAction(addBookmarkAction);
  bookmarksMenu->addAction(viewBookmarksAction);
  // Dynamically populate bookmark items or show a dialog

  QMenu *toolsMenu = menuBar()->addMenu("&Tools");
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

  connect(addressBar, &QLineEdit::returnPressed, this, &MainWindow::goToUrl);

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
    QString urlString = addressBar->text();
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
    addressBar->setText(url.toString());
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
      progressBar->setVisible(true);
      progressBar->setValue(progress);
      statusLabel->setText(QString("Loading... %1%").arg(progress));
    } else if (progress == 100) {
      progressBar->setVisible(false);
      statusLabel->setText("Ready");
    }
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
