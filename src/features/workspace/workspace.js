/**
 * Workspace Enhancement JavaScript
 * Handles workspace management and switching
 */

class WorkspaceHandler {
  constructor() {
    this.workspaces = [];
    this.currentWorkspaceId = null;
    this.isVisible = false;
    this.selectedIndex = 0;
    this.init();
  }

  init() {
    this.setupKeyboardShortcuts();
    this.setupWorkspaceUI();
    this.loadWorkspaces();
    console.log("WorkspaceHandler initialized");
  }

  setupKeyboardShortcuts() {
    document.addEventListener("keydown", (e) => {
      // Ctrl+Shift+W または Cmd+Shift+W でワークスペース切り替え
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === "W") {
        e.preventDefault();
        this.showWorkspaceSwitcher();
      }

      // Ctrl+Alt+N または Cmd+Alt+N で新しいワークスペース
      if ((e.ctrlKey || e.metaKey) && e.altKey && e.key === "n") {
        e.preventDefault();
        this.showCreateWorkspaceDialog();
      }

      // ワークスペーススイッチャーが開いている時のキー操作
      if (this.isVisible) {
        if (e.key === "ArrowDown" || e.key === "Tab") {
          e.preventDefault();
          this.selectNext();
        } else if (e.key === "ArrowUp" || (e.shiftKey && e.key === "Tab")) {
          e.preventDefault();
          this.selectPrevious();
        } else if (e.key === "Enter") {
          e.preventDefault();
          this.switchToSelected();
        } else if (e.key === "Escape") {
          e.preventDefault();
          this.hideWorkspaceSwitcher();
        }
      }
    });
  }

  setupWorkspaceUI() {
    this.createWorkspaceSwitcherElement();
    this.applyWorkspaceStyles();
  }

  applyWorkspaceStyles() {
    const style = document.createElement("style");
    style.textContent = `
            .workspace-indicator {
                position: fixed;
                top: 10px;
                right: 10px;
                background-color: #007ACC;
                color: white;
                padding: 6px 12px;
                border-radius: 4px;
                font-size: 12px;
                font-weight: 500;
                z-index: 1000;
                transition: all 0.3s ease;
                cursor: pointer;
            }

            .workspace-indicator:hover {
                background-color: #005a9e;
                transform: scale(1.05);
            }

            .workspace-notification {
                position: fixed;
                top: 20px;
                right: 20px;
                background-color: #10b981;
                color: white;
                padding: 12px 16px;
                border-radius: 6px;
                font-size: 14px;
                z-index: 1001;
                animation: slideInFromRight 0.3s ease-out;
            }

            @keyframes slideInFromRight {
                from {
                    transform: translateX(100%);
                    opacity: 0;
                }
                to {
                    transform: translateX(0);
                    opacity: 1;
                }
            }
        `;
    document.head.appendChild(style);
  }

  createWorkspaceSwitcherElement() {
    const overlay = document.createElement("div");
    overlay.className = "workspace-overlay";
    overlay.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: rgba(0, 0, 0, 0.3);
            z-index: 10000;
            display: none;
            align-items: center;
            justify-content: center;
        `;

    overlay.innerHTML = `
            <div class="workspace-switcher">
                <div class="workspace-switcher-header">ワークスペース切り替え</div>
                <div class="workspace-switcher-list"></div>
            </div>
        `;

    document.body.appendChild(overlay);
    this.switcherElement = overlay;
    this.switcherList = overlay.querySelector(".workspace-switcher-list");

    // オーバーレイクリックで閉じる
    overlay.addEventListener("click", (e) => {
      if (e.target === overlay) {
        this.hideWorkspaceSwitcher();
      }
    });
  }

  loadWorkspaces() {
    // デフォルトワークスペースを作成
    this.workspaces = [
      {
        id: "default",
        name: "メインワークスペース",
        description: "デフォルトのブラウジング環境",
        color: "#007ACC",
        tabCount: 1,
        lastAccessed: new Date(),
        isActive: true,
      },
    ];

    this.currentWorkspaceId = "default";
    this.updateWorkspaceIndicator();

    // C++側からワークスペース情報を取得
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "loadWorkspaces",
      });
    }
  }

  showWorkspaceSwitcher() {
    this.isVisible = true;
    this.selectedIndex = this.workspaces.findIndex(
      (ws) => ws.id === this.currentWorkspaceId
    );
    this.switcherElement.style.display = "flex";
    this.renderWorkspaces();
    this.updateSelection();
  }

  hideWorkspaceSwitcher() {
    this.isVisible = false;
    this.switcherElement.style.display = "none";
  }

  renderWorkspaces() {
    this.switcherList.innerHTML = this.workspaces
      .map(
        (workspace, index) => `
            <div class="workspace-switcher-item ${
              workspace.id === this.currentWorkspaceId ? "selected" : ""
            }"
                 data-index="${index}">
                <div class="workspace-switcher-icon" style="background-color: ${
                  workspace.color
                }">
                    ${workspace.name.charAt(0).toUpperCase()}
                </div>
                <div class="workspace-switcher-name">${this.escapeHtml(
                  workspace.name
                )}</div>
            </div>
        `
      )
      .join("");

    // アイテムクリックイベント
    this.switcherList
      .querySelectorAll(".workspace-switcher-item")
      .forEach((item, index) => {
        item.addEventListener("click", () => {
          this.selectedIndex = index;
          this.switchToSelected();
        });

        item.addEventListener("mouseenter", () => {
          this.selectedIndex = index;
          this.updateSelection();
        });
      });
  }

  updateSelection() {
    const items = this.switcherList.querySelectorAll(
      ".workspace-switcher-item"
    );
    items.forEach((item, index) => {
      item.classList.toggle("selected", index === this.selectedIndex);
    });
  }

  selectNext() {
    if (this.workspaces.length > 0) {
      this.selectedIndex = (this.selectedIndex + 1) % this.workspaces.length;
      this.updateSelection();
    }
  }

  selectPrevious() {
    if (this.workspaces.length > 0) {
      this.selectedIndex =
        this.selectedIndex === 0
          ? this.workspaces.length - 1
          : this.selectedIndex - 1;
      this.updateSelection();
    }
  }

  switchToSelected() {
    if (this.workspaces.length > 0 && this.selectedIndex >= 0) {
      const workspace = this.workspaces[this.selectedIndex];
      this.switchToWorkspace(workspace.id);
      this.hideWorkspaceSwitcher();
    }
  }

  switchToWorkspace(workspaceId) {
    const workspace = this.workspaces.find((ws) => ws.id === workspaceId);
    if (!workspace) return;

    this.currentWorkspaceId = workspaceId;
    workspace.lastAccessed = new Date();

    // 他のワークスペースを非アクティブに
    this.workspaces.forEach((ws) => (ws.isActive = false));
    workspace.isActive = true;

    console.log("Switching to workspace:", workspace.name);

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "switchWorkspace",
        data: { workspaceId },
      });
    }

    this.updateWorkspaceIndicator();
    this.showNotification(
      `ワークスペース "${workspace.name}" に切り替えました`
    );
  }

  showCreateWorkspaceDialog() {
    const dialog = this.createWorkspaceDialog();
    document.body.appendChild(dialog);

    const nameInput = dialog.querySelector("#workspace-name");
    nameInput.focus();
  }

  createWorkspaceDialog() {
    const overlay = document.createElement("div");
    overlay.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background-color: rgba(0, 0, 0, 0.3);
            z-index: 10001;
            display: flex;
            align-items: center;
            justify-content: center;
        `;

    const colors = [
      "#007ACC",
      "#dc2626",
      "#059669",
      "#d97706",
      "#7c3aed",
      "#db2777",
      "#0891b2",
      "#65a30d",
    ];

    overlay.innerHTML = `
            <div class="workspace-dialog">
                <h3>新しいワークスペースを作成</h3>
                <div class="workspace-form-group">
                    <label for="workspace-name">ワークスペース名</label>
                    <input type="text" id="workspace-name" placeholder="例: 開発用" required>
                </div>
                <div class="workspace-form-group">
                    <label for="workspace-description">説明（任意）</label>
                    <textarea id="workspace-description" placeholder="このワークスペースの用途を入力してください"></textarea>
                </div>
                <div class="workspace-form-group">
                    <label>カラー</label>
                    <div class="workspace-color-picker">
                        ${colors
                          .map(
                            (color, index) => `
                            <div class="workspace-color-option ${
                              index === 0 ? "selected" : ""
                            }"
                                 data-color="${color}"
                                 style="background-color: ${color}"></div>
                        `
                          )
                          .join("")}
                    </div>
                </div>
                <div class="workspace-dialog-actions">
                    <button type="button" class="workspace-btn secondary" data-action="cancel">キャンセル</button>
                    <button type="button" class="workspace-btn" data-action="create">作成</button>
                </div>
            </div>
        `;

    // イベントリスナー
    const nameInput = overlay.querySelector("#workspace-name");
    const descInput = overlay.querySelector("#workspace-description");
    const colorOptions = overlay.querySelectorAll(".workspace-color-option");
    let selectedColor = colors[0];

    colorOptions.forEach((option) => {
      option.addEventListener("click", () => {
        colorOptions.forEach((opt) => opt.classList.remove("selected"));
        option.classList.add("selected");
        selectedColor = option.dataset.color;
      });
    });

    overlay.addEventListener("click", (e) => {
      const action = e.target.dataset.action;
      if (action === "cancel" || e.target === overlay) {
        document.body.removeChild(overlay);
      } else if (action === "create") {
        const name = nameInput.value.trim();
        if (name) {
          this.createWorkspace({
            name,
            description: descInput.value.trim(),
            color: selectedColor,
          });
          document.body.removeChild(overlay);
        }
      }
    });

    // Enterキーで作成
    nameInput.addEventListener("keypress", (e) => {
      if (e.key === "Enter") {
        const name = nameInput.value.trim();
        if (name) {
          this.createWorkspace({
            name,
            description: descInput.value.trim(),
            color: selectedColor,
          });
          document.body.removeChild(overlay);
        }
      }
    });

    return overlay;
  }

  createWorkspace(data) {
    const workspace = {
      id: this.generateWorkspaceId(),
      name: data.name,
      description: data.description || "",
      color: data.color,
      tabCount: 0,
      lastAccessed: new Date(),
      isActive: false,
    };

    this.workspaces.push(workspace);

    console.log("Creating workspace:", workspace);

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "createWorkspace",
        data: workspace,
      });
    }

    this.showNotification(`ワークスペース "${workspace.name}" を作成しました`);
  }

  updateWorkspaceIndicator() {
    let indicator = document.querySelector(".workspace-indicator");
    if (!indicator) {
      indicator = document.createElement("div");
      indicator.className = "workspace-indicator";
      document.body.appendChild(indicator);

      indicator.addEventListener("click", () => {
        this.showWorkspaceSwitcher();
      });
    }

    const currentWorkspace = this.workspaces.find(
      (ws) => ws.id === this.currentWorkspaceId
    );
    if (currentWorkspace) {
      indicator.textContent = currentWorkspace.name;
      indicator.style.backgroundColor = currentWorkspace.color;
    }
  }

  showNotification(message) {
    const notification = document.createElement("div");
    notification.className = "workspace-notification";
    notification.textContent = message;
    document.body.appendChild(notification);

    setTimeout(() => {
      if (document.body.contains(notification)) {
        document.body.removeChild(notification);
      }
    }, 3000);
  }

  // C++側からの呼び出し用メソッド
  updateWorkspaceList(workspaces) {
    this.workspaces = workspaces;
    this.updateWorkspaceIndicator();
  }

  updateTabCount(workspaceId, count) {
    const workspace = this.workspaces.find((ws) => ws.id === workspaceId);
    if (workspace) {
      workspace.tabCount = count;
    }
  }

  generateWorkspaceId() {
    return (
      "workspace_" + Date.now() + "_" + Math.random().toString(36).substr(2, 9)
    );
  }

  escapeHtml(text) {
    const div = document.createElement("div");
    div.textContent = text;
    return div.innerHTML;
  }
}

// ページ読み込み時に初期化
if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", () => {
    window.workspaceHandler = new WorkspaceHandler();
  });
} else {
  window.workspaceHandler = new WorkspaceHandler();
}
