// Common Test Page JavaScript Functionality

class TestPageHelper {
  constructor() {
    this.clickCount = 0;
    this.logElement = null;
    this.init();
  }

  init() {
    this.setupLogging();
    this.setupEventListeners();
    this.logPageLoad();
  }

  setupLogging() {
    this.logElement = document.getElementById("log");
    if (!this.logElement) {
      // Create log element if it doesn't exist
      this.logElement = document.createElement("div");
      this.logElement.id = "log";
      this.logElement.className = "log-area";

      // Try to append to existing container or body
      const container =
        document.querySelector(".container") ||
        document.querySelector(".test-area") ||
        document.body;
      container.appendChild(this.logElement);
    }
  }

  log(message) {
    const timestamp = new Date().toLocaleTimeString();
    const logMessage = `[${timestamp}] ${message}`;

    if (this.logElement) {
      this.logElement.innerHTML += logMessage + "<br>";
      this.logElement.scrollTop = this.logElement.scrollHeight;
    }

    console.log(logMessage);
  }

  clearLog() {
    if (this.logElement) {
      this.logElement.innerHTML = "";
    }
    this.log("ログがクリアされました");
  }

  updateClickCounter(item) {
    this.clickCount++;

    const clickCountElement = document.getElementById("click-count");
    const lastClickElement =
      document.getElementById("last-click") ||
      document.getElementById("last-action");

    if (clickCountElement) {
      clickCountElement.textContent = this.clickCount;
    }

    if (lastClickElement) {
      lastClickElement.textContent = item;
    }
  }

  handleButtonClick(buttonName) {
    this.log(`ボタンクリック: ${buttonName}`);
    this.updateClickCounter(buttonName);
  }

  handleLinkClick(linkName) {
    this.log(`リンククリック: ${linkName}`);
    this.updateClickCounter(linkName);
  }

  testClick(source) {
    this.clickCount++;

    const clickCountElement = document.getElementById("click-count");
    const lastActionElement = document.getElementById("last-action");

    if (clickCountElement) {
      clickCountElement.textContent = this.clickCount;
    }

    if (lastActionElement) {
      lastActionElement.textContent =
        source + " (" + new Date().toLocaleTimeString() + ")";
    }

    console.log("Test Click:", source, "Count:", this.clickCount);
    this.log(`Test Click: ${source} (Count: ${this.clickCount})`);

    // Add visual feedback for JavaScript link clicks
    if (source.includes("JavaScript")) {
      document.body.style.backgroundColor = "#e8f5e8";
      setTimeout(() => {
        document.body.style.backgroundColor = "";
      }, 1000);
      alert(
        "JavaScriptリンクが正常に動作しました！クリック回数: " + this.clickCount
      );
    }
  }

  setupEventListeners() {
    // Add event listeners for debugging
    document.addEventListener("mousedown", (e) => {
      this.log(
        `MouseDown: ${e.target.tagName} at (${e.clientX}, ${e.clientY}) button: ${e.button}`
      );
    });

    document.addEventListener("mouseup", (e) => {
      this.log(
        `MouseUp: ${e.target.tagName} at (${e.clientX}, ${e.clientY}) button: ${e.button}`
      );
    });

    document.addEventListener("click", (e) => {
      this.log(
        `Click: ${e.target.tagName} at (${e.clientX}, ${e.clientY}) button: ${e.button}`
      );

      // Page Level Click Event logging with detailed information
      console.log("Page Level Click Event:", {
        target: e.target,
        tagName: e.target.tagName,
        className: e.target.className,
        id: e.target.id,
        href: e.target.href,
        type: e.target.type,
        value: e.target.value,
        defaultPrevented: e.defaultPrevented,
        bubbles: e.bubbles,
        cancelable: e.cancelable,
        isTrusted: e.isTrusted,
        timestamp: e.timeStamp,
        button: e.button,
        buttons: e.buttons,
        x: e.clientX,
        y: e.clientY,
      });
    });

    // Focus events
    document.addEventListener("focusin", (e) => {
      console.log("Focus IN:", e.target.tagName, e.target.type);
    });

    document.addEventListener("focusout", (e) => {
      console.log("Focus OUT:", e.target.tagName, e.target.type);
    });
  }

