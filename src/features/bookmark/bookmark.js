/**
 * Bookmark Enhancement JavaScript
 * Handles bookmark-related UI enhancements and interactions
 */

class BookmarkHandler {
  constructor() {
    this.bookmarks = [];
    this.init();
  }

  init() {
    this.setupBookmarkShortcuts();
    this.setupBookmarkUI();
    console.log("BookmarkHandler initialized");
  }

  setupBookmarkShortcuts() {
    // Ctrl+D または Cmd+D でブックマーク追加
    document.addEventListener("keydown", (e) => {
      if ((e.ctrlKey || e.metaKey) && e.key === "d") {
        e.preventDefault();
        this.showAddBookmarkDialog();
      }
    });

    // Ctrl+Shift+B または Cmd+Shift+B でブックマークリスト表示
    document.addEventListener("keydown", (e) => {
      if ((e.ctrlKey || e.metaKey) && e.shiftKey && e.key === "B") {
        e.preventDefault();
        this.showBookmarkList();
      }
    });
  }

  setupBookmarkUI() {
    // ブックマークボタンのスタイル適用
    this.applyBookmarkStyles();

    // ブックマークリストの初期化
    this.initializeBookmarkList();
  }

  applyBookmarkStyles() {
    const style = document.createElement("style");
    style.textContent = `
            .bookmark-button {
                background: #007ACC;
                color: white;
                border: none;
                border-radius: 4px;
                padding: 6px 12px;
                cursor: pointer;
                font-size: 12px;
                transition: background-color 0.2s ease;
            }

            .bookmark-button:hover {
                background: #005a9e;
            }

            .bookmark-star {
                color: #fbbf24;
                font-size: 16px;
            }

            .bookmark-star.active {
                color: #f59e0b;
            }
        `;
    document.head.appendChild(style);
  }

  showAddBookmarkDialog() {
    // ブックマーク追加ダイアログを表示
    const title = document.title || "Untitled";
    const url = window.location.href;

    console.log("Adding bookmark:", { title, url });

    // QtWebEngineでC++側にシグナルを送信
    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "addBookmark",
        data: { title, url },
      });
    }
  }

  showBookmarkList() {
    // ブックマークリストを表示
    console.log("Showing bookmark list");

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "showBookmarks",
      });
    }
  }

  initializeBookmarkList() {
    // ページ内のリンクにブックマーク機能を追加
    const links = document.querySelectorAll("a[href]");
    links.forEach((link) => {
      this.addBookmarkIcon(link);
    });
  }

  addBookmarkIcon(element) {
    const bookmarkIcon = document.createElement("span");
    bookmarkIcon.className = "bookmark-star";
    bookmarkIcon.innerHTML = "☆";
    bookmarkIcon.title = "ブックマークに追加";

    bookmarkIcon.addEventListener("click", (e) => {
      e.preventDefault();
      e.stopPropagation();
      this.toggleBookmark(element, bookmarkIcon);
    });

    element.style.position = "relative";
    element.appendChild(bookmarkIcon);
  }

  toggleBookmark(element, icon) {
    const isBookmarked = icon.classList.contains("active");

    if (isBookmarked) {
      this.removeBookmark(element, icon);
    } else {
      this.addBookmark(element, icon);
    }
  }

  addBookmark(element, icon) {
    const title = element.textContent || element.title || "Bookmark";
    const url = element.href;

    icon.innerHTML = "★";
    icon.classList.add("active");
    icon.title = "ブックマークから削除";

    console.log("Bookmark added:", { title, url });

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "addBookmark",
        data: { title, url },
      });
    }
  }

  removeBookmark(element, icon) {
    const url = element.href;

    icon.innerHTML = "☆";
    icon.classList.remove("active");
    icon.title = "ブックマークに追加";

    console.log("Bookmark removed:", url);

    if (window.qt && window.qt.webChannelTransport) {
      window.qt.webChannelTransport.send({
        type: "removeBookmark",
        data: { url },
      });
    }
  }

  // C++側からの呼び出し用メソッド
  updateBookmarkStatus(url, isBookmarked) {
    const links = document.querySelectorAll(`a[href="${url}"]`);
    links.forEach((link) => {
      const star = link.querySelector(".bookmark-star");
      if (star) {
        if (isBookmarked) {
          star.innerHTML = "★";
          star.classList.add("active");
          star.title = "ブックマークから削除";
        } else {
          star.innerHTML = "☆";
          star.classList.remove("active");
          star.title = "ブックマークに追加";
        }
      }
    });
  }
}

// ページ読み込み時に初期化
if (document.readyState === "loading") {
  document.addEventListener("DOMContentLoaded", () => {
    window.bookmarkHandler = new BookmarkHandler();
  });
} else {
  window.bookmarkHandler = new BookmarkHandler();
}
