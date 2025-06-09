#include "commandpalettemanager.h"
#include "../main-window/mainwindow.h"
#include "../picture-in-picture/pictureinpicturemanager.h"
#include "../tab-widget/verticaltabwidget.h"
#include "../webview/webview.h"
#include "../workspace/workspacemanager.h"
#include "commandpalettedialog.h"
#include <QAction>
#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QInputDialog>
#include <QKeySequence>
#include <QMessageBox>
#include <QTextStream>
#include <QWebEnginePage>

CommandPaletteManager::CommandPaletteManager(MainWindow *parent)
    : QObject(parent), mainWindow(parent), commandPaletteDialog(nullptr), commandPaletteAction(nullptr) {
  loadSearchHistory();
  // CommandPaletteDialogの初期化は遅延させる（初回使用時に作成）
}

CommandPaletteManager::~CommandPaletteManager() {
  saveSearchHistory();
}

void CommandPaletteManager::setupActions() {
  qDebug() << "CommandPaletteManager::setupActions() called";

  // Command palette action with Cmd+T and Ctrl+T shortcuts
  commandPaletteAction = new QAction("Command Palette", mainWindow);
  // Set multiple shortcuts: Cmd+T and Ctrl+T
  QList<QKeySequence> shortcuts;
  shortcuts << QKeySequence(Qt::CTRL | Qt::Key_T); // Ctrl+T
  commandPaletteAction->setShortcuts(shortcuts);
  commandPaletteAction->setStatusTip("Open command palette (Cmd+T or Ctrl+T)");
  commandPaletteAction->setToolTip("Open command palette (Cmd+T or Ctrl+T)");

  qDebug() << "Created commandPaletteAction with shortcuts:";
  for (const QKeySequence &shortcut : commandPaletteAction->shortcuts()) {
    qDebug() << "  -" << shortcut.toString();
  }
  qDebug() << "Action parent:" << commandPaletteAction->parent();

  connect(commandPaletteAction, &QAction::triggered, this, &CommandPaletteManager::onQuickSearchTriggered);
  qDebug() << "Connected triggered signal to onQuickSearchTriggered slot";

#ifdef QT_DEBUG
  // デバッグモード時のテストページアクション
  openTestPageAction = new QAction("Open Test Page", mainWindow);
  openTestPageAction->setShortcut(QKeySequence(Qt::META | Qt::SHIFT | Qt::Key_T)); // Cmd+Shift+T
  openTestPageAction->setStatusTip("Open test pages for debugging (Cmd+Shift+T)");
  openTestPageAction->setToolTip("Open test pages for debugging (Cmd+Shift+T)");

  connect(openTestPageAction, &QAction::triggered, this, &CommandPaletteManager::onOpenTestPageTriggered);
#endif
}

void CommandPaletteManager::showCommandPalette() {
  qDebug() << "Showing command palette..."; // デバッグ出力

  // 遅延初期化：初回使用時にダイアログを作成
  if (!commandPaletteDialog) {
    qDebug() << "Creating command palette dialog for the first time...";
    commandPaletteDialog = new CommandPaletteDialog(mainWindow);

    // コマンドとクイック検索のシグナル接続
    connect(commandPaletteDialog, &CommandPaletteDialog::commandRequested,
            this, &CommandPaletteManager::handleCommand);
    connect(commandPaletteDialog, &CommandPaletteDialog::searchRequested,
            this, QOverload<const QString &>::of(&CommandPaletteManager::handleQuickSearch));

    qDebug() << "Command palette dialog created and connected successfully";
  }

  if (commandPaletteDialog) {
    qDebug() << "Dialog exists, showing it..."; // デバッグ出力
    commandPaletteDialog->setSearchHistory(searchHistory);
    commandPaletteDialog->showCentered();
  } else {
    qDebug() << "Dialog is still null after creation attempt!"; // デバッグ出力
  }
}

#ifdef QT_DEBUG
void CommandPaletteManager::openTestPage() {
  // testsフォルダ内のHTMLファイルのリストを作成
  // testsフォルダのパスを構築
  QString appDir = QCoreApplication::applicationDirPath();
  QDir projectDir(appDir);

  // プロジェクトのルートディレクトリを見つける
  while (!projectDir.exists("tests") && projectDir.cdUp()) {
    // 親ディレクトリに移動
  }

  QDir testsDir(projectDir.absoluteFilePath("tests"));
  QStringList testFiles;

  if (testsDir.exists()) {
    // HTMLファイルのみを取得
    QStringList filters;
    filters << "*.html" << "*.htm";
    testFiles = testsDir.entryList(filters, QDir::Files, QDir::Name);
  }

  if (testFiles.isEmpty()) {
    QMessageBox::warning(mainWindow, "No Test Files", "No HTML test files found in the tests directory.");
    return;
  }

  bool ok;
  QString selectedFile = QInputDialog::getItem(mainWindow, "Select Test Page",
                                               "Choose a test page to open:",
                                               testFiles, 0, false, &ok);

  if (ok && !selectedFile.isEmpty()) {
    // testsフォルダのパスを構築
    QString appDir = QCoreApplication::applicationDirPath();
    QDir projectDir(appDir);

    // プロジェクトのルートディレクトリを見つける
    while (!projectDir.exists("tests") && projectDir.cdUp()) {
      // 親ディレクトリに移動
    }

    QString testFilePath = projectDir.absoluteFilePath("tests/" + selectedFile);
    QUrl testUrl = QUrl::fromLocalFile(testFilePath);

    if (QFile::exists(testFilePath)) {
      // 新しいタブでテストページを開く
      mainWindow->newTab();
      if (WebView *webView = mainWindow->currentWebView()) {
        webView->load(testUrl);
      }
    } else {
      QMessageBox::warning(mainWindow, "Test Page Not Found",
                           QString("Test file not found: %1").arg(testFilePath));
    }
  }
}
#endif

