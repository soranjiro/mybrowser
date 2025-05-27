#include "quicksearchdialog.h"
#include <QApplication>
#include <QDesktopServices>
#include <QGraphicsDropShadowEffect>
#include <QGraphicsOpacityEffect>
#include <QLabel>
#include <QPropertyAnimation>
#include <QScreen>
#include <QShowEvent>
#include <QUrl>

QuickSearchDialog::QuickSearchDialog(QWidget *parent)
    : QDialog(parent), selectedSuggestionIndex(-1) {
  setupUI();

  // Timer for delayed search suggestions
  searchTimer = new QTimer(this);
  searchTimer->setSingleShot(true);
  searchTimer->setInterval(300); // 300ms delay
  connect(searchTimer, &QTimer::timeout, this, &QuickSearchDialog::updateSuggestions);

  // Animation removed for immediate display

  // Install event filter on parent to detect clicks outside the dialog
  if (parent) {
    parent->installEventFilter(this);
  }
  // Also install on the application to catch global clicks
  QApplication::instance()->installEventFilter(this);
}

QuickSearchDialog::~QuickSearchDialog() {
  // Remove event filters
  if (parent()) {
    parent()->removeEventFilter(this);
  }
  QApplication::instance()->removeEventFilter(this);
}

QString QuickSearchDialog::getSearchQuery() const {
  return searchLineEdit->text();
}

void QuickSearchDialog::setSearchHistory(const QStringList &history) {
  searchHistory = history;
}

void QuickSearchDialog::setupUI() {
  // Set dialog properties for Spotlight-like appearance
  setWindowFlags(Qt::Dialog | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint);
  setAttribute(Qt::WA_TranslucentBackground);
  setModal(true);

  // Main layout
  mainLayout = new QVBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Container widget with rounded corners and shadow
  QWidget *containerWidget = new QWidget();
  containerWidget->setStyleSheet(
      "QWidget {"
      "  background-color: rgba(30, 30, 30, 245);"
      "  border: 1px solid rgba(255, 255, 255, 0.1);"
      "  border-radius: 12px;"
      "}");

  // Add drop shadow effect
  QGraphicsDropShadowEffect *shadowEffect = new QGraphicsDropShadowEffect();
  shadowEffect->setBlurRadius(30);
  shadowEffect->setColor(QColor(0, 0, 0, 100));
  shadowEffect->setOffset(0, 4);
  containerWidget->setGraphicsEffect(shadowEffect);

  QVBoxLayout *containerLayout = new QVBoxLayout(containerWidget);
  containerLayout->setContentsMargins(20, 20, 20, 20);
  containerLayout->setSpacing(12);

  // Search icon and title
  QHBoxLayout *titleLayout = new QHBoxLayout();
  titleLayout->addStretch();
  containerLayout->addLayout(titleLayout);

  // Search input field
  searchLineEdit = new QLineEdit();
  searchLineEdit->setPlaceholderText("Search Google, enter URL, or type '>' for commands...");
  searchLineEdit->setMinimumHeight(50); // Set minimum height for better visibility
  searchLineEdit->setStyleSheet(
      "QLineEdit {"
      "  background-color: rgba(255, 255, 255, 0.08);"
      "  border: 2px solid rgba(255, 255, 255, 0.1);"
      "  border-radius: 8px;"
      "  padding: 2px 2px;"
      "  font-size: 18px;"
      "  color: white;"
      "  selection-background-color: rgba(0, 122, 255, 0.4);"
      "}"
      "QLineEdit::placeholder {"
      "  color: rgba(255, 255, 255, 0.5);"
      "}");

  connect(searchLineEdit, &QLineEdit::textChanged, this, &QuickSearchDialog::onTextChanged);
  connect(searchLineEdit, &QLineEdit::returnPressed, this, &QuickSearchDialog::executeSearch);

  containerLayout->addWidget(searchLineEdit);

  // Suggestions list
  suggestionsWidget = new QListWidget();
  suggestionsWidget->setMaximumHeight(200);
  suggestionsWidget->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
  suggestionsWidget->setStyleSheet(
      "QListWidget {"
      "  background-color: transparent;"
      "  border: none;"
      "  outline: none;"
      "}"
      "QListWidget::item {"
      "  color: rgba(255, 255, 255, 0.8);"
      "  padding: 8px 12px;"
      "  border: none;"
      "  border-radius: 6px;"
      "  margin: 2px 0px;"
      "  font-size: 14px;"
      "}"
      "QListWidget::item:hover {"
      "  background-color: rgba(0, 122, 255, 0.2);"
      "  color: white;"
      "}"
      "QListWidget::item:selected {"
      "  background-color: rgba(0, 122, 255, 0.4);"
      "  color: white;"
      "}");

  connect(suggestionsWidget, &QListWidget::itemClicked, this, &QuickSearchDialog::onSuggestionClicked);
  // Always show suggestions widget initially
  suggestionsWidget->show();

  containerLayout->addWidget(suggestionsWidget);

  // Add instructions
  QLabel *instructionLabel = new QLabel("Press Enter to search/execute • ↑↓ to navigate • '>' for commands • Esc to cancel");
  instructionLabel->setStyleSheet(
      "QLabel {"
      "  color: rgba(255, 255, 255, 0.4);"
      "  font-size: 11px;"
      "  background-color: transparent;"
      "}");
  instructionLabel->setAlignment(Qt::AlignCenter);
  containerLayout->addWidget(instructionLabel);

  mainLayout->addWidget(containerWidget);

  // Set dialog size - much larger for command palette with always-visible suggestions
  setFixedSize(800, 450); // Increased height to accommodate always-visible suggestions
}

