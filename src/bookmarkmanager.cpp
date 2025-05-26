#include "bookmarkmanager.h"
#include <QApplication>
#include <QClipboard>
#include <QDesktopServices>
#include <QHeaderView>
#include <QSplitter>
#include <QUuid>

BookmarkManager::BookmarkManager(QObject *parent)
    : QObject(parent), dockWidget(nullptr), treeWidget(nullptr), rootItem(nullptr) {

  // Setup settings path
  QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(appDataPath);
  if (!dir.exists()) {
    dir.mkpath(appDataPath);
  }
  settingsPath = appDataPath + "/bookmarks.json";

  // Create root item
  rootItem = new BookmarkItem("Root", "");
  rootItem->isFolder = true;

  // Don't load bookmarks here - wait until UI is set up
}

BookmarkManager::~BookmarkManager() {
  saveBookmarks();
  delete rootItem;
}

QDockWidget *BookmarkManager::createBookmarkDock(QWidget *parent) {
  dockWidget = new QDockWidget("Bookmarks", parent);
  dockWidget->setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetFloatable);
  dockWidget->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

  setupUI();
  setupContextMenu();

  // Load bookmarks after UI is set up
  loadBookmarks();
  populateTreeWidget();

  return dockWidget;
}

void BookmarkManager::setupUI() {
  QWidget *widget = new QWidget();
  QVBoxLayout *layout = new QVBoxLayout(widget);

  // Buttons
  QHBoxLayout *buttonLayout = new QHBoxLayout();
  addBookmarkButton = new QPushButton("Add Bookmark");
  addFolderButton = new QPushButton("Add Folder");
  deleteButton = new QPushButton("Delete");
  renameButton = new QPushButton("Rename");

  addBookmarkButton->setToolTip("Add new bookmark");
  addFolderButton->setToolTip("Add new folder");
  deleteButton->setToolTip("Delete selected item");
  renameButton->setToolTip("Rename selected item");

  buttonLayout->addWidget(addBookmarkButton);
  buttonLayout->addWidget(addFolderButton);
  buttonLayout->addStretch();
  buttonLayout->addWidget(renameButton);
  buttonLayout->addWidget(deleteButton);

  layout->addLayout(buttonLayout);

  // Tree widget
  treeWidget = new QTreeWidget();
  treeWidget->setHeaderLabels(QStringList() << "Title" << "URL");
  treeWidget->setContextMenuPolicy(Qt::CustomContextMenu);
  treeWidget->setRootIsDecorated(true);
  treeWidget->setAlternatingRowColors(true);
  treeWidget->setExpandsOnDoubleClick(false);
  treeWidget->header()->setStretchLastSection(true);
  treeWidget->header()->resizeSection(0, 200);

  layout->addWidget(treeWidget);

  dockWidget->setWidget(widget);

  // Connect signals
  connect(addBookmarkButton, &QPushButton::clicked, this, &BookmarkManager::onAddBookmarkClicked);
  connect(addFolderButton, &QPushButton::clicked, this, &BookmarkManager::onAddFolderClicked);
  connect(deleteButton, &QPushButton::clicked, this, &BookmarkManager::onDeleteItemClicked);
  connect(renameButton, &QPushButton::clicked, this, &BookmarkManager::onRenameItemClicked);
  connect(treeWidget, &QTreeWidget::itemDoubleClicked, this, &BookmarkManager::onBookmarkDoubleClicked);
  connect(treeWidget, &QTreeWidget::customContextMenuRequested, this, &BookmarkManager::onBookmarkContextMenu);
}

