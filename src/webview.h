#ifndef WEBVIEW_H
#define WEBVIEW_H

#include <QWebEnginePage>
#include <QWebEngineProfile>
#include <QWebEngineView>

class WebView : public QWebEngineView {
  Q_OBJECT

public:
  WebView(QWidget *parent = nullptr);
  void setPage(QWebEnginePage *page); // Allow setting a custom page if needed

protected:
  QWebEngineView *createWindow(QWebEnginePage::WebWindowType type) override;

signals:
  // Forward signals from QWebEnginePage if needed, or connect directly in MainWindow
  void titleChanged(const QString &title);
  void urlChanged(const QUrl &url);
  void loadProgress(int progress);
  void loadFinished(bool ok);
  void loadStarted();
};

#endif // WEBVIEW_H