void QuickSearchDialog::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Escape:
    reject();
    break;
  case Qt::Key_Down:
    event->accept();
    selectNextSuggestion();
    break;
  case Qt::Key_Up:
    event->accept();
    selectPreviousSuggestion();
    break;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    event->accept();
    executeSearch();
    break;
  case Qt::Key_Tab:
    // Tab cycles through suggestions like Down arrow
    event->accept();
    selectNextSuggestion();
    break;
  case Qt::Key_Backtab: // Shift+Tab
    // Shift+Tab cycles backwards like Up arrow
    event->accept();
    selectPreviousSuggestion();
    break;
  default:
    // For text input, make sure the search input has focus
    if (event->text().length() > 0 && event->text().at(0).isPrint()) {
      searchLineEdit->setFocus();
      selectedSuggestionIndex = -1;
      suggestionsWidget->clearSelection();
    }
    QDialog::keyPressEvent(event);
  }
}

void QuickSearchDialog::showEvent(QShowEvent *event) {
  QDialog::showEvent(event);

  // Center the dialog on screen
  if (QScreen *screen = QApplication::primaryScreen()) {
    QRect screenGeometry = screen->geometry();
    QRect dialogGeometry = geometry();

    int x = (screenGeometry.width() - dialogGeometry.width()) / 2;
    int y = screenGeometry.height() / 4; // Position at 1/4 from top like Spotlight
    move(x, y);
  }

  // Reset selection state
  selectedSuggestionIndex = -1;
  suggestionsWidget->clearSelection();

  // Focus on search field
  searchLineEdit->setFocus();
  searchLineEdit->selectAll();

  // Always show suggestions (even when empty)
  populateSuggestions("");
  // Ensure suggestions are always visible
  suggestionsWidget->show();
}

void QuickSearchDialog::onTextChanged(const QString &text) {
  searchTimer->stop();
  selectedSuggestionIndex = -1;
  suggestionsWidget->clearSelection(); // Clear visual selection

  if (!text.isEmpty()) {
    searchTimer->start();
  } else {
    populateSuggestions("");
  }

  // Return focus to search input
  searchLineEdit->setFocus();
}

void QuickSearchDialog::onSuggestionClicked() {
  if (QListWidgetItem *item = suggestionsWidget->currentItem()) {
    QString suggestion = item->text();

    // Handle different types of suggestions
    if (suggestion.startsWith("⌘")) {
      // Command suggestion
      QString command = suggestion.mid(suggestion.indexOf(" ") + 1);
      searchLineEdit->setText(">" + command);
      executeCommand(command);
    } else if (suggestion.startsWith("🔍") || suggestion.startsWith("🌐") ||
               suggestion.startsWith("🕒") || suggestion.startsWith("💡")) {
      // Search suggestion - extract the actual query
      if (suggestion.contains(" ")) {
        QString query = suggestion.mid(suggestion.indexOf(" ") + 1);
        if (query.startsWith("\"") && query.endsWith("\"")) {
          query = query.mid(1, query.length() - 2);
        }
        searchLineEdit->setText(query);
        emit searchRequested(query);
        accept();
      }
    } else {
      // Plain suggestion
      searchLineEdit->setText(suggestion.trimmed());
      executeSearch();
    }
  }
}

void QuickSearchDialog::updateSuggestions() {
  populateSuggestions(searchLineEdit->text());
}

