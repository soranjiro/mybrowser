#ifndef UI_CONSTANTS_H
#define UI_CONSTANTS_H

#include <QString>

namespace UIConstants {
// Color scheme
const QString PRIMARY_COLOR = "#007ACC";
const QString PRIMARY_HOVER = "#005a9e";
const QString PRIMARY_PRESSED = "#004578";
const QString DANGER_COLOR = "#dc3545";
const QString DANGER_HOVER = "#c82333";
const QString DANGER_PRESSED = "#bd2130";

// Background colors
const QString DARK_BG = "#2d2d30";
const QString LIGHT_BG = "#f8f9fa";
const QString WHITE_BG = "#ffffff";
const QString BORDER_COLOR = "#d1d5db";

// Typography
const QString DEFAULT_FONT_SIZE = "13px";
const QString LARGE_FONT_SIZE = "14px";
const QString SMALL_FONT_SIZE = "12px";

// Dimensions
const int TAB_WIDGET_WIDTH = 250;
const int TAB_ITEM_HEIGHT = 40;
const int BUTTON_PADDING = 8;
const int BORDER_RADIUS = 6;
} // namespace UIConstants

#endif // UI_CONSTANTS_H
