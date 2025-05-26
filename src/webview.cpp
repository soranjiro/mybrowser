#include "webview.h"
#include "mainwindow.h" // To potentially access MainWindow for new tab creation logic

WebView::WebView(QWidget *parent) : QWebEngineView(parent) {
  // Basic settings, can be expanded
  QWebEnginePage *defaultPage = new QWebEnginePage(QWebEngineProfile::defaultProfile(), this);
  setPage(defaultPage);

  // Forward signals that MainWindow might be interested in
  connect(page(), &QWebEnginePage::titleChanged, this, &WebView::titleChanged);
  connect(page(), &QWebEnginePage::urlChanged, this, &WebView::urlChanged);
  connect(page(), &QWebEnginePage::loadProgress, this, &WebView::loadProgress);
  connect(page(), &QWebEnginePage::loadFinished, this, &WebView::loadFinished);
  connect(page(), &QWebEnginePage::loadStarted, this, &WebView::loadStarted);

  // Enable developer tools if needed (Ctrl+Shift+I)
  // page()->setProperty("_q_webEngineDevToolsPort", QVariant::fromValue(QString("localhost:0")));
}

void WebView::setPage(QWebEnginePage *page) {
  QWebEngineView::setPage(page);
}

// This function is called when a link requests to be opened in a new window
// (e.g., target="_blank" or JavaScript window.open())
QWebEngineView *WebView::createWindow(QWebEnginePage::WebWindowType type) {
  // Find the MainWindow instance to call its newTab method
  MainWindow *mainWindow = nullptr;
  QWidget *parent = this->parentWidget();
  while (parent) {
    mainWindow = qobject_cast<MainWindow *>(parent);
    if (mainWindow)
      break;
    parent = parent->parentWidget();
  }

  if (mainWindow) {
    // Create a new tab in the main window
    mainWindow->newTab(); // This creates a new WebView and sets it as current
    WebView *newView = mainWindow->currentWebView();

    // For pop-ups, we might want to handle them differently,
    // but for now, just open them in a new tab.
    // if (type == QWebEnginePage::WebBrowserWindow || type == QWebEnginePage::WebDialog) {
    // }
    return newView;
  }
  return nullptr; // Should not happen if embedded in MainWindow
}