void QuickSearchDialog::populateSuggestions(const QString &query) {
  suggestionsWidget->clear();
  currentSuggestions.clear();
  currentCommands.clear();

  if (query.isEmpty()) {
    // Show recent searches and common commands
    if (!searchHistory.isEmpty()) {
      suggestionsWidget->addItem("🕒 Recent Searches");
      for (int i = 0; i < qMin(3, searchHistory.size()); ++i) {
        QString item = QString("   %1").arg(searchHistory.at(i));
        suggestionsWidget->addItem(item);
        currentSuggestions.append(searchHistory.at(i));
      }
    }

    // Show common commands
    suggestionsWidget->addItem("⌘ Common Commands");
    QStringList commonCommands = {
        "> New Tab",
        "> New Workspace",
        "> Add Bookmark",
        "> Close Tab",
        "> Reload Page"};

    for (const QString &cmd : commonCommands) {
      suggestionsWidget->addItem(QString("   %1").arg(cmd));
      currentCommands.append(cmd);
    }

    // If no history, show default suggestions
    if (searchHistory.isEmpty()) {
      suggestionsWidget->addItem("💡 Popular Suggestions");
      QStringList defaultSuggestions = {
          "   GitHub",
          "   YouTube",
          "   Google Drive",
          "   Stack Overflow"};

      for (const QString &suggestion : defaultSuggestions) {
        suggestionsWidget->addItem(suggestion);
        currentSuggestions.append(suggestion.trimmed());
      }
    }

  } else if (query.startsWith(">")) {
    // Command mode
    populateCommands(query.mid(1).trimmed());
  } else {
    // Search mode
    // Generate search suggestions
    QStringList suggestions;

    // Add direct search suggestion
    suggestions << QString("🔍 Search for \"%1\"").arg(query);
    currentSuggestions.append(query);

    // Add URL suggestion if it looks like a URL
    if (query.contains(".") && !query.contains(" ")) {
      QString url = query;
      if (!url.startsWith("http")) {
        url = "https://" + url;
      }
      suggestions << QString("🌐 Go to %1").arg(url);
      currentSuggestions.append(url);
    }

    // Add matching items from search history
    for (const QString &historyItem : searchHistory) {
      if (historyItem.contains(query, Qt::CaseInsensitive) && historyItem != query) {
        suggestions << QString("🕒 %1").arg(historyItem);
        currentSuggestions.append(historyItem);
        if (suggestions.size() >= 4)
          break;
      }
    }

    // Add some Google-style search suggestions if we have space
    if (suggestions.size() < 8) { // Increased from 6 to 8 for more suggestions
      QStringList googleStyleSuggestions = {
          query + " meaning",
          query + " tutorial",
          query + " how to",
          query + " what is",
          query + " documentation",
          query + " examples",
          query + " download",
          query + " github",
          query + " stack overflow",
          query + " reddit"};

      for (const QString &suggestion : googleStyleSuggestions) {
        if (suggestions.size() >= 8)
          break;
        suggestions << QString("💡 %1").arg(suggestion);
        currentSuggestions.append(suggestion);
      }
    }

    for (const QString &suggestion : suggestions) {
      suggestionsWidget->addItem(suggestion);
    }
  }

  // Always ensure suggestions widget is visible
  if (!suggestionsWidget->isVisible()) {
    suggestionsWidget->show();
  }

  // Adjust dialog size based on content (no animation)
  int itemHeight = 35;
  int minItems = 6; // Minimum items to always show
  int actualItems = qMax(minItems, suggestionsWidget->count());
  int maxHeight = qMin(300, actualItems * itemHeight);

  // Resize immediately without animation
  int targetHeight = 180 + maxHeight; // Base height + suggestions height
  resize(800, targetHeight);
}

void QuickSearchDialog::selectNextSuggestion() {
  if (suggestionsWidget->count() == 0)
    return;

  // Skip header items (those that don't start with spaces or emoji)
  do {
    selectedSuggestionIndex++;
    if (selectedSuggestionIndex >= suggestionsWidget->count()) {
      selectedSuggestionIndex = 0;
    }
  } while (selectedSuggestionIndex < suggestionsWidget->count() &&
           isHeaderItem(suggestionsWidget->item(selectedSuggestionIndex)));

  suggestionsWidget->setCurrentRow(selectedSuggestionIndex);
  suggestionsWidget->setFocus(); // Ensure widget has focus for visual selection
}

void QuickSearchDialog::selectPreviousSuggestion() {
  if (suggestionsWidget->count() == 0)
    return;

  // Skip header items (those that don't start with spaces or emoji)
  do {
    selectedSuggestionIndex--;
    if (selectedSuggestionIndex < 0) {
      selectedSuggestionIndex = suggestionsWidget->count() - 1;
    }
  } while (selectedSuggestionIndex >= 0 &&
           isHeaderItem(suggestionsWidget->item(selectedSuggestionIndex)));

  suggestionsWidget->setCurrentRow(selectedSuggestionIndex);
  suggestionsWidget->setFocus(); // Ensure widget has focus for visual selection
}