void BookmarkManager::setupContextMenu() {
  contextMenu = new QMenu(treeWidget);

  openAction = new QAction("Open", this);
  openInNewTabAction = new QAction("Open in New Tab", this);
  addBookmarkAction = new QAction("Add Bookmark Here", this);
  addFolderAction = new QAction("Add Folder Here", this);
  deleteAction = new QAction("Delete", this);
  renameAction = new QAction("Rename", this);

  contextMenu->addAction(openAction);
  contextMenu->addAction(openInNewTabAction);
  contextMenu->addSeparator();
  contextMenu->addAction(addBookmarkAction);
  contextMenu->addAction(addFolderAction);
  contextMenu->addSeparator();
  contextMenu->addAction(renameAction);
  contextMenu->addAction(deleteAction);

  connect(openAction, &QAction::triggered, [this]() {
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (item) {
      BookmarkItem *bookmark = getBookmarkItemFromTreeItem(item);
      if (bookmark && !bookmark->isFolder) {
        emit bookmarkActivated(QUrl(bookmark->url));
      }
    }
  });

  connect(openInNewTabAction, &QAction::triggered, [this]() {
    QTreeWidgetItem *item = treeWidget->currentItem();
    if (item) {
      BookmarkItem *bookmark = getBookmarkItemFromTreeItem(item);
      if (bookmark && !bookmark->isFolder) {
        emit openBookmarkInNewTab(QUrl(bookmark->url));
      }
    }
  });

  connect(addBookmarkAction, &QAction::triggered, this, &BookmarkManager::onAddBookmarkClicked);
  connect(addFolderAction, &QAction::triggered, this, &BookmarkManager::onAddFolderClicked);
  connect(deleteAction, &QAction::triggered, this, &BookmarkManager::onDeleteItemClicked);
  connect(renameAction, &QAction::triggered, this, &BookmarkManager::onRenameItemClicked);
}

void BookmarkManager::addBookmark(const QString &title, const QUrl &url, BookmarkItem *parentFolder) {
  if (!parentFolder) {
    parentFolder = rootItem;
  }

  BookmarkItem *bookmark = new BookmarkItem(title, url.toString());
  bookmark->id = generateBookmarkId();
  bookmark->parent = parentFolder;
  bookmark->isFolder = false;

  parentFolder->children.append(bookmark);

  // Add to tree widget
  QTreeWidgetItem *parentTreeItem = getTreeItemFromBookmarkItem(parentFolder);
  if (!parentTreeItem) {
    parentTreeItem = treeWidget->invisibleRootItem();
  }

  QTreeWidgetItem *treeItem = new QTreeWidgetItem(parentTreeItem);
  treeItem->setText(0, title);
  treeItem->setText(1, url.toString());
  treeItem->setIcon(0, QIcon(":/icons/bookmark.png")); // You can add icons later

  treeToBookmarkMap[treeItem] = bookmark;
  bookmarkToTreeMap[bookmark] = treeItem;

  if (parentTreeItem) {
    parentTreeItem->setExpanded(true);
  }

  saveBookmarks();
}

void BookmarkManager::addFolder(const QString &name, BookmarkItem *parent) {
  if (!parent) {
    parent = rootItem;
  }

  BookmarkItem *folder = new BookmarkItem(name, "");
  folder->id = generateBookmarkId();
  folder->parent = parent;
  folder->isFolder = true;

  parent->children.append(folder);

  // Add to tree widget
  QTreeWidgetItem *parentTreeItem = getTreeItemFromBookmarkItem(parent);
  if (!parentTreeItem) {
    parentTreeItem = treeWidget->invisibleRootItem();
  }

  QTreeWidgetItem *treeItem = new QTreeWidgetItem(parentTreeItem);
  treeItem->setText(0, name);
  treeItem->setText(1, "");
  treeItem->setIcon(0, QIcon(":/icons/folder.png")); // You can add icons later

  treeToBookmarkMap[treeItem] = folder;
  bookmarkToTreeMap[folder] = treeItem;

  if (parentTreeItem) {
    parentTreeItem->setExpanded(true);
  }

  saveBookmarks();
}

void BookmarkManager::onAddBookmarkClicked() {
  QString title = QInputDialog::getText(dockWidget, "Add Bookmark", "Bookmark title:");
  if (title.isEmpty())
    return;

  QString url = QInputDialog::getText(dockWidget, "Add Bookmark", "Bookmark URL:");
  if (url.isEmpty())
    return;

  BookmarkItem *parent = rootItem;
  QTreeWidgetItem *currentItem = treeWidget->currentItem();
  if (currentItem) {
    BookmarkItem *currentBookmark = getBookmarkItemFromTreeItem(currentItem);
    if (currentBookmark && currentBookmark->isFolder) {
      parent = currentBookmark;
    } else if (currentBookmark && currentBookmark->parent) {
      parent = currentBookmark->parent;
    }
  }

  addBookmark(title, QUrl(url), parent);
}

