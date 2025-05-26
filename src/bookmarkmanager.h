#ifndef BOOKMARKMANAGER_H
#define BOOKMARKMANAGER_H

#include <QAction>
#include <QDir>
#include <QDockWidget>
#include <QFile>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMenu>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QStandardPaths>
#include <QTreeWidget>
#include <QTreeWidgetItem>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>

class MainWindow;

struct BookmarkItem {
  QString title;
  QString url;
  QString id;
  bool isFolder;
  QList<BookmarkItem *> children;
  BookmarkItem *parent;

  BookmarkItem() : isFolder(false), parent(nullptr) {}
  BookmarkItem(const QString &t, const QString &u = QString())
      : title(t), url(u), isFolder(u.isEmpty()), parent(nullptr) {}
  ~BookmarkItem() {
    qDeleteAll(children);
  }
};

class BookmarkManager : public QObject {
  Q_OBJECT

public:
  explicit BookmarkManager(QObject *parent = nullptr);
  ~BookmarkManager();

  QDockWidget *createBookmarkDock(QWidget *parent);
  void addBookmark(const QString &title, const QUrl &url, BookmarkItem *parentFolder = nullptr);
  void addFolder(const QString &name, BookmarkItem *parent = nullptr);

  BookmarkItem *getRootItem() const { return rootItem; }
  void saveBookmarks();
  void loadBookmarks();

public slots:
  void onAddBookmarkClicked();
  void onAddFolderClicked();
  void onDeleteItemClicked();
  void onRenameItemClicked();
  void onBookmarkDoubleClicked(QTreeWidgetItem *item, int column);
  void onBookmarkContextMenu(const QPoint &pos);

signals:
  void bookmarkActivated(const QUrl &url);
  void openBookmarkInNewTab(const QUrl &url);

private:
  void setupUI();
  void setupContextMenu();
  void populateTreeWidget();
  void addItemToTree(BookmarkItem *item, QTreeWidgetItem *parentTreeItem = nullptr);
  BookmarkItem *getBookmarkItemFromTreeItem(QTreeWidgetItem *treeItem);
  QTreeWidgetItem *getTreeItemFromBookmarkItem(BookmarkItem *bookmarkItem);
  void saveBookmarkItem(BookmarkItem *item, QJsonObject &obj);
  BookmarkItem *loadBookmarkItem(const QJsonObject &obj, BookmarkItem *parent);
  QString generateBookmarkId() const;
  BookmarkItem *findBookmarkById(const QString &id, BookmarkItem *parent = nullptr);
  void deleteBookmarkItem(BookmarkItem *item);

  QDockWidget *dockWidget;
  QTreeWidget *treeWidget;
  QPushButton *addBookmarkButton;
  QPushButton *addFolderButton;
  QPushButton *deleteButton;
  QPushButton *renameButton;

  QMenu *contextMenu;
  QAction *openAction;
  QAction *openInNewTabAction;
  QAction *addBookmarkAction;
  QAction *addFolderAction;
  QAction *deleteAction;
  QAction *renameAction;

  BookmarkItem *rootItem;
  QString settingsPath;

  // Map to track tree items and bookmark items
  QMap<QTreeWidgetItem *, BookmarkItem *> treeToBookmarkMap;
  QMap<BookmarkItem *, QTreeWidgetItem *> bookmarkToTreeMap;
};

#endif // BOOKMARKMANAGER_H
