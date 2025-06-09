// filepath: /Users/user/Documents/03_app/mybrowser/src/features/picture-in-picture/pip.js
// Picture-in-Picture JavaScript Functionality with Enhanced Error Handling

// ===== エラーハンドリングとパフォーマンス最適化の定数 =====
window.PIP_CONFIG = {
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
        return videos.filter(function (v) {
          return v.readyState >= 2 && !v.paused;
        });
      },
    },
    "tver.jp": {
      selectors: [
        ".tver-player video",
        'div[class*="Player"] video',
        ".media-player video",
        'video[class*="player"]',
        ".video-container video",
        "video.tver-video",
        ".player-wrapper video",
        "video[autoplay]",
        'video:not([width="0"]):not([height="0"])',
        ".video-js video",
        "video[poster]",
        "video[controls]",
        "video[preload]",
        'div[id*="player"] video',
        ".jwplayer video",
        "video.jw-video",
        "video[data-*]",
      ],
      waitTime: 10000,
      attributes: ["disablepictureinpicture", "controlslist"],
      customLogic: function (videos) {
        // TVar固有のロジック
        return videos.filter(function (v) {
          return v.videoWidth > 100 && v.videoHeight > 100;
        });
      },
    },
  },
};

// ===== ユーティリティ関数 =====
window.PiPUtils = {
  // エラーハンドリング付きの非同期実行
  safeExecute: function (fn, context, timeout) {
    context = context || "Unknown";
    timeout = timeout || 10000;

    return new Promise(function (resolve, reject) {
      try {
        var timeoutId = setTimeout(function () {
          reject(new Error("Timeout: " + context));
        }, timeout);

        var result = fn();

        if (result && typeof result.then === "function") {
          result
            .then(function (data) {
              clearTimeout(timeoutId);
              resolve({ success: true, result: data });
            })
            .catch(function (error) {
              clearTimeout(timeoutId);
              console.error("[PiP Error] " + context + ":", error);
              resolve({ success: false, error: error });
            });
        } else {
          clearTimeout(timeoutId);
          resolve({ success: true, result: result });
        }
      } catch (error) {
        console.error("[PiP Error] " + context + ":", error);
        resolve({ success: false, error: error });
      }
    });
  },

  // スロットリング関数
  throttle: function (func, delay) {
    var timeoutId;
    var lastExecTime = 0;
    return function () {
      var args = Array.prototype.slice.call(arguments);
      var currentTime = Date.now();

      if (currentTime - lastExecTime > delay) {
        func.apply(this, args);
        lastExecTime = currentTime;
      } else {
        clearTimeout(timeoutId);
        timeoutId = setTimeout(
          function () {
            func.apply(this, args);
            lastExecTime = Date.now();
          }.bind(this),
          delay - (currentTime - lastExecTime)
        );
      }
    };
  },

  // デバウンス関数
  debounce: function (func, delay) {
    var timeoutId;
    return function () {
      var args = Array.prototype.slice.call(arguments);
      clearTimeout(timeoutId);
      timeoutId = setTimeout(
        function () {
          func.apply(this, args);
        }.bind(this),
        delay
      );
    };
  },

  // 要素の可視性をチェック
  isElementVisible: function (element) {
    if (!element) return false;

    var rect = element.getBoundingClientRect();
    var style = window.getComputedStyle(element);

    return (
      rect.width > 0 &&
      rect.height > 0 &&
      style.display !== "none" &&
      style.visibility !== "hidden" &&
      style.opacity !== "0"
    );
  },

  // 動画要素の品質スコア計算
  calculateVideoScore: function (video, siteConfig) {
    var score = 0;

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
        var customVideos = siteConfig.customLogic([video]);
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
  },

  // サイト設定を取得
  getSiteConfig: function (url) {
    url = url || window.location.href;
    for (var domain in window.PIP_CONFIG.SITE_CONFIGS) {
      if (url.includes(domain)) {
        return window.PIP_CONFIG.SITE_CONFIGS[domain];
      }
    }
    return null;
  },

  // パフォーマンス監視
  startPerformanceMonitoring: function () {
    if (window.pipPerformanceMonitor) return;

    window.pipPerformanceMonitor = {
      startTime: Date.now(),
      operations: [],

      log: function (operation, duration) {
        this.operations.push({
          operation: operation,
          duration: duration,
          timestamp: Date.now(),
        });

        if (this.operations.length > 100) {
          this.operations = this.operations.slice(-50);
        }
      },

      getStats: function () {
        var totalTime = Date.now() - this.startTime;
        var avgDuration =
          this.operations.reduce(function (sum, op) {
            return sum + op.duration;
          }, 0) / this.operations.length;

        return {
          totalTime: totalTime,
          operationCount: this.operations.length,
          averageDuration: avgDuration || 0,
          recentOperations: this.operations.slice(-10),
        };
      },
    };
  },
};