void QuickSearchDialog::executeSearch() {
  QString query = searchLineEdit->text().trimmed();

  // If a suggestion is selected, use that instead
  if (selectedSuggestionIndex >= 0 && selectedSuggestionIndex < suggestionsWidget->count()) {
    QListWidgetItem *selectedItem = suggestionsWidget->item(selectedSuggestionIndex);
    if (selectedItem && !isHeaderItem(selectedItem)) {
      QString suggestion = selectedItem->text();

      if (suggestion.startsWith("⌘")) {
        // Command suggestion
        QString command = suggestion.mid(suggestion.indexOf(" ") + 1);
        executeCommand(command);
        accept();
        return;
      } else if (suggestion.startsWith("🔍") || suggestion.startsWith("🌐") ||
                 suggestion.startsWith("🕒") || suggestion.startsWith("💡")) {
        // Search suggestion - extract the actual query
        if (suggestion.contains(" ")) {
          QString extractedQuery = suggestion.mid(suggestion.indexOf(" ") + 1);
          if (extractedQuery.startsWith("\"") && extractedQuery.endsWith("\"")) {
            extractedQuery = extractedQuery.mid(1, extractedQuery.length() - 2);
          }
          emit searchRequested(extractedQuery);
          accept();
          return;
        }
      } else if (suggestion.startsWith("   ")) {
        // Indented suggestion (from history or commands)
        QString cleanSuggestion = suggestion.trimmed();
        if (cleanSuggestion.startsWith(">")) {
          executeCommand(cleanSuggestion);
        } else {
          emit searchRequested(cleanSuggestion);
        }
        accept();
        return;
      }
    }
  }

  // No valid suggestion selected, use raw query
  if (query.startsWith(">")) {
    executeCommand(query);
  } else {
    emit searchRequested(query);
  }
  accept();
}

void QuickSearchDialog::populateCommands(const QString &query) {
  QStringList allCommands = {
      "New Tab",
      "New Window",
      "Close Tab",
      "Close Window",
      "Reload Page",
      "Hard Reload",
      "Stop Loading",
      "Go Back",
      "Go Forward",
      "Zoom In",
      "Zoom Out",
      "Reset Zoom",
      "Add Bookmark",
      "Show Bookmarks",
      "New Workspace",
      "Switch Workspace",
      "Rename Workspace",
      "Delete Workspace",
      "Show History",
      "Clear History",
      "Show Downloads",
      "Developer Tools",
      "Picture-in-Picture",
      "View Source",
      "Print Page",
      "Save Page",
      "Find in Page",
      "Toggle Fullscreen",
      "Show Sidebar",
      "Hide Sidebar"};

  // Filter commands based on query
  QStringList matchingCommands;
  if (query.isEmpty()) {
    matchingCommands = allCommands.mid(0, 10); // Show first 10 commands
  } else {
    for (const QString &cmd : allCommands) {
      if (cmd.contains(query, Qt::CaseInsensitive)) {
        matchingCommands.append(cmd);
        if (matchingCommands.size() >= 10)
          break;
      }
    }
  }

  // Add commands to suggestions
  for (const QString &cmd : matchingCommands) {
    QString displayText = QString("⌘ %1").arg(cmd);
    suggestionsWidget->addItem(displayText);
    currentCommands.append(cmd);
  }
}

void QuickSearchDialog::executeCommand(const QString &command) {
  QString cmd = command;
  if (cmd.startsWith(">")) {
    cmd = cmd.mid(1).trimmed();
  }

  emit commandRequested(cmd);
}

void QuickSearchDialog::animateResize(int newHeight) {
  // Animation removed - resize immediately
  resize(800, newHeight);
}

// Helper function to check if an item is a header item
bool QuickSearchDialog::isHeaderItem(QListWidgetItem *item) {
  if (!item)
    return false;
  QString text = item->text();
  return text.startsWith("🕒 Recent") ||
         text.startsWith("⌘ Common") ||
         text.startsWith("💡 Popular");
}

// Event filter to handle clicks outside the dialog
bool QuickSearchDialog::eventFilter(QObject *object, QEvent *event) {
  if (event->type() == QEvent::MouseButtonPress) {
    QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);

    // Check if the click is outside the dialog
    if (!geometry().contains(mouseEvent->globalPosition().toPoint())) {
      // Click was outside the dialog, close it
      reject();
      return true;
    }
  }

  return QDialog::eventFilter(object, event);
}