  logPageLoad() {
    window.addEventListener("load", () => {
      this.log("ページが読み込まれました");

      const loadTimeElement = document.getElementById("load-time");
      if (loadTimeElement) {
        loadTimeElement.textContent = new Date().toLocaleString();
      }

      console.log("Page loaded successfully");

      // Log specific test page type
      const title = document.title;
      if (title) {
        this.log(`${title}が準備完了`);
      }
    });
  }
}

// Picture-in-Picture Test Functions
class PiPTestHelper {
  constructor() {
    this.testHelper = window.testHelper;
  }

  log(message) {
    if (this.testHelper) {
      this.testHelper.log(message);
    } else {
      console.log(message);
    }
  }

  testPiPSupport() {
    this.log("=== PiP サポート確認開始 ===");
    this.log(
      "pictureInPictureEnabled: " + ("pictureInPictureEnabled" in document)
    );
    this.log(
      "document.pictureInPictureEnabled: " + document.pictureInPictureEnabled
    );
    this.log(
      "requestPictureInPicture: " +
        typeof HTMLVideoElement.prototype.requestPictureInPicture
    );

    const videos = document.querySelectorAll("video");
    this.log("動画要素数: " + videos.length);

    videos.forEach((video, index) => {
      this.log(
        `動画${index + 1}: ${video.src || video.currentSrc || "ソースなし"}`
      );
    });
  }

  async requestPiP() {
    this.log("=== PiP 開始要求 ===");
    const video = document.querySelector("video");

    if (!video) {
      this.log("エラー: 動画が見つかりません");
      return;
    }

    try {
      if (video.paused) {
        await video.play();
        this.log("動画を再生開始");
      }

      const pipWindow = await video.requestPictureInPicture();
      this.log("PiP 開始成功!");
      this.log("PiP ウィンドウ: " + pipWindow);
    } catch (error) {
      this.log("PiP エラー: " + error.message);
    }
  }

  async exitPiP() {
    this.log("=== PiP 終了要求 ===");

    try {
      if (document.pictureInPictureElement) {
        await document.exitPictureInPicture();
        this.log("PiP 終了成功");
      } else {
        this.log("PiP は開始されていません");
      }
    } catch (error) {
      this.log("PiP 終了エラー: " + error.message);
    }
  }

  testVideoState() {
    this.log("=== ビデオ状態確認 ===");
    const video = document.querySelector("video");

    if (!video) {
      this.log("エラー: 動画が見つかりません");
      return;
    }

    this.log("一時停止中: " + video.paused);
    this.log("再生時間: " + video.currentTime);
    this.log("長さ: " + video.duration);
    this.log("準備状態: " + video.readyState);
    this.log("PiP無効化属性: " + video.hasAttribute("disablepictureinpicture"));
  }

  clearLog() {
    if (this.testHelper) {
      this.testHelper.clearLog();
    } else {
      const logElement = document.getElementById("log");
      if (logElement) {
        logElement.innerHTML = "";
      }
    }
  }
}

// Global initialization
window.addEventListener("DOMContentLoaded", () => {
  // Initialize test helper
  window.testHelper = new TestPageHelper();

  // Initialize PiP test helper
  window.pipTestHelper = new PiPTestHelper();

  // Expose global functions for backward compatibility
  window.log = (message) => window.testHelper.log(message);
  window.clearLog = () => window.testHelper.clearLog();
  window.testClick = (source) => window.testHelper.testClick(source);
  window.handleButtonClick = (buttonName) =>
    window.testHelper.handleButtonClick(buttonName);
  window.handleLinkClick = (linkName) =>
    window.testHelper.handleLinkClick(linkName);

  // PiP test functions
  window.testPiPSupport = () => window.pipTestHelper.testPiPSupport();
  window.requestPiP = () => window.pipTestHelper.requestPiP();
  window.exitPiP = () => window.pipTestHelper.exitPiP();
  window.testVideoState = () => window.pipTestHelper.testVideoState();
});
