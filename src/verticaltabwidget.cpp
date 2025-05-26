#include "verticaltabwidget.h"
#include <QApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidgetItem>
#include <QStyle>
#include <QToolButton>
#include <QVBoxLayout>

VerticalTabWidget::VerticalTabWidget(QWidget *parent)
    : QWidget(parent), tabsClosable(false), movable(false) {
  setupUI();
}

VerticalTabWidget::~VerticalTabWidget() {
}

void VerticalTabWidget::setupUI() {
  mainLayout = new QHBoxLayout(this);
  mainLayout->setContentsMargins(0, 0, 0, 0);
  mainLayout->setSpacing(0);

  // Left side - tab list
  QWidget *tabListContainer = new QWidget(this);
  tabListContainer->setFixedWidth(200);
  tabListContainer->setStyleSheet("QWidget { background-color: #f0f0f0; border-right: 1px solid #ccc; }");

  tabListLayout = new QVBoxLayout(tabListContainer);
  tabListLayout->setContentsMargins(5, 5, 5, 5);

  // New tab button
  newTabButton = new QPushButton("+ New Tab", tabListContainer);
  newTabButton->setStyleSheet("QPushButton { padding: 8px; margin: 2px; background-color: #e0e0e0; border: 1px solid #ccc; border-radius: 4px; } QPushButton:hover { background-color: #d0d0d0; }");
  connect(newTabButton, &QPushButton::clicked, this, &VerticalTabWidget::onNewTabClicked);
  tabListLayout->addWidget(newTabButton);

  // Tab list
  tabListWidget = new QListWidget(tabListContainer);
  tabListWidget->setStyleSheet(
      "QListWidget { background-color: transparent; border: none; outline: none; }"
      "QListWidget::item { padding: 8px; margin: 1px; border-radius: 4px; }"
      "QListWidget::item:selected { background-color: #007ACC; color: white; }"
      "QListWidget::item:hover { background-color: #e0e0e0; }");
  connect(tabListWidget, &QListWidget::currentItemChanged,
          this, &VerticalTabWidget::onTabListItemChanged);
  tabListLayout->addWidget(tabListWidget);

  mainLayout->addWidget(tabListContainer);

  // Right side - content area
  contentWidget = new QStackedWidget(this);
  mainLayout->addWidget(contentWidget, 1);
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
  // This signal should be connected to MainWindow's newTab slot
  // For now, we'll emit a signal that can be connected externally
  emit tabCloseRequested(-1); // Special case for new tab
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
    QHBoxLayout *layout = new QHBoxLayout(itemWidget);
    layout->setContentsMargins(4, 2, 4, 2);

    QLabel *label = new QLabel(text);
    layout->addWidget(label, 1);

    QToolButton *closeButton = new QToolButton();
    closeButton->setText("×");
    closeButton->setStyleSheet("QToolButton { border: none; color: #666; font-weight: bold; } QToolButton:hover { color: #000; background-color: #ddd; }");
    closeButton->setFixedSize(16, 16);
    closeButton->setProperty("tabIndex", index);
    connect(closeButton, &QToolButton::clicked, this, &VerticalTabWidget::onCloseTabClicked);
    layout->addWidget(closeButton);

    item->setSizeHint(itemWidget->sizeHint());
    tabListWidget->setItemWidget(item, itemWidget);
  } else {
    item->setText(text);
  }

  return item;
}
