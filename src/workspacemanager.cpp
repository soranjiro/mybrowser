#include "workspacemanager.h"
#include "verticaltabwidget.h"
#include "webview.h"
#include <QDebug>
#include <QUuid>

WorkspaceManager::WorkspaceManager(QObject *parent)
    : QObject(parent), tabWidget(nullptr) {

  // Setup settings path
  QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  QDir dir(appDataPath);
  if (!dir.exists()) {
    dir.mkpath(appDataPath);
  }
  settingsPath = appDataPath + "/workspaces.json";

  loadWorkspacesFromFile();
  if (workspaces.isEmpty()) {
    setupDefaultWorkspace();
  }
}

WorkspaceManager::~WorkspaceManager() {
  saveCurrentWorkspace();
  saveWorkspacesToFile();
}

void WorkspaceManager::setTabWidget(VerticalTabWidget *widget) {
  tabWidget = widget;
}

QWidget *WorkspaceManager::createWorkspaceToolbar(QWidget *parent) {
  QWidget *toolbar = new QWidget(parent);
  toolbar->setStyleSheet(
      "QWidget { "
      "  background: qlineargradient(x1: 0, y1: 0, x2: 0, y2: 1, "
      "    stop: 0 #f8f9fa, stop: 1 #e9ecef); "
      "  border-bottom: 2px solid #007ACC; "
      "  padding: 8px; "
      "}");

  QHBoxLayout *layout = new QHBoxLayout(toolbar);
  layout->setContentsMargins(15, 8, 15, 8);
  layout->setSpacing(12);

  // Workspace label
  QLabel *label = new QLabel("Workspace:", toolbar);
  label->setStyleSheet(
      "QLabel { "
      "  color: #2d2d30; "
      "  font-weight: bold; "
      "  font-size: 14px; "
      "}");
  layout->addWidget(label);

  // Workspace selector - improved styling
  workspaceComboBox = new QComboBox(toolbar);
  workspaceComboBox->setMinimumWidth(180);
  workspaceComboBox->setStyleSheet(
      "QComboBox { "
      "  padding: 8px 12px; "
      "  background-color: white; "
      "  border: 2px solid #d1d5db; "
      "  border-radius: 6px; "
      "  font-size: 13px; "
      "  font-weight: 500; "
      "} "
      "QComboBox:hover { "
      "  border-color: #007ACC; "
      "} "
      "QComboBox:focus { "
      "  border-color: #0066cc; "
      "  outline: none; "
      "} "
      "QComboBox::drop-down { "
      "  border: none; "
      "  padding-right: 8px; "
      "} "
      "QComboBox::down-arrow { "
      "  image: none; "
      "  border: 2px solid #6b7280; "
      "  border-radius: 2px; "
      "  width: 8px; height: 8px; "
      "  border-top: none; "
      "  border-right: none; "
      "}");
  connect(workspaceComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
          this, &WorkspaceManager::onWorkspaceChanged);
  layout->addWidget(workspaceComboBox);

  // Workspace management buttons with improved styling
  QString buttonStyle =
      "QPushButton { "
      "  padding: 8px 16px; "
      "  background-color: #007ACC; "
      "  color: white; "
      "  border: none; "
      "  border-radius: 6px; "
      "  font-weight: bold; "
      "  font-size: 12px; "
      "  min-width: 70px; "
      "} "
      "QPushButton:hover { "
      "  background-color: #005a9e; "
      "} "
      "QPushButton:pressed { "
      "  background-color: #004578; "
      "}";

  QString dangerButtonStyle =
      "QPushButton { "
      "  padding: 8px 16px; "
      "  background-color: #dc3545; "
      "  color: white; "
      "  border: none; "
      "  border-radius: 6px; "
      "  font-weight: bold; "
      "  font-size: 12px; "
      "  min-width: 70px; "
      "} "
      "QPushButton:hover { "
      "  background-color: #c82333; "
      "} "
      "QPushButton:pressed { "
      "  background-color: #bd2130; "
      "}";

  newWorkspaceButton = new QPushButton("+ New", toolbar);
  newWorkspaceButton->setToolTip("Create new workspace");
  newWorkspaceButton->setStyleSheet(buttonStyle);
  connect(newWorkspaceButton, &QPushButton::clicked, this, &WorkspaceManager::onNewWorkspaceClicked);
  layout->addWidget(newWorkspaceButton);

  renameWorkspaceButton = new QPushButton("Rename", toolbar);
  renameWorkspaceButton->setToolTip("Rename current workspace");
  renameWorkspaceButton->setStyleSheet(buttonStyle);
  connect(renameWorkspaceButton, &QPushButton::clicked, this, &WorkspaceManager::onRenameWorkspaceClicked);
  layout->addWidget(renameWorkspaceButton);

  deleteWorkspaceButton = new QPushButton("Delete", toolbar);
  deleteWorkspaceButton->setToolTip("Delete current workspace");
  deleteWorkspaceButton->setStyleSheet(dangerButtonStyle);
  connect(deleteWorkspaceButton, &QPushButton::clicked, this, &WorkspaceManager::onDeleteWorkspaceClicked);
  layout->addWidget(deleteWorkspaceButton);

  layout->addStretch();

  updateWorkspaceComboBox();

  return toolbar;
}

