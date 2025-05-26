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

  // Animation for smooth resizing
  resizeAnimation = new QPropertyAnimation(this, "size");
  resizeAnimation->setDuration(200);
  resizeAnimation->setEasingCurve(QEasingCurve::OutCubic);

  // Animation for fading suggestions
  fadeAnimation = new QPropertyAnimation();
  fadeAnimation->setDuration(150);
  fadeAnimation->setEasingCurve(QEasingCurve::OutCubic);
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
  QLabel *searchIcon = new QLabel("⌘");
  searchIcon->setStyleSheet(
      "QLabel {"
      "  font-size: 20px;"
      "  color: rgba(255, 255, 255, 0.8);"
      "}");

  QLabel *titleLabel = new QLabel("Command Palette");
  titleLabel->setStyleSheet(
      "QLabel {"
      "  color: rgba(255, 255, 255, 0.9);"
      "  font-size: 16px;"
      "  font-weight: 500;"
      "  background-color: transparent;"
      "}");

  titleLayout->addWidget(searchIcon);
  titleLayout->addWidget(titleLabel);
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
      "  padding: 2px 2px;" // Increased padding for better appearance
      "  font-size: 18px;"    // Larger font size
      "  color: white;"
      "  selection-background-color: rgba(0, 122, 255, 0.4);"
      "}"
      "QLineEdit:focus {"
      "  border-color: rgba(0, 122, 255, 0.8);"
      "  background-color: rgba(255, 255, 255, 0.12);"
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
  suggestionsWidget->hide(); // Initially hidden

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

  // Set dialog size - much larger for command palette
  setFixedSize(800, 300); // Initial larger size for better visibility
}

void QuickSearchDialog::keyPressEvent(QKeyEvent *event) {
  switch (event->key()) {
  case Qt::Key_Escape:
    reject();
    break;
  case Qt::Key_Down:
    selectNextSuggestion();
    break;
  case Qt::Key_Up:
    selectPreviousSuggestion();
    break;
  case Qt::Key_Return:
  case Qt::Key_Enter:
    executeSearch();
    break;
  default:
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

  // Focus on search field
  searchLineEdit->setFocus();
  searchLineEdit->selectAll();

  // Show some default suggestions
  populateSuggestions("");
}

void QuickSearchDialog::onTextChanged(const QString &text) {
  searchTimer->stop();
  selectedSuggestionIndex = -1;

  if (!text.isEmpty()) {
    searchTimer->start();
  } else {
    populateSuggestions("");
  }
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

    // Add some common search suggestions if we have space
    if (suggestions.size() < 6) {
      QStringList commonSuggestions = {
          query + " tutorial",
          query + " documentation",
          query + " download",
          query + " github",
          query + " reddit"};

      for (const QString &suggestion : commonSuggestions) {
        suggestions << QString("💡 %1").arg(suggestion);
        currentSuggestions.append(suggestion);
        if (suggestions.size() >= 6)
          break;
      }
    }

    for (const QString &suggestion : suggestions) {
      suggestionsWidget->addItem(suggestion);
    }
  }

  // Show/hide suggestions widget and adjust dialog size
  if (suggestionsWidget->count() > 0) {
    if (!suggestionsWidget->isVisible()) {
      suggestionsWidget->show();
      // Set initial opacity for fade-in effect
      QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect();
      effect->setOpacity(0.0);
      suggestionsWidget->setGraphicsEffect(effect);

      fadeAnimation->setTargetObject(effect);
      fadeAnimation->setPropertyName("opacity");
      fadeAnimation->setStartValue(0.0);
      fadeAnimation->setEndValue(1.0);
      fadeAnimation->start();
    }
    int itemHeight = 35;
    int maxHeight = qMin(250, suggestionsWidget->count() * itemHeight);
    animateResize(130 + maxHeight + 20);
  } else {
    suggestionsWidget->hide();
    animateResize(130);
  }
}

void QuickSearchDialog::selectNextSuggestion() {
  if (suggestionsWidget->count() == 0)
    return;

  selectedSuggestionIndex++;
  if (selectedSuggestionIndex >= suggestionsWidget->count()) {
    selectedSuggestionIndex = 0;
  }

  suggestionsWidget->setCurrentRow(selectedSuggestionIndex);
}

void QuickSearchDialog::selectPreviousSuggestion() {
  if (suggestionsWidget->count() == 0)
    return;

  selectedSuggestionIndex--;
  if (selectedSuggestionIndex < 0) {
    selectedSuggestionIndex = suggestionsWidget->count() - 1;
  }

  suggestionsWidget->setCurrentRow(selectedSuggestionIndex);
}

void QuickSearchDialog::executeSearch() {
  QString query = searchLineEdit->text().trimmed();

  // If a suggestion is selected, use that instead
  if (selectedSuggestionIndex >= 0) {
    if (query.startsWith(">") || !currentCommands.isEmpty()) {
      // Command mode
      QString command;
      if (selectedSuggestionIndex < currentCommands.size()) {
        command = currentCommands.at(selectedSuggestionIndex);
      } else {
        command = query;
      }
      executeCommand(command);
    } else {
      // Search mode
      if (selectedSuggestionIndex < currentSuggestions.size()) {
        query = currentSuggestions.at(selectedSuggestionIndex);
      }
      emit searchRequested(query);
    }
  } else {
    // No suggestion selected, use raw query
    if (query.startsWith(">")) {
      executeCommand(query);
    } else {
      emit searchRequested(query);
    }
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
  QSize currentSize = size();
  QSize newSize(800, newHeight); // Updated width for command palette

  if (currentSize == newSize) {
    return; // No change needed
  }

  resizeAnimation->setStartValue(currentSize);
  resizeAnimation->setEndValue(newSize);
  resizeAnimation->start();
}
