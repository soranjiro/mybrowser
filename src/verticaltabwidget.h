#ifndef VERTICALTABWIDGET_H
#define VERTICALTABWIDGET_H

#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QListWidgetItem>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <QVBoxLayout>
#include <QWidget>

class WebView;

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

signals:
  void currentChanged(int index);
  void tabCloseRequested(int index);

private slots:
  void onTabListItemChanged(QListWidgetItem *current, QListWidgetItem *previous);
  void onCloseTabClicked();
  void onNewTabClicked();

private:
  void setupUI();
  void updateTabList();
  QListWidgetItem *createTabItem(const QString &text, int index);

  QHBoxLayout *mainLayout;
  QVBoxLayout *tabListLayout;
  QListWidget *tabListWidget;
  QStackedWidget *contentWidget;
  QPushButton *newTabButton;

  bool tabsClosable;
  bool movable;
  QList<QWidget *> tabWidgets;
  QList<QString> tabTexts;
};

#endif // VERTICALTABWIDGET_H
