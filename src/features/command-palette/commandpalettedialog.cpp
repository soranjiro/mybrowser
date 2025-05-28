#include "commandpalettedialog.h"
#include <QApplication>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QKeyEvent>
#include <QListWidgetItem>
#include <QScreen>

CommandPaletteDialog::CommandPaletteDialog(QWidget *parent)
    : QDialog(parent), selectedIndex(-1) {
  setupUI();

  // タイマーセットアップ
  searchTimer = new QTimer(this);
  searchTimer->setSingleShot(true);
  searchTimer->setInterval(200);
  connect(searchTimer, &QTimer::timeout, this, &CommandPaletteDialog::updateSuggestions);

  // グローバルイベントフィルターをインストール
  QApplication::instance()->installEventFilter(this);
}

CommandPaletteDialog::~CommandPaletteDialog() {
  QApplication::instance()->removeEventFilter(this);
}

void CommandPaletteDialog::setupUI() {
  // Spotlightライクなダイアログ設定
  setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setModal(true);

  // メインレイアウト
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);

  // コンテナウィジェット
  QWidget *container = new QWidget();
  container->setStyleSheet(
      "QWidget {"
      "  background-color: rgba(25, 25, 25, 240);"
      "  border: 1px solid rgba(255, 255, 255, 0.15);"
      "  border-radius: 16px;"
      "}");

  // シャドウエフェクト
  QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect();
  shadow->setBlurRadius(40);
  shadow->setColor(QColor(0, 0, 0, 120));
  shadow->setOffset(0, 8);
  container->setGraphicsEffect(shadow);

  QVBoxLayout *containerLayout = new QVBoxLayout(container);
  containerLayout->setContentsMargins(24, 24, 24, 24);
  containerLayout->setSpacing(16);

  // 検索入力フィールド
  searchInput = new QLineEdit();
  searchInput->setPlaceholderText("Search or type '>' for commands...");
  searchInput->setStyleSheet(
      "QLineEdit {"
      "  background-color: rgba(255, 255, 255, 0.08);"
      "  border: 2px solid rgba(255, 255, 255, 0.12);"
      "  border-radius: 12px;"
      "  padding: 16px 20px;"
      "  font-size: 20px;"
      "  font-weight: 500;"
      "  color: white;"
      "  selection-background-color: rgba(0, 122, 255, 0.4);"
      "}"
      "QLineEdit:focus {"
      "  border-color: rgba(0, 122, 255, 0.6);"
      "  background-color: rgba(255, 255, 255, 0.12);"
      "}"
      "QLineEdit::placeholder {"
      "  color: rgba(255, 255, 255, 0.5);"
      "}");

  connect(searchInput, &QLineEdit::textChanged, this, &CommandPaletteDialog::onTextChanged);
  connect(searchInput, &QLineEdit::returnPressed, this, &CommandPaletteDialog::executeSelected);

  containerLayout->addWidget(searchInput);

  // 候補リスト
  suggestionsList = new QListWidget();
  suggestionsList->setStyleSheet(
      "QListWidget {"
      "  background-color: transparent;"
      "  border: none;"
      "  outline: none;"
      "  border-radius: 8px;"
      "}"
      "QListWidget::item {"
      "  color: rgba(255, 255, 255, 0.85);"
      "  padding: 12px 16px;"
      "  border: none;"
      "  border-radius: 8px;"
      "  margin: 2px 0px;"
      "  font-size: 15px;"
      "  background-color: transparent;"
      "}"
      "QListWidget::item:hover {"
      "  background-color: rgba(0, 122, 255, 0.15);"
      "  color: white;"
      "}"
      "QListWidget::item:selected {"
      "  background-color: rgba(0, 122, 255, 0.4);"
      "  color: white;"
      "}");

  suggestionsList->setMaximumHeight(320);
  suggestionsList->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  suggestionsList->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

  connect(suggestionsList, &QListWidget::itemClicked, this, &CommandPaletteDialog::onItemClicked);

  containerLayout->addWidget(suggestionsList);

  // ヘルプテキスト
  QLabel *helpLabel = new QLabel("↑↓ Navigate • Enter Select • Esc Cancel");
  helpLabel->setStyleSheet(
      "QLabel {"
      "  color: rgba(255, 255, 255, 0.4);"
      "  font-size: 12px;"
      "  padding: 8px 0px;"
      "}");
  helpLabel->setAlignment(Qt::AlignCenter);
  containerLayout->addWidget(helpLabel);

  mainLayout->addWidget(container);

  // ダイアログサイズ設定
  setFixedSize(700, 480);
}

