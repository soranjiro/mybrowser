// Picture-in-Picture JavaScript Functionality

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

  addDragFunctionality(floatingContainer, closeButton) {
    const handler = this;

    floatingContainer.onmousedown = (e) => {
      if (e.target === closeButton) return;
      handler.isDragging = true;
      handler.dragStartX = e.clientX - floatingContainer.offsetLeft;
      handler.dragStartY = e.clientY - floatingContainer.offsetTop;
    };

    document.onmousemove = (e) => {
      if (handler.isDragging) {
        floatingContainer.style.left = e.clientX - handler.dragStartX + "px";
        floatingContainer.style.top = e.clientY - handler.dragStartY + "px";
      }
    };

    document.onmouseup = () => {
      handler.isDragging = false;
    };
  }

  // メイン実行関数
  async executePictureInPicture() {
    console.log("=== Picture-in-Picture実行開始 ===");

    // Step 1: ページ内のdisablepictureinpicture属性を削除
    const videos = document.querySelectorAll("video[disablepictureinpicture]");
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
        errorMessage += "代替実装としてフローティングウィンドウを提供します。";
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

    console.log(`${videoCount} 個の動画をPicture-in-Picture対応に設定しました`);

    // observer を返して外部からアクセス可能にする
    window._pipObserver = observer;

    return videoCount;
  }
}

// グローバルインスタンスを作成
window.pipHandler = new PictureInPictureHandler();
