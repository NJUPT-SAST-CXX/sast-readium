#ifndef SEARCHADAPTER_H
#define SEARCHADAPTER_H

#include <QList>
#include <QObject>
#include <QRectF>
#include <QString>

// Forward declarations
class SearchEngine;
class PDFViewerPage;
class SearchResult;

/**
 * @brief SearchAdapter - 搜索引擎适配器
 *
 * 桥接 ElaWidgetTools UI 和现有的 SearchEngine
 * 负责：
 * - 搜索执行
 * - 搜索结果管理
 * - 搜索导航
 */
class SearchAdapter : public QObject {
    Q_OBJECT

public:
    explicit SearchAdapter(QObject* parent = nullptr);
    ~SearchAdapter() override;

    void setSearchEngine(SearchEngine* engine);
    void setPDFViewerPage(PDFViewerPage* page);

public slots:
    void search(const QString& query, bool caseSensitive, bool wholeWords,
                bool regex);
    void stopSearch();
    void clearResults();
    void goToNextResult();
    void goToPreviousResult();
    void goToResult(int index);

signals:
    void searchStarted();
    void searchFinished(int resultCount);
    void searchProgress(int current, int total);
    void resultFound(int pageNumber, const QList<QRectF>& highlights);
    void currentResultChanged(int index, int total);
    void errorOccurred(const QString& error);

private:
    SearchEngine* m_searchEngine;
    PDFViewerPage* m_pdfViewerPage;
    QList<SearchResult> m_results;
    int m_currentResultIndex;

    void connectEngineSignals();
    void updateCurrentResult();
};

#endif  // SEARCHADAPTER_H
