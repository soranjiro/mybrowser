#ifndef COMMANDPALETTEMANAGER_H
#define COMMANDPALETTEMANAGER_H

#include <QAction>
#include <QObject>
#include <QStringList>

class MainWindow;
class CommandPaletteDialog;

/**
 * @brief コマンドパレット機能を管理するクラス
 *
 * このクラスは以下の機能を提供します：
 * - コマンドパレットの作成と管理
 * - 検索履歴の管理
 * - コマンドの実行
 * - キーボードショートカットの設定
 */
class CommandPaletteManager : public QObject {
  Q_OBJECT

public:
  explicit CommandPaletteManager(MainWindow *parent = nullptr);
  ~CommandPaletteManager();

  // アクションの設定
  void setupActions();

  // コマンドパレットの表示
  void showCommandPalette();
#ifdef QT_DEBUG
  void openTestPage();
#endif

  // 検索履歴の管理
  void addToSearchHistory(const QString &query);
  void clearSearchHistory();
  QStringList getSearchHistory() const { return searchHistory; }

  // アクションの取得
  QAction *getQuickSearchAction() const { return quickSearchAction; }
  QAction *getCommandPaletteAction() const { return commandPaletteAction; }
#ifdef QT_DEBUG
  QAction *getOpenTestPageAction() const { return openTestPageAction; }
#endif

public slots:
  void handleQuickSearch(const QString &query);
  void handleQuickSearch(const QString &query, bool inNewTab);
  void handleCommand(const QString &command);

private slots:
  void onQuickSearchTriggered();
#ifdef QT_DEBUG
  void onOpenTestPageTriggered();
#endif

private:
  MainWindow *mainWindow;
  CommandPaletteDialog *commandPaletteDialog;
  QAction *quickSearchAction;
  QAction *commandPaletteAction;
#ifdef QT_DEBUG
  QAction *openTestPageAction;
#endif
  QStringList searchHistory;

  // 検索履歴の永続化
  void saveSearchHistory();
  void loadSearchHistory();

  // コマンド実行
  void executeNavigationCommand(const QString &command);
  void executeZoomCommand(const QString &command);
  void executeBookmarkCommand(const QString &command);
  void executeWorkspaceCommand(const QString &command);
  void executeHistoryCommand(const QString &command);
  void executeDeveloperCommand(const QString &command);
  void executePageCommand(const QString &command);
  void executeWindowCommand(const QString &command);
  void executeSettingsCommand(const QString &command);
};

#endif // COMMANDPALETTEMANAGER_H
