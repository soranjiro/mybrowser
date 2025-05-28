/**
 * Tab Widget Enhancement JavaScript
 * Handles tab management and interactions
 */

class TabWidgetHandler {
  constructor() {
    this.tabs = [];
    this.activeTabId = null;
    this.draggedTab = null;
    this.init();
  }

  init() {
    this.setupKeyboardShortcuts();
    this.setupTabHandlers();
    this.applyTabStyles();
    console.log("TabWidgetHandler initialized");
  }

  setupKeyboardShortcuts() {
    document.addEventListener("keydown", (e) => {
      // Ctrl+T または Cmd+T で新しいタブ
      if ((e.ctrlKey || e.metaKey) && e.key === "t") {
        e.preventDefault();
        this.openNewTab();
      }

      // Ctrl+W または Cmd+W でタブを閉じる
      if ((e.ctrlKey || e.metaKey) && e.key === "w") {
        e.preventDefault();
        this.closeCurrentTab();
      }

      // Ctrl+Tab または Cmd+Alt+→ で次のタブ
      if (
        (e.ctrlKey && e.key === "Tab") ||
        (e.metaKey && e.altKey && e.key === "ArrowRight")
      ) {
        e.preventDefault();
        this.nextTab();
      }

      // Ctrl+Shift+Tab または Cmd+Alt+← で前のタブ
      if (
        (e.ctrlKey && e.shiftKey && e.key === "Tab") ||
        (e.metaKey && e.altKey && e.key === "ArrowLeft")
      ) {
        e.preventDefault();
        this.previousTab();
      }

      // Ctrl+数字 または Cmd+数字 で指定番号のタブ
      if ((e.ctrlKey || e.metaKey) && e.key >= "1" && e.key <= "9") {
        e.preventDefault();
        const tabIndex = parseInt(e.key) - 1;
        this.switchToTab(tabIndex);
      }
    });
  }

  setupTabHandlers() {
    // マウス中クリックでタブを閉じる
    document.addEventListener("auxclick", (e) => {
      if (e.button === 1) {
        // 中クリック
        const tabElement = e.target.closest(".vertical-tab-item");
        if (tabElement) {
          e.preventDefault();
          const tabId = tabElement.dataset.tabId;
          this.closeTab(tabId);
        }
      }
    });

    // 右クリックでコンテキストメニュー
    document.addEventListener("contextmenu", (e) => {
      const tabElement = e.target.closest(".vertical-tab-item");
      if (tabElement) {
        e.preventDefault();
        const tabId = tabElement.dataset.tabId;
        this.showTabContextMenu(e, tabId);
      }
    });
  }

  applyTabStyles() {
    const style = document.createElement("style");
    style.textContent = `
            .tab-favicon {
                width: 16px;
                height: 16px;
                margin-right: 8px;
            }

            .tab-title {
                flex: 1;
                white-space: nowrap;
                overflow: hidden;
                text-overflow: ellipsis;
            }

            .tab-close-btn {
                width: 16px;
                height: 16px;
                border: none;
                background: none;
                cursor: pointer;
                border-radius: 50%;
                display: flex;
                align-items: center;
                justify-content: center;
                opacity: 0.7;
                transition: all 0.2s ease;
            }

            .tab-close-btn:hover {
                background-color: #ef4444;
                color: white;
                opacity: 1;
            }

            .tab-loading {
                animation: pulse 1.5s ease-in-out infinite;
            }

            @keyframes pulse {
                0% { opacity: 1; }
                50% { opacity: 0.5; }
                100% { opacity: 1; }
            }
        `;
    document.head.appendChild(style);
  }

