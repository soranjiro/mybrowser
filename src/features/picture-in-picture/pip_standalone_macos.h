#ifndef PIP_STANDALONE_MACOS_H
#define PIP_STANDALONE_MACOS_H

class QWidget;

/**
 * @brief macOS固有のウィンドウ設定関数
 *
 * これらの関数はObjective-C++で実装され、NSWindow APIを使用して
 * macOSのSpaces間でのウィンドウ表示を制御します。
 */

/**
 * @brief ウィンドウをすべてのSpacesで表示するよう設定
 * @param widget 設定対象のQWidget
 */
void configureWindowForAllSpaces(QWidget *widget);

/**
 * @brief ウィンドウを浮動表示かつすべてのSpacesに固定
 * @param widget 設定対象のQWidget
 */
void makeWindowFloatingAndSticky(QWidget *widget);

/**
 * @brief ウィンドウが現在のSpaceで表示されているかチェック
 * @param widget チェック対象のQWidget
 * @return 表示されている場合true
 */
bool isWindowVisibleOnCurrentSpace(QWidget *widget);

/**
 * @brief ウィンドウの状態をデバッグ出力
 * @param widget デバッグ対象のQWidget
 */
void debugWindowState(QWidget *widget);

#endif // PIP_STANDALONE_MACOS_H