void CommandPaletteDialog::setSearchHistory(const QStringList &history) {
  searchHistory = history;
}

void CommandPaletteDialog::showCentered() {
  qDebug() << "CommandPaletteDialog::showCentered() called"; // デバッグ出力

  // 画面中央に表示
  if (QScreen *screen = QApplication::primaryScreen()) {
    QRect screenGeometry = screen->availableGeometry();
    qDebug() << "Screen geometry:" << screenGeometry; // デバッグ出力

    // ダイアログサイズを取得
    resize(700, 480); // サイズを明示的に設定
    QRect dialogGeometry = geometry();
    qDebug() << "Dialog geometry:" << dialogGeometry; // デバッグ出力

    int x = screenGeometry.x() + (screenGeometry.width() - width()) / 2;
    int y = screenGeometry.y() + screenGeometry.height() / 4; // Spotlightのように少し上に配置

    qDebug() << "Moving dialog to:" << x << y; // デバッグ出力
    move(x, y);
  }

  qDebug() << "Showing dialog..."; // デバッグ出力
  show();
  activateWindow();
  raise();
  searchInput->setFocus();
  searchInput->selectAll();

  // 初期候補を表示
  populateSuggestions("");
  qDebug() << "Dialog should be visible now"; // デバッグ出力
}

void CommandPaletteDialog::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Escape:
    reject();
    break;
  case Qt::Key_Up:
    event->accept();
    selectPreviousItem();
    break;
  case Qt::Key_Down:
    event->accept();
    selectNextItem();
    break;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    event->accept();
    executeSelected();
    break;
  default:
    QDialog::keyPressEvent(event);
    break;
  }
}

void CommandPaletteDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);
  selectedIndex = -1;
  suggestionsList->clearSelection();
}

bool CommandPaletteDialog::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
    if (!geometry().contains(mouseEvent->globalPosition().toPoint())) {
      reject();
      return true;
    }
  }
  return QDialog::eventFilter(object, event);
}

void CommandPaletteDialog::onTextChanged(const QString &text) {
  searchTimer->stop();
  selectedIndex = -1;
  suggestionsList->clearSelection();

  searchTimer->start();
}

void CommandPaletteDialog::onItemClicked() {
  executeSelected();
}

void CommandPaletteDialog::updateSuggestions() {
  QString text = searchInput->text();
  if (text.startsWith(">")) {
    populateCommands(text.mid(1).trimmed());
  } else {
    populateSuggestions(text);
  }
}

void CommandPaletteDialog::populateSuggestions(const QString &query) {
  suggestionsList->clear();

  if (query.isEmpty()) {
    // 履歴を表示
    if (!searchHistory.isEmpty()) {
      QListWidgetItem *headerItem = new QListWidgetItem("🕒 Recent Searches");
      headerItem->setData(Qt::UserRole, "header");
      headerItem->setFlags(headerItem->flags() & ~Qt::ItemIsSelectable);
      suggestionsList->addItem(headerItem);

      for (int i = 0; i < qMin(5, searchHistory.size()); ++i) {
        QListWidgetItem *item = new QListWidgetItem(QString("   %1").arg(searchHistory[i]));
        item->setData(Qt::UserRole, "search");
        suggestionsList->addItem(item);
      }
    }

    // コマンド候補
    QListWidgetItem *cmdHeaderItem = new QListWidgetItem("⌘ Commands (type '>' for more)");
    cmdHeaderItem->setData(Qt::UserRole, "header");
    cmdHeaderItem->setFlags(cmdHeaderItem->flags() & ~Qt::ItemIsSelectable);
    suggestionsList->addItem(cmdHeaderItem);

    QStringList quickCommands = {"New Tab", "Close Tab", "Reload", "History", "Bookmarks"};
    for (const QString &cmd : quickCommands) {
      QListWidgetItem *item = new QListWidgetItem(QString("   %1").arg(cmd));
      item->setData(Qt::UserRole, "command");
      suggestionsList->addItem(item);
    }
  } else {
    // 検索候補
    QListWidgetItem *searchItem = new QListWidgetItem(QString("🔍 Search for \"%1\"").arg(query));
    searchItem->setData(Qt::UserRole, "search");
    suggestionsList->addItem(searchItem);

    // URL候補
    if (query.contains(".") && !query.contains(" ")) {
      QListWidgetItem *urlItem = new QListWidgetItem(QString("🌐 Go to %1").arg(query));
      urlItem->setData(Qt::UserRole, "url");
      suggestionsList->addItem(urlItem);
    }

    // 履歴から関連項目
    for (const QString &history : searchHistory) {
      if (history.contains(query, Qt::CaseInsensitive) && history != query) {
        QListWidgetItem *item = new QListWidgetItem(QString("🕒 %1").arg(history));
        item->setData(Qt::UserRole, "search");
        suggestionsList->addItem(item);

        if (suggestionsList->count() >= 8)
          break; // 最大8項目
      }
    }
  }
}