void WorkspaceManager::saveCurrentWorkspace() {
  if (!tabWidget || currentWorkspaceId.isEmpty())
    return;

  // Find current workspace
  for (int i = 0; i < workspaces.size(); ++i) {
    if (workspaces[i].id == currentWorkspaceId) {
      Workspace &workspace = workspaces[i];

      // Clear existing data
      workspace.tabUrls.clear();
      workspace.tabTitles.clear();
      workspace.activeTabIndex = tabWidget->currentIndex();

      // Save current tabs
      for (int j = 0; j < tabWidget->count(); ++j) {
        WebView *webView = qobject_cast<WebView *>(tabWidget->widget(j));
        if (webView) {
          workspace.tabUrls.append(webView->url().toString());
          workspace.tabTitles.append(webView->title().isEmpty() ? "Untitled" : webView->title());
        }
      }
      break;
    }
  }
}

void WorkspaceManager::loadWorkspace(const QString &workspaceId) {
  saveCurrentWorkspace(); // Save current workspace before switching

  // Find target workspace
  Workspace *targetWorkspace = nullptr;
  for (auto &workspace : workspaces) {
    if (workspace.id == workspaceId) {
      targetWorkspace = &workspace;
      break;
    }
  }

  if (!targetWorkspace)
    return;

  currentWorkspaceId = workspaceId;

  // Close all current tabs
  emit requestCloseAllTabs();

  // Load workspace tabs
  if (targetWorkspace->tabUrls.isEmpty()) {
    // Create a default tab if workspace is empty
    emit requestNewTab("https://www.google.com");
  } else {
    for (const QString &url : targetWorkspace->tabUrls) {
      emit requestNewTab(url);
    }

    // Set active tab
    if (targetWorkspace->activeTabIndex >= 0 &&
        targetWorkspace->activeTabIndex < targetWorkspace->tabUrls.size()) {
      // This will be handled by the main window after tabs are created
    }
  }

  emit workspaceChanged(workspaceId);
}

void WorkspaceManager::createNewWorkspace(const QString &name) {
  QString id = generateWorkspaceId();
  Workspace newWorkspace(name, id);
  workspaces.append(newWorkspace);

  updateWorkspaceComboBox();

  // Switch to new workspace
  for (int i = 0; i < workspaceComboBox->count(); ++i) {
    if (workspaceComboBox->itemData(i).toString() == id) {
      workspaceComboBox->setCurrentIndex(i);
      break;
    }
  }
}

void WorkspaceManager::deleteWorkspace(const QString &workspaceId) {
  if (workspaces.size() <= 1) {
    QMessageBox::warning(nullptr, "Cannot Delete", "Cannot delete the last workspace.");
    return;
  }

  for (int i = 0; i < workspaces.size(); ++i) {
    if (workspaces[i].id == workspaceId) {
      workspaces.removeAt(i);
      break;
    }
  }

  updateWorkspaceComboBox();

  // Switch to first available workspace if current was deleted
  if (currentWorkspaceId == workspaceId && !workspaces.isEmpty()) {
    workspaceComboBox->setCurrentIndex(0);
  }
}

void WorkspaceManager::renameWorkspace(const QString &workspaceId, const QString &newName) {
  for (auto &workspace : workspaces) {
    if (workspace.id == workspaceId) {
      workspace.name = newName;
      updateWorkspaceComboBox();
      break;
    }
  }
}

QStringList WorkspaceManager::getWorkspaceNames() const {
  QStringList names;
  for (const auto &workspace : workspaces) {
    names.append(workspace.name);
  }
  return names;
}

QString WorkspaceManager::getCurrentWorkspaceId() const {
  return currentWorkspaceId;
}

QString WorkspaceManager::getCurrentWorkspaceName() const {
  for (const auto &workspace : workspaces) {
    if (workspace.id == currentWorkspaceId) {
      return workspace.name;
    }
  }
  return QString();
}

void WorkspaceManager::onWorkspaceChanged() {
  if (!workspaceComboBox)
    return;

  QString selectedId = workspaceComboBox->currentData().toString();
  if (!selectedId.isEmpty() && selectedId != currentWorkspaceId) {
    loadWorkspace(selectedId);
  }
}

