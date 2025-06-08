#ifdef Q_OS_MACOS

#include "pipwindow.h"
#include <QWindow>
#include <QDebug>

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>

void PiPWindow::setupMacOSWindowBehavior() {
    // まずウィンドウを表示してNSWindowを取得可能にする
    setAttribute(Qt::WA_ShowWithoutActivating);
    show();

    // QWindowからNSWindowを取得
    if (QWindow *qWindow = windowHandle()) {
        NSView *nsView = reinterpret_cast<NSView *>(qWindow->winId());
        NSWindow *nsWindow = [nsView window];

        if (nsWindow) {
            // ウィンドウレベルを設定（全てのSpacesで表示）
            [nsWindow setLevel:kCGFloatingWindowLevel];

            // Collection Behaviorを設定してSpaces間での表示を制御
            NSUInteger behavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                                 NSWindowCollectionBehaviorStationary |
                                 NSWindowCollectionBehaviorIgnoresCycle |
                                 NSWindowCollectionBehaviorFullScreenAuxiliary;
            [nsWindow setCollectionBehavior:behavior];

            // デアクティベーション時に隠れないようにする
            [nsWindow setHidesOnDeactivate:NO];

            qDebug() << "macOS window behavior configured for PiP across Spaces";
        }
    }
}

#endif // Q_OS_MACOS
