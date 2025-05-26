#ifndef WORKSPACEMANAGER_H
#define WORKSPACEMANAGER_H

#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLineEdit>
#include <QMessageBox>
#include <QObject>
#include <QPushButton>
#include <QStandardPaths>
#include <QVBoxLayout>
#include <QWidget>

class VerticalTabWidget;

struct Workspace {
  QString name;
  QString id;
  QStringList tabUrls;
  QStringList tabTitles;
  int activeTabIndex;

  Workspace() : activeTabIndex(0) {}
  Workspace(const QString &n, const QString &i) : name(n), id(i), activeTabIndex(0) {}
};

class WorkspaceManager : public QObject {
  Q_OBJECT

public:
  explicit WorkspaceManager(QObject *parent = nullptr);
  ~WorkspaceManager();

  void setTabWidget(VerticalTabWidget *tabWidget);
  QWidget *createWorkspaceToolbar(QWidget *parent);

  void saveCurrentWorkspace();
  void loadWorkspace(const QString &workspaceId);
  void createNewWorkspace(const QString &name);
  void deleteWorkspace(const QString &workspaceId);
  void renameWorkspace(const QString &workspaceId, const QString &newName);

  QStringList getWorkspaceNames() const;
  QString getCurrentWorkspaceId() const;
  QString getCurrentWorkspaceName() const;

public slots:
  void onWorkspaceChanged();
  void onNewWorkspaceClicked();
  void onDeleteWorkspaceClicked();
  void onRenameWorkspaceClicked();

signals:
  void workspaceChanged(const QString &workspaceId);
  void requestNewTab(const QString &url);
  void requestCloseAllTabs();

private:
  void setupDefaultWorkspace();
  void saveWorkspacesToFile();
  void loadWorkspacesFromFile();
  QString generateWorkspaceId() const;
  void updateWorkspaceComboBox();

  VerticalTabWidget *tabWidget;
  QComboBox *workspaceComboBox;
  QPushButton *newWorkspaceButton;
  QPushButton *deleteWorkspaceButton;
  QPushButton *renameWorkspaceButton;

  QList<Workspace> workspaces;
  QString currentWorkspaceId;
  QString settingsPath;
};

#endif // WORKSPACEMANAGER_H
