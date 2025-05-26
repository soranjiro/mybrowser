#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QAction>
#include <QContextMenuEvent>
#include <QGestureEvent>
#include <QKeyEvent>
#include <QMenu>
#include <QSwipeGesture>
#include <QWebEngineHistory>
#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>

// Custom page class to handle JavaScript console messages
class CustomWebEnginePage : public QWebEnginePage {
  Q_OBJECT

public:
  CustomWebEnginePage(QWebEngineProfile *profile, QObject *parent = nullptr);

protected:
  void javaScriptConsoleMessage(JavaScriptConsoleMessageLevel level, const QString &message, int lineNumber, const QString &sourceID) override;
};

class WebView : public QWebEngineView {
  Q_OBJECT

public:
  WebView(QWidget *parent = nullptr);
  ~WebView();
  void setPage(QWebEnginePage *page); // Allow setting a custom page if needed

public slots:
  void showDevTools(); // Show developer tools

protected:
  QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;
  bool event(QEvent *event) override;
  bool gestureEvent(QGestureEvent *event);
  void swipeTriggered(QSwipeGesture *gesture);
  void keyPressEvent(QKeyEvent *event) override;
  void contextMenuEvent(QContextMenuEvent *event) override;

private:
  QWebEngineView *devToolsView; // Developer tools window

signals:
  // Forward signals from QWebEnginePage if needed, or connect directly in MainWindow
  void titleChanged(const QString &title);
  void urlChanged(const QUrl &url);
  void loadProgress(int progress);
  void loadFinished(bool ok);
  void loadStarted();
};

#endif // WEBVIEW_H
