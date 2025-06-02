#ifndef VERTICALTABWIDGET_H
#define VERTICALTABWIDGET_H

#include <QComboBox>
#include <QCursor>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPropertyAnimation>
#include <QPushButton>
#include <QResizeEvent>
#include <QStackedWidget>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class WebView;
class WorkspaceManager;
class BookmarkManager;

class VerticalTabWidget : public QWidget {
  Q_OBJECT

public:
  explicit VerticalTabWidget(QWidget *parent = nullptr);
  ~VerticalTabWidget();

  int addTab(QWidget *widget, const QString &text);
  void removeTab(int index);
  void setCurrentIndex(int index);
  int currentIndex() const;
  QWidget *currentWidget() const;
  QWidget *widget(int index) const;
  int count() const;
  void setTabText(int index, const QString &text);
  QString tabText(int index) const;
  void setTabsClosable(bool closable);
  void setMovable(bool movable);

  // Getter methods for layout
  QListWidget *getTabList() const { return tabListWidget; }
  QStackedWidget *getStackedWidget() const { return contentWidget; }
  QLineEdit *getIntegratedAddressBar() const { return integratedAddressBar; }

  // New methods for integrated UI
  void setWorkspaceManager(WorkspaceManager *manager);
  void setBookmarkManager(BookmarkManager *manager);
  void setAddressBar(QLineEdit *addressBar);
  void showSidebar();
  void hideSidebar();
  bool isSidebarVisible() const { return sidebarVisible; }

signals:
  void currentChanged(int index);
  void tabCloseRequested(int index);
  void addressBarReturnPressed();
  void newTabRequested();

private slots:
  void onTabListItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
  void onCloseTabClicked();
  void onNewTabClicked();
  void onSidebarTimerTimeout();

protected:
  void enterEvent(QEnterEvent *event) override;
  void leaveEvent(QEvent *event) override;
  void resizeEvent(QResizeEvent *event) override;
  void mouseMoveEvent(QMouseEvent *event) override;
  bool eventFilter(QObject *obj, QEvent *event) override;

private:
  void setupUI();
  void setupSidebar();
  void updateTabList();
  QListWidgetItem *createTabItem(const QString &text, int index);

  QHBoxLayout *mainLayout;
  QVBoxLayout *tabListLayout;
  QListWidget *tabListWidget;
  QStackedWidget *contentWidget;
  QPushButton *newTabButton;

  // Sidebar components
  QWidget *sidebarWidget;
  QLineEdit *integratedAddressBar;
  QWidget *workspaceToolbar;
  QWidget *bookmarkPanel;

  // Animation and overlay
  QPropertyAnimation *sidebarAnimation;
  QTimer *hideTimer;
  bool sidebarVisible;

  // Managers
  WorkspaceManager *workspaceManager;
  BookmarkManager *bookmarkManager;

  bool tabsClosable;
  bool movable;
  QList<QWidget *> tabWidgets;
  QList<QString> tabTexts;
};

#endif // VERTICALTABWIDGET_H
