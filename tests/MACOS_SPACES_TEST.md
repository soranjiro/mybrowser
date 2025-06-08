# macOS Spaces 対応 PiP 機能テストガイド

このガイドでは、新しく実装された macOS Spaces 対応の Picture-in-Picture（PiP）機能をテストする手順を説明します。

## 前提条件

1. macOS で Mission Control の複数 Spaces が有効になっていること
2. MyBrowser アプリケーションが正常にビルドされていること

## テスト手順

### 1. 基本的な PiP 機能のテスト

1. MyBrowser を起動します
2. `pip_test_unified.html`ページが表示されることを確認
3. ページ内の画像をクリックします
4. 青い半透明の PiP ウィンドウが表示されることを確認

### 2. キーボードショートカットのテスト

1. ブラウザ内で `Ctrl+Alt+I` を押します
2. PiP ウィンドウが作成されることを確認
3. `Ctrl+Alt+X` を押します
4. すべての PiP ウィンドウが閉じることを確認

### 3. macOS Spaces 互換性のテスト

#### Spaces が有効な場合：

1. **PiP ウィンドウを作成**

   - 画像をクリックして PiP ウィンドウを表示

2. **新しい Space に移動**

   - `Control + →` または `Control + ←` で Space を切り替え
   - または Mission Control（F3）で新しい Space に移動

3. **PiP ウィンドウの表示確認**

   - PiP ウィンドウが新しい Space でも表示されていることを確認
   - ウィンドウが期待通りに「浮いている」状態であることを確認

4. **複数 Spaces での動作確認**
   - 3 つ以上の Space を作成
   - 各 Space 間を移動しながら PiP ウィンドウが常に表示されることを確認

### 4. PiP ウィンドウの操作テスト

1. **ドラッグ移動**

   - PiP ウィンドウをドラッグして移動できることを確認

2. **ダブルクリックで閉じる**

   - PiP ウィンドウをダブルクリックして閉じることを確認

3. **閉じるボタン**
   - 右上の「×」ボタンをクリックして閉じることを確認

## 期待される動作

### ✅ 成功ケース

- PiP ウィンドウがすべての Spaces で表示される
- ウィンドウレベルが適切に設定され、常に最前面に表示される
- ドラッグ移動が正常に動作する
- 閉じる操作が正常に動作する

### ❌ 失敗ケース（報告が必要）

- PiP ウィンドウが Space 切り替え時に非表示になる
- ウィンドウが他のアプリケーションの後ろに隠れる
- ドラッグ操作が機能しない

## 技術的詳細

この実装では以下の macOS 固有の機能を使用しています：

```objective-c
[window setCollectionBehavior:NSWindowCollectionBehaviorCanJoinAllSpaces |
                             NSWindowCollectionBehaviorStationary |
                             NSWindowCollectionBehaviorFullScreenAuxiliary];
[window setLevel:NSPopUpMenuWindowLevel];
```

- `NSWindowCollectionBehaviorCanJoinAllSpaces`: すべての Spaces で表示
- `NSWindowCollectionBehaviorStationary`: Space 作成時にウィンドウを移動させない
- `NSWindowCollectionBehaviorFullScreenAuxiliary`: フルスクリーン時も表示
- `NSPopUpMenuWindowLevel`: 高い優先度レベル

## トラブルシューティング

### PiP ウィンドウが表示されない

1. コンソールログを確認: `macOS PiP window behavior configured` が出力されているか
2. Qt 権限設定を確認
3. アプリケーションを再起動

### Spaces 切り替え時にウィンドウが消える

1. System Preferences > Mission Control の設定を確認
2. アプリケーションのウィンドウ権限を確認

## ログ出力例

正常な動作時のログ出力：

```
PictureInPictureManager initialized (simplified version)
PiP actions setup completed
macOS PiP window behavior configured
Loading image from URL: "demo://sample-image"
PiP window showing image: "Sample Image for PiP Test"
PiP window created for image: "Sample Image for PiP Test"
```

---

**注意**: このテストは実際の macOS 環境で Spaces が有効な状態で実行する必要があります。
