/**
 * Command Palette Enhancement JavaScript
 * Handles command palette functionality and keyboard shortcuts
 */

class CommandPaletteHandler {
  constructor() {
    this.commands = [];
    this.isVisible = false;
    this.selectedIndex = 0;
    this.filteredCommands = [];
    this.init();
  }

  init() {
    this.setupKeyboardShortcuts();
    this.loadCommands();
    this.createPaletteElement();
    console.log("CommandPaletteHandler initialized");
  }

  setupKeyboardShortcuts() {
    // Ctrl+Shift+P または Cmd+Shift+P でコマンドパレット表示
    document.addEventListener("keydown", (e) => {
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === "P") {
        e.preventDefault();
        this.toggle();
      }

      // ESCキーで閉じる
      if (e.key === "Escape" && this.isVisible) {
        e.preventDefault();
        this.hide();
      }
    });
  }

  loadCommands() {
    this.commands = [
      {
        id: "reload",
        title: "ページを再読み込み",
        description: "現在のページを再読み込みします",
        icon: "🔄",
        shortcut: "Ctrl+R",
        action: () => window.location.reload(),
      },
      {
        id: "back",
        title: "前のページに戻る",
        description: "ブラウザの履歴で前のページに戻ります",
        icon: "←",
        shortcut: "Alt+←",
        action: () => history.back(),
      },
      {
        id: "forward",
        title: "次のページに進む",
        description: "ブラウザの履歴で次のページに進みます",
        icon: "→",
        shortcut: "Alt+→",
        action: () => history.forward(),
      },
      {
        id: "home",
        title: "ホームページに移動",
        description: "ホームページに移動します",
        icon: "🏠",
        shortcut: "Ctrl+H",
        action: () => this.goHome(),
      },
      {
        id: "bookmark",
        title: "ページをブックマーク",
        description: "現在のページをブックマークに追加します",
        icon: "⭐",
        shortcut: "Ctrl+D",
        action: () => this.addBookmark(),
      },
      {
        id: "fullscreen",
        title: "フルスクリーン切り替え",
        description: "フルスクリーンモードのオン/オフを切り替えます",
        icon: "⛶",
        shortcut: "F11",
        action: () => this.toggleFullscreen(),
      },
      {
        id: "devtools",
        title: "開発者ツールを開く",
        description: "ブラウザの開発者ツールを開きます",
        icon: "🛠",
        shortcut: "F12",
        action: () => this.openDevTools(),
      },
      {
        id: "search",
        title: "ページ内検索",
        description: "ページ内でテキストを検索します",
        icon: "🔍",
        shortcut: "Ctrl+F",
        action: () => this.openSearch(),
      },
      {
        id: "print",
        title: "ページを印刷",
        description: "現在のページを印刷します",
        icon: "🖨",
        shortcut: "Ctrl+P",
        action: () => window.print(),
      },
      {
        id: "zoom-in",
        title: "ズームイン",
        description: "ページを拡大します",
        icon: "🔍+",
        shortcut: "Ctrl++",
        action: () => this.zoomIn(),
      },
      {
        id: "zoom-out",
        title: "ズームアウト",
        description: "ページを縮小します",
        icon: "🔍-",
        shortcut: "Ctrl+-",
        action: () => this.zoomOut(),
      },
      {
        id: "zoom-reset",
        title: "ズームリセット",
        description: "ズームレベルを100%に戻します",
        icon: "🔍=",
        shortcut: "Ctrl+0",
        action: () => this.zoomReset(),
      },
    ];
  }

  createPaletteElement() {
    const overlay = document.createElement("div");
    overlay.className = "command-palette-overlay";
    overlay.style.display = "none";

    overlay.innerHTML = `
            <div class="command-palette">
                <div class="command-palette-header">
                    <input type="text" class="command-palette-input" placeholder="コマンドを入力してください...">
                </div>
                <div class="command-palette-list"></div>
                <div class="command-palette-footer">
                    <div class="command-palette-hint">
                        <span><span class="command-key">↑↓</span> 選択</span>
                        <span><span class="command-key">Enter</span> 実行</span>
                        <span><span class="command-key">Esc</span> 閉じる</span>
                    </div>
                </div>
            </div>
        `;

    document.body.appendChild(overlay);
    this.paletteElement = overlay;
    this.inputElement = overlay.querySelector(".command-palette-input");
    this.listElement = overlay.querySelector(".command-palette-list");

    this.setupPaletteEvents();
  }

  setupPaletteEvents() {
    // 入力フィールドのイベント
    this.inputElement.addEventListener("input", (e) => {
      this.filterCommands(e.target.value);
    });

    this.inputElement.addEventListener("keydown", (e) => {
      if (e.key === "ArrowDown") {
        e.preventDefault();
        this.selectNext();
      } else if (e.key === "ArrowUp") {
        e.preventDefault();
        this.selectPrevious();
      } else if (e.key === "Enter") {
        e.preventDefault();
        this.executeSelected();
      }
    });

    // オーバーレイクリックで閉じる
    this.paletteElement.addEventListener("click", (e) => {
      if (e.target === this.paletteElement) {
        this.hide();
      }
    });
  }

  toggle() {
    if (this.isVisible) {
      this.hide();
    } else {
      this.show();
    }
  }

  show() {
    this.isVisible = true;
    this.paletteElement.style.display = "flex";
    this.inputElement.value = "";
    this.inputElement.focus();
    this.filterCommands("");
    this.selectedIndex = 0;
    this.updateSelection();
  }

  hide() {
    this.isVisible = false;
    this.paletteElement.style.display = "none";
  }

  filterCommands(query) {
    const lowercaseQuery = query.toLowerCase();
    this.filteredCommands = this.commands.filter(
      (command) =>
        command.title.toLowerCase().includes(lowercaseQuery) ||
        command.description.toLowerCase().includes(lowercaseQuery)
    );

    this.selectedIndex = 0;
    this.renderCommands();
    this.updateSelection();
  }

  renderCommands() {
    if (this.filteredCommands.length === 0) {
      this.listElement.innerHTML = `
                <div class="command-no-results">
                    <span class="command-no-results-icon">🔍</span>
                    <div class="command-no-results-text">該当するコマンドが見つかりません</div>
                </div>
            `;
      return;
    }

    this.listElement.innerHTML = this.filteredCommands
      .map(
        (command, index) => `
            <div class="command-item" data-index="${index}">
                <div class="command-icon">${command.icon}</div>
                <div class="command-content">
                    <div class="command-title">${command.title}</div>
                    <div class="command-description">${command.description}</div>
                </div>
                <div class="command-shortcut">${command.shortcut}</div>
            </div>
        `
      )
      .join("");

    // アイテムクリックイベント
    this.listElement
      .querySelectorAll(".command-item")
      .forEach((item, index) => {
        item.addEventListener("click", () => {
          this.selectedIndex = index;
          this.executeSelected();
        });

        item.addEventListener("mouseenter", () => {
          this.selectedIndex = index;
          this.updateSelection();
        });
      });
  }

  updateSelection() {
    const items = this.listElement.querySelectorAll(".command-item");
    items.forEach((item, index) => {
      item.classList.toggle("selected", index === this.selectedIndex);
    });
  }

  selectNext() {
    if (this.filteredCommands.length > 0) {
      this.selectedIndex =
        (this.selectedIndex + 1) % this.filteredCommands.length;
      this.updateSelection();
    }
  }

  selectPrevious() {
    if (this.filteredCommands.length > 0) {
      this.selectedIndex =
        this.selectedIndex === 0
          ? this.filteredCommands.length - 1
          : this.selectedIndex - 1;
      this.updateSelection();
    }
  }

  executeSelected() {
    if (this.filteredCommands.length > 0 && this.selectedIndex >= 0) {
      const command = this.filteredCommands[this.selectedIndex];
      this.hide();
      command.action();
    }
  }

  // コマンドアクション
  goHome() {
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({ type: "goHome" });
    }
  }

  addBookmark() {
    if (window.bookmarkHandler) {
      window.bookmarkHandler.showAddBookmarkDialog();
    }
  }

  toggleFullscreen() {
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({ type: "toggleFullscreen" });
    }
  }

  openDevTools() {
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({ type: "openDevTools" });
    }
  }

  openSearch() {
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({ type: "openSearch" });
    }
  }

  zoomIn() {
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({ type: "zoomIn" });
    }
  }

  zoomOut() {
    if (window.qt && window.qt.webChannelTransport.send) {
      window.qt.webChannelTransport.send({ type: "zoomOut" });
    }
  }

  zoomReset() {
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({ type: "zoomReset" });
    }
  }
}

// ページ読み込み時に初期化
if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", () => {
    window.commandPaletteHandler = new CommandPaletteHandler();
  });
} else {
  window.commandPaletteHandler = new CommandPaletteHandler();
}
