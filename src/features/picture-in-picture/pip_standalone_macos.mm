#include "pip_standalone_macos.h"

#ifdef Q_OS_MACOS

#import <Cocoa/Cocoa.h>
#import <CoreGraphics/CoreGraphics.h>
#include <QWidget>
#include <QWindow>
#include <QDebug>

void configureWindowForAllSpaces(QWidget* widget) {
    if (!widget) {
        qWarning() << "Widget is null, cannot configure for all spaces";
        return;
    }

    // ウィンドウハンドルを取得
    QWindow *qWindow = widget->windowHandle();
    if (!qWindow) {
        qWarning() << "QWindow handle is null, cannot configure for all spaces";
        return;
    }

    // NSViewとNSWindowを取得
    NSView *nsView = reinterpret_cast<NSView*>(qWindow->winId());
    if (!nsView) {
        qWarning() << "NSView is null, cannot configure for all spaces";
        return;
    }

    NSWindow *nsWindow = [nsView window];
    if (!nsWindow) {
        qWarning() << "NSWindow is null, cannot configure for all spaces";
        return;
    }

    // NSWindowのコレクション動作を設定してSpaces間で表示
    NSUInteger behavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                          NSWindowCollectionBehaviorStationary |
                          NSWindowCollectionBehaviorIgnoresCycle |
                          NSWindowCollectionBehaviorFullScreenAuxiliary;

    [nsWindow setCollectionBehavior:behavior];

    // 最高レベルのウィンドウレベルを設定（通常のウィンドウより上に表示）
    [nsWindow setLevel:kCGFloatingWindowLevel + 1];

    // 非アクティブ化時も非表示にしない
    [nsWindow setHidesOnDeactivate:NO];

    // フォーカスなしで表示
    [nsWindow makeKeyAndOrderFront:nil];

    qDebug() << "NSWindow configured for all Spaces:";
    qDebug() << "  - Collection behavior:" << behavior;
    qDebug() << "  - Window level:" << kCGFloatingWindowLevel + 1;
    qDebug() << "  - Hides on deactivate: NO";
}

void makeWindowFloatingAndSticky(QWidget* widget) {
    if (!widget) {
        return;
    }

    QWindow *qWindow = widget->windowHandle();
    if (!qWindow) {
        return;
    }

    NSView *nsView = reinterpret_cast<NSView*>(qWindow->winId());
    if (!nsView) {
        return;
    }

    NSWindow *nsWindow = [nsView window];
    if (!nsWindow) {
        return;
    }

    // より強力な設定でSpaces間表示を保証
    NSUInteger behavior = NSWindowCollectionBehaviorCanJoinAllSpaces |
                          NSWindowCollectionBehaviorStationary |
                          NSWindowCollectionBehaviorIgnoresCycle |
                          NSWindowCollectionBehaviorFullScreenAuxiliary |
                          NSWindowCollectionBehaviorMoveToActiveSpace;

    [nsWindow setCollectionBehavior:behavior];

    // さらに高いレベルを設定
    [nsWindow setLevel:kCGScreenSaverWindowLevel];

    // 常に最前面
    [nsWindow setHidesOnDeactivate:NO];
    [nsWindow orderFrontRegardless];

    qDebug() << "NSWindow configured as floating and sticky across Spaces";
}

bool isWindowVisibleOnCurrentSpace(QWidget* widget) {
    if (!widget) {
        return false;
    }

    QWindow *qWindow = widget->windowHandle();
    if (!qWindow) {
        return false;
    }

    NSView *nsView = reinterpret_cast<NSView*>(qWindow->winId());
    if (!nsView) {
        return false;
    }

    NSWindow *nsWindow = [nsView window];
    if (!nsWindow) {
        return false;
    }

    // ウィンドウが現在のSpaceで表示されているかチェック
    BOOL isVisible = [nsWindow isVisible] && [nsWindow isOnActiveSpace];

    qDebug() << "Window visibility on current space:" << (isVisible ? "YES" : "NO");

    return isVisible;
}

void debugWindowState(QWidget* widget) {
    if (!widget) {
        qDebug() << "Widget is null";
        return;
    }

    QWindow *qWindow = widget->windowHandle();
    if (!qWindow) {
        qDebug() << "QWindow handle is null";
        return;
    }

    NSView *nsView = reinterpret_cast<NSView*>(qWindow->winId());
    if (!nsView) {
        qDebug() << "NSView is null";
        return;
    }

    NSWindow *nsWindow = [nsView window];
    if (!nsWindow) {
        qDebug() << "NSWindow is null";
        return;
    }

    qDebug() << "=== NSWindow Debug Info ===";
    qDebug() << "  Window level:" << [nsWindow level];
    qDebug() << "  Collection behavior:" << [nsWindow collectionBehavior];
    qDebug() << "  Is visible:" << ([nsWindow isVisible] ? "YES" : "NO");
    qDebug() << "  Is on active space:" << ([nsWindow isOnActiveSpace] ? "YES" : "NO");
    qDebug() << "  Hides on deactivate:" << ([nsWindow hidesOnDeactivate] ? "YES" : "NO");
    qDebug() << "  Is key window:" << ([nsWindow isKeyWindow] ? "YES" : "NO");
    qDebug() << "  Is main window:" << ([nsWindow isMainWindow] ? "YES" : "NO");
    qDebug() << "=========================";
}

#else

// macOS以外では空の実装
void configureWindowForAllSpaces(QWidget*) {
    // 何もしない
}

void makeWindowFloatingAndSticky(QWidget*) {
    // 何もしない
}

bool isWindowVisibleOnCurrentSpace(QWidget*) {
    return true;
}

void debugWindowState(QWidget*) {
    // 何もしない
}

#endif // Q_OS_MACOS