void CommandPaletteDialog::populateCommands(const QString &query) {
  suggestionsList->clear();

  QStringList allCommands = {
      "New Tab", "Close Tab", "New Window", "Close Window",
      "Reload", "Hard Reload", "Stop", "Go Back", "Go Forward",
      "Zoom In", "Zoom Out", "Reset Zoom", "Toggle Fullscreen",
      "Add Bookmark", "Show Bookmarks", "Show History", "Clear History",
      "Show Downloads", "Developer Tools", "View Source",
      "New Workspace", "Switch Workspace", "Rename Workspace",
      "Picture in Picture", "Find in Page", "Print Page", "Save Page"};

  // クエリでフィルタリング
  QStringList matchingCommands;
  if (query.isEmpty()) {
    matchingCommands = allCommands;
  } else {
    for (const QString &cmd : allCommands) {
      if (cmd.contains(query, Qt::CaseInsensitive)) {
        matchingCommands.append(cmd);
      }
    }
  }

  // コマンド候補を追加
  for (const QString &cmd : matchingCommands) {
    QListWidgetItem *item = new QListWidgetItem(QString("⌘ %1").arg(cmd));
    item->setData(Qt::UserRole, "command");
    suggestionsList->addItem(item);

    if (suggestionsList->count() >= 12)
      break; // 最大12項目
  }
}

void CommandPaletteDialog::selectNextItem() {
  if (suggestionsList->count() == 0)
    return;

  do {
    selectedIndex++;
    if (selectedIndex >= suggestionsList->count()) {
      selectedIndex = 0;
    }
  } while (isHeaderItem(suggestionsList->item(selectedIndex)) && selectedIndex < suggestionsList->count());

  suggestionsList->setCurrentRow(selectedIndex);
}

void CommandPaletteDialog::selectPreviousItem() {
  if (suggestionsList->count() == 0)
    return;

  do {
    selectedIndex--;
    if (selectedIndex < 0) {
      selectedIndex = suggestionsList->count() - 1;
    }
  } while (isHeaderItem(suggestionsList->item(selectedIndex)) && selectedIndex >= 0);

  suggestionsList->setCurrentRow(selectedIndex);
}

void CommandPaletteDialog::executeSelected() {
  QString query = searchInput->text().trimmed();

  // 選択された項目がある場合
  if (selectedIndex >= 0 && selectedIndex < suggestionsList->count()) {
    QListWidgetItem *selectedItem = suggestionsList->item(selectedIndex);
    if (selectedItem && !isHeaderItem(selectedItem)) {
      QString itemType = selectedItem->data(Qt::UserRole).toString();
      QString itemText = selectedItem->text();

      if (itemType == "command") {
        // コマンド実行
        QString command = itemText.startsWith("⌘ ") ? itemText.mid(2) : itemText.trimmed();
        executeCommand(command);
        return;
      } else if (itemType == "search" || itemType == "url") {
        // 検索またはURL
        QString searchQuery = itemText;
        if (searchQuery.startsWith("🔍 Search for \"") && searchQuery.endsWith("\"")) {
          searchQuery = searchQuery.mid(16, searchQuery.length() - 17);
        } else if (searchQuery.startsWith("🌐 Go to ")) {
          searchQuery = searchQuery.mid(10);
        } else if (searchQuery.startsWith("🕒 ")) {
          searchQuery = searchQuery.mid(3);
        } else if (searchQuery.startsWith("   ")) {
          searchQuery = searchQuery.trimmed();
        }
        executeSearch(searchQuery);
        return;
      }
    }
  }

  // 選択項目がない場合、直接入力を処理
  if (query.startsWith(">")) {
    executeCommand(query.mid(1).trimmed());
  } else if (!query.isEmpty()) {
    executeSearch(query);
  }
}

void CommandPaletteDialog::executeSearch(const QString &query) {
  emit searchRequested(query);
  accept();
}

void CommandPaletteDialog::executeCommand(const QString &command) {
  emit commandRequested(command);
  accept();
}

bool CommandPaletteDialog::isHeaderItem(QListWidgetItem *item) const {
  if (!item)
    return false;
  return item->data(Qt::UserRole).toString() == "header";
}