  openNewTab(url = "about:blank") {
    const tabId = this.generateTabId();
    const tab = {
      id: tabId,
      title: url === "about:blank" ? "新しいタブ" : "Loading...",
      url: url,
      favicon: "🌐",
      isLoading: true,
      canGoBack: false,
      canGoForward: false,
    };

    this.tabs.push(tab);
    this.activeTabId = tabId;

    console.log("Opening new tab:", tab);

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "openNewTab",
        data: { tabId, url },
      });
    }

    this.updateTabDisplay();
  }

  closeTab(tabId) {
    const tabIndex = this.tabs.findIndex((tab) => tab.id === tabId);
    if (tabIndex === -1) return;

    this.tabs.splice(tabIndex, 1);

    // アクティブタブが閉じられた場合
    if (this.activeTabId === tabId) {
      if (this.tabs.length > 0) {
        // 次のタブまたは前のタブをアクティブにする
        const newActiveIndex =
          tabIndex < this.tabs.length ? tabIndex : tabIndex - 1;
        this.activeTabId = this.tabs[newActiveIndex].id;
      } else {
        this.activeTabId = null;
      }
    }

    console.log("Closing tab:", tabId);

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "closeTab",
        data: { tabId },
      });
    }

    this.updateTabDisplay();
  }

  closeCurrentTab() {
    if (this.activeTabId) {
      this.closeTab(this.activeTabId);
    }
  }

  switchToTab(tabIndex) {
    if (tabIndex >= 0 && tabIndex < this.tabs.length) {
      const tab = this.tabs[tabIndex];
      this.activateTab(tab.id);
    }
  }

  activateTab(tabId) {
    const tab = this.tabs.find((t) => t.id === tabId);
    if (!tab) return;

    this.activeTabId = tabId;

    console.log("Activating tab:", tabId);

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "activateTab",
        data: { tabId },
      });
    }

    this.updateTabDisplay();
  }

  nextTab() {
    if (this.tabs.length <= 1) return;

    const currentIndex = this.tabs.findIndex(
      (tab) => tab.id === this.activeTabId
    );
    const nextIndex = (currentIndex + 1) % this.tabs.length;
    this.activateTab(this.tabs[nextIndex].id);
  }

  previousTab() {
    if (this.tabs.length <= 1) return;

    const currentIndex = this.tabs.findIndex(
      (tab) => tab.id === this.activeTabId
    );
    const prevIndex =
      currentIndex === 0 ? this.tabs.length - 1 : currentIndex - 1;
    this.activateTab(this.tabs[prevIndex].id);
  }

  updateTabDisplay() {
    // タブリストの表示更新
    const tabList = document.querySelector(".vertical-tab-list");
    if (tabList) {
      this.renderTabs(tabList);
    }
  }

  renderTabs(container) {
    container.innerHTML = this.tabs
      .map(
        (tab) => `
            <div class="vertical-tab-item ${
              tab.id === this.activeTabId ? "active" : ""
            } ${tab.isLoading ? "loading" : ""}"
                 data-tab-id="${tab.id}">
                <div class="vertical-tab-icon">${tab.favicon}</div>
                <div class="vertical-tab-content">
                    <div class="vertical-tab-title">${this.escapeHtml(
                      tab.title
                    )}</div>
                    <div class="vertical-tab-url">${this.escapeHtml(
                      tab.url
                    )}</div>
                </div>
                <button class="vertical-tab-close" title="タブを閉じる">×</button>
            </div>
        `
      )
      .join("");

    // 新しいタブボタンを追加
    const newTabButton = document.createElement("button");
    newTabButton.className = "vertical-tab-new";
    newTabButton.innerHTML =
      '<span class="vertical-tab-new-icon">+</span>新しいタブ';
    newTabButton.addEventListener("click", () => this.openNewTab());
    container.appendChild(newTabButton);

    // イベントリスナーを追加
    container.querySelectorAll(".vertical-tab-item").forEach((item) => {
      const tabId = item.dataset.tabId;

      item.addEventListener("click", (e) => {
        if (!e.target.closest(".vertical-tab-close")) {
          this.activateTab(tabId);
        }
      });

      const closeBtn = item.querySelector(".vertical-tab-close");
      closeBtn.addEventListener("click", (e) => {
        e.stopPropagation();
        this.closeTab(tabId);
      });
    });
  }

  showTabContextMenu(event, tabId) {
    const tab = this.tabs.find((t) => t.id === tabId);
    if (!tab) return;

    const menu = document.createElement("div");
    menu.className = "vertical-tab-context-menu";
    menu.style.left = event.clientX + "px";
    menu.style.top = event.clientY + "px";

    menu.innerHTML = `
            <div class="vertical-tab-context-item" data-action="reload">
                <span>🔄</span> 再読み込み
            </div>
            <div class="vertical-tab-context-item" data-action="duplicate">
                <span>📋</span> タブを複製
            </div>
            <div class="vertical-tab-context-separator"></div>
            <div class="vertical-tab-context-item" data-action="close">
                <span>❌</span> タブを閉じる
            </div>
            <div class="vertical-tab-context-item" data-action="closeOthers">
                <span>🚫</span> 他のタブを閉じる
            </div>
            <div class="vertical-tab-context-item danger" data-action="closeAll">
                <span>💀</span> すべてのタブを閉じる
            </div>
        `;

    document.body.appendChild(menu);

    // メニューアイテムのクリックイベント
    menu.addEventListener("click", (e) => {
      const action = e.target.closest("[data-action]")?.dataset.action;
      if (action) {
        this.handleContextMenuAction(action, tabId);
      }
      document.body.removeChild(menu);
    });

    // メニュー外クリックで閉じる
    setTimeout(() => {
      document.addEventListener(
        "click",
        () => {
          if (document.body.contains(menu)) {
            document.body.removeChild(menu);
          }
        },
        { once: true }
      );
    }, 0);
  }

  handleContextMenuAction(action, tabId) {
    switch (action) {
      case "reload":
        this.reloadTab(tabId);
        break;
      case "duplicate":
        this.duplicateTab(tabId);
        break;
      case "close":
        this.closeTab(tabId);
        break;
      case "closeOthers":
        this.closeOtherTabs(tabId);
        break;
      case "closeAll":
        this.closeAllTabs();
        break;
    }
  }

  reloadTab(tabId) {
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "reloadTab",
        data: { tabId },
      });
    }
  }

  duplicateTab(tabId) {
    const tab = this.tabs.find((t) => t.id === tabId);
    if (tab) {
      this.openNewTab(tab.url);
    }
  }

  closeOtherTabs(keepTabId) {
    const tabsToClose = this.tabs.filter((tab) => tab.id !== keepTabId);
    tabsToClose.forEach((tab) => this.closeTab(tab.id));
  }

  closeAllTabs() {
    const tabsToClose = [...this.tabs];
    tabsToClose.forEach((tab) => this.closeTab(tab.id));
  }

  // C++側からの呼び出し用メソッド
  updateTab(tabId, data) {
    const tab = this.tabs.find((t) => t.id === tabId);
    if (tab) {
      Object.assign(tab, data);
      this.updateTabDisplay();
    }
  }

  setTabLoading(tabId, isLoading) {
    const tab = this.tabs.find((t) => t.id === tabId);
    if (tab) {
      tab.isLoading = isLoading;
      this.updateTabDisplay();
    }
  }

  generateTabId() {
    return "tab_" + Date.now() + "_" + Math.random().toString(36).substr(2, 9);
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
    window.tabWidgetHandler = new TabWidgetHandler();
  });
} else {
  window.tabWidgetHandler = new TabWidgetHandler();
}
