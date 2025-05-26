#include "verticaltabwidget.h"
#include <QApplication>
#include <QComboBox>
#include <QCursor>
#include <QEnterEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QResizeEvent>
#include <QStyle>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

VerticalTabWidget::VerticalTabWidget(QWidget *parent)
    : QWidget(parent), tabsClosable(false), movable(false),
      sidebarVisible(false), workspaceManager(nullptr), bookmarkManager(nullptr) {
  // Enable mouse tracking for hover detection
  setMouseTracking(true);

  setupUI();
  setupSidebar();

  // Install event filter on parent for global mouse tracking
  if (parent) {
    parent->installEventFilter(this);
  }
}

VerticalTabWidget::~VerticalTabWidget() {
}

void VerticalTabWidget::setupUI() {
  mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Content area (takes full screen by default)
  contentWidget = new QStackedWidget(this);
  mainLayout->addWidget(contentWidget);

  // Create overlay sidebar (initially hidden)
  sidebarWidget = new QWidget(this);
  sidebarWidget->setFixedWidth(280); // Slightly smaller for better UI
  sidebarWidget->setStyleSheet(
      "QWidget { "
      "  background-color: rgba(35, 35, 38, 250); " // Slightly darker and more opaque
      "  border-right: 1px solid rgba(70, 70, 76, 200); "
      "  border-radius: 0px 8px 8px 0px; " // Rounded right corners
      "}");
  sidebarWidget->hide();

  // Position sidebar as overlay
  sidebarWidget->setParent(this);
  sidebarWidget->raise();
}

void VerticalTabWidget::setupSidebar() {
  // Create sidebar layout
  tabListLayout = new QVBoxLayout(sidebarWidget);
  tabListLayout->setContentsMargins(8, 8, 8, 8);
  tabListLayout->setSpacing(8);

  // Add event filter to sidebar for better hover detection
  sidebarWidget->installEventFilter(this);

  // Integrated address bar
  integratedAddressBar = new QLineEdit(sidebarWidget);
  integratedAddressBar->setPlaceholderText("Enter URL or search...");
  integratedAddressBar->setStyleSheet(
      "QLineEdit { "
      "  padding: 8px 12px; "
      "  border: 1px solid rgba(160, 160, 160, 0.4); "
      "  border-radius: 4px; "
      "  background-color: rgba(255, 255, 255, 0.08); "
      "  color: rgba(240, 240, 240, 1); " // High contrast text
      "  font-size: 13px; "
      "} "
      "QLineEdit:focus { "
      "  border-color: rgba(0, 122, 204, 0.8); " // Blue focus border
      "  color: rgba(255, 255, 255, 1); "
      "  background-color: rgba(255, 255, 255, 0.12); "
      "} "
      "QLineEdit::placeholder { "
      "  color: rgba(160, 160, 160, 0.8); "
      "}");
  connect(integratedAddressBar, &QLineEdit::returnPressed,
          this, &VerticalTabWidget::addressBarReturnPressed);
  tabListLayout->addWidget(integratedAddressBar);

  // Workspace toolbar placeholder
  workspaceToolbar = new QWidget(sidebarWidget);
  workspaceToolbar->setMinimumHeight(40);
  tabListLayout->addWidget(workspaceToolbar);

  // New tab button
  newTabButton = new QPushButton("+ New Tab", sidebarWidget);
  newTabButton->setStyleSheet(
      "QPushButton { "
      "  padding: 8px 12px; "
      "  border: 1px solid rgba(160, 160, 160, 0.4); "
      "  border-radius: 4px; "
      "  background-color: rgba(255, 255, 255, 0.08); "
      "  color: rgba(220, 220, 220, 1); " // Clear text color
      "  font-weight: bold; "
      "  font-size: 12px; "
      "} "
      "QPushButton:hover { "
      "  color: rgba(255, 255, 255, 1); " // Brighter text on hover
      "  background-color: rgba(255, 255, 255, 0.12); "
      "  border-color: rgba(0, 122, 204, 0.6); "
      "} "
      "QPushButton:pressed { "
      "  color: rgba(255, 255, 255, 1); "
      "  background-color: rgba(255, 255, 255, 0.16); "
      "  border-color: rgba(0, 122, 204, 0.8); "
      "}");
  connect(newTabButton, &QPushButton::clicked, this, &VerticalTabWidget::onNewTabClicked);
  tabListLayout->addWidget(newTabButton);

  // Tab list
  tabListWidget = new QListWidget(sidebarWidget);
  tabListWidget->setStyleSheet(
      "QListWidget { "
      "  background-color: transparent; "
      "  border: none; "
      "  outline: none; "
      "  font-size: 12px; "
      "} "
      "QListWidget::item { "
      "  padding: 8px 10px; "
      "  margin: 2px 0px; "
      "  border-radius: 4px; "
      "  color: rgba(200, 200, 200, 1); " // Dimmed text for inactive tabs
      "  background-color: transparent; "
      "  border-left: 3px solid transparent; "
      "} "
      "QListWidget::item:selected { "
      "  color: rgba(255, 255, 255, 1); " // Bright white for selected tab
      "  font-weight: bold; "
      "  background-color: rgba(255, 255, 255, 0.05); "
      "  border-left: 3px solid rgba(0, 122, 204, 1); " // Blue accent line
      "} "
      "QListWidget::item:hover:!selected { "
      "  color: rgba(230, 230, 230, 1); " // Slightly brighter on hover
      "  background-color: rgba(255, 255, 255, 0.03); "
      "}");
  connect(tabListWidget, &QListWidget::currentItemChanged,
          this, &VerticalTabWidget::onTabListItemChanged);
  tabListLayout->addWidget(tabListWidget);

  // Bookmark panel placeholder
  bookmarkPanel = new QWidget(sidebarWidget);
  bookmarkPanel->setMinimumHeight(100);
  tabListLayout->addWidget(bookmarkPanel);

  // Animation setup
  sidebarAnimation = new QPropertyAnimation(sidebarWidget, "geometry", this);
  sidebarAnimation->setDuration(250);                       // Slightly longer for smoother animation
  sidebarAnimation->setEasingCurve(QEasingCurve::OutCubic); // Smoother easing

  // Timer for auto-hide
  hideTimer = new QTimer(this);
  hideTimer->setSingleShot(true);
  hideTimer->setInterval(50); // Hide after 50 milliseconds for better UX
  connect(hideTimer, &QTimer::timeout, this, &VerticalTabWidget::onSidebarTimerTimeout);
}

