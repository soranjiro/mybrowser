#ifndef QUICKSEARCHDIALOG_H
#define QUICKSEARCHDIALOG_H

#include <QCompleter>
#include <QDialog>
#include <QEvent>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QMouseEvent>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>

class QuickSearchDialog : public QDialog {
  Q_OBJECT

public:
  explicit QuickSearchDialog(QWidget *parent = nullptr);
  ~QuickSearchDialog(); // Add destructor
  QString getSearchQuery() const;
  void setSearchHistory(const QStringList &history);

signals:
  void searchRequested(const QString &query);
  void commandRequested(const QString &command);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void showEvent(QShowEvent *event) override;
  bool eventFilter(QObject *object, QEvent *event) override;

private slots:
  void onTextChanged(const QString &text);
  void onSuggestionClicked();
  void updateSuggestions();

private:
  void setupUI();
  void populateSuggestions(const QString &query);
  void populateCommands(const QString &query);
  void selectNextSuggestion();
  void selectPreviousSuggestion();
  void executeSearch();
  void executeCommand(const QString &command);
  void animateResize(int newHeight);

  QLineEdit *searchLineEdit;
  QListWidget *suggestionsWidget;
  QVBoxLayout *mainLayout;
  QTimer *searchTimer;
  QPropertyAnimation *resizeAnimation;
  QPropertyAnimation *fadeAnimation;
  QStringList searchHistory;
  QStringList currentSuggestions;
  QStringList currentCommands;
  int selectedSuggestionIndex;
};

#endif // QUICKSEARCHDIALOG_H
