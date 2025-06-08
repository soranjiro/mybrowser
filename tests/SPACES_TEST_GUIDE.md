# macOS Spaces 対応 スタンドアロン PiP テストガイド

## 🎯 テスト目標

スタンドアロン PiP ウィンドウが macOS の Spaces 間で常に表示されることを確認

## ✅ 完了済み

1. スタンドアロン PiP プロセスの実装
2. Objective-C++による NSWindow API 統合
3. IPC 通信による画像表示機能
4. macOS 固有のウィンドウ設定（NSWindowCollectionBehavior）

## 🧪 実行中のプロセス

- **メインブラウザ**: ./MyBrowser（Port: 9222）
- **スタンドアロン PiP**: ./PiPStandalone（IPC: MyBrowser_PiP_IPC）
- **テスト画像**: IPC Test Image（1x1 PNG、70 bytes）

## 📋 Spaces テスト手順

### ステップ 1: Mission Control でテスト環境準備

1. **Mission Control**を開く（F3 キーまたは 3 本指で上にスワイプ）
2. 画面上部の「+」をクリックして新しい Desktop を作成
3. 最低 2 つの Space があることを確認

### ステップ 2: PiP ウィンドウの状態確認

現在の PiP ウィンドウの設定:

```
NSWindowCollectionBehavior:
- CanJoinAllSpaces: 有効
- Stationary: 有効
- IgnoresCycle: 有効
- FullScreenAuxiliary: 有効

ウィンドウレベル: kCGFloatingWindowLevel + 1
```

### ステップ 3: Space 間移動テスト

1. **Space 1**で PiP ウィンドウが表示されていることを確認
2. **Control + →**（または 3 本指で右にスワイプ）で Space 2 に移動
3. PiP ウィンドウが Space 2 でも表示されることを確認
4. **Control + ←**で Space 1 に戻る
5. PiP ウィンドウが引き続き表示されることを確認

### ステップ 4: ウィンドウのフォーカステスト

1. Space 内の他のアプリをクリック
2. PiP ウィンドウが隠れないことを確認
3. PiP ウィンドウをドラッグして移動可能なことを確認

## 🔍 デバッグ情報

ターミナルで以下の情報を確認:

```
独立プロセスPiP: Objective-C++によるSpaces設定完了
showEvent: macOS Spaces設定を再適用しました
displayImage: macOS Spaces設定を再適用しました
```

## 🚀 追加テスト

### より大きな画像でテスト

```bash
cd /Users/user/Documents/03_app/mybrowser
python3 tests/test_pip_ipc.py
```

### メインブラウザからの統合テスト

1. メインブラウザで`Ctrl+Alt+S`を押す
2. テストページで画像をクリック
3. スタンドアロン PiP に画像が表示されることを確認

## 📊 期待される動作

- ✅ PiP ウィンドウが全ての Space で表示される
- ✅ Space 切り替え時にウィンドウが消えない
- ✅ 他のアプリのフォーカスでウィンドウが隠れない
- ✅ ドラッグ＆ドロップで移動可能
- ✅ NSWindow API による適切なレベル設定

## 🐛 問題が発生した場合

1. プロセスを再起動: `pkill -f PiPStandalone && ./PiPStandalone`
2. ログを確認: ターミナル出力をチェック
3. ウィンドウ状態をデバッグ: `debugWindowState()`関数が呼び出される

## 🎉 成功の確認

Space 間を移動しても PiP ウィンドウが常に見える状態が達成されれば、
macOS Spaces 対応のスタンドアロン PiP 実装が成功です！
