# Test Pages

このディレクトリにはブラウザ機能をテストするためのテストページが含まれています。

## Test Pages

### `click_test.html`

包括的なクリック機能テスト:

- ボタンクリック機能テスト
- リンククリック機能テスト（通常、JavaScript、内部、外部）
- stopPropagation を使った入れ子要素のクリックイベントテスト
- マウスイベントのデバッグとログ出力
- インタラクションの視覚的フィードバック

### `pip_test_unified.html` ⭐ **NEW - 統合テストページ**

**macOS Spaces 対応 Picture-in-Picture 機能**の包括的テスト:

- カスタム PiP 実装のテスト（`MacOSPiPWindow`）
- macOS 仮想ワークスペース（Spaces）互換性
- 画像ベースの PiP 機能
- キーボードショートカット（`Ctrl+Alt+I`, `Ctrl+Alt+X`）
- JavaScript 連携とイベントハンドリング
- ドラッグ&ドロップ機能
- リアルタイムログ出力

### macOS Spaces 互換性テスト

詳細な手順は `MACOS_SPACES_TEST.md` を参照してください。

## 廃止されたファイル（backup/フォルダに移動済み）

- `pip_test.html` - 基本的な PiP テスト
- `native_pip_test.html` - ネイティブ PiP テスト
- `pip_independent_test.html` - 独立 PiP テスト
- `standalone_pip_test.html` - スタンドアロン PiP テスト

## 使用方法

1. MyBrowser をビルドして実行
2. file://URL を使ってこれらのテストページにアクセス
3. 各ページで説明されている特定の機能をテスト
4. ブラウザコンソールでデバッグ出力を確認

## 新しい PiP 機能の特徴

### 🎯 簡素化されたアーキテクチャ

- 9 個以上の PiP アクションから 1 個の`createImagePiP()`メソッドに簡素化
- 複雑なビデオ PiP 機能を削除し、画像表示に特化

### 🖥️ macOS Spaces 互換性

- `NSWindowCollectionBehavior`を使用した Spaces 対応
- すべての仮想ワークスペースで PiP ウィンドウが表示
- `NSPopUpMenuWindowLevel`による高優先度表示

### ⌨️ 簡素化されたキーボードショートカット

- `Ctrl+Alt+I`: 画像 PiP 作成
- `Ctrl+Alt+X`: すべての PiP ウィンドウを閉じる

### 🔧 技術的改善

- Objective-C++（`.mm`）ファイルで macOS API との適切な統合
- Qt6 の新しい API に対応（`globalPosition()`など）
- シンプルで保守しやすいコードベース

## 注意事項

- 一部のテストには特定の Chromium フラグが必要（main.cpp で自動設定）
- PiP 機能は Qt WebEngine で制限がある場合があります
- テスト結果は Qt と Chromium のバージョンによって異なる場合があります
- **新しい PiP 機能は macOS 専用です**
