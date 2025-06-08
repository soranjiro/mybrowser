#!/usr/bin/env python3
"""
スタンドアロンPiPのIPC通信テスト用スクリプト
"""
import socket
import json
import sys
import base64
from pathlib import Path


def create_test_image_data():
    """テスト用の小さなPNG画像データを作成"""
    # 1x1の透明PNG（最小限のPNGファイル）
    png_data = base64.b64decode(
        "iVBORw0KGgoAAAANSUhEUgAAAAEAAAABCAYAAAAfFcSJAAAADUlEQVR42mNk+M9QDwADhgGAWjR9awAAAABJRU5ErkJggg=="
    )
    return base64.b64encode(png_data).decode("utf-8")


def test_pip_ipc():
    """スタンドアロンPiPとのIPC通信をテスト"""
    server_name = "MyBrowser_PiP_IPC"

    try:
        # Unix domain socketで接続
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        # macOSのQLocalServerは/private/var/folders内の一時ディレクトリを使用
        import tempfile

        temp_dir = tempfile.gettempdir()
        socket_path = f"{temp_dir}/{server_name}"

        print(f"🔗 スタンドアロンPiPサーバーに接続試行: {socket_path}")

        sock.connect(socket_path)
        print("✅ 接続成功！")

        # テスト画像データを送信
        test_data = {
            "type": "display_image",
            "title": "IPC Test Image",
            "imageData": create_test_image_data(),
        }

        message = json.dumps(test_data).encode("utf-8")
        print(f"送信データ: {message[:100]}...")

        print(f"📤 データ送信中... ({len(message)} bytes)")
        # 長さプレフィックスなしで直接JSONを送信
        sock.send(message)

        print("✅ 画像データ送信完了")
        print("💡 スタンドアロンPiPウィンドウで画像が表示されるはずです")

        sock.close()

    except FileNotFoundError:
        print("❌ エラー: スタンドアロンPiPサーバーが見つかりません")
        print("💡 まず ./PiPStandalone を起動してください")
    except ConnectionRefusedError:
        print("❌ エラー: 接続が拒否されました")
        print("💡 スタンドアロンPiPプロセスが起動していることを確認してください")
    except Exception as e:
        print(f"❌ エラー: {e}")


if __name__ == "__main__":
    print("🧪 スタンドアロンPiP IPC通信テスト")
    print("=" * 50)
    test_pip_ipc()
