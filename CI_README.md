# MyBrowser Build Status

![CI Build](https://github.com/USER/mybrowser/workflows/CI%20Build/badge.svg)

## 🔧 Continuous Integration

このプロジェクトは GitHub Actions を使用して自動ビルドテストを行っています。

### 🚦 CI トリガー

- **main ブランチへの push**: すべてのプラットフォームでビルドテスト実行
- **main ブランチへの PR**: ビルドテスト + コード品質チェック実行

### 🖥️ サポートプラットフォーム

| OS                | Debug Build | Release Build |
| ----------------- | ----------- | ------------- |
| 🐧 Ubuntu Latest  | ✅          | ✅            |
| 🍎 macOS Latest   | ✅          | ✅            |
| 🪟 Windows Latest | ✅          | ✅            |

### 📦 依存関係

- **Qt6** (WebEngine, Widgets)
- **CMake** 3.16+
- **C++17** コンパイラ

### 🔍 CI チェック項目

1. **ビルドテスト**

   - Debug/Release 両方のビルド成功確認
   - 実行ファイルの生成確認
   - 全プラットフォームでの互換性確認

2. **コード品質** (PR のみ)

   - ソースコード構造チェック
   - CMakeLists.txt 構文確認
   - ビルドスクリプト検証

3. **アーティファクト**
   - ビルド成功時、実行ファイルを 7 日間保存
   - プラットフォーム別にダウンロード可能

### 🚀 手動ビルド

ローカルでのビルドは以下のスクリプトを使用：

```bash
# Debug ビルド
./build_debug.sh

# Release ビルド
./build_release.sh
```

### 📋 CI 設定ファイル

- `.github/workflows/ci.yml` - GitHub Actions ワークフロー設定
- `CMakeLists.txt` - CMake ビルド設定
- `build_debug.sh` / `build_release.sh` - ビルドスクリプト
