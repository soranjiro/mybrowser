// Picture-in-Picture JavaScript Functionality

// ===== エラーハンドリングとパフォーマンス最適化の定数 =====
const PIP_CONFIG = {
  // タイムアウト設定
  DETECTION_TIMEOUT: 30000, // 30秒
  RETRY_DELAY: 1000, // 1秒
  MAX_RETRIES: 5,

  // パフォーマンス設定
  THROTTLE_DELAY: 100, // スロットリング遅延（ミリ秒）
  DEBOUNCE_DELAY: 300, // デバウンス遅延（ミリ秒）

  // フレームキャプチャ設定
  FRAME_CAPTURE_FPS: 15, // フレームレート
  FRAME_CAPTURE_MAX_DURATION: 600000, // 10分

  // サイト固有設定
  SITE_CONFIGS: {
    "youtube.com": {
      selectors: [
        "ytd-app video",
        'video[src*="googlevideo"]',
        ".ytp-video-container video",
        "#movie_player video",
        ".html5-video-player video",
        "video.video-stream",
        'video[class*="video"]',
        ".ytp-html5-video",
        "video[autoplay]",
        'video:not([width="0"]):not([height="0"])',
        "ytd-player video",
        ".ytd-player-container video",
        "video[poster]",
        "video[controls]",
        "video[preload]",
        ".player-container video",
        "video.html5-main-video",
        'div[id*="player"] video',
      ],
      waitTime: 8000,
      attributes: ["disablepictureinpicture", "controlslist"],
      customLogic: function (videos) {
        // YouTube固有のロジック
        return videos.filter((v) => v.readyState >= 2 && !v.paused);
      },
    },
    "tver.jp": {
      selectors: [
        ".tver-player video",
        "div[class*='Player'] video",
        ".media-player video",
        "video[class*='player']",
        ".video-container video",
        "video.tver-video",
        ".player-wrapper video",
        "video[autoplay]",
        "video:not([width='0']):not([height='0'])",
        ".video-js video",
        "video[poster]",
        "video[controls]",
        "video[preload]",
        "div[id*='player'] video",
        ".jwplayer video",
        "video.jw-video",
        "video[data-*]",
      ],
      waitTime: 10000,
      attributes: ["disablepictureinpicture", "controlslist"],
      customLogic: function (videos) {
        // TVar固有のロジック
        return videos.filter((v) => v.videoWidth > 100 && v.videoHeight > 100);
      },
    },
  },
};

// ===== ユーティリティ関数 =====
class PiPUtils {
  // エラーハンドリング付きの非同期実行
  static async safeExecute(fn, context = "Unknown", timeout = 10000) {
    try {
      const timeoutPromise = new Promise((_, reject) =>
        setTimeout(() => reject(new Error(`Timeout: ${context}`)), timeout)
      );

      const result = await Promise.race([fn(), timeoutPromise]);
      return { success: true, result };
    } catch (error) {
      console.error(`[PiP Error] ${context}:`, error);
      return { success: false, error };
    }
  }

  // スロットリング関数
  static throttle(func, delay) {
    let timeoutId;
    let lastExecTime = 0;
    return function (...args) {
      const currentTime = Date.now();

      if (currentTime - lastExecTime > delay) {
        func.apply(this, args);
        lastExecTime = currentTime;
      } else {
        clearTimeout(timeoutId);
        timeoutId = setTimeout(() => {
          func.apply(this, args);
          lastExecTime = Date.now();
        }, delay - (currentTime - lastExecTime));
      }
    };
  }

  // デバウンス関数
  static debounce(func, delay) {
    let timeoutId;
    return function (...args) {
      clearTimeout(timeoutId);
      timeoutId = setTimeout(() => func.apply(this, args), delay);
    };
  }

  // 要素の可視性をチェック
  static isElementVisible(element) {
    if (!element) return false;

    const rect = element.getBoundingClientRect();
    const style = window.getComputedStyle(element);

    return (
      rect.width > 0 &&
      rect.height > 0 &&
      style.display !== "none" &&
      style.visibility !== "hidden" &&
      style.opacity !== "0"
    );
  }

  // 動画要素の品質スコア計算
  static calculateVideoScore(video, siteConfig = null) {
    let score = 0;

    try {
      // 基本的なスコア計算
      if (video.videoWidth && video.videoHeight) {
        score += Math.log(video.videoWidth * video.videoHeight) * 2;
      }

      if (video.readyState >= 3) score += 30;
      else if (video.readyState >= 2) score += 20;
      else if (video.readyState >= 1) score += 10;

      if (!video.paused) score += 25;
      if (video.currentTime > 0) score += 15;
      if (video.duration > 0) score += 10;
      if (!video.muted) score += 5;

      // 可視性チェック
      if (this.isElementVisible(video)) score += 20;

      // サイト固有のボーナス
      if (siteConfig && siteConfig.customLogic) {
        const customVideos = siteConfig.customLogic([video]);
        if (customVideos.length > 0) score += 50;
      }

      // 属性チェック
      if (video.hasAttribute("disablepictureinpicture")) score -= 10;
      if (video.hasAttribute("autoplay")) score += 5;
      if (video.hasAttribute("controls")) score += 5;
      if (video.poster) score += 5;
    } catch (error) {
      console.warn("[PiP] Error calculating video score:", error);
    }

    return Math.max(0, score);
  }

  // サイト設定を取得
  static getSiteConfig(url = window.location.href) {
    for (const [domain, config] of Object.entries(PIP_CONFIG.SITE_CONFIGS)) {
      if (url.includes(domain)) {
        return config;
      }
    }
    return null;
  }

  // パフォーマンス監視
  static startPerformanceMonitoring() {
    if (window.pipPerformanceMonitor) return;

    window.pipPerformanceMonitor = {
      startTime: Date.now(),
      operations: [],

      log: function (operation, duration) {
        this.operations.push({
          operation,
          duration,
          timestamp: Date.now(),
        });

        if (this.operations.length > 100) {
          this.operations = this.operations.slice(-50);
        }
      },

      getStats: function () {
        const totalTime = Date.now() - this.startTime;
        const avgDuration =
          this.operations.reduce((sum, op) => sum + op.duration, 0) /
          this.operations.length;

        return {
          totalTime,
          operationCount: this.operations.length,
          averageDuration: avgDuration || 0,
          recentOperations: this.operations.slice(-10),
        };
      },
    };
  }
}

// パフォーマンス監視を開始
PiPUtils.startPerformanceMonitoring();

