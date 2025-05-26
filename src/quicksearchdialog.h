#ifndef QUICKSEARCHDIALOG_H
#define QUICKSEARCHDIALOG_H

#include <QCompleter>
#include <QDialog>
#include <QKeyEvent>
#include <QLineEdit>
#include <QListWidget>
#include <QPropertyAnimation>
#include <QTimer>
#include <QVBoxLayout>

class QuickSearchDialog : public QDialog {
  Q_OBJECT

public:
  explicit QuickSearchDialog(QWidget *parent = nullptr);
  QString getSearchQuery() const;
  void setSearchHistory(const QStringList &history);

signals:
  void searchRequested(const QString &query);

protected:
  void keyPressEvent(QKeyEvent *event) override;
  void showEvent(QShowEvent *event) override;

private slots:
  void onTextChanged(const QString &text);
  void onSuggestionClicked();
  void updateSuggestions();

private:
  void setupUI();
  void populateSuggestions(const QString &query);
  void selectNextSuggestion();
  void selectPreviousSuggestion();
  void executeSearch();
  void animateResize(int newHeight);

  QLineEdit *searchLineEdit;
  QListWidget *suggestionsWidget;
  QVBoxLayout *mainLayout;
  QTimer *searchTimer;
  QPropertyAnimation *resizeAnimation;
  QPropertyAnimation *fadeAnimation;
  QStringList searchHistory;
  QStringList currentSuggestions;
  int selectedSuggestionIndex;
};

#endif // QUICKSEARCHDIALOG_H