void CommandPaletteManager::onQuickSearchTriggered() {
  qDebug() << "Command palette triggered! (Cmd+T pressed)"; // デバッグ出力
  showCommandPalette();
}

#ifdef QT_DEBUG
void CommandPaletteManager::onOpenTestPageTriggered() {
  openTestPage();
}
#endif

void CommandPaletteManager::handleQuickSearch(const QString &query) {
  handleQuickSearch(query, true); // 常に新しいタブで開く
}

void CommandPaletteManager::handleQuickSearch(const QString &query, bool inNewTab) {
  if (query.trimmed().isEmpty()) {
    return;
  }

  // 検索履歴に追加
  addToSearchHistory(query);

  QString urlString = query.trimmed();
  QUrl url;

  // URLとして直接アクセス可能かチェック
  if (urlString.contains('.') && !urlString.contains(' ')) {
    if (urlString.startsWith("http://") || urlString.startsWith("https://")) {
      url = QUrl(urlString);
    } else {
      url = QUrl("https://" + urlString);
    }
  } else {
    // 検索クエリとして処理
    QString defaultSearchEngineUrl = "https://www.google.com/search?q=%1";
    url = QUrl(defaultSearchEngineUrl.arg(QString(QUrl::toPercentEncoding(urlString))));
  }

  if (url.isValid()) {
    if (inNewTab) {
      // 新しいタブで開く
      mainWindow->newTab();
      if (WebView *webView = mainWindow->currentWebView()) {
        webView->load(url);
      }
    } else {
      // 現在のタブで開く
      if (WebView *webView = mainWindow->currentWebView()) {
        webView->load(url);
      }
    }
  }
}

void CommandPaletteManager::handleCommand(const QString &command) {
  QString cmd = command.toLower().trimmed();

  // コマンドカテゴリ別に処理を分散
  if (cmd.contains("tab") || cmd.contains("back") || cmd.contains("forward") ||
      cmd.contains("reload") || cmd.contains("stop")) {
    executeNavigationCommand(cmd);
  } else if (cmd.contains("zoom")) {
    executeZoomCommand(cmd);
  } else if (cmd.contains("bookmark")) {
    executeBookmarkCommand(cmd);
  } else if (cmd.contains("workspace")) {
    executeWorkspaceCommand(cmd);
  } else if (cmd.contains("history")) {
    executeHistoryCommand(cmd);
  } else if (cmd.contains("devtools") || cmd.contains("developer") ||
             cmd.contains("picture") || cmd.contains("pip") || cmd.contains("source")) {
    executeDeveloperCommand(cmd);
  } else if (cmd.contains("print") || cmd.contains("save") || cmd.contains("find")) {
    executePageCommand(cmd);
  } else if (cmd.contains("fullscreen") || cmd.contains("sidebar")) {
    executeWindowCommand(cmd);
  } else if (cmd.contains("settings") || cmd.contains("preferences")) {
    executeSettingsCommand(cmd);
  } else {
    QMessageBox::information(mainWindow, "Command Palette",
                             QString("Unknown command: %1").arg(command));
  }
}

void CommandPaletteManager::executeNavigationCommand(const QString &command) {
  if (command == "new tab" || command == "new") {
    mainWindow->newTab();
  } else if (command == "close tab" || command == "close") {
    mainWindow->closeCurrentTab();
  } else if (command == "reload page" || command == "reload" || command == "refresh") {
    mainWindow->reloadPage();
  } else if (command == "hard reload" || command == "force reload") {
    if (WebView *view = mainWindow->currentWebView()) {
      view->triggerPageAction(QWebEnginePage::ReloadAndBypassCache);
    }
  } else if (command == "stop loading" || command == "stop") {
    mainWindow->stopLoading();
  } else if (command == "go back" || command == "back") {
    mainWindow->goBack();
  } else if (command == "go forward" || command == "forward") {
    mainWindow->goForward();
  }
}

