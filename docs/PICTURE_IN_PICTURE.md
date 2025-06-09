# Picture-in-Picture (PiP) 機能

MyBrowser の Picture-in-Picture 機能について説明します。

## 機能概要

MyBrowser の PiP 機能は、Web ページ上の画像を独立したウィンドウで表示できる機能です。macOS Spaces に完全対応し、Mission Control でも独立したウィンドウとして表示されます。

## 主な特徴

### 🎯 実際の表示内容をキャプチャ

- 画像の URL ではなく、ブラウザで実際に表示されている内容をキャプチャ
- ランダム画像（picsum.photos など）でも、表示されているものと同じ画像が PiP に表示される
- JavaScript で Canvas を使用して Base64 データとして取得

### 📺 視覚的フィードバック

- PiP 起動時に元の画像に「📺 PiP 中」オーバーレイを表示
- アニメーション付きの境界線とグラデーション効果
- キーボードショートカット使用時の通知表示

### 🖼️ コンテナ PiP 対応

- 画像だけでなく、その親要素（コンテナ）も含めて PiP 表示可能
- 画像に関連するタイトルや説明文なども一緒に PiP 化

### 🚀 macOS Spaces 完全対応

- NSPanel を使用した独立ウィンドウ
- すべての Space で表示される（`NSWindowCollectionBehaviorCanJoinAllSpaces`）
- フルスクリーンアプリの上にも表示（`NSWindowCollectionBehaviorFullScreenAuxiliary`）
- Mission Control で独立したウィンドウとして認識

## 使用方法

### キーボードショートカット

- **Ctrl+Alt+I**: 選択された画像またはページ内最大画像を PiP 表示
- **Ctrl+Alt+X**: すべての PiP ウィンドウを閉じる

### メニューから操作

1. メニューバー → View → Picture-in-Picture → Image Picture-in-Picture
2. または、メニューバー → View → Picture-in-Picture → Close All PiP Windows

### マウス操作

- PiP ウィンドウはドラッグして移動可能
- ダブルクリックで閉じる
- 右上の × ボタンで閉じる

## 技術的詳細

### JavaScript 統合

```javascript
// 画像キャプチャのサンプルコード
const canvas = document.createElement("canvas");
const ctx = canvas.getContext("2d");
canvas.width = rect.width;
canvas.height = rect.height;
ctx.drawImage(targetImage, 0, 0, canvas.width, canvas.height);
const imageData = canvas.toDataURL("image/png");
```

### C++/Qt 実装

- **PictureInPictureManager**: メイン管理クラス
- **MacOSPiPWindow**: macOS 特化の PiP ウィンドウ実装
- **WebView 統合**: JavaScript 実行とシグナル/スロット連携

### Base64 データ処理

- URL 方式から Base64 方式に変更
- ネットワーク依存を排除
- キャッシュされた画像内容を直接取得

## 対応ブラウザ機能

- [x] 画像要素の自動検出
- [x] ホバー/フォーカス状態の画像優先選択
- [x] 最大面積画像の自動選択
- [x] Base64 データでの画像キャプチャ
- [x] 元画像への PiP 状態表示
- [x] アニメーション効果
- [x] コンテナ要素の PiP 対応

## macOS 統合機能

- [x] NSPanel 使用の独立ウィンドウ
- [x] Spaces 間移動対応
- [x] フルスクリーン表示対応
- [x] Mission Control 表示対応
- [x] 常に最前面表示
- [x] ドラッグ移動対応
- [x] リサイズ対応

## 今後の拡張予定

- [ ] 動画要素の PiP 対応
- [ ] Web 要素全般の PiP 対応
- [ ] PiP ウィンドウの位置記憶機能
- [ ] 複数画像の一括 PiP 機能
- [ ] PiP ウィンドウのグループ化

## 注意事項

- macOS 専用機能（NSPanel 使用）
- Canvas 描画をサポートする Web ページでのみ完全動作
- 一部の Cross-Origin 画像では制限あり
- WebView 内での JavaScript 実行が必要

---

_MyBrowser Picture-in-Picture 機能 - macOS Spaces 完全対応_