// パフォーマンス監視を開始
window.PiPUtils.startPerformanceMonitoring();

// PictureInPictureHandlerが既に存在するかチェック
if (typeof window.PictureInPictureHandler === "undefined") {
  function PictureInPictureHandler() {
    this.isDragging = false;
    this.dragStartX = 0;
    this.dragStartY = 0;
  }

  // PiP環境の準備
  PictureInPictureHandler.prototype.createPiPEnvironment = function () {
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

        var self = this;
        return new Promise(function (resolve, reject) {
          try {
            // disablepictureinpicture属性をチェック
            if (self.hasAttribute("disablepictureinpicture")) {
              console.log(
                "⚠️ disablepictureinpicture属性が設定されています - 削除します"
              );
              self.removeAttribute("disablepictureinpicture");
            }

            // フローティングウィンドウを作成
            self.createFloatingVideoWindow();

            // モックのPiPウィンドウオブジェクトを返す
            var mockPiPWindow = {
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
      var existingWindow = document.getElementById("pip-floating-window");
      if (existingWindow) {
        existingWindow.remove();
      }

      // フローティングコンテナを作成
      var floatingContainer = document.createElement("div");
      floatingContainer.id = "pip-floating-window";

      // フローティングコンテナのスタイルを設定
      this.applyFloatingContainerStyles(floatingContainer);

      // ビデオクローンを作成
      var videoClone = this.cloneNode(true);
      this.applyVideoCloneStyles(videoClone);

      // 元の動画と同期
      videoClone.currentTime = this.currentTime;
      if (!this.paused) {
        videoClone.play();
      }

      // 閉じるボタンを追加
      var closeButton = this.createCloseButton(floatingContainer);

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
  };

  // メイン実行関数
  PictureInPictureHandler.prototype.executePictureInPicture = function () {
    var self = this;

    return window.PiPUtils.safeExecute(
      function () {
        console.log("=== Picture-in-Picture実行開始 ===");

        // Step 1: 環境を作成
        self.createPiPEnvironment();

        // Step 2: 動画配信サイトかどうかをチェック
        var currentDomain = window.location.hostname.toLowerCase();
        var isVideoStreamingSite = self.detectVideoStreamingSite(currentDomain);

        console.log("現在のサイト: " + currentDomain);
        console.log(
          "動画配信サイト判定: " + (isVideoStreamingSite ? "YES" : "NO")
        );

        // Step 3: 動画配信サイトの場合は強制PiP機能を使用
        if (isVideoStreamingSite) {
          console.log("🎯 動画配信サイト向け強制PiP機能を実行中...");
          try {
            var result = self.forceVideoStreamingPiP();
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

        // Step 4: 通常のPiP処理
        console.log("🔄 通常のPiP処理を実行中...");

        // disablepictureinpicture属性を削除
        var videos = document.querySelectorAll(
          "video[disablepictureinpicture]"
        );
        for (var i = 0; i < videos.length; i++) {
          console.log("📺 disablepictureinpicture属性を削除:", videos[i]);
          videos[i].removeAttribute("disablepictureinpicture");
        }

        // 動画の準備と実行
        var allVideos = document.querySelectorAll("video");
        console.log("📹 見つかった動画:" + allVideos.length + "個");

        if (allVideos.length === 0) {
          alert("このページには動画が見つかりませんでした。");
          return;
        }

        // 最初のビデオを対象に選択
        var targetVideo = allVideos[0];
        for (var j = 0; j < allVideos.length; j++) {
          var video = allVideos[j];
          if (!video.paused && video.readyState >= 2) {
            targetVideo = video;
            break;
          }
        }

        console.log("🎯 対象動画を選択:", targetVideo);

        // Picture-in-Picture実行
        if (targetVideo.paused) {
          console.log("▶️ 動画を再生開始...");
          targetVideo.play();
        }

        console.log("🔄 Picture-in-Picture をリクエスト中...");
        return targetVideo
          .requestPictureInPicture()
          .then(function (pipWindow) {
            console.log("✅ Picture-in-Picture が開始されました!");
            console.log("📊 PiPウィンドウ:", pipWindow);

            alert(
              "Picture-in-Picture シミュレーションが開始されました！\n\n" +
                "右上にフローティングビデオウィンドウが表示されています。\n" +
                "ドラッグして移動したり、×ボタンで閉じたりできます。"
            );
            return pipWindow;
          })
          .catch(function (error) {
            console.error("❌ Picture-in-Picture エラー:", error);

            // エラーの場合も強制PiP機能を試行
            console.log("🔄 エラー発生のため強制PiP機能を試行中...");
            try {
              var fallbackResult = self.forceVideoStreamingPiP();
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

            var errorMessage = "Picture-in-Picture の開始に失敗しました。\n\n";

            if (error.name === "NotSupportedError") {
              errorMessage +=
                "この環境では Picture-in-Picture がサポートされていません。\n";
              errorMessage +=
                "Qt WebEngine の制限により、ネイティブ PiP は利用できませんが、\n";
              errorMessage +=
                "代替実装としてフローティングウィンドウを提供します。";
            } else if (error.name === "NotAllowedError") {
              errorMessage +=
                "Picture-in-Picture の使用が許可されていません。\n";
              errorMessage += "ユーザーの操作が必要です。";
            } else if (error.name === "InvalidStateError") {
              errorMessage += "動画の状態が無効です。\n";
              errorMessage += "動画を再生してから再試行してください。";
            } else {
              errorMessage += "エラー詳細: " + error.message;
            }

            alert(errorMessage);
            throw error;
          });
      },
      "Picture-in-Picture実行",
      30000
    );
  };

  // 動画配信サイトの検出
  PictureInPictureHandler.prototype.detectVideoStreamingSite = function (
    domain
  ) {
    var streamingSites = [
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

    for (var i = 0; i < streamingSites.length; i++) {
      if (domain.includes(streamingSites[i])) {
        return true;
      }
    }
    return false;
  };

  // 動画配信サイト向け強制PiP機能
  PictureInPictureHandler.prototype.forceVideoStreamingPiP = function () {
    console.log("=== 動画配信サイト向け強制PiP開始 ===");

    // 動画配信サイトの検出
    var currentDomain = window.location.hostname.toLowerCase();
    var isVideoStreamingSite = this.detectVideoStreamingSite(currentDomain);

    console.log("現在のサイト: " + currentDomain);
    console.log("動画配信サイト判定: " + (isVideoStreamingSite ? "YES" : "NO"));

    // 1. サイト固有の強化された動画検出を試行
    var siteSpecificVideo = this.tryStreamingSiteSpecificPiP(currentDomain);
    if (siteSpecificVideo) {
      console.log("✅ サイト固有動画検出でPiP成功");
      return siteSpecificVideo;
    }

    // 2. 強化されたカスタムプレイヤーの検出
    var customPlayer = this.tryEnhancedCustomPlayerPiP();
    if (customPlayer) {
      console.log("✅ 強化カスタムプレイヤーでPiP成功");
      return customPlayer;
    }

    // 3. 通常のVideo要素でPiPを試行（改良版）
    var normalVideo = this.tryEnhancedNormalVideoPiP();
    if (normalVideo) {
      console.log("✅ 強化通常Video要素でPiP成功");
      return normalVideo;
    }

    console.log("❌ すべてのPiP手法が失敗しました");
    return null;
  };

  // ストリーミングサイト固有の動画検出
  PictureInPictureHandler.prototype.tryStreamingSiteSpecificPiP = function (
    domain
  ) {
    console.log("🎯 サイト固有の動画検出を開始:", domain);

    var siteConfig = window.PiPUtils.getSiteConfig();
    if (!siteConfig) {
      console.log("サイト固有設定が見つかりません");
      return null;
    }

    return this.findAndCreatePiPFromSelectors(
      siteConfig.selectors,
      "サイト固有検出",
      siteConfig
    );
  };

  // 強化されたカスタムプレイヤーの検出
  PictureInPictureHandler.prototype.tryEnhancedCustomPlayerPiP = function () {
    console.log("🎛️ 強化カスタムプレイヤーの検出中...");

    var enhancedSelectors = [
      // YouTube特有
      "ytd-app video",
      'video[src*="googlevideo"]',
      ".ytp-video-container video",
      "#movie_player video",
      ".html5-video-player video",
      "video.video-stream",
      'video[class*="video"]',
      ".ytp-html5-video",
      "video.html5-main-video",

      // TVar / TVer特有
      ".tver-player video",
      'div[class*="Player"] video',
      ".media-player video",
      'video[class*="player"]',
      ".video-container video",
      ".video-js video",
      ".jwplayer video",

      // 一般的なプレイヤー
      "[data-player] video",
      '[id*="player"] video',
      '[class*="player"] video',
      '[class*="Player"] video',
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
    ];

    return this.findAndCreatePiPFromSelectors(
      enhancedSelectors,
      "強化カスタムプレイヤー"
    );
  };

  // 強化された通常Video要素の検出
  PictureInPictureHandler.prototype.tryEnhancedNormalVideoPiP = function () {
    console.log("📺 強化通常Video要素の検出中...");

    var allVideos = Array.from
      ? Array.from(document.querySelectorAll("video"))
      : Array.prototype.slice.call(document.querySelectorAll("video"));
    console.log("発見された動画要素: " + allVideos.length + "個");

    if (allVideos.length === 0) return null;

    // 優先度を使って動画をソート
    var sortedVideos = [];
    for (var i = 0; i < allVideos.length; i++) {
      var video = allVideos[i];
      var score = window.PiPUtils.calculateVideoScore(video);
      if (score > 0) {
        sortedVideos.push({ element: video, score: score });
      }
    }

    sortedVideos.sort(function (a, b) {
      return b.score - a.score;
    });

    for (var j = 0; j < sortedVideos.length; j++) {
      var item = sortedVideos[j];
      var video = item.element;
      try {
        console.log(
          "動画処理中 (スコア: " + item.score + "):",
          video.src || video.currentSrc
        );

        this.forceRemoveDisablePiP(video);

        var rect = video.getBoundingClientRect();
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
        this.createForceVideoFloatingWindow(video);
        return video;
      } catch (error) {
        console.log("動画要素処理エラー:", error);
        continue;
      }
    }

    console.log("❌ 強化通常Video要素検出で適切な動画が見つかりませんでした");
    return null;
  };

  // セレクターからPiPを作成するヘルパー関数
  PictureInPictureHandler.prototype.findAndCreatePiPFromSelectors = function (
    selectors,
    context,
    siteConfig
  ) {
    context = context || "検出";
    var bestVideo = null;
    var bestScore = -1;

    for (var i = 0; i < selectors.length; i++) {
      var selector = selectors[i];
      try {
        var videos = Array.from
          ? Array.from(document.querySelectorAll(selector))
          : Array.prototype.slice.call(document.querySelectorAll(selector));
        console.log(
          context +
            ' - セレクター "' +
            selector +
            '": ' +
            videos.length +
            "個の要素"
        );

        for (var j = 0; j < videos.length; j++) {
          var video = videos[j];
          if (!video || video.tagName !== "VIDEO") continue;

          this.forceRemoveDisablePiP(video);

          var score = window.PiPUtils.calculateVideoScore(video, siteConfig);
          if (score > bestScore) {
            bestScore = score;
            bestVideo = video;
          }

          var rect = video.getBoundingClientRect();
          console.log(context + " - 動画チェック (" + selector + "):", {
            width: rect.width,
            height: rect.height,
            readyState: video.readyState,
            score: score,
            src: video.src || video.currentSrc,
          });
        }
      } catch (error) {
        console.log(
          context + ' - セレクター "' + selector + '" でエラー:',
          error
        );
      }
    }

    if (bestVideo && bestScore > 0) {
      console.log(
        "✅ " + context + "で最良の動画発見 (スコア: " + bestScore + "):",
        bestVideo.src || bestVideo.currentSrc
      );
      this.createForceVideoFloatingWindow(bestVideo);
      return bestVideo;
    }

    console.log("❌ " + context + "で適切な動画が見つかりませんでした");
    return null;
  };

  // disablepictureinpicture属性を強制削除（強化版）
  PictureInPictureHandler.prototype.forceRemoveDisablePiP = function (video) {
    if (!video) return;

    try {
      // 属性を削除
      var attributes = [
        "disablepictureinpicture",
        "disablePictureInPicture",
        "disable-picture-in-picture",
      ];
      for (var i = 0; i < attributes.length; i++) {
        if (video.hasAttribute(attributes[i])) {
          video.removeAttribute(attributes[i]);
          console.log(attributes[i] + " 属性を削除しました");
        }
      }

      // プロパティを強制的にfalseに設定
      try {
        Object.defineProperty(video, "disablePictureInPicture", {
          value: false,
          writable: false,
          configurable: true,
        });
      } catch (e) {
        video.disablePictureInPicture = false;
      }

      // MutationObserverで属性の再追加を監視
      if (window.MutationObserver) {
        var observer = new MutationObserver(function (mutations) {
          mutations.forEach(function (mutation) {
            if (
              mutation.type === "attributes" &&
              mutation.attributeName &&
              mutation.attributeName
                .toLowerCase()
                .includes("disablepictureinpicture")
            ) {
              console.log("PiP無効化属性の再追加を検出・防止しました");
              video.removeAttribute(mutation.attributeName);
            }
          });
        });

        observer.observe(video, {
          attributes: true,
          attributeFilter: attributes,
        });
      }
    } catch (error) {
      console.log("disablePiP削除エラー:", error);
    }
  };

  // 強制動画フローティングウィンドウを作成
  PictureInPictureHandler.prototype.createForceVideoFloatingWindow = function (
    video
  ) {
    if (!video) return;

    console.log("🎬 強制フローティングビデオウィンドウを作成中...");

    // 既存のフローティングウィンドウがあれば削除
    var existingWindow = document.getElementById("pip-floating-window");
    if (existingWindow) {
      existingWindow.remove();
    }

    var floatingContainer = document.createElement("div");
    floatingContainer.id = "pip-floating-window";

    var rect = video.getBoundingClientRect();
    var pipWidth = Math.min(400, Math.max(320, rect.width * 0.5));
    var pipHeight = Math.min(300, Math.max(180, rect.height * 0.5));

    floatingContainer.style.cssText =
      "position: fixed;" +
      "top: 20px;" +
      "right: 20px;" +
      "width: " +
      pipWidth +
      "px;" +
      "height: " +
      pipHeight +
      "px;" +
      "background: black;" +
      "border: 2px solid #007ACC;" +
      "border-radius: 8px;" +
      "box-shadow: 0 4px 20px rgba(0,0,0,0.5);" +
      "z-index: 999999;" +
      "cursor: move;" +
      "overflow: hidden;" +
      "resize: both;" +
      "min-width: 200px;" +
      "min-height: 150px;";

    // ビデオクローンを作成
    var videoClone = video.cloneNode(true);
    videoClone.style.cssText =
      "width: 100%;" + "height: 100%;" + "object-fit: contain;";

    // 元の動画と同期
    videoClone.currentTime = video.currentTime;
    if (!video.paused) {
      videoClone.play().catch(function (error) {
        console.log("動画再生エラー:", error);
      });
    }

    // 閉じるボタンを作成
    var closeButton = document.createElement("button");
    closeButton.textContent = "×";
    closeButton.style.cssText =
      "position: absolute;" +
      "top: 5px;" +
      "right: 5px;" +
      "width: 25px;" +
      "height: 25px;" +
      "background: rgba(0,0,0,0.7);" +
      "color: white;" +
      "border: none;" +
      "border-radius: 50%;" +
      "cursor: pointer;" +
      "font-size: 14px;" +
      "z-index: 1000000;" +
      "display: flex;" +
      "align-items: center;" +
      "justify-content: center;";

    closeButton.onclick = function () {
      floatingContainer.remove();
      document.pictureInPictureElement = null;
      console.log("🔚 フローティングPiPウィンドウを閉じました");
    };

    // ドラッグ機能を追加
    this.addDragFunctionality(floatingContainer);

    // 要素を組み立て
    floatingContainer.appendChild(videoClone);
    floatingContainer.appendChild(closeButton);
    document.body.appendChild(floatingContainer);

    // Document の pictureInPictureElement を設定
    document.pictureInPictureElement = video;

    console.log("✅ 強制フローティングPiPウィンドウが作成されました");
    return floatingContainer;
  };

  // ドラッグ機能を追加
  PictureInPictureHandler.prototype.addDragFunctionality = function (
    element,
    dragHandle
  ) {
    var self = this;
    var isDragging = false;
    var dragStartX = 0;
    var dragStartY = 0;

    var targetElement = dragHandle || element;

    var onMouseDown = function (e) {
      if (e.target.tagName === "BUTTON") return;

      isDragging = true;
      dragStartX = e.clientX - element.offsetLeft;
      dragStartY = e.clientY - element.offsetTop;

      element.style.transition = "none";
      e.preventDefault();
    };

    var onMouseMove = function (e) {
      if (isDragging) {
        var newLeft = e.clientX - dragStartX;
        var newTop = e.clientY - dragStartY;

        var maxLeft = window.innerWidth - element.offsetWidth;
        var maxTop = window.innerHeight - element.offsetHeight;

        element.style.left = Math.max(0, Math.min(newLeft, maxLeft)) + "px";
        element.style.top = Math.max(0, Math.min(newTop, maxTop)) + "px";
        element.style.right = "auto";
      }
    };

    var onMouseUp = function () {
      if (isDragging) {
        isDragging = false;
        element.style.transition = "transform 0.2s ease, box-shadow 0.2s ease";
      }
    };

    targetElement.addEventListener("mousedown", onMouseDown);
    document.addEventListener("mousemove", onMouseMove);
    document.addEventListener("mouseup", onMouseUp);
  };

  // すべてのPiPウィンドウを終了
  PictureInPictureHandler.prototype.exitAllPiP = function () {
    console.log("🔚 全てのPiPウィンドウを終了中...");

    var pipWindows = document.querySelectorAll('[id*="pip"], [class*="pip"]');
    for (var i = 0; i < pipWindows.length; i++) {
      pipWindows[i].remove();
    }

    document.pictureInPictureElement = null;
    console.log("✅ " + pipWindows.length + "個のPiPウィンドウを終了しました");
  };

  // デバッグ情報表示
  PictureInPictureHandler.prototype.showPiPDebugInfo = function () {
    console.log("🔍 PiP機能デバッグ情報:");

    var domain = window.location.hostname.toLowerCase();
    var isStreamingSite = this.detectVideoStreamingSite(domain);
    console.log("サイト情報:", {
      domain: domain,
      url: window.location.href,
      isStreamingSite: isStreamingSite,
      title: document.title,
    });

    var allVideos = Array.from
      ? Array.from(document.querySelectorAll("video"))
      : Array.prototype.slice.call(document.querySelectorAll("video"));
    console.log("動画要素: " + allVideos.length + "個発見");

    for (var i = 0; i < allVideos.length; i++) {
      var video = allVideos[i];
      var rect = video.getBoundingClientRect();
      var score = window.PiPUtils.calculateVideoScore(video);
      console.log("動画 " + (i + 1) + ":", {
        src: video.src || video.currentSrc || "ソースなし",
        size: rect.width + "x" + rect.height,
        readyState: video.readyState,
        paused: video.paused,
        currentTime: video.currentTime,
        duration: video.duration,
        disablePiP: video.hasAttribute("disablepictureinpicture"),
        score: score,
      });
    }

    var pipWindows = document.querySelectorAll('[id*="pip"], [class*="pip"]');
    console.log("アクティブなPiPウィンドウ: " + pipWindows.length + "個");

    console.log("ブラウザサポート:", {
      pictureInPictureEnabled: document.pictureInPictureEnabled,
      requestPictureInPicture:
        typeof HTMLVideoElement.prototype.requestPictureInPicture,
      currentPiPElement: document.pictureInPictureElement ? "あり" : "なし",
    });

    return {
      domain: domain,
      isStreamingSite: isStreamingSite,
      videoCount: allVideos.length,
      pipWindowCount: pipWindows.length,
      browserSupport: document.pictureInPictureEnabled,
    };
  };

  // パフォーマンス監視
  PictureInPictureHandler.prototype.monitorPiPPerformance = function () {
    console.log("📊 PiP機能パフォーマンス監視を開始");

    if (window.pipPerformanceMonitor) {
      var stats = window.pipPerformanceMonitor.getStats();
      console.log("📈 PiPパフォーマンス統計:", stats);

      if (performance && performance.memory) {
        console.log("- メモリ使用量:", {
          used:
            (performance.memory.usedJSHeapSize / 1024 / 1024).toFixed(2) + "MB",
          total:
            (performance.memory.totalJSHeapSize / 1024 / 1024).toFixed(2) +
            "MB",
          limit:
            (performance.memory.jsHeapSizeLimit / 1024 / 1024).toFixed(2) +
            "MB",
        });
      }
    }
  };

  // グローバル関数のエクスポート
  PictureInPictureHandler.prototype.exportGlobalFunctions = function () {
    window.executePictureInPicture = this.executePictureInPicture.bind(this);
    window.exitAllPiP = this.exitAllPiP.bind(this);
    window.showPiPDebugInfo = this.showPiPDebugInfo.bind(this);
    window.monitorPiPPerformance = this.monitorPiPPerformance.bind(this);
  };

  // グローバルインスタンスを作成
  window.pipHandler = new PictureInPictureHandler();
  window.pipHandler.exportGlobalFunctions();
  console.log("✅ Enhanced PictureInPictureHandler initialized successfully");
} else {
  console.log(
    "⚠️ PictureInPictureHandler already exists, skipping initialization"
  );
}