void WorkspaceManager::onNewWorkspaceClicked() {
  bool ok;
  QString name = QInputDialog::getText(nullptr, "New Workspace",
                                       "Enter workspace name:",
                                       QLineEdit::Normal, QString(), &ok);
  if (ok && !name.isEmpty()) {
    createNewWorkspace(name);
  }
}

void WorkspaceManager::onDeleteWorkspaceClicked() {
  if (workspaces.size() <= 1) {
    QMessageBox::warning(nullptr, "Cannot Delete", "Cannot delete the last workspace.");
    return;
  }

  int ret = QMessageBox::question(nullptr, "Delete Workspace",
                                  QString("Are you sure you want to delete workspace '%1'?")
                                      .arg(getCurrentWorkspaceName()),
                                  QMessageBox::Yes | QMessageBox::No);

  if (ret == QMessageBox::Yes) {
    deleteWorkspace(currentWorkspaceId);
  }
}

void WorkspaceManager::onRenameWorkspaceClicked() {
  bool ok;
  QString currentName = getCurrentWorkspaceName();
  QString newName = QInputDialog::getText(nullptr, "Rename Workspace",
                                          "Enter new name:",
                                          QLineEdit::Normal, currentName, &ok);
  if (ok && !newName.isEmpty() && newName != currentName) {
    renameWorkspace(currentWorkspaceId, newName);
  }
}

void WorkspaceManager::setupDefaultWorkspace() {
  Workspace defaultWorkspace("Default", generateWorkspaceId());
  workspaces.append(defaultWorkspace);
  currentWorkspaceId = defaultWorkspace.id;
}

void WorkspaceManager::saveWorkspacesToFile() {
  QJsonArray workspaceArray;

  for (const auto &workspace : workspaces) {
    QJsonObject workspaceObj;
    workspaceObj["name"] = workspace.name;
    workspaceObj["id"] = workspace.id;
    workspaceObj["activeTabIndex"] = workspace.activeTabIndex;

    QJsonArray urlArray;
    for (const QString &url : workspace.tabUrls) {
      urlArray.append(url);
    }
    workspaceObj["tabUrls"] = urlArray;

    QJsonArray titleArray;
    for (const QString &title : workspace.tabTitles) {
      titleArray.append(title);
    }
    workspaceObj["tabTitles"] = titleArray;

    workspaceArray.append(workspaceObj);
  }

  QJsonObject rootObj;
  rootObj["workspaces"] = workspaceArray;
  rootObj["currentWorkspaceId"] = currentWorkspaceId;

  QJsonDocument doc(rootObj);

  QFile file(settingsPath);
  if (file.open(QIODevice::WriteOnly)) {
    file.write(doc.toJson());
  }
}

void WorkspaceManager::loadWorkspacesFromFile() {
  QFile file(settingsPath);
  if (!file.open(QIODevice::ReadOnly)) {
    return;
  }

  QByteArray data = file.readAll();
  QJsonDocument doc = QJsonDocument::fromJson(data);
  QJsonObject rootObj = doc.object();

  currentWorkspaceId = rootObj["currentWorkspaceId"].toString();

  QJsonArray workspaceArray = rootObj["workspaces"].toArray();
  workspaces.clear();

  for (const auto &value : workspaceArray) {
    QJsonObject workspaceObj = value.toObject();

    Workspace workspace;
    workspace.name = workspaceObj["name"].toString();
    workspace.id = workspaceObj["id"].toString();
    workspace.activeTabIndex = workspaceObj["activeTabIndex"].toInt();

    QJsonArray urlArray = workspaceObj["tabUrls"].toArray();
    for (const auto &urlValue : urlArray) {
      workspace.tabUrls.append(urlValue.toString());
    }

    QJsonArray titleArray = workspaceObj["tabTitles"].toArray();
    for (const auto &titleValue : titleArray) {
      workspace.tabTitles.append(titleValue.toString());
    }

    workspaces.append(workspace);
  }
}

QString WorkspaceManager::generateWorkspaceId() const {
  return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

void WorkspaceManager::updateWorkspaceComboBox() {
  if (!workspaceComboBox)
    return;

  QString currentId = workspaceComboBox->currentData().toString();

  workspaceComboBox->clear();
  for (const auto &workspace : workspaces) {
    workspaceComboBox->addItem(workspace.name, workspace.id);
  }

  // Restore selection
  for (int i = 0; i < workspaceComboBox->count(); ++i) {
    if (workspaceComboBox->itemData(i).toString() == currentId) {
      workspaceComboBox->setCurrentIndex(i);
      break;
    }
  }
}
