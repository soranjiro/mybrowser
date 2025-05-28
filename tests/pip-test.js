// Picture-in-Picture Test JavaScript

function log(message) {
  const logDiv = document.getElementById("log");
  const timestamp = new Date().toLocaleTimeString();
  logDiv.innerHTML += `[${timestamp}] ${message}\n`;
  logDiv.scrollTop = logDiv.scrollHeight;
  console.log(message);
}

function clearLog() {
  document.getElementById("log").innerHTML = "";
}

function testPiPSupport() {
  log("=== PiP サポート確認開始 ===");

  // 基本的なAPIチェック
  if ("pictureInPictureEnabled" in document) {
    log(
      "✅ document.pictureInPictureEnabled: " + document.pictureInPictureEnabled
    );
  } else {
    log("❌ document.pictureInPictureEnabled は利用できません");
  }

  // ビデオ要素のPiPサポートチェック
  const video = document.getElementById("testVideo");
  if (video) {
    log("✅ ビデオ要素が見つかりました");

    if ("requestPictureInPicture" in video) {
      log("✅ video.requestPictureInPicture メソッドが利用可能");
    } else {
      log("❌ video.requestPictureInPicture メソッドが利用できません");
    }

    if ("disablePictureInPicture" in video) {
      log(`📌 video.disablePictureInPicture: ${video.disablePictureInPicture}`);
    } else {
      log("❌ video.disablePictureInPicture プロパティが利用できません");
    }

    log(
      `📌 ビデオの再生状態: paused=${video.paused}, currentTime=${video.currentTime}`
    );
    log(
      `📌 ビデオの準備状態: readyState=${video.readyState}, networkState=${video.networkState}`
    );
  } else {
    log("❌ ビデオ要素が見つかりません");
  }

  // 現在のPiP状態
  if ("pictureInPictureElement" in document) {
    log(
      `📌 現在のPiP要素: ${document.pictureInPictureElement ? "あり" : "なし"}`
    );
  }

  log("=== PiP サポート確認完了 ===");
}

async function playVideo() {
  const video = document.getElementById("testVideo");
  if (!video) {
    log("❌ ビデオ要素が見つかりません");
    return;
  }

  try {
    await video.play();
    log("▶️ ビデオ再生開始");
  } catch (error) {
    log(`❌ ビデオ再生エラー: ${error.message}`);
  }
}

async function startPiP() {
  log("=== PiP 開始試行 ===");

  const video = document.getElementById("testVideo");
  if (!video) {
    log("❌ ビデオ要素が見つかりません");
    return;
  }

  try {
    // ビデオが一時停止中の場合は再生
    if (video.paused) {
      log("📹 ビデオを再生中...");
      await video.play();
    }

    log("🚀 PiP モード開始中...");
    const pipWindow = await video.requestPictureInPicture();

    log("✅ PiP モード開始成功!");
    log(`📏 PiP ウィンドウサイズ: ${pipWindow.width} x ${pipWindow.height}`);

    // PiPウィンドウのイベントリスナー
    pipWindow.addEventListener("resize", () => {
      log(
        `📏 PiP ウィンドウサイズ変更: ${pipWindow.width} x ${pipWindow.height}`
      );
    });
  } catch (error) {
    log(`❌ PiP 開始エラー: ${error.name} - ${error.message}`);

    if (error.name === "NotSupportedError") {
      log("💡 この環境ではPiPがサポートされていません");
    } else if (error.name === "NotAllowedError") {
      log("💡 PiP使用が許可されていません（ユーザーアクションが必要）");
    } else if (error.name === "InvalidStateError") {
      log("💡 ビデオの状態が無効です");
    }
  }
}

async function exitPiP() {
  log("=== PiP 終了試行 ===");

  try {
    if (document.pictureInPictureElement) {
      await document.exitPictureInPicture();
      log("✅ PiP モード終了成功");
    } else {
      log("ℹ️ 現在PiPモードではありません");
    }
  } catch (error) {
    log(`❌ PiP 終了エラー: ${error.name} - ${error.message}`);
  }
}

function testVideoState() {
  log("=== ビデオ状態確認 ===");

  const video = document.getElementById("testVideo");
  if (!video) {
    log("❌ ビデオ要素が見つかりません");
    return;
  }

  log(`📌 paused: ${video.paused}`);
  log(`📌 currentTime: ${video.currentTime}`);
  log(`📌 duration: ${video.duration}`);
  log(`📌 readyState: ${video.readyState}`);
  log(`📌 networkState: ${video.networkState}`);
  log(`📌 videoWidth: ${video.videoWidth}`);
  log(`📌 videoHeight: ${video.videoHeight}`);
  log(`📌 disablePictureInPicture: ${video.disablePictureInPicture}`);
}

// ページ読み込み時の初期化
window.addEventListener("load", () => {
  log("🚀 PiP テストページが読み込まれました");
  testPiPSupport();
});

// PiPイベントリスナー
document.addEventListener("enterpictureinpicture", (event) => {
  log("🎉 PiP モード開始イベント発生");
});

document.addEventListener("leavepictureinpicture", (event) => {
  log("👋 PiP モード終了イベント発生");
});