// PictureInPictureHandlerが既に存在するかチェック
if (typeof window.PictureInPictureHandler === "undefined") {
  class PictureInPictureHandler {
    constructor() {
      this.isDragging = false;
      this.dragStartX = 0;
      this.dragStartY = 0;
    }

    // PiP環境の準備
    createPiPEnvironment() {
      console.log("🛠️ Picture-in-Picture環境を準備中...");

      // Document API の拡張
      if (!document.hasOwnProperty("pictureInPictureEnabled")) {
        Object.defineProperty(document, "pictureInPictureEnabled", {
          value: true,
          writable: false,
        });
      }

      if (!document.hasOwnProperty("pictureInPictureElement")) {
        Object.defineProperty(document, "pictureInPictureElement", {
          value: null,
          writable: true,
        });
      }

      // 既存のrequestPictureInPictureが無い場合のモック実装
      if (!HTMLVideoElement.prototype.requestPictureInPicture) {
        HTMLVideoElement.prototype.requestPictureInPicture = function () {
          console.log("📺 Picture-in-Picture をシミュレーション開始...");

          return new Promise((resolve, reject) => {
            try {
              // disablepictureinpicture属性をチェック
              if (this.hasAttribute("disablepictureinpicture")) {
                console.log(
                  "⚠️ disablepictureinpicture属性が設定されています - 削除します"
                );
                this.removeAttribute("disablepictureinpicture");
              }

              // フローティングウィンドウを作成
              this.createFloatingVideoWindow();

              // モックのPiPウィンドウオブジェクトを返す
              const mockPiPWindow = {
                width: 320,
                height: 180,
                resizeBy: function (x, y) {
                  console.log("PiP resize:", x, y);
                },
                addEventListener: function (type, listener) {
                  console.log("PiP event listener:", type);
                },
              };

              resolve(mockPiPWindow);
            } catch (error) {
              reject(
                new DOMException(
                  "Picture-in-Picture シミュレーション失敗",
                  "NotSupportedError"
                )
              );
            }
          });
        };
      }

      // フローティングビデオウィンドウ作成関数
      HTMLVideoElement.prototype.createFloatingVideoWindow = function () {
        console.log("🎬 フローティングビデオウィンドウを作成中...");

        // 既存のフローティングウィンドウがあれば削除
        const existingWindow = document.getElementById("pip-floating-window");
        if (existingWindow) {
          existingWindow.remove();
        }

        // CSSが読み込まれていない場合、インラインスタイルで作成
        const floatingContainer = document.createElement("div");
        floatingContainer.id = "pip-floating-window";

        // フローティングコンテナのスタイルを設定
        this.applyFloatingContainerStyles(floatingContainer);

        // ビデオクローンを作成
        const videoClone = this.cloneNode(true);
        this.applyVideoCloneStyles(videoClone);

        // 元の動画と同期
        videoClone.currentTime = this.currentTime;
        if (!this.paused) {
          videoClone.play();
        }

        // 閉じるボタンを追加
        const closeButton = this.createCloseButton(floatingContainer);

        // ドラッグ機能を追加
        this.addDragFunctionality(floatingContainer, closeButton);

        // 要素を組み立て
        floatingContainer.appendChild(videoClone);
        floatingContainer.appendChild(closeButton);
        document.body.appendChild(floatingContainer);

        // Document の pictureInPictureElement を設定
        document.pictureInPictureElement = this;

        console.log("✅ フローティングPiPウィンドウが作成されました");
      };
    }

    applyFloatingContainerStyles(container) {
      const hasCSS =
        document.querySelector('link[href*="pip.css"]') ||
        document.querySelector("style[data-pip-styles]");

      if (!hasCSS) {
        container.style.cssText = `
                position: fixed;
                top: 20px;
                right: 20px;
                width: 320px;
                height: 180px;
                background: black;
                border: 2px solid #333;
                border-radius: 8px;
                box-shadow: 0 4px 20px rgba(0,0,0,0.3);
                z-index: 999999;
                cursor: move;
                overflow: hidden;
            `;
      }
    }

    applyVideoCloneStyles(videoClone) {
      const hasCSS =
        document.querySelector('link[href*="pip.css"]') ||
        document.querySelector("style[data-pip-styles]");

      if (!hasCSS) {
        videoClone.style.cssText = `
                width: 100%;
                height: 100%;
                object-fit: contain;
            `;
      }
    }

    createCloseButton(floatingContainer) {
      const closeButton = document.createElement("button");
      closeButton.textContent = "×";
      closeButton.className = "close-button";

      const hasCSS =
        document.querySelector('link[href*="pip.css"]') ||
        document.querySelector("style[data-pip-styles]");

      if (!hasCSS) {
        closeButton.style.cssText = `
                position: absolute;
                top: 5px;
                right: 5px;
                width: 25px;
                height: 25px;
                background: rgba(0,0,0,0.7);
                color: white;
                border: none;
                border-radius: 50%;
                cursor: pointer;
                font-size: 14px;
                z-index: 1000000;
                display: flex;
                align-items: center;
                justify-content: center;
            `;
      }

      const originalVideo = this;
      closeButton.onclick = () => {
        floatingContainer.remove();
        // PiP終了イベントを発火
        const pipExitEvent = new Event("leavepictureinpicture");
        originalVideo.dispatchEvent(pipExitEvent);
        document.pictureInPictureElement = null;
        console.log("🔚 フローティングPiPウィンドウを閉じました");
      };

      return closeButton;
    }

    addDragFunctionality(floatingContainer, dragHandle) {
      const handler = this;
      let isDragging = false;
      let dragStartX = 0;
      let dragStartY = 0;

      // ドラッグハンドル（ヘッダーバーまたは指定した要素）でマウスダウン
      const onMouseDown = (e) => {
        // リサイズハンドルや閉じるボタンなどは除外
        if (e.target.tagName === "BUTTON" || e.target.closest("button")) {
          return;
        }

        isDragging = true;
        dragStartX = e.clientX - floatingContainer.offsetLeft;
        dragStartY = e.clientY - floatingContainer.offsetTop;

        // ドラッグ中の視覚的フィードバック
        floatingContainer.style.transition = "none";
        if (dragHandle && dragHandle.style) {
          dragHandle.style.opacity = "0.9";
        }

        e.preventDefault();
      };

      const onMouseMove = (e) => {
        if (isDragging) {
          const newLeft = e.clientX - dragStartX;
          const newTop = e.clientY - dragStartY;

          // 画面境界チェック
          const maxLeft = window.innerWidth - floatingContainer.offsetWidth;
          const maxTop = window.innerHeight - floatingContainer.offsetHeight;

          floatingContainer.style.left =
            Math.max(0, Math.min(newLeft, maxLeft)) + "px";
          floatingContainer.style.top =
            Math.max(0, Math.min(newTop, maxTop)) + "px";
          floatingContainer.style.right = "auto"; // rightプロパティをリセット
        }
      };

      const onMouseUp = () => {
        if (isDragging) {
          isDragging = false;
          floatingContainer.style.transition =
            "transform 0.2s ease, box-shadow 0.2s ease";
          if (dragHandle && dragHandle.style) {
            dragHandle.style.opacity = "1";
          }
        }
      };

      // ドラッグハンドルが指定されている場合はそれに、そうでなければコンテナ全体にイベントを追加
      if (dragHandle && dragHandle !== floatingContainer) {
        dragHandle.addEventListener("mousedown", onMouseDown);
        dragHandle.style.cursor = "move";
      } else {
        floatingContainer.addEventListener("mousedown", onMouseDown);
      }

      // グローバルなマウスイベント
      document.addEventListener("mousemove", onMouseMove);
      document.addEventListener("mouseup", onMouseUp);

      // クリーンアップ関数を返す（必要に応じて）
      return () => {
        if (dragHandle && dragHandle !== floatingContainer) {
          dragHandle.removeEventListener("mousedown", onMouseDown);
        } else {
          floatingContainer.removeEventListener("mousedown", onMouseDown);
        }
        document.removeEventListener("mousemove", onMouseMove);
        document.removeEventListener("mouseup", onMouseUp);
      };
    }

    // メイン実行関数
    async executePictureInPicture() {
      console.log("=== Picture-in-Picture実行開始 ===");

      // Step 1: 環境を作成
      this.createPiPEnvironment();

      // Step 2: 動画配信サイトかどうかをチェック
      const currentDomain = window.location.hostname.toLowerCase();
      const isVideoStreamingSite = this.detectVideoStreamingSite(currentDomain);

      console.log(`現在のサイト: ${currentDomain}`);
      console.log(`動画配信サイト判定: ${isVideoStreamingSite ? "YES" : "NO"}`);

      // Step 3: 動画配信サイトの場合は強制PiP機能を使用
      if (isVideoStreamingSite) {
        console.log("🎯 動画配信サイト向け強制PiP機能を実行中...");
        try {
          const result = this.forceVideoStreamingPiP();
          if (result) {
            console.log("✅ 動画配信サイト向けPiPが成功しました");
            alert(
              "動画配信サイト向けPicture-in-Picture が開始されました！\n\n" +
                "サイトがPiPを無効化していても、強制的にPiPウィンドウを表示しています。\n" +
                "ドラッグして移動したり、×ボタンで閉じたりできます。"
            );
            return result;
          }
        } catch (error) {
          console.error("❌ 動画配信サイト向けPiP失敗:", error);
        }
      }

      // Step 4: 通常のPiP処理（配信サイトでない場合、または配信サイトでも失敗した場合）
      console.log("🔄 通常のPiP処理を実行中...");

      // ページ内のdisablepictureinpicture属性を削除
      const videos = document.querySelectorAll(
        "video[disablepictureinpicture]"
      );
      videos.forEach((video) => {
        console.log("📺 disablepictureinpicture属性を削除:", video);
        video.removeAttribute("disablepictureinpicture");
      });

      // 動画の準備と実行
      const allVideos = document.querySelectorAll("video");
      console.log("📹 見つかった動画:", allVideos.length + "個");

      if (allVideos.length === 0) {
        alert("このページには動画が見つかりませんでした。");
        return;
      }

      // 最初のビデオを対象に選択
      let targetVideo = allVideos[0];
      for (const video of allVideos) {
        if (!video.paused && video.readyState >= 2) {
          targetVideo = video;
          break;
        }
      }

      console.log("🎯 対象動画を選択:", targetVideo);

      // Picture-in-Picture実行
      try {
        // 動画が一時停止中の場合は再生
        if (targetVideo.paused) {
          console.log("▶️ 動画を再生開始...");
          await targetVideo.play();
        }

        console.log("🔄 Picture-in-Picture をリクエスト中...");
        const pipWindow = await targetVideo.requestPictureInPicture();

        console.log("✅ Picture-in-Picture が開始されました!");
        console.log("📊 PiPウィンドウ:", pipWindow);

        alert(
          "Picture-in-Picture シミュレーションが開始されました！\n\n" +
            "右上にフローティングビデオウィンドウが表示されています。\n" +
            "ドラッグして移動したり、×ボタンで閉じたりできます。"
        );
      } catch (error) {
        console.error("❌ Picture-in-Picture エラー:", error);

        // エラーの場合も強制PiP機能を試行
        console.log("🔄 エラー発生のため強制PiP機能を試行中...");
        try {
          const fallbackResult = this.forceVideoStreamingPiP();
          if (fallbackResult) {
            console.log("✅ フォールバック強制PiPが成功しました");
            alert(
              "通常のPiPは失敗しましたが、代替実装でPiPウィンドウを表示しています。\n\n" +
                "ドラッグして移動したり、×ボタンで閉じたりできます。"
            );
            return fallbackResult;
          }
        } catch (fallbackError) {
          console.error("❌ フォールバック強制PiPも失敗:", fallbackError);
        }

        let errorMessage = "Picture-in-Picture の開始に失敗しました。\n\n";

        if (error.name === "NotSupportedError") {
          errorMessage +=
            "この環境では Picture-in-Picture がサポートされていません。\n";
          errorMessage +=
            "Qt WebEngine の制限により、ネイティブ PiP は利用できませんが、\n";
          errorMessage +=
            "代替実装としてフローティングウィンドウを提供します。";
        } else if (error.name === "NotAllowedError") {
          errorMessage += "Picture-in-Picture の使用が許可されていません。\n";
          errorMessage += "ユーザーの操作が必要です。";
        } else if (error.name === "InvalidStateError") {
          errorMessage += "動画の状態が無効です。\n";
          errorMessage += "動画を再生してから再試行してください。";
        } else {
          errorMessage += "エラー詳細: " + error.message;
        }

        alert(errorMessage);
      }

      console.log("=== PiP実装完了 ===");
    }

    // PiPサポート検出
    detectPiPSupport() {
      console.log("Picture-in-Picture サポート状況:");
      console.log(
        "- pictureInPictureEnabled in document:",
        "pictureInPictureEnabled" in document
      );
      console.log(
        "- document.pictureInPictureEnabled:",
        document.pictureInPictureEnabled
      );
      console.log(
        "- HTMLVideoElement.prototype.requestPictureInPicture:",
        typeof HTMLVideoElement.prototype.requestPictureInPicture
      );

      const videos = document.querySelectorAll("video");
      console.log("動画要素の数:", videos.length);

      videos.forEach((video, index) => {
        console.log(`動画 ${index + 1}:`, {
          src: video.src || video.currentSrc,
          duration: video.duration,
          paused: video.paused,
          readyState: video.readyState,
          disablePiP: video.hasAttribute("disablepictureinpicture"),
        });
      });
    }

    // Picture-in-Picture終了
    async exitPictureInPicture() {
      try {
        if (document.pictureInPictureElement) {
          await document.exitPictureInPicture();
          console.log("Picture-in-Picture モードを終了しました");
        } else {
          console.log("現在Picture-in-Pictureモードではありません");
        }
      } catch (error) {
        console.error("PiP終了エラー:", error);
      }
    }

    // すべての動画をPicture-in-Picture対応にする
    enablePiPForAllVideos() {
      console.log("すべての動画をPicture-in-Picture対応に設定中...");

      // 全ての動画要素をPiP対応にする関数
      const enablePiPForAllVideos = () => {
        const videos = document.querySelectorAll("video");
        console.log("見つかった動画要素の数:", videos.length);

        videos.forEach((video, index) => {
          console.log(`動画 ${index + 1} をPiP対応に設定中...`);

          // disablepictureinpicture属性を削除（大文字小文字の全パターン）
          [
            "disablepictureinpicture",
            "disablePictureInPicture",
            "disable-picture-in-picture",
          ].forEach((attr) => {
            if (video.hasAttribute(attr)) {
              video.removeAttribute(attr);
              console.log(`${attr} 属性を削除しました`);
            }
          });

          // disablePictureInPictureプロパティを強制的にfalseに設定
          try {
            Object.defineProperty(video, "disablePictureInPicture", {
              value: false,
              writable: false,
              configurable: true,
            });
            console.log(
              `動画 ${
                index + 1
              }: disablePictureInPicture プロパティを false に固定しました`
            );
          } catch (e) {
            video.disablePictureInPicture = false;
            console.log(
              `動画 ${
                index + 1
              }: disablePictureInPicture プロパティを false に設定しました`
            );
          }

          // 追加のセキュリティ対策：setAttribute を監視してdisable属性の再設定を防ぐ
          const originalSetAttribute = video.setAttribute;
          video.setAttribute = function (name, value) {
            if (
              name.toLowerCase().includes("disablepictureinpicture") ||
              name.toLowerCase().includes("disable-picture-in-picture")
            ) {
              console.log(`試行された ${name} 属性の設定をブロックしました`);
              return;
            }
            return originalSetAttribute.call(this, name, value);
          };
        });

        return videos.length;
      };

      // MutationObserverで動的に追加される動画も監視
      const setupVideoObserver = () => {
        const observer = new MutationObserver(function (mutations) {
          mutations.forEach(function (mutation) {
            mutation.addedNodes.forEach(function (node) {
              if (node.nodeType === Node.ELEMENT_NODE) {
                // 追加されたノードが動画要素か、動画要素を含むか確認
                const videos =
                  node.tagName === "VIDEO"
                    ? [node]
                    : node.querySelectorAll("video");
                videos.forEach((video) => {
                  console.log("新しい動画要素をPiP対応に設定中...");

                  // 同様の処理を適用
                  [
                    "disablepictureinpicture",
                    "disablePictureInPicture",
                    "disable-picture-in-picture",
                  ].forEach((attr) => {
                    if (video.hasAttribute(attr)) {
                      video.removeAttribute(attr);
                    }
                  });

                  try {
                    Object.defineProperty(video, "disablePictureInPicture", {
                      value: false,
                      writable: false,
                      configurable: true,
                    });
                  } catch (e) {
                    video.disablePictureInPicture = false;
                  }

                  // setAttribute監視も設定
                  const originalSetAttribute = video.setAttribute;
                  video.setAttribute = function (name, value) {
                    if (
                      name.toLowerCase().includes("disablepictureinpicture") ||
                      name.toLowerCase().includes("disable-picture-in-picture")
                    ) {
                      console.log(
                        `試行された ${name} 属性の設定をブロックしました`
                      );
                      return;
                    }
                    return originalSetAttribute.call(this, name, value);
                  };
                });
              }
            });
          });
        });

        observer.observe(document.body, {
          childList: true,
          subtree: true,
          attributes: true,
          attributeFilter: [
            "disablepictureinpicture",
            "disable-picture-in-picture",
          ],
        });

        console.log("動画要素の監視を開始しました");
        return observer;
      };

      // 即座に実行
      const videoCount = enablePiPForAllVideos();

      // 動的な動画の監視を開始
      const observer = setupVideoObserver();

      // HTMLVideoElementのプロトタイプレベルでも対策
      try {
        Object.defineProperty(
          HTMLVideoElement.prototype,
          "disablePictureInPicture",
          {
            value: false,
            writable: false,
            configurable: true,
          }
        );
        console.log(
          "HTMLVideoElement.prototype.disablePictureInPicture を false に固定しました"
        );
      } catch (e) {
        console.log("プロトタイプレベルの設定に失敗:", e);
      }

      console.log(
        `${videoCount} 個の動画をPicture-in-Picture対応に設定しました`
      );

      // observer を返して外部からアクセス可能にする
      window._pipObserver = observer;

      return videoCount;
    }

    // 独自PiP機能: 任意の要素をPiPにする
    createElementPiP() {
      console.log("=== 要素選択型Picture-in-Picture開始 ===");

      // 要素選択モードの開始
      const overlay = document.createElement("div");
      overlay.id = "pip-element-selector-overlay";
      overlay.style.cssText = `
            position: fixed;
            top: 0;
            left: 0;
            right: 0;
            bottom: 0;
            background: rgba(0, 0, 0, 0.5);
            z-index: 999998;
            cursor: crosshair;
        `;

      const instructions = document.createElement("div");
      instructions.style.cssText = `
            position: fixed;
            top: 20px;
            left: 50%;
            transform: translateX(-50%);
            background: rgba(0, 0, 0, 0.9);
            color: white;
            padding: 20px 30px;
            border-radius: 12px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            font-size: 14px;
            z-index: 999999;
            text-align: center;
            box-shadow: 0 8px 32px rgba(0, 0, 0, 0.3);
            border: 1px solid rgba(255, 255, 255, 0.2);
        `;
      instructions.innerHTML = `
            <div style="font-weight: bold; margin-bottom: 8px; font-size: 16px;">🎯 要素選択モード</div>
            <div style="margin-bottom: 8px;">PiPにしたい要素をクリックしてください</div>
            <div style="font-size: 12px; opacity: 0.8;">ESCキーでキャンセル | 任意の要素を浮動ウィンドウ化</div>
        `;

      let highlightedElement = null;
      const originalOutlines = new Map();

      const highlightElement = (element) => {
        if (highlightedElement && highlightedElement !== element) {
          // 前の要素のハイライトを削除
          const originalOutline = originalOutlines.get(highlightedElement);
          if (originalOutline !== undefined) {
            highlightedElement.style.outline = originalOutline;
          } else {
            highlightedElement.style.removeProperty("outline");
          }
        }

        highlightedElement = element;
        if (element && element !== overlay && element !== instructions) {
          // 現在のoutlineを保存
          originalOutlines.set(element, element.style.outline || "");
          element.style.outline = "3px solid #007ACC";
          element.style.outlineOffset = "2px";

          // 要素情報をツールチップとして表示
          const elementInfo = this.getElementInfo(element);
          instructions.innerHTML = `
            <div style="font-weight: bold; margin-bottom: 8px; font-size: 16px;">🎯 要素選択モード</div>
            <div style="margin-bottom: 8px;">選択中: ${elementInfo}</div>
            <div style="font-size: 12px; opacity: 0.8;">クリックでPiP化 | ESCでキャンセル</div>
        `;
        }
      };

      const cleanup = () => {
        document.removeEventListener("mouseover", onMouseOver);
        document.removeEventListener("click", onClick);
        document.removeEventListener("keydown", onKeyDown);
        if (highlightedElement) {
          const originalOutline = originalOutlines.get(highlightedElement);
          if (originalOutline !== undefined) {
            highlightedElement.style.outline = originalOutline;
          } else {
            highlightedElement.style.removeProperty("outline");
          }
        }
        if (overlay.parentNode) {
          overlay.remove();
        }
      };

      const onMouseOver = (e) => {
        e.stopPropagation();
        highlightElement(e.target);
      };

      const onClick = (e) => {
        e.preventDefault();
        e.stopPropagation();

        if (e.target === overlay || e.target === instructions) {
          return;
        }

        const selectedElement = e.target;
        cleanup();
        this.createElementPiPWindow(selectedElement);
      };

      const onKeyDown = (e) => {
        if (e.key === "Escape") {
          cleanup();
        }
      };

      document.addEventListener("mouseover", onMouseOver);
      document.addEventListener("click", onClick);
      document.addEventListener("keydown", onKeyDown);

      overlay.appendChild(instructions);
      document.body.appendChild(overlay);
    }

    // 要素情報を取得するヘルパーメソッド
    getElementInfo(element) {
      const tagName = element.tagName.toLowerCase();
      const className = element.className
        ? `.${element.className.split(" ").join(".")}`
        : "";
      const id = element.id ? `#${element.id}` : "";
      const textContent = element.textContent
        ? element.textContent.substring(0, 30) + "..."
        : "";

      if (element.tagName === "IMG") {
        return `画像 (${
          element.alt || element.src.split("/").pop() || "image"
        })`;
      } else if (element.tagName === "VIDEO") {
        return `動画 (${element.title || "video"})`;
      } else if (element.tagName === "IFRAME") {
        return `フレーム (${element.title || element.src || "iframe"})`;
      } else if (textContent) {
        return `${tagName}${id}${className} - "${textContent}"`;
      } else {
        return `${tagName}${id}${className}`;
      }
    }

    // 選択された要素のPiPウィンドウを作成
    createElementPiPWindow(element) {
      console.log("🎯 選択された要素でPiPウィンドウを作成:", element);

      const pipId = "pip-element-" + Date.now();
      const existingPip = document.getElementById(pipId);
      if (existingPip) {
        existingPip.remove();
      }

      // PiPコンテナを作成
      const pipContainer = document.createElement("div");
      pipContainer.id = pipId;
      pipContainer.className = "pip-element-window";

      // 要素のクローンを作成
      const elementClone = element.cloneNode(true);

      // 元の要素のスタイルを取得してクローンに適用
      const computedStyle = window.getComputedStyle(element);
      const originalRect = element.getBoundingClientRect();
      const elementInfo = this.getElementInfo(element);

      // PiPコンテナのスタイル設定（より洗練されたデザイン）
      const pipWidth = Math.min(450, originalRect.width);
      const pipHeight = Math.min(350, originalRect.height + 35); // ヘッダー分の高さを追加

      pipContainer.style.cssText = `
            position: fixed;
            top: 50px;
            right: 50px;
            width: ${pipWidth}px;
            height: ${pipHeight}px;
            background: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0, 0, 0, 0.15), 0 4px 12px rgba(0, 0, 0, 0.1);
            z-index: 999999;
            overflow: hidden;
            resize: both;
            min-width: 220px;
            min-height: 180px;
            backdrop-filter: blur(10px);
            transition: transform 0.2s ease, box-shadow 0.2s ease;
        `;

      // ヘッダーバーを作成
      const headerBar = document.createElement("div");
      headerBar.className = "pip-header-bar";
      headerBar.style.cssText = `
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 35px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            display: flex;
            align-items: center;
            padding: 0 12px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            font-size: 12px;
            font-weight: 500;
            color: white;
            cursor: move;
            user-select: none;
            border-radius: 12px 12px 0 0;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        `;

      // ヘッダータイトルを作成
      const headerTitle = document.createElement("div");
      headerTitle.style.cssText = `
            flex: 1;
            font-size: 12px;
            font-weight: 600;
            opacity: 0.95;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        `;
      headerTitle.textContent = `📋 ${elementInfo}`;

      // ウィンドウコントロールボタン群を作成
      const windowControls = document.createElement("div");
      windowControls.style.cssText = `
            display: flex;
            gap: 6px;
            align-items: center;
        `;

      // 最小化ボタン
      const minimizeButton = document.createElement("button");
      minimizeButton.innerHTML = "−";
      minimizeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #ffcc02;
            color: #996600;
            font-size: 10px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease;
        `;

      // 最大化ボタン
      const maximizeButton = document.createElement("button");
      maximizeButton.innerHTML = "□";
      maximizeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #00ca56;
            color: #006629;
            font-size: 8px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease;
        `;

      // 閉じるボタン（洗練されたデザイン）
      const closeButton = document.createElement("button");
      closeButton.innerHTML = "×";
      closeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #ff5f56;
            color: white;
            font-size: 12px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease, transform 0.1s ease;
        `;

      // ボタンのホバー効果
      [minimizeButton, maximizeButton, closeButton].forEach((button) => {
        button.addEventListener("mouseenter", () => {
          button.style.opacity = "0.8";
          button.style.transform = "scale(1.1)";
        });
        button.addEventListener("mouseleave", () => {
          button.style.opacity = "1";
          button.style.transform = "scale(1)";
        });
      });

      // クローン要素のスタイル調整（ヘッダー分を考慮）
      elementClone.style.cssText = `
            width: 100%;
            height: calc(100% - 35px);
            margin: 0;
            margin-top: 35px;
            padding: 15px;
            box-sizing: border-box;
            overflow: auto;
            background: ${computedStyle.backgroundColor || "#fafafa"};
            font-family: ${computedStyle.fontFamily};
            font-size: ${computedStyle.fontSize};
            color: ${computedStyle.color};
            border-radius: 0 0 12px 12px;
        `;

      // ボタンイベントハンドラー
      minimizeButton.onclick = (e) => {
        e.stopPropagation();
        pipContainer.style.transform =
          pipContainer.style.transform === "scale(0.3)"
            ? "scale(1)"
            : "scale(0.3)";
      };

      maximizeButton.onclick = (e) => {
        e.stopPropagation();
        const isMaximized = pipContainer.style.width === "80vw";
        if (isMaximized) {
          pipContainer.style.width = pipWidth + "px";
          pipContainer.style.height = pipHeight + "px";
          pipContainer.style.top = "50px";
          pipContainer.style.left = "auto";
          pipContainer.style.right = "50px";
        } else {
          pipContainer.style.width = "80vw";
          pipContainer.style.height = "70vh";
          pipContainer.style.top = "10vh";
          pipContainer.style.left = "10vw";
          pipContainer.style.right = "auto";
        }
      };

      closeButton.onclick = (e) => {
        e.stopPropagation();
        pipContainer.style.transition =
          "transform 0.2s ease, opacity 0.2s ease";
        pipContainer.style.transform = "scale(0.8)";
        pipContainer.style.opacity = "0";
        setTimeout(() => {
          pipContainer.remove();
          console.log("🔚 要素PiPウィンドウを閉じました");
        }, 200);
      };

      // ウィンドウコントロールを組み立て
      windowControls.appendChild(minimizeButton);
      windowControls.appendChild(maximizeButton);
      windowControls.appendChild(closeButton);

      // ヘッダーバーを組み立て
      headerBar.appendChild(headerTitle);
      headerBar.appendChild(windowControls);

      // ドラッグ機能を追加（ヘッダーバーのみでドラッグ可能）
      this.addDragFunctionality(pipContainer, headerBar);

      // ウィンドウのホバー効果
      pipContainer.addEventListener("mouseenter", () => {
        pipContainer.style.boxShadow =
          "0 15px 50px rgba(0, 0, 0, 0.2), 0 6px 18px rgba(0, 0, 0, 0.15)";
        pipContainer.style.transform = "translateY(-2px)";
      });

      pipContainer.addEventListener("mouseleave", () => {
        pipContainer.style.boxShadow =
          "0 10px 40px rgba(0, 0, 0, 0.15), 0 4px 12px rgba(0, 0, 0, 0.1)";
        pipContainer.style.transform = "translateY(0)";
      });

      // 要素を組み立て
      pipContainer.appendChild(headerBar);
      pipContainer.appendChild(elementClone);
      document.body.appendChild(pipContainer);

      console.log("✅ 要素PiPウィンドウが作成されました");
    }

    // ページ全体のPiP
    createPagePiP() {
      console.log("=== ページ全体Picture-in-Picture開始 ===");

      const pipId = "pip-page-" + Date.now();
      const existingPip = document.getElementById(pipId);
      if (existingPip) {
        existingPip.remove();
      }

      // PiPコンテナを作成
      const pipContainer = document.createElement("div");
      pipContainer.id = pipId;
      pipContainer.className = "pip-page-window";

      pipContainer.style.cssText = `
            position: fixed;
            top: 20px;
            left: 20px;
            width: 500px;
            height: 435px;
            background: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0, 0, 0, 0.15), 0 4px 12px rgba(0, 0, 0, 0.1);
            z-index: 999999;
            overflow: hidden;
            resize: both;
            min-width: 300px;
            min-height: 235px;
            backdrop-filter: blur(10px);
            transition: transform 0.2s ease, box-shadow 0.2s ease;
        `;

      // ヘッダーバーを作成（macOSライク）
      const headerBar = document.createElement("div");
      headerBar.className = "pip-header-bar";
      headerBar.style.cssText = `
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 35px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            display: flex;
            align-items: center;
            padding: 0 12px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            font-size: 12px;
            font-weight: 500;
            color: white;
            cursor: move;
            user-select: none;
            border-radius: 12px 12px 0 0;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        `;

      // ヘッダータイトルを作成
      const headerTitle = document.createElement("div");
      headerTitle.style.cssText = `
            flex: 1;
            font-size: 12px;
            font-weight: 600;
            opacity: 0.95;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        `;
      headerTitle.textContent = `📄 ${document.title || window.location.href}`;

      // ウィンドウコントロールボタン群を作成
      const windowControls = document.createElement("div");
      windowControls.style.cssText = `
            display: flex;
            gap: 6px;
            align-items: center;
        `;

      // 最小化ボタン
      const minimizeButton = document.createElement("button");
      minimizeButton.innerHTML = "−";
      minimizeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #ffcc02;
            color: #996600;
            font-size: 10px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease, transform 0.1s ease;
        `;

      // 最大化ボタン
      const maximizeButton = document.createElement("button");
      maximizeButton.innerHTML = "□";
      maximizeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #00ca56;
            color: #006629;
            font-size: 8px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease, transform 0.1s ease;
        `;

      // 閉じるボタン
      const closeButton = document.createElement("button");
      closeButton.innerHTML = "×";
      closeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #ff5f56;
            color: white;
            font-size: 12px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease, transform 0.1s ease;
        `;

      // ボタンのホバー効果
      [minimizeButton, maximizeButton, closeButton].forEach((button) => {
        button.addEventListener("mouseenter", () => {
          button.style.opacity = "0.8";
          button.style.transform = "scale(1.1)";
        });
        button.addEventListener("mouseleave", () => {
          button.style.opacity = "1";
          button.style.transform = "scale(1)";
        });
      });

      // ボタンイベントハンドラー
      minimizeButton.onclick = (e) => {
        e.stopPropagation();
        pipContainer.style.transform =
          pipContainer.style.transform === "scale(0.3)"
            ? "scale(1)"
            : "scale(0.3)";
      };

      maximizeButton.onclick = (e) => {
        e.stopPropagation();
        const isMaximized = pipContainer.style.width === "80vw";
        if (isMaximized) {
          pipContainer.style.width = "500px";
          pipContainer.style.height = "435px";
          pipContainer.style.top = "20px";
          pipContainer.style.left = "20px";
        } else {
          pipContainer.style.width = "80vw";
          pipContainer.style.height = "70vh";
          pipContainer.style.top = "10vh";
          pipContainer.style.left = "10vw";
        }
      };

      closeButton.onclick = (e) => {
        e.stopPropagation();
        pipContainer.style.transition =
          "transform 0.2s ease, opacity 0.2s ease";
        pipContainer.style.transform = "scale(0.8)";
        pipContainer.style.opacity = "0";
        setTimeout(() => {
          pipContainer.remove();
          console.log("🔚 ページPiPウィンドウを閉じました");
        }, 200);
      };

      // ドラッグ機能を追加（ヘッダーバーのみでドラッグ可能）
      this.addDragFunctionality(pipContainer, headerBar);

      // ウィンドウのホバー効果
      pipContainer.addEventListener("mouseenter", () => {
        pipContainer.style.boxShadow =
          "0 15px 50px rgba(0, 0, 0, 0.2), 0 6px 18px rgba(0, 0, 0, 0.15)";
        pipContainer.style.transform = "translateY(-2px)";
      });

      pipContainer.addEventListener("mouseleave", () => {
        pipContainer.style.boxShadow =
          "0 10px 40px rgba(0, 0, 0, 0.15), 0 4px 12px rgba(0, 0, 0, 0.1)";
        pipContainer.style.transform = "translateY(0)";
      });

      // 現在のページの縮小版を iframe に設定
      const iframe = document.createElement("iframe");
      iframe.style.cssText = `
            width: 100%;
            height: calc(100% - 35px);
            border: none;
            margin-top: 35px;
            transform: scale(0.5);
            transform-origin: top left;
            border-radius: 0 0 12px 12px;
        `;

      iframe.src = window.location.href;

      // 要素を組み立て
      pipContainer.appendChild(headerBar);
      pipContainer.appendChild(iframe);
      document.body.appendChild(pipContainer);

      console.log("✅ ページPiPウィンドウが作成されました");
    }

    // スクリーンショット風PiP
    createScreenshotPiP() {
      console.log("=== スクリーンショット型Picture-in-Picture開始 ===");

      const pipId = "pip-screenshot-" + Date.now();
      const existingPip = document.getElementById(pipId);
      if (existingPip) {
        existingPip.remove();
      }

      // PiPコンテナを作成
      const pipContainer = document.createElement("div");
      pipContainer.id = pipId;
      pipContainer.className = "pip-screenshot-window";

      pipContainer.style.cssText = `
            position: fixed;
            top: 100px;
            right: 20px;
            width: 400px;
            height: 335px;
            background: #ffffff;
            border: 1px solid #e0e0e0;
            border-radius: 12px;
            box-shadow: 0 10px 40px rgba(0, 0, 0, 0.15), 0 4px 12px rgba(0, 0, 0, 0.1);
            z-index: 999999;
            overflow: hidden;
            resize: both;
            min-width: 200px;
            min-height: 185px;
            backdrop-filter: blur(10px);
            transition: transform 0.2s ease, box-shadow 0.2s ease;
        `;

      // ヘッダーバーを作成（macOSライク）
      const headerBar = document.createElement("div");
      headerBar.className = "pip-header-bar";
      headerBar.style.cssText = `
            position: absolute;
            top: 0;
            left: 0;
            right: 0;
            height: 35px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            display: flex;
            align-items: center;
            padding: 0 12px;
            font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
            font-size: 12px;
            font-weight: 500;
            color: white;
            cursor: move;
            user-select: none;
            border-radius: 12px 12px 0 0;
            box-shadow: 0 2px 8px rgba(0, 0, 0, 0.1);
        `;

      // ヘッダータイトルを作成
      const headerTitle = document.createElement("div");
      headerTitle.style.cssText = `
            flex: 1;
            font-size: 12px;
            font-weight: 600;
            opacity: 0.95;
            white-space: nowrap;
            overflow: hidden;
            text-overflow: ellipsis;
        `;
      headerTitle.textContent = `📸 スナップショット - ${
        document.title || window.location.href
      }`;

      // ウィンドウコントロールボタン群を作成
      const windowControls = document.createElement("div");
      windowControls.style.cssText = `
            display: flex;
            gap: 6px;
            align-items: center;
        `;

      // 最小化ボタン
      const minimizeButton = document.createElement("button");
      minimizeButton.innerHTML = "−";
      minimizeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #ffcc02;
            color: #996600;
            font-size: 10px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease, transform 0.1s ease;
        `;

      // 最大化ボタン
      const maximizeButton = document.createElement("button");
      maximizeButton.innerHTML = "□";
      maximizeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #00ca56;
            color: #006629;
            font-size: 8px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease, transform 0.1s ease;
        `;

      // 閉じるボタン
      const closeButton = document.createElement("button");
      closeButton.innerHTML = "×";
      closeButton.style.cssText = `
            width: 16px;
            height: 16px;
            border-radius: 50%;
            border: none;
            background: #ff5f56;
            color: white;
            font-size: 12px;
            font-weight: bold;
            cursor: pointer;
            display: flex;
            align-items: center;
            justify-content: center;
            transition: opacity 0.2s ease, transform 0.1s ease;
        `;

      // ボタンのホバー効果
      [minimizeButton, maximizeButton, closeButton].forEach((button) => {
        button.addEventListener("mouseenter", () => {
          button.style.opacity = "0.8";
          button.style.transform = "scale(1.1)";
        });
        button.addEventListener("mouseleave", () => {
          button.style.opacity = "1";
          button.style.transform = "scale(1)";
        });
      });

      // ボタンイベントハンドラー
      minimizeButton.onclick = (e) => {
        e.stopPropagation();
        pipContainer.style.transform =
          pipContainer.style.transform === "scale(0.3)"
            ? "scale(1)"
            : "scale(0.3)";
      };

      maximizeButton.onclick = (e) => {
        e.stopPropagation();
        const isMaximized = pipContainer.style.width === "80vw";
        if (isMaximized) {
          pipContainer.style.width = "400px";
          pipContainer.style.height = "335px";
          pipContainer.style.top = "100px";
          pipContainer.style.left = "auto";
          pipContainer.style.right = "20px";
        } else {
          pipContainer.style.width = "80vw";
          pipContainer.style.height = "70vh";
          pipContainer.style.top = "10vh";
          pipContainer.style.left = "10vw";
          pipContainer.style.right = "auto";
        }
      };

      closeButton.onclick = (e) => {
        e.stopPropagation();
        pipContainer.style.transition =
          "transform 0.2s ease, opacity 0.2s ease";
        pipContainer.style.transform = "scale(0.8)";
        pipContainer.style.opacity = "0";
        setTimeout(() => {
          pipContainer.remove();
          console.log("🔚 スクリーンショットPiPウィンドウを閉じました");
        }, 200);
      };

      // ドラッグ機能を追加（ヘッダーバーのみでドラッグ可能）
      this.addDragFunctionality(pipContainer, headerBar);

      // ウィンドウのホバー効果
      pipContainer.addEventListener("mouseenter", () => {
        pipContainer.style.boxShadow =
          "0 15px 50px rgba(0, 0, 0, 0.2), 0 6px 18px rgba(0, 0, 0, 0.15)";
        pipContainer.style.transform = "translateY(-2px)";
      });

      pipContainer.addEventListener("mouseleave", () => {
        pipContainer.style.boxShadow =
          "0 10px 40px rgba(0, 0, 0, 0.15), 0 4px 12px rgba(0, 0, 0, 0.1)";
        pipContainer.style.transform = "translateY(0)";
      });

      // 要素を組み立て
      pipContainer.appendChild(headerBar);
      pipContainer.appendChild(captureArea);
      document.body.appendChild(pipContainer);

      console.log("✅ スクリーンショットPiPウィンドウが作成されました");
    }

    // 動画配信サイト向け強制PiP機能
    forceVideoStreamingPiP() {
      console.log("=== 動画配信サイト向け強制PiP開始 ===");

      // 動画配信サイトの検出
      const currentDomain = window.location.hostname.toLowerCase();
      const isVideoStreamingSite = this.detectVideoStreamingSite(currentDomain);

      console.log(`現在のサイト: ${currentDomain}`);
      console.log(`動画配信サイト判定: ${isVideoStreamingSite ? "YES" : "NO"}`);

      // 1. サイト固有の強化された動画検出を試行
      const siteSpecificVideo = this.tryStreamingSiteSpecificPiP(currentDomain);
      if (siteSpecificVideo) {
        console.log("✅ サイト固有動画検出でPiP成功");
        return siteSpecificVideo;
      }

      // 2. 強化されたカスタムプレイヤーの検出
      const customPlayer = this.tryEnhancedCustomPlayerPiP();
      if (customPlayer) {
        console.log("✅ 強化カスタムプレイヤーでPiP成功");
        return customPlayer;
      }

      // 3. 通常のVideo要素でPiPを試行（改良版）
      const normalVideo = this.tryEnhancedNormalVideoPiP();
      if (normalVideo) {
        console.log("✅ 強化通常Video要素でPiP成功");
        return normalVideo;
      }

      // 4. iframe内動画の検出とPiP
      const iframeVideo = this.tryIframeVideoPiP();
      if (iframeVideo) {
        console.log("✅ iframe内Video要素でPiP成功");
        return iframeVideo;
      }

      // 5. フレームキャプチャによる疑似PiP
      const frameCapture = this.tryFrameCapturePiP();
      if (frameCapture) {
        console.log("✅ フレームキャプチャによる疑似PiP成功");
        return frameCapture;
      }

      console.log("❌ すべてのPiP手法が失敗しました");

      // 6. 最後の手段：動画領域を自動検出してキャプチャ
      return this.tryVideoAreaCapture();
    }

    // 動画配信サイトの検出
    detectVideoStreamingSite(domain) {
      const streamingSites = [
        "youtube.com",
        "youtu.be",
        "tver.jp",
        "abema.tv",
        "netflix.com",
        "amazonprime.com",
        "hulu.jp",
        "dazn.com",
        "paravi.jp",
        "fod.fujitv.co.jp",
        "gyao.yahoo.co.jp",
        "niconico.jp",
        "twitch.tv",
        "bilibili.com",
        "dailymotion.com",
        "vimeo.com",
      ];

      return streamingSites.some((site) => domain.includes(site));
    }

    // ストリーミングサイト固有の動画検出
    tryStreamingSiteSpecificPiP(domain) {
      console.log("🎯 サイト固有の動画検出を開始:", domain);

      let selectors = [];
      let waitForLoad = false;
      let maxWaitTime = 5000; // 5秒

      if (domain.includes("youtube.com") || domain.includes("youtu.be")) {
        console.log("🔴 YouTube特化検出を実行");
        selectors = [
          "ytd-app video",
          'video[src*="googlevideo"]',
          ".ytp-video-container video",
          "#movie_player video",
          ".html5-video-player video",
          "video.video-stream",
          'video[class*="video"]',
          ".ytp-html5-video",
          "video[autoplay]",
          'video:not([width="0"]):not([height="0"])',
          "ytd-player video",
          ".ytd-player-container video",
          "video[poster]",
          "video[controls]",
          "video[preload]",
          ".player-container video",
          "video.html5-main-video",
          'div[id*="player"] video',
        ];
        waitForLoad = true;
        maxWaitTime = 8000; // YouTubeは読み込みが遅い場合があるので延長
      } else if (domain.includes("tver.jp")) {
        console.log("📺 TVar特化検出を実行");
        selectors = [
          ".tver-player video",
          "div[class*='Player'] video",
          ".media-player video",
          "video[class*='player']",
          ".video-container video",
          "video.tver-video",
          ".player-wrapper video",
          "video[autoplay]",
          "video:not([width='0']):not([height='0'])",
          ".video-js video",
          "video[poster]",
          "video[controls]",
          "video[preload]",
          "div[id*='player'] video",
          ".jwplayer video",
          "video.jw-video",
          "video[data-*]",
        ];
        waitForLoad = true;
        maxWaitTime = 10000; // TVerも読み込みが遅い場合があるので延長
      } else if (domain.includes("abema.tv")) {
        console.log("🌟 ABEMA特化検出を実行");
        selectors = [
          ".com-vod-VODPlayer video",
          ".com-tv-TVPlayer video",
          "video[src*='abema']",
          ".abema-player video",
          ".video-player video",
        ];
        waitForLoad = true;
      } else if (domain.includes("netflix.com")) {
        console.log("🎬 Netflix特化検出を実行");
        selectors = [
          ".VideoContainer video",
          ".NFPlayer video",
          "video[src*='netflix']",
          "video[src*='nflx']",
          ".watch-video video",
        ];
        waitForLoad = true;
      } else if (domain.includes("twitch.tv")) {
        console.log("🟣 Twitch特化検出を実行");
        selectors = [
          ".video-player video",
          ".player-video video",
          "video[src*='twitch']",
          ".tw-video video",
        ];
        waitForLoad = true;
      }

      return this.findVideoWithSelectors(selectors, waitForLoad, maxWaitTime);
    }

    // セレクターを使用して動画を検索（待機機能付き）
    async findVideoWithSelectors(
      selectors,
      waitForLoad = false,
      maxWaitTime = 5000
    ) {
      console.log("🔍 セレクターによる動画検索開始:", selectors);

      const findVideo = () => {
        for (let selector of selectors) {
          try {
            const videos = Array.from(document.querySelectorAll(selector));
            console.log(
              `セレクター "${selector}" で ${videos.length} 個の要素を発見`
            );

            for (let video of videos) {
              if (!video || video.tagName !== "VIDEO") continue;

              const rect = video.getBoundingClientRect();
              console.log(`動画要素チェック:`, {
                selector: selector,
                src: video.src || video.currentSrc,
                width: rect.width,
                height: rect.height,
                readyState: video.readyState,
                paused: video.paused,
              });

              // サイズチェック（より柔軟に）
              if (rect.width >= 200 && rect.height >= 100) {
                // 準備状態をチェック（より柔軟に）
                if (video.readyState >= 1) {
                  // HAVE_METADATA以上
                  console.log("✅ 適切な動画要素を発見:", selector);

                  // disablepictureinpicture属性を強制削除
                  this.forceRemoveDisablePiP(video);

                  // フローティングウィンドウを作成
                  this.createForceVideoFloatingWindow(video);
                  return video;
                }
              }
            }
          } catch (error) {
            console.log(`セレクター "${selector}" でエラー:`, error);
          }
        }
        return null;
      };

      // 即座に検索を試行
      let result = findVideo();
      if (result) return result;

      // 待機が必要な場合は、動画の読み込みを待つ
      if (waitForLoad) {
        console.log(
          "⏳ 動画の読み込みを待機中... (最大 " + maxWaitTime / 1000 + " 秒)"
        );

        const startTime = Date.now();
        const checkInterval = 500; // 500msごとにチェック

        return new Promise((resolve) => {
          const intervalId = setInterval(() => {
            const currentTime = Date.now();

            if (currentTime - startTime > maxWaitTime) {
              console.log("⏰ 動画検索のタイムアウト");
              clearInterval(intervalId);
              resolve(null);
              return;
            }

            const video = findVideo();
            if (video) {
              console.log("✅ 待機後に動画要素を発見");
              clearInterval(intervalId);
              resolve(video);
            }
          }, checkInterval);
        });
      }

      return null;
    }

    // 動画要素の品質を評価する関数
    evaluateVideoQuality(video) {
      if (!video || video.tagName !== "VIDEO") return -1;

      let score = 0;
      const rect = video.getBoundingClientRect();

      // サイズによるスコア（大きいほど良い）
      const area = rect.width * rect.height;
      if (area > 500000) score += 10; // 大型動画
      else if (area > 200000) score += 7; // 中型動画
      else if (area > 50000) score += 4; // 小型動画
      else if (area > 10000) score += 2; // 最小限動画
      else return -1; // 小さすぎる

      // readyStateによるスコア
      if (video.readyState >= 3) score += 5; // 充分なデータ
      else if (video.readyState >= 2) score += 3; // 現在のフレーム
      else if (video.readyState >= 1) score += 1; // メタデータ
      else return -1; // データ不足

      // 再生状態によるスコア
      if (!video.paused) score += 3; // 再生中
      if (video.currentTime > 0) score += 2; // 開始済み
      if (video.duration > 0) score += 1; // 持続時間あり

      // ソースの存在によるスコア
      if (video.src || video.currentSrc) score += 2;

      // 可視性によるスコア
      const style = window.getComputedStyle(video);
      if (style.display !== "none" && style.visibility !== "hidden") {
        score += 2;
      }
      if (style.opacity !== "0") score += 1;

      // 特定の属性による減点
      if (video.hasAttribute("disablepictureinpicture")) score -= 1;
      if (video.muted) score -= 1;

      // サイト固有のボーナス
      const domain = window.location.hostname.toLowerCase();
      if (domain.includes("youtube.com") || domain.includes("youtu.be")) {
        if (
          video.classList.contains("video-stream") ||
          video.classList.contains("html5-main-video")
        ) {
          score += 5; // YouTube特化ボーナス
        }
      } else if (domain.includes("tver.jp")) {
        if (video.closest(".player") || video.closest(".video-player")) {
          score += 5; // TVar特化ボーナス
        }
      }

      console.log(
        `動画品質評価: スコア${score}, サイズ${rect.width}x${rect.height}, readyState${video.readyState}`
      );
      return score;
    }

    // 強化されたカスタムプレイヤーの検出
    tryEnhancedCustomPlayerPiP() {
      console.log("🎛️ 強化カスタムプレイヤーの検出中...");

      // より包括的なセレクター
      const enhancedSelectors = [
        // YouTube特有（更新）
        "ytd-app video",
        'video[src*="googlevideo"]',
        ".ytp-video-container video",
        "#movie_player video",
        ".html5-video-player video",
        "video.video-stream",
        'video[class*="video"]',
        ".ytp-html5-video",
        "video[autoplay]",
        'video:not([width="0"]):not([height="0"])',
        "ytd-player video",
        ".ytd-player-container video",
        "video[poster]",
        "video[controls]",
        "video[preload]",
        ".player-container video",
        "video.html5-main-video",
        'div[id*="player"] video',

        // TVar / TVer特有（強化）
        ".tver-player video",
        "div[class*='Player'] video",
        ".media-player video",
        "video[class*='player']",
        ".video-container video",
        "video.tver-video",
        ".player-wrapper video",
        "video[autoplay]",
        "video:not([width='0']):not([height='0'])",
        ".video-js video",
        "video[poster]",
        "video[controls]",
        "video[preload]",
        "div[id*='player'] video",
        ".jwplayer video",
        "video.jw-video",
        "video[data-*]",

        // 一般的なプレイヤー（強化）
        "[data-player] video",
        "[id*='player'] video",
        "[class*='player'] video",
        "[class*='Player'] video",
        "[data-video] video",
        ".video video",

        // HTML5プレイヤー
        ".html5-player video",
        ".html5-video video",
        ".vjs-tech video",
        ".video-js .vjs-tech",

        // カスタムプレイヤー
        ".plyr video",
        ".mediaelement video",
        ".jwplayer .jwvideo",

        // 埋め込みプレイヤー
        "[data-embed] video",
        "[class*='embed'] video",
      ];

      return this.findAndCreatePiPFromSelectors(
        enhancedSelectors,
        "強化カスタムプレイヤー"
      );
    }

    // 強化された通常Video要素の検出
    tryEnhancedNormalVideoPiP() {
      console.log("📺 強化通常Video要素の検出中...");

      const allVideos = Array.from(document.querySelectorAll("video"));
      console.log(`発見された動画要素: ${allVideos.length}個`);

      if (allVideos.length === 0) return null;

      // 優先度を使って動画をソート
      const sortedVideos = allVideos
        .map((video) => ({
          element: video,
          score: this.evaluateVideoQuality(video),
        }))
        .filter((item) => item.score > 0)
        .sort((a, b) => b.score - a.score);

      console.log(
        "動画要素の評価結果:",
        sortedVideos.map((item) => ({
          score: item.score,
          src: item.element.src || item.element.currentSrc,
          size: `${item.element.getBoundingClientRect().width}x${
            item.element.getBoundingClientRect().height
          }`,
        }))
      );

      for (let item of sortedVideos) {
        const video = item.element;
        try {
          console.log(
            `動画処理中 (スコア: ${item.score}):`,
            video.src || video.currentSrc
          );

          this.forceRemoveDisablePiP(video);

          const rect = video.getBoundingClientRect();
          if (rect.width < 150 || rect.height < 100) {
            console.log(
              "動画サイズが小さすぎます:",
              rect.width,
              "x",
              rect.height
            );
            continue;
          }

          console.log(
            "✅ 適切な動画要素を発見、PiP作成中:",
            video.src || video.currentSrc
          );

          // フローティングウィンドウを作成
          this.createForceVideoFloatingWindow(video);
          return video;
        } catch (error) {
          console.log(`動画要素処理エラー:`, error);
          continue;
        }
      }

      console.log("❌ 強化通常Video要素検出で適切な動画が見つかりませんでした");
      return null;
    }

    // セレクターからPiPを作成するヘルパー関数
    findAndCreatePiPFromSelectors(selectors, context = "検出") {
      let bestVideo = null;
      let bestScore = -1;

      for (let selector of selectors) {
        try {
          const videos = Array.from(document.querySelectorAll(selector));
          console.log(
            `${context} - セレクター "${selector}": ${videos.length}個の要素`
          );

          for (let video of videos) {
            if (!video || video.tagName !== "VIDEO") continue;

            this.forceRemoveDisablePiP(video);

            // 動画品質を評価
            const score = this.evaluateVideoQuality(video);
            if (score > bestScore) {
              bestScore = score;
              bestVideo = video;
            }

            const rect = video.getBoundingClientRect();
            console.log(`${context} - 動画チェック (${selector}):`, {
              width: rect.width,
              height: rect.height,
              readyState: video.readyState,
              score: score,
              src: video.src || video.currentSrc,
            });
          }
        } catch (error) {
          console.log(`${context} - セレクター "${selector}" でエラー:`, error);
        }
      }

      if (bestVideo && bestScore > 0) {
        console.log(
          `✅ ${context}で最良の動画発見 (スコア: ${bestScore}):`,
          bestVideo.src || bestVideo.currentSrc
        );
        this.createForceVideoFloatingWindow(bestVideo);
        return bestVideo;
      }

      console.log(`❌ ${context}で適切な動画が見つかりませんでした`);
      return null;
    }

    // iframe内動画の検出とPiP
    tryIframeVideoPiP() {
      console.log("🖼️ iframe内動画の検出中...");

      const iframes = Array.from(document.querySelectorAll("iframe"));
      console.log(`発見されたiframe: ${iframes.length}個`);

      for (let iframe of iframes) {
        try {
          const iframeDoc =
            iframe.contentDocument || iframe.contentWindow.document;
          if (!iframeDoc) {
            console.log("iframe内容にアクセスできません (CORS制限の可能性)");
            continue;
          }

          const videos = Array.from(iframeDoc.querySelectorAll("video"));
          console.log(`iframe内動画: ${videos.length}個`);

          for (let video of videos) {
            this.forceRemoveDisablePiP(video);

            const rect = video.getBoundingClientRect();
            if (
              rect.width >= 200 &&
              rect.height >= 150 &&
              video.readyState >= 2
            ) {
              console.log(
                "✅ iframe内動画でPiP作成:",
                video.src || video.currentSrc
              );
              this.createForceVideoFloatingWindow(video);
              return video;
            }
          }
        } catch (error) {
          console.log("iframe処理エラー:", error);
        }
      }

      console.log("❌ iframe内で適切な動画が見つかりませんでした");
      return null;
    }

    // フレームキャプチャによる疑似PiP（強化版）
    tryFrameCapturePiP() {
      console.log("📸 フレームキャプチャによる疑似PiP開始...");

      const allVideos = Array.from(document.querySelectorAll("video"));
      console.log(`フレームキャプチャ対象動画: ${allVideos.length}個`);

      if (allVideos.length === 0) return null;

      // 優先度でソート
      const sortedVideos = allVideos
        .map((video) => ({
          element: video,
          score: this.evaluateVideoQuality(video),
        }))
        .filter((item) => item.score >= 0) // スコア0以上のものを対象
        .sort((a, b) => b.score - a.score);

      for (let item of sortedVideos) {
        const video = item.element;
        try {
          const rect = video.getBoundingClientRect();

          // readyStateチェックを緩和
          if (
            video.readyState >= 1 &&
            rect.width >= 100 &&
            rect.height >= 100
          ) {
            console.log(
              `フレームキャプチャ実行: ${video.src || video.currentSrc}`
            );

            const canvas = document.createElement("canvas");
            const ctx = canvas.getContext("2d");

            canvas.width = rect.width;
            canvas.height = rect.height;

            try {
              ctx.drawImage(video, 0, 0, canvas.width, canvas.height);

              console.log("✅ フレームキャプチャ成功");
              this.createFrameCapturePiPWindow(video, canvas, ctx);

              // 自動フレーム更新を開始
              this.startFrameUpdateInterval(video, canvas, ctx);

              return video;
            } catch (drawError) {
              console.log("フレーム描画エラー:", drawError);
              continue;
            }
          }
        } catch (error) {
          console.log("フレームキャプチャエラー:", error);
          continue;
        }
      }

      console.log("❌ フレームキャプチャできる動画が見つかりませんでした");
      return null;
    }

    // フレームキャプチャPiPウィンドウを作成
    createFrameCapturePiPWindow(video, canvas, ctx) {
      console.log("🖼️ フレームキャプチャPiPウィンドウを作成中...");

      // 既存のPiPウィンドウを削除
      const existingPiP = document.getElementById("frame-capture-pip-window");
      if (existingPiP) {
        existingPiP.remove();
      }

      // フローティングコンテナを作成
      const pipContainer = document.createElement("div");
      pipContainer.id = "frame-capture-pip-window";
      pipContainer.className = "frame-capture-pip-window";

      const rect = video.getBoundingClientRect();
      const pipWidth = Math.min(400, rect.width);
      const pipHeight = Math.min(300, rect.height);

      pipContainer.style.cssText = `
        position: fixed;
        top: 50px;
        right: 50px;
        width: ${pipWidth}px;
        height: ${pipHeight}px;
        background: #000;
        border: 2px solid #007ACC;
        border-radius: 12px;
        box-shadow: 0 10px 40px rgba(0, 0, 0, 0.3);
        z-index: 999999;
        overflow: hidden;
        resize: both;
        min-width: 200px;
        min-height: 150px;
        backdrop-filter: blur(10px);
        transition: transform 0.2s ease, box-shadow 0.2s ease;
      `;

      // ヘッダーバーを作成
      const headerBar = document.createElement("div");
      headerBar.style.cssText = `
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        height: 25px;
        background: linear-gradient(135deg, #007ACC 0%, #005a9e 100%);
        display: flex;
        align-items: center;
        padding: 0 8px;
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        font-size: 11px;
        color: white;
        cursor: move;
        user-select: none;
        border-radius: 10px 10px 0 0;
      `;

      const headerTitle = document.createElement("div");
      headerTitle.style.cssText = `flex: 1; font-weight: 600;`;
      headerTitle.textContent = "📸 フレームキャプチャ";

      const closeButton = document.createElement("button");
      closeButton.innerHTML = "×";
      closeButton.style.cssText = `
        width: 20px;
        height: 20px;
        border: none;
        background: rgba(255, 255, 255, 0.2);
        color: white;
        border-radius: 50%;
        cursor: pointer;
        font-size: 14px;
        display: flex;
        align-items: center;
        justify-content: center;
      `;

      closeButton.onclick = () => {
        pipContainer.remove();
        console.log("🔚 フレームキャプチャPiPウィンドウを閉じました");
      };

      // キャンバスのスタイル設定
      canvas.style.cssText = `
        width: 100%;
        height: calc(100% - 25px);
        margin-top: 25px;
        object-fit: contain;
        border-radius: 0 0 10px 10px;
      `;

      // ドラッグ機能を追加
      this.addDragFunctionality(pipContainer, headerBar);

      // 要素を組み立て
      headerBar.appendChild(headerTitle);
      headerBar.appendChild(closeButton);
      pipContainer.appendChild(headerBar);
      pipContainer.appendChild(canvas);
      document.body.appendChild(pipContainer);

      console.log("✅ フレームキャプチャPiPウィンドウが作成されました");
    }

    // フレーム更新インターバルを開始
    startFrameUpdateInterval(video, canvas, ctx) {
      console.log("🔄 フレーム自動更新を開始");

      const updateInterval = setInterval(() => {
        try {
          if (!video || video.readyState < 1) {
            console.log("動画が無効になったため更新停止");
            clearInterval(updateInterval);
            return;
          }

          // PiPウィンドウが存在するかチェック
          const pipWindow = document.getElementById("frame-capture-pip-window");
          if (!pipWindow) {
            console.log("PiPウィンドウが閉じられたため更新停止");
            clearInterval(updateInterval);
            return;
          }

          // フレームを更新
          ctx.drawImage(video, 0, 0, canvas.width, canvas.height);
        } catch (error) {
          console.log("フレーム更新エラー:", error);
        }
      }, 1000); // 1秒間隔で更新

      // 10分後に自動停止
      setTimeout(() => {
        clearInterval(updateInterval);
        console.log("📸 フレーム更新を10分後に自動停止");
      }, 10 * 60 * 1000);
    }

    // 動画領域自動検出キャプチャ
    tryVideoAreaCapture() {
      console.log("🎪 動画領域自動検出キャプチャを開始...");

      // 動画らしい領域を検出するヒューリスティック
      const videoLikeSelectors = [
        "[id*='video']",
        "[class*='video']",
        "[id*='player']",
        "[class*='player']",
        "[data-video]",
        "[data-player]",
        "canvas",
        "iframe[src*='youtube']",
        "iframe[src*='vimeo']",
        "iframe[src*='twitch']",
        "div[style*='background-image']",
      ];

      for (let selector of videoLikeSelectors) {
        const elements = Array.from(document.querySelectorAll(selector));
        console.log(`セレクター "${selector}": ${elements.length}個の要素`);

        for (let element of elements) {
          const rect = element.getBoundingClientRect();
          const area = rect.width * rect.height;

          // 動画らしいサイズの要素を検出
          if (area > 50000 && rect.width > 200 && rect.height > 150) {
            console.log("動画らしい領域を発見:", {
              element: element.tagName,
              size: `${rect.width}x${rect.height}`,
              area: area,
            });

            try {
              this.createAreaCapturePiPWindow(element);
              console.log("✅ 動画領域キャプチャPiP成功");
              return element;
            } catch (error) {
              console.log("領域キャプチャエラー:", error);
              continue;
            }
          }
        }
      }

      console.log("❌ 動画領域自動検出に失敗しました");
      return null;
    }

    // 領域キャプチャPiPウィンドウを作成
    createAreaCapturePiPWindow(element) {
      console.log("🎪 領域キャプチャPiPウィンドウを作成中...");

      // 既存のPiPウィンドウを削除
      const existingPiP = document.getElementById("area-capture-pip-window");
      if (existingPiP) {
        existingPiP.remove();
      }

      // フローティングコンテナを作成
      const pipContainer = document.createElement("div");
      pipContainer.id = "area-capture-pip-window";
      pipContainer.className = "area-capture-pip-window";

      const rect = element.getBoundingClientRect();
      const pipWidth = Math.min(400, rect.width * 0.5);
      const pipHeight = Math.min(300, rect.height * 0.5);

      pipContainer.style.cssText = `
        position: fixed;
        top: 60px;
        left: 60px;
        width: ${pipWidth}px;
        height: ${pipHeight}px;
        background: #000;
        border: 2px solid #FF6B35;
        border-radius: 12px;
        box-shadow: 0 10px 40px rgba(0, 0, 0, 0.3);
        z-index: 999999;
        overflow: hidden;
        resize: both;
        min-width: 200px;
        min-height: 150px;
        backdrop-filter: blur(10px);
        transition: transform 0.2s ease, box-shadow 0.2s ease;
      `;

      // ヘッダーバーを作成
      const headerBar = document.createElement("div");
      headerBar.style.cssText = `
        position: absolute;
        top: 0;
        left: 0;
        right: 0;
        height: 25px;
        background: linear-gradient(135deg, #FF6B35 0%, #e55a2e 100%);
        display: flex;
        align-items: center;
        padding: 0 8px;
        font-family: -apple-system, BlinkMacSystemFont, 'Segoe UI', Roboto, sans-serif;
        font-size: 11px;
        color: white;
        cursor: move;
        user-select: none;
        border-radius: 10px 10px 0 0;
      `;

      const headerTitle = document.createElement("div");
      headerTitle.style.cssText = `flex: 1; font-weight: 600;`;
      headerTitle.textContent = "🎪 領域キャプチャ";

      const closeButton = document.createElement("button");
      closeButton.innerHTML = "×";
      closeButton.style.cssText = `
        width: 18px;
        height: 18px;
        border: none;
        background: rgba(255, 255, 255, 0.2);
        color: white;
        border-radius: 50%;
        cursor: pointer;
        font-size: 12px;
        display: flex;
        align-items: center;
        justify-content: center;
      `;

      closeButton.onclick = () => {
        pipContainer.remove();
        console.log("🔚 領域キャプチャPiPウィンドウを閉じました");
      };

      // 要素のクローンを作成
      const elementClone = element.cloneNode(true);
      elementClone.style.cssText = `
        width: 100%;
        height: calc(100% - 25px);
        margin-top: 25px;
        object-fit: contain;
        border-radius: 0 0 10px 10px;
        transform: scale(0.8);
        transform-origin: top left;
      `;

      // ドラッグ機能を追加
      this.addDragFunctionality(pipContainer, headerBar);

      // 要素を組み立て
      headerBar.appendChild(headerTitle);
      headerBar.appendChild(closeButton);
      pipContainer.appendChild(headerBar);
      pipContainer.appendChild(elementClone);
      document.body.appendChild(pipContainer);

      console.log("✅ 領域キャプチャPiPウィンドウが作成されました");
    }

    // デバッグ情報表示
    showPiPDebugInfo() {
      console.log("🔍 PiP機能デバッグ情報:");

      // 現在のサイト情報
      const domain = window.location.hostname.toLowerCase();
      const isStreamingSite = this.detectVideoStreamingSite(domain);
      console.log("サイト情報:", {
        domain: domain,
        url: window.location.href,
        isStreamingSite: isStreamingSite,
        title: document.title,
      });

      // 動画要素の状況
      const allVideos = Array.from(document.querySelectorAll("video"));
      console.log(`動画要素: ${allVideos.length}個発見`);

      allVideos.forEach((video, index) => {
        const rect = video.getBoundingClientRect();
        const score = this.evaluateVideoQuality(video);
        console.log(`動画 ${index + 1}:`, {
          src: video.src || video.currentSrc || "ソースなし",
          size: `${rect.width}x${rect.height}`,
          readyState: video.readyState,
          paused: video.paused,
          currentTime: video.currentTime,
          duration: video.duration,
          disablePiP: video.hasAttribute("disablepictureinpicture"),
          score: score,
        });
      });

      // アクティブなPiPウィンドウ
      const pipWindows = document.querySelectorAll(
        '[id*="pip"], [class*="pip"]'
      );
      console.log(`アクティブなPiPウィンドウ: ${pipWindows.length}個`);

      pipWindows.forEach((window, index) => {
        console.log(`PiPウィンドウ ${index + 1}:`, {
          id: window.id,
          className: window.className,
          size: `${window.offsetWidth}x${window.offsetHeight}`,
          position: `${window.style.left}, ${window.style.top}`,
        });
      });

      // ブラウザサポート状況
      console.log("ブラウザサポート:", {
        pictureInPictureEnabled: document.pictureInPictureEnabled,
        requestPictureInPicture:
          typeof HTMLVideoElement.prototype.requestPictureInPicture,
        currentPiPElement: document.pictureInPictureElement ? "あり" : "なし",
      });

      return {
        domain,
        isStreamingSite,
        videoCount: allVideos.length,
        pipWindowCount: pipWindows.length,
        browserSupport: document.pictureInPictureEnabled,
      };
    }

    // パフォーマンス監視
    monitorPiPPerformance() {
      console.log("📊 PiP機能パフォーマンス監視を開始");

      const startTime = performance.now();
      let operationCount = 0;

      const originalLog = console.log;
      console.log = function (...args) {
        if (
          args[0] &&
          typeof args[0] === "string" &&
          (args[0].includes("PiP") || args[0].includes("動画"))
        ) {
          operationCount++;
        }
        return originalLog.apply(console, args);
      };

      // 5秒後に統計を表示
      setTimeout(() => {
        const endTime = performance.now();
        const duration = endTime - startTime;

        console.log = originalLog; // 元に戻す

        console.log("📈 PiPパフォーマンス統計:");
        console.log(`- 監視時間: ${duration.toFixed(2)}ms`);
        console.log(`- PiP関連操作: ${operationCount}回`);
        console.log(
          `- 平均操作頻度: ${(operationCount / (duration / 1000)).toFixed(
            2
          )}回/秒`
        );

        // メモリ使用量（可能な場合）
        if (performance.memory) {
          console.log("- メモリ使用量:", {
            used: `${(performance.memory.usedJSHeapSize / 1024 / 1024).toFixed(
              2
            )}MB`,
            total: `${(
              performance.memory.totalJSHeapSize /
              1024 /
              1024
            ).toFixed(2)}MB`,
            limit: `${(
              performance.memory.jsHeapSizeLimit /
              1024 /
              1024
            ).toFixed(2)}MB`,
          });
        }
      }, 5000);
    }
  }

  // グローバルインスタンスを作成（既に存在しない場合のみ）
  if (typeof window.pipHandler === "undefined") {
    window.pipHandler = new PictureInPictureHandler();
    console.log("✅ PictureInPictureHandler initialized successfully");
  } else {
    console.log(
      "⚠️ PictureInPictureHandler already exists, skipping initialization"
    );
  }

  // グローバル関数のエクスポート
  window.pipHandler.exportGlobalFunctions();
} // 条件文の終了