int VerticalTabWidget::addTab(QWidget *widget, const QString &text) {
  int index = tabWidgets.size();
  tabWidgets.append(widget);
  tabTexts.append(text);

  contentWidget->addWidget(widget);

  QListWidgetItem *item = createTabItem(text, index);
  tabListWidget->addItem(item);

  return index;
}

void VerticalTabWidget::removeTab(int index) {
  if (index < 0 || index >= tabWidgets.size())
    return;

  QWidget *widget = tabWidgets.at(index);
  tabWidgets.removeAt(index);
  tabTexts.removeAt(index);

  contentWidget->removeWidget(widget);

  delete tabListWidget->takeItem(index);

  // Update remaining items
  updateTabList();

  // Adjust current index if necessary
  if (tabListWidget->count() > 0) {
    int newIndex = qMin(index, tabListWidget->count() - 1);
    tabListWidget->setCurrentRow(newIndex);
  }
}

void VerticalTabWidget::setCurrentIndex(int index) {
  if (index < 0 || index >= tabWidgets.size())
    return;

  tabListWidget->setCurrentRow(index);
  contentWidget->setCurrentIndex(index);
}

int VerticalTabWidget::currentIndex() const {
  return tabListWidget->currentRow();
}

QWidget *VerticalTabWidget::currentWidget() const {
  int index = currentIndex();
  if (index >= 0 && index < tabWidgets.size()) {
    return tabWidgets.at(index);
  }
  return nullptr;
}

QWidget *VerticalTabWidget::widget(int index) const {
  if (index >= 0 && index < tabWidgets.size()) {
    return tabWidgets.at(index);
  }
  return nullptr;
}

int VerticalTabWidget::count() const {
  return tabWidgets.size();
}

void VerticalTabWidget::setTabText(int index, const QString &text) {
  if (index >= 0 && index < tabTexts.size()) {
    tabTexts[index] = text;
    QListWidgetItem *item = tabListWidget->item(index);
    if (item) {
      item->setText(text);
    }
  }
}

QString VerticalTabWidget::tabText(int index) const {
  if (index >= 0 && index < tabTexts.size()) {
    return tabTexts.at(index);
  }
  return QString();
}