void BookmarkManager::onAddFolderClicked() {
  QString name = QInputDialog::getText(dockWidget, "Add Folder", "Folder name:");
  if (name.isEmpty())
    return;

  BookmarkItem *parent = rootItem;
  QTreeWidgetItem *currentItem = treeWidget->currentItem();
  if (currentItem) {
    BookmarkItem *currentBookmark = getBookmarkItemFromTreeItem(currentItem);
    if (currentBookmark && currentBookmark->isFolder) {
      parent = currentBookmark;
    } else if (currentBookmark && currentBookmark->parent) {
      parent = currentBookmark->parent;
    }
  }

  addFolder(name, parent);
}

void BookmarkManager::onDeleteItemClicked() {
  QTreeWidgetItem *currentItem = treeWidget->currentItem();
  if (!currentItem)
    return;

  BookmarkItem *bookmark = getBookmarkItemFromTreeItem(currentItem);
  if (!bookmark)
    return;

  QString message = bookmark->isFolder ? QString("Delete folder '%1' and all its contents?").arg(bookmark->title) : QString("Delete bookmark '%1'?").arg(bookmark->title);

  int ret = QMessageBox::question(dockWidget, "Delete Item", message,
                                  QMessageBox::Yes | QMessageBox::No);
  if (ret == QMessageBox::Yes) {
    deleteBookmarkItem(bookmark);
    saveBookmarks();
  }
}

void BookmarkManager::onRenameItemClicked() {
  QTreeWidgetItem *currentItem = treeWidget->currentItem();
  if (!currentItem)
    return;

  BookmarkItem *bookmark = getBookmarkItemFromTreeItem(currentItem);
  if (!bookmark)
    return;

  QString newTitle = QInputDialog::getText(dockWidget, "Rename Item",
                                           "New name:", QLineEdit::Normal, bookmark->title);
  if (newTitle.isEmpty() || newTitle == bookmark->title)
    return;

  bookmark->title = newTitle;
  currentItem->setText(0, newTitle);
  saveBookmarks();
}

void BookmarkManager::onBookmarkDoubleClicked(QTreeWidgetItem *item, int column) {
  Q_UNUSED(column)

  BookmarkItem *bookmark = getBookmarkItemFromTreeItem(item);
  if (bookmark && !bookmark->isFolder) {
    emit bookmarkActivated(QUrl(bookmark->url));
  }
}

void BookmarkManager::onBookmarkContextMenu(const QPoint &pos) {
  QTreeWidgetItem *item = treeWidget->itemAt(pos);
  if (!item)
    return;

  BookmarkItem *bookmark = getBookmarkItemFromTreeItem(item);
  if (!bookmark)
    return;

  // Update context menu actions based on item type
  openAction->setEnabled(!bookmark->isFolder);
  openInNewTabAction->setEnabled(!bookmark->isFolder);

  contextMenu->exec(treeWidget->mapToGlobal(pos));
}

void BookmarkManager::populateTreeWidget() {
  treeWidget->clear();
  treeToBookmarkMap.clear();
  bookmarkToTreeMap.clear();

  // Add root children
  for (BookmarkItem *child : rootItem->children) {
    addItemToTree(child);
  }
}

void BookmarkManager::addItemToTree(BookmarkItem *item, QTreeWidgetItem *parentTreeItem) {
  if (!parentTreeItem) {
    parentTreeItem = treeWidget->invisibleRootItem();
  }

  QTreeWidgetItem *treeItem = new QTreeWidgetItem(parentTreeItem);
  treeItem->setText(0, item->title);
  treeItem->setText(1, item->url);

  if (item->isFolder) {
    treeItem->setIcon(0, QIcon(":/icons/folder.png"));
    treeItem->setExpanded(true);

    // Add children
    for (BookmarkItem *child : item->children) {
      addItemToTree(child, treeItem);
    }
  } else {
    treeItem->setIcon(0, QIcon(":/icons/bookmark.png"));
  }

  treeToBookmarkMap[treeItem] = item;
  bookmarkToTreeMap[item] = treeItem;
}