void CommandPaletteManager::executeZoomCommand(const QString &command) {
  WebView *view = mainWindow->currentWebView();
  if (!view)
    return;

  if (command == "zoom in" || command == "zoom+") {
    qreal factor = view->zoomFactor();
    view->setZoomFactor(factor * 1.25);
  } else if (command == "zoom out" || command == "zoom-") {
    qreal factor = view->zoomFactor();
    view->setZoomFactor(factor * 0.8);
  } else if (command == "reset zoom" || command == "zoom reset") {
    view->setZoomFactor(1.0);
  }
}

void CommandPaletteManager::executeBookmarkCommand(const QString &command) {
  if (command == "add bookmark" || command == "bookmark") {
    mainWindow->addBookmark();
  } else if (command == "show bookmarks" || command == "bookmarks") {
    mainWindow->showBookmarks();
  }
}

void CommandPaletteManager::executeWorkspaceCommand(const QString &command) {
  WorkspaceManager *workspaceManager = mainWindow->getWorkspaceManager();
  if (!workspaceManager)
    return;

  if (command == "new workspace") {
    workspaceManager->createNewWorkspace("New Workspace");
  } else if (command == "switch workspace") {
    QMessageBox::information(mainWindow, "Command Palette",
                             "Workspace switching UI not implemented yet");
  } else if (command == "rename workspace") {
    workspaceManager->renameWorkspace(workspaceManager->getCurrentWorkspaceId(), "");
  } else if (command == "delete workspace") {
    workspaceManager->deleteWorkspace(workspaceManager->getCurrentWorkspaceId());
  }
}

void CommandPaletteManager::executeHistoryCommand(const QString &command) {
  if (command == "show history" || command == "history") {
    mainWindow->showHistory();
  } else if (command == "clear history") {
    clearSearchHistory();
    QMessageBox::information(mainWindow, "Command Palette", "Search history cleared");
  } else if (command == "show downloads" || command == "downloads") {
    QMessageBox::information(mainWindow, "Command Palette",
                             "Downloads view not implemented yet");
  }
}

void CommandPaletteManager::executeDeveloperCommand(const QString &command) {
  if (command == "developer tools" || command == "devtools" || command == "dev tools") {
    mainWindow->showDevTools();
  } else if (command == "picture-in-picture" || command == "pip" || command == "picture in picture") {
    // PictureInPictureManagerに処理を委譲
    if (PictureInPictureManager *pipManager = mainWindow->getPictureInPictureManager()) {
      pipManager->createImagePiP(mainWindow->currentWebView());
    }
  } else if (command == "view source" || command == "source") {
    if (WebView *view = mainWindow->currentWebView()) {
      view->triggerPageAction(QWebEnginePage::ViewSource);
    }
#ifdef QT_DEBUG
  } else if (command == "open test page" || command == "test page" || command == "test") {
    openTestPage();
#endif
  }
}

void CommandPaletteManager::executePageCommand(const QString &command) {
  if (command == "print page" || command == "print") {
    QMessageBox::information(mainWindow, "Command Palette",
                             "Print functionality not implemented yet");
  } else if (command == "save page" || command == "save") {
    if (WebView *view = mainWindow->currentWebView()) {
      view->triggerPageAction(QWebEnginePage::SavePage);
    }
  } else if (command == "find in page" || command == "find") {
    QMessageBox::information(mainWindow, "Command Palette",
                             "Find in page not implemented yet");
  }
}

void CommandPaletteManager::executeWindowCommand(const QString &command) {
  if (command == "toggle fullscreen" || command == "fullscreen") {
    if (mainWindow->isFullScreen()) {
      mainWindow->showNormal();
    } else {
      mainWindow->showFullScreen();
    }
  } else if (command == "show sidebar" || command == "sidebar") {
    mainWindow->getTabWidget()->showSidebar();
  } else if (command == "hide sidebar") {
    mainWindow->getTabWidget()->hideSidebar();
  }
}

void CommandPaletteManager::executeSettingsCommand(const QString &command) {
  if (command == "settings" || command == "preferences") {
    mainWindow->showSettings();
  }
}

void CommandPaletteManager::addToSearchHistory(const QString &query) {
  if (!query.trimmed().isEmpty() && !searchHistory.contains(query)) {
    searchHistory.prepend(query);

    // 履歴のサイズ制限
    while (searchHistory.size() > 50) {
      searchHistory.removeLast();
    }

    // CommandPaletteDialogに更新された履歴を設定
    if (commandPaletteDialog) {
      commandPaletteDialog->setSearchHistory(searchHistory);
    }
  }
}

void CommandPaletteManager::clearSearchHistory() {
  searchHistory.clear();
  saveSearchHistory();

  if (commandPaletteDialog) {
    commandPaletteDialog->setSearchHistory(searchHistory);
  }
}

void CommandPaletteManager::saveSearchHistory() {
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

void CommandPaletteManager::loadSearchHistory() {
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
