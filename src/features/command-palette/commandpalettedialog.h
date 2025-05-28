#ifndef COMMANDPALETTEDIALOG_H
#define COMMANDPALETTEDIALOG_H

#include <QDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>

class CommandPaletteDialog : public QDialog {
  Q_OBJECT

public:
  explicit CommandPaletteDialog(QWidget *parent = nullptr);
  ~CommandPaletteDialog();

  void setSearchHistory(const QStringList &history);
  void showCentered();

signals:
  void searchRequested(const QString &query);
  void commandRequested(const QString &command);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *object, QEvent *event) override;

private slots:
  void onTextChanged(const QString &text);
  void onItemClicked();
  void updateSuggestions();

private:
  void setupUI();
  void populateSuggestions(const QString &query);
  void populateCommands(const QString &query);
  void selectNextItem();
  void selectPreviousItem();
  void executeSelected();
  void executeSearch(const QString &query);
  void executeCommand(const QString &command);
  bool isHeaderItem(QListWidgetItem *item) const;

  QLineEdit *searchInput;
  QListWidget *suggestionsList;
  QVBoxLayout *mainLayout;
  QTimer *searchTimer;
  QStringList searchHistory;
  int selectedIndex;
};

#endif // COMMANDPALETTEDIALOG_H