BookmarkItem *BookmarkManager::getBookmarkItemFromTreeItem(QTreeWidgetItem *treeItem) {
  return treeToBookmarkMap.value(treeItem, nullptr);
}

QTreeWidgetItem *BookmarkManager::getTreeItemFromBookmarkItem(BookmarkItem *bookmarkItem) {
  return bookmarkToTreeMap.value(bookmarkItem, nullptr);
}

void BookmarkManager::saveBookmarks() {
  QJsonObject rootObj;
  QJsonArray childrenArray;

  for (BookmarkItem *child : rootItem->children) {
    QJsonObject childObj;
    saveBookmarkItem(child, childObj);
    childrenArray.append(childObj);
  }

  rootObj["children"] = childrenArray;

  QJsonDocument doc(rootObj);
  QFile file(settingsPath);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(doc.toJson());
  }
}

void BookmarkManager::loadBookmarks() {
  QFile file(settingsPath);
  if (!file.open(QIODevice::ReadOnly)) {
    // Create default folders
    addFolder("Bookmarks Bar");
    addFolder("Other Bookmarks");
    return;
  }

  QByteArray data = file.readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  QJsonObject rootObj = doc.object();

  QJsonArray childrenArray = rootObj["children"].toArray();
  for (const QJsonValue &value : childrenArray) {
    BookmarkItem *child = loadBookmarkItem(value.toObject(), rootItem);
    if (child) {
      rootItem->children.append(child);
    }
  }
}

void BookmarkManager::saveBookmarkItem(BookmarkItem *item, QJsonObject &obj) {
  obj["id"] = item->id;
  obj["title"] = item->title;
  obj["url"] = item->url;
  obj["isFolder"] = item->isFolder;

  if (item->isFolder && !item->children.isEmpty()) {
    QJsonArray childrenArray;
    for (BookmarkItem *child : item->children) {
      QJsonObject childObj;
      saveBookmarkItem(child, childObj);
      childrenArray.append(childObj);
    }
    obj["children"] = childrenArray;
  }
}

BookmarkItem *BookmarkManager::loadBookmarkItem(const QJsonObject &obj, BookmarkItem *parent) {
  QString id = obj["id"].toString();
  QString title = obj["title"].toString();
  QString url = obj["url"].toString();
  bool isFolder = obj["isFolder"].toBool();

  BookmarkItem *item = new BookmarkItem(title, url);
  item->id = id.isEmpty() ? generateBookmarkId() : id;
  item->parent = parent;
  item->isFolder = isFolder;

  if (isFolder && obj.contains("children")) {
    QJsonArray childrenArray = obj["children"].toArray();
    for (const QJsonValue &value : childrenArray) {
      BookmarkItem *child = loadBookmarkItem(value.toObject(), item);
      if (child) {
        item->children.append(child);
      }
    }
  }

  return item;
}

QString BookmarkManager::generateBookmarkId() const {
  return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

BookmarkItem *BookmarkManager::findBookmarkById(const QString &id, BookmarkItem *parent) {
  if (!parent) {
    parent = rootItem;
  }

  if (parent->id == id) {
    return parent;
  }

  for (BookmarkItem *child : parent->children) {
    BookmarkItem *result = findBookmarkById(id, child);
    if (result) {
      return result;
    }
  }

  return nullptr;
}

void BookmarkManager::deleteBookmarkItem(BookmarkItem *item) {
  if (!item || item == rootItem)
    return;

  // Remove from tree widget
  QTreeWidgetItem *treeItem = getTreeItemFromBookmarkItem(item);
  if (treeItem) {
    delete treeItem;
    treeToBookmarkMap.remove(treeItem);
  }
  bookmarkToTreeMap.remove(item);

  // Remove from parent
  if (item->parent) {
    item->parent->children.removeOne(item);
  }

  // Delete the item (this will also delete all children)
  delete item;
}