void VerticalTabWidget::setTabsClosable(bool closable) {
  tabsClosable = closable;
  updateTabList();
}

void VerticalTabWidget::setMovable(bool movable) {
  this->movable = movable;
  // TODO: Implement drag and drop functionality if needed
}

void VerticalTabWidget::onTabListItemChanged(QListWidgetItem *current, QListWidgetItem *previous) {
  Q_UNUSED(previous)

  if (!current)
    return;

  int index = tabListWidget->row(current);
  if (index >= 0 && index < tabWidgets.size()) {
    contentWidget->setCurrentIndex(index);
    emit currentChanged(index);
  }
}

void VerticalTabWidget::onCloseTabClicked() {
  QToolButton *button = qobject_cast<QToolButton *>(sender());
  if (!button)
    return;

  int index = button->property("tabIndex").toInt();
  emit tabCloseRequested(index);
}

void VerticalTabWidget::onNewTabClicked() {
  emit newTabRequested();
}

void VerticalTabWidget::updateTabList() {
  for (int i = 0; i < tabListWidget->count(); ++i) {
    QListWidgetItem *item = tabListWidget->item(i);
    if (item && i < tabTexts.size()) {
      item->setText(tabTexts.at(i));
    }
  }
}

QListWidgetItem *VerticalTabWidget::createTabItem(const QString &text, int index) {
  QListWidgetItem *item = new QListWidgetItem();

  if (tabsClosable) {
    // Create a custom widget for the item with close button
    QWidget *itemWidget = new QWidget();
    itemWidget->setStyleSheet(
        "QWidget { "
        "  background-color: transparent; "
        "  border-radius: 6px; "
        "}");

    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(8, 6, 8, 6);

    QLabel *label = new QLabel(text);
    label->setStyleSheet(
        "QLabel { "
        "  color: #cccccc; "
        "  font-size: 13px; "
        "  background-color: transparent; "
        "}");
    layout->addWidget(label, 1);

    QToolButton *closeButton = new QToolButton();
    closeButton->setText("×");
    closeButton->setStyleSheet(
        "QToolButton { "
        "  border: none; "
        "  color: #999999; "
        "  font-weight: bold; "
        "  font-size: 16px; "
        "  background-color: transparent; "
        "  border-radius: 3px; "
        "} "
        "QToolButton:hover { "
        "  color: white; "
        "  background-color: #dc3545; "
        "}");
    closeButton->setFixedSize(20, 20);
    closeButton->setProperty("tabIndex", index);
    connect(closeButton, &QToolButton::clicked, this, &VerticalTabWidget::onCloseTabClicked);
    layout->addWidget(closeButton);

    item->setSizeHint(QSize(240, 40)); // Fixed size for consistency
    tabListWidget->setItemWidget(item, itemWidget);
  } else {
    item->setText(text);
    item->setSizeHint(QSize(240, 40));
  }

  return item;
}

// New methods for overlay functionality
void VerticalTabWidget::setWorkspaceManager(WorkspaceManager *manager) {
  workspaceManager = manager;
  if (manager && workspaceToolbar) {
    // Clear existing content
    QLayout *layout = workspaceToolbar->layout();
    if (layout) {
      QLayoutItem *item;
      while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
      }
      delete layout;
    }

    // Create workspace toolbar layout
    QHBoxLayout *toolbarLayout = new QHBoxLayout(workspaceToolbar);
    toolbarLayout->setContentsMargins(4, 4, 4, 4);
    toolbarLayout->setSpacing(4);

    // Add workspace selection combo box
    QComboBox *workspaceCombo = new QComboBox(workspaceToolbar);
    workspaceCombo->setStyleSheet(
        "QComboBox { "
        "  padding: 4px 8px; "
        "  border: 1px solid rgba(255, 255, 255, 0.3); "
        "  border-radius: 3px; "
        "  background-color: rgba(255, 255, 255, 0.1); "
        "  color: white; "
        "  font-size: 11px; "
        "  min-width: 120px; "
        "} "
        "QComboBox:hover { "
        "  background-color: rgba(255, 255, 255, 0.2); "
        "} "
        "QComboBox::drop-down { "
        "  border: none; "
        "} "
        "QComboBox::down-arrow { "
        "  color: white; "
        "}");

    // Populate with workspace names (this would be connected to WorkspaceManager)
    workspaceCombo->addItem("Default Workspace");
    workspaceCombo->addItem("Work");
    workspaceCombo->addItem("Personal");

    toolbarLayout->addWidget(new QLabel("Workspace:", workspaceToolbar));
    toolbarLayout->addWidget(workspaceCombo);
    toolbarLayout->addStretch();
  }
}

