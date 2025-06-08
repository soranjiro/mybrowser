#ifndef PIP_STANDALONE_H
#define PIP_STANDALONE_H

#include <QApplication>
#include <QDataStream>
#include <QDebug>
#include <QGraphicsDropShadowEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLocalServer>
#include <QLocalSocket>
#include <QPushButton>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

#ifdef Q_OS_MACOS
#include <QWindow>
#endif

/**
 * @brief 独立プロセスで動作するPiPウィンドウ
 *
 * このクラスは：
 * - 別プロセスとして動作
 * - macOSのSpaces間で独立表示
 * - ソケット通信でメインアプリと連携
 */
class PiPStandaloneWindow : public QWidget {
  Q_OBJECT

public:
  explicit PiPStandaloneWindow(QWidget *parent = nullptr);
  ~PiPStandaloneWindow();

  void setupServer();
  void displayImage(const QByteArray &imageData, const QString &title);

protected:
  void mousePressEvent(QMouseEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  void mouseReleaseEvent(QMouseEvent *event) override;
  void paintEvent(QPaintEvent *event) override;
  void showEvent(QShowEvent *event) override;

private slots:
  void onNewConnection();
  void onDataReceived();
  void onCloseButtonClicked();

private:
  void processJsonData(const QByteArray &data);
  void processQDataStreamData(const QByteArray &data);

private:
  void setupUI();
  void setupStyles();
  void setupMacOSBehavior();

  // UI要素
  QVBoxLayout *mainLayout;
  QHBoxLayout *titleBarLayout;
  QWidget *titleBar;
  QLabel *titleLabel;
  QPushButton *closeButton;
  QLabel *imageLabel;

  // サーバー通信
  QLocalServer *server;
  QLocalSocket *clientSocket;

  // ドラッグ機能
  bool isDragging;
  QPoint dragStartPosition;

  // エフェクト
  QGraphicsDropShadowEffect *shadowEffect;

  static const QString SERVER_NAME;
};

#endif // PIP_STANDALONE_H
