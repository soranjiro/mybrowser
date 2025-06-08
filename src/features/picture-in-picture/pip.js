// Picture-in-Picture JavaScript Functionality

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

      // Step 1: ページ内のdisablepictureinpicture属性を削除
      const videos = document.querySelectorAll(
        "video[disablepictureinpicture]"
      );
      videos.forEach((video) => {
        console.log("📺 disablepictureinpicture属性を削除:", video);
        video.removeAttribute("disablepictureinpicture");
      });

      // Step 2: 環境を作成
      this.createPiPEnvironment();

      // Step 3: 動画の準備と実行
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

      // Step 4: Picture-in-Picture実行
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

      // iframe を作成
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

      // ウィンドウコントロールを組み立て
      windowControls.appendChild(minimizeButton);
      windowControls.appendChild(maximizeButton);
      windowControls.appendChild(closeButton);

      // ヘッダーバーを組み立て
      headerBar.appendChild(headerTitle);
      headerBar.appendChild(windowControls);

      // 現在のページの縮小版を iframe に設定
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

      // スクリーンショット的な表示を作成
      const canvas = document.createElement("canvas");
      const ctx = canvas.getContext("2d");
      canvas.style.cssText = `
            width: 100%;
            height: calc(100% - 35px);
            margin-top: 35px;
            object-fit: contain;
            border-radius: 0 0 12px 12px;
            background: #fafafa;
        `;

      // 簡易的なページキャプチャ (html2canvasの代替)
      canvas.width = window.innerWidth;
      canvas.height = window.innerHeight;

      // 背景を描画
      ctx.fillStyle =
        window.getComputedStyle(document.body).backgroundColor || "#ffffff";
      ctx.fillRect(0, 0, canvas.width, canvas.height);

      // テキストでページ内容を表示（簡易版）
      ctx.fillStyle = "#333333";
      ctx.font = "16px Arial";
      ctx.fillText("📄 " + (document.title || "Untitled Page"), 20, 50);
      ctx.font = "12px Arial";
      ctx.fillText("URL: " + window.location.href, 20, 80);

      // ページの主要テキストを抽出して描画
      const textContent = document.body.innerText.substring(0, 500);
      const lines = textContent.split("\n").slice(0, 15);
      lines.forEach((line, index) => {
        if (line.trim()) {
          ctx.fillText(line.substring(0, 50), 20, 110 + index * 20);
        }
      });

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

      // ウィンドウコントロールを組み立て
      windowControls.appendChild(minimizeButton);
      windowControls.appendChild(maximizeButton);
      windowControls.appendChild(closeButton);

      // ヘッダーバーを組み立て
      headerBar.appendChild(headerTitle);
      headerBar.appendChild(windowControls);

      // 要素を組み立て
      pipContainer.appendChild(headerBar);
      pipContainer.appendChild(canvas);
      document.body.appendChild(pipContainer);

      console.log("✅ スクリーンショットPiPウィンドウが作成されました");
    }

    // 全てのPiPウィンドウを閉じる
    exitAllPiP() {
      console.log("=== 全Picture-in-Pictureウィンドウを終了 ===");

      // 標準のPiPを終了
      if (document.pictureInPictureElement) {
        document.exitPictureInPicture().catch(console.error);
      }

      // 独自のPiPウィンドウを全て削除
      const pipWindows = document.querySelectorAll(
        '[id^="pip-"], .pip-element-window, .pip-page-window, .pip-screenshot-window, #pip-floating-window'
      );
      pipWindows.forEach((window) => {
        window.remove();
      });

      // 要素選択モードのオーバーレイも削除
      const overlay = document.getElementById("pip-element-selector-overlay");
      if (overlay) {
        overlay.remove();
      }

      console.log(`✅ ${pipWindows.length} 個のPiPウィンドウを閉じました`);
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
} // 条件文の終了
