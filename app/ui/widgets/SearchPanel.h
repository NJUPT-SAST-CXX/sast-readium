#ifndef SEARCHPANEL_H
#define SEARCHPANEL_H

#include <QList>
#include <QString>
#include <QWidget>

// Forward declarations
class ElaLineEdit;
class ElaComboBox;
class ElaCheckBox;
class ElaPushButton;
class ElaListView;
class QListWidget;
class SearchEngine;
class SearchResult;

/**
 * @brief ElaSearchPanel - 搜索面板组件
 *
 * 功能：
 * - 全文搜索
 * - 正则表达式搜索
 * - 大小写敏感选项
 * - 全词匹配选项
 * - 搜索结果列表
 * - 结果导航（上一个/下一个）
 *
 * 复用现有业务逻辑：
 * - SearchEngine - 搜索引擎
 */
class SearchPanel : public QWidget {
    Q_OBJECT

public:
    explicit SearchPanel(QWidget* parent = nullptr);
    ~SearchPanel() override;

    // 搜索控制
    void startSearch(const QString& query);
    void stopSearch();
    void clearResults();

    // 选项
    void setCaseSensitive(bool enabled);
    void setWholeWords(bool enabled);
    void setRegexEnabled(bool enabled);

    // 业务逻辑集成
    void setSearchEngine(SearchEngine* engine);

signals:
    void searchRequested(const QString& query, bool caseSensitive,
                         bool wholeWords, bool regex);
    void resultSelected(int pageNumber, int resultIndex);
    void nextResultRequested();
    void previousResultRequested();

protected:
    void changeEvent(QEvent* event) override;

private:
    // UI 组件
    ElaLineEdit* m_searchInput;
    ElaPushButton* m_searchBtn;
    ElaPushButton* m_clearBtn;
    ElaCheckBox* m_caseSensitiveCheck;
    ElaCheckBox* m_wholeWordsCheck;
    ElaCheckBox* m_regexCheck;
    QListWidget* m_resultsList;
    ElaPushButton* m_prevBtn;
    ElaPushButton* m_nextBtn;

    // 业务逻辑
    SearchEngine* m_searchEngine;

    // 状态
    QList<SearchResult> m_results;
    int m_currentResultIndex;

    void setupUi();
    void connectSignals();
    void retranslateUi();
    void updateResultsList();
};

#endif  // SEARCHPANEL_H