void VerticalTabWidget::setBookmarkManager(BookmarkManager *manager) {
  bookmarkManager = manager;
  if (manager && bookmarkPanel) {
    // Clear existing content
    QLayout *layout = bookmarkPanel->layout();
    if (layout) {
      QLayoutItem *item;
      while ((item = layout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
      }
      delete layout;
    }

    // Create bookmark panel layout
    QVBoxLayout *bookmarkLayout = new QVBoxLayout(bookmarkPanel);
    bookmarkLayout->setContentsMargins(4, 4, 4, 4);
    bookmarkLayout->setSpacing(4);

    // Add bookmarks header
    QLabel *bookmarksLabel = new QLabel("Bookmarks", bookmarkPanel);
    bookmarksLabel->setStyleSheet(
        "QLabel { "
        "  color: rgba(255, 255, 255, 0.9); "
        "  font-weight: bold; "
        "  font-size: 12px; "
        "  padding: 4px 0px; "
        "}");
    bookmarkLayout->addWidget(bookmarksLabel);

    // Add bookmark list (simplified)
    QListWidget *bookmarkList = new QListWidget(bookmarkPanel);
    bookmarkList->setStyleSheet(
        "QListWidget { "
        "  background-color: transparent; "
        "  border: none; "
        "  outline: none; "
        "  font-size: 11px; "
        "} "
        "QListWidget::item { "
        "  padding: 4px 6px; "
        "  margin: 1px 0px; "
        "  border-radius: 3px; "
        "  color: rgba(255, 255, 255, 0.7); "
        "  background-color: transparent; "
        "} "
        "QListWidget::item:hover { "
        "  color: white; "
        "  background-color: rgba(255, 255, 255, 0.1); "
        "} "
        "QListWidget::item:selected { "
        "  color: white; "
        "  background-color: rgba(255, 255, 255, 0.2); "
        "}");

    // Add some sample bookmarks
    bookmarkList->addItem("📁 Work");
    bookmarkList->addItem("  📄 GitHub");
    bookmarkList->addItem("  📄 Documentation");
    bookmarkList->addItem("📁 Personal");
    bookmarkList->addItem("  📄 Gmail");
    bookmarkList->addItem("  📄 News");

    // Connect bookmark selection
    connect(bookmarkList, &QListWidget::itemClicked, [this](QListWidgetItem *item) {
      QString text = item->text().trimmed();
      if (text.startsWith("📄")) {
        // This is a bookmark item, emit a signal or handle URL loading
        // For now, we'll emit a custom signal that MainWindow can connect to
        QString url;
        if (text.contains("GitHub"))
          url = "https://github.com";
        else if (text.contains("Documentation"))
          url = "https://doc.qt.io";
        else if (text.contains("Gmail"))
          url = "https://gmail.com";
        else if (text.contains("News"))
          url = "https://news.google.com";

        if (!url.isEmpty() && integratedAddressBar) {
          integratedAddressBar->setText(url);
          emit addressBarReturnPressed();
        }
      }
    });

    bookmarkList->setMaximumHeight(150);
    bookmarkLayout->addWidget(bookmarkList);

    // Add bookmark actions
    QHBoxLayout *bookmarkActions = new QHBoxLayout();
    bookmarkActions->setSpacing(4);

    QPushButton *addBookmarkBtn = new QPushButton("+ Add", bookmarkPanel);
    addBookmarkBtn->setStyleSheet(
        "QPushButton { "
        "  padding: 3px 8px; "
        "  border: 1px solid rgba(255, 255, 255, 0.3); "
        "  border-radius: 3px; "
        "  background-color: rgba(255, 255, 255, 0.1); "
        "  color: white; "
        "  font-size: 10px; "
        "} "
        "QPushButton:hover { "
        "  background-color: rgba(255, 255, 255, 0.2); "
        "}");

    bookmarkActions->addWidget(addBookmarkBtn);
    bookmarkActions->addStretch();
    bookmarkLayout->addLayout(bookmarkActions);
  }
}

void VerticalTabWidget::setAddressBar(QLineEdit *addressBar) {
  // Connect external address bar to integrated one
  if (addressBar && integratedAddressBar) {
    connect(addressBar, &QLineEdit::textChanged,
            integratedAddressBar, &QLineEdit::setText);
    connect(integratedAddressBar, &QLineEdit::textChanged,
            addressBar, &QLineEdit::setText);
  }
}

void VerticalTabWidget::showSidebar() {
  if (sidebarVisible)
    return;

  sidebarVisible = true;
  hideTimer->stop();

  // Position sidebar at left edge
  sidebarWidget->setGeometry(0, 0, 280, height());
  sidebarWidget->show();
  sidebarWidget->raise();

  // Animate sliding in
  sidebarAnimation->setStartValue(QRect(-280, 0, 280, height()));
  sidebarAnimation->setEndValue(QRect(0, 0, 280, height()));
  sidebarAnimation->start();
}

void VerticalTabWidget::hideSidebar() {
  if (!sidebarVisible)
    return;

  sidebarVisible = false;

  // Animate sliding out
  sidebarAnimation->setStartValue(QRect(0, 0, 280, height()));
  sidebarAnimation->setEndValue(QRect(-280, 0, 280, height()));

  connect(sidebarAnimation, &QPropertyAnimation::finished, [this]() {
    sidebarWidget->hide();
    disconnect(sidebarAnimation, &QPropertyAnimation::finished, nullptr, nullptr);
  });

  sidebarAnimation->start();
}

void VerticalTabWidget::onSidebarTimerTimeout() {
  hideSidebar();
}

void VerticalTabWidget::enterEvent(QEnterEvent *event) {
  Q_UNUSED(event)

  // Check if mouse is on the left edge (first 25 pixels when sidebar is hidden)
  if (!sidebarVisible) {
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    if (mousePos.x() <= 25) {
      showSidebar();
    }
  }
}

void VerticalTabWidget::leaveEvent(QEvent *event) {
  Q_UNUSED(event)

  // Start timer to hide sidebar after delay only if mouse is not over sidebar
  if (sidebarVisible) {
    QPoint mousePos = mapFromGlobal(QCursor::pos());
    QRect sidebarRect = sidebarWidget->geometry();

    if (!sidebarRect.contains(mousePos)) {
      hideTimer->start();
    }
  }
}

void VerticalTabWidget::mouseMoveEvent(QMouseEvent *event) {
  QWidget::mouseMoveEvent(event);

  QPoint mousePos = event->pos();

  // Show sidebar when mouse is near left edge (0-25 pixels)
  if (!sidebarVisible && mousePos.x() <= 25) {
    showSidebar();
  }

  // Hide sidebar when mouse moves right of sidebar area (> 300 pixels)
  if (sidebarVisible && mousePos.x() > 300) {
    hideTimer->start();
  }
}

void VerticalTabWidget::resizeEvent(QResizeEvent *event) {
  QWidget::resizeEvent(event);

  // Update sidebar geometry if visible
  if (sidebarVisible && sidebarWidget->isVisible()) {
    sidebarWidget->setGeometry(0, 0, 280, height());
  }
}

bool VerticalTabWidget::eventFilter(QObject *obj, QEvent *event) {
  if (obj == parent()) {
    // Filter mouse move events on the parent window
    if (event->type() == QEvent::MouseMove) {
      QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
      QPoint globalPos = mouseEvent->globalPosition().toPoint();
      QPoint localPos = mapFromGlobal(globalPos);

      // Show sidebar when mouse is near left edge (0-25 pixels)
      if (!sidebarVisible && localPos.x() <= 25 && localPos.x() >= 0) {
        showSidebar();
      }

      // Hide sidebar when mouse moves significantly right of sidebar
      if (sidebarVisible && localPos.x() > 320) {
        hideTimer->start();
      }
    }
  }

  if (obj == sidebarWidget) {
    if (event->type() == QEvent::Enter) {
      // Mouse entered sidebar, stop hide timer
      hideTimer->stop();
    } else if (event->type() == QEvent::Leave) {
      // Mouse left sidebar, start hide timer
      if (sidebarVisible) {
        hideTimer->start();
      }
    }
  }

  return QWidget::eventFilter(obj, event);
}
