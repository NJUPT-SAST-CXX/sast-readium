#include "SearchAdapter.h"

// Search
#include "search/SearchConfiguration.h"
#include "search/SearchEngine.h"

// Pages
#include "ui/pages/PDFViewerPage.h"

// Logging
#include "logging/SimpleLogging.h"

SearchAdapter::SearchAdapter(QObject* parent)
    : QObject(parent),
      m_searchEngine(nullptr),
      m_pdfViewerPage(nullptr),
      m_currentResultIndex(-1) {
    SLOG_INFO("SearchAdapter: Constructor");
}

SearchAdapter::~SearchAdapter() { SLOG_INFO("SearchAdapter: Destructor"); }

void SearchAdapter::setSearchEngine(SearchEngine* engine) {
    if (m_searchEngine) {
        disconnect(m_searchEngine, nullptr, this, nullptr);
    }

    m_searchEngine = engine;

    if (m_searchEngine) {
        connectEngineSignals();
    }
}

void SearchAdapter::setPDFViewerPage(PDFViewerPage* page) {
    m_pdfViewerPage = page;
}

void SearchAdapter::connectEngineSignals() {
    if (!m_searchEngine) {
        return;
    }

    // 连接搜索引擎的信号
    connect(m_searchEngine, &SearchEngine::searchStarted, this, [this]() {
        SLOG_INFO("SearchAdapter: Search started");
        m_results.clear();
        m_currentResultIndex = -1;
        emit searchStarted();
    });

    connect(m_searchEngine, &SearchEngine::searchFinished, this,
            [this](const QList<SearchResult>& results) {
                SLOG_INFO_F("SearchAdapter: Search finished with {} results",
                            static_cast<int>(results.size()));
                m_results = results;
                m_currentResultIndex = results.isEmpty() ? -1 : 0;
                emit searchFinished(results.size());

                if (!results.isEmpty()) {
                    updateCurrentResult();
                }
            });

    connect(m_searchEngine, &SearchEngine::searchProgress, this,
            [this](int current, int total) {
                emit searchProgress(current, total);
            });

    connect(m_searchEngine, &SearchEngine::searchError, this,
            [this](const QString& error) {
                SLOG_ERROR_F("SearchAdapter: Search error: {}",
                             error.toStdString());
                emit errorOccurred(error);
            });
}

void SearchAdapter::search(const QString& query, bool caseSensitive,
                           bool wholeWords, bool regex) {
    SLOG_INFO_F(
        "SearchAdapter: Searching for: {} (caseSensitive: {}, wholeWords: {}, "
        "regex: {})",
        query.toStdString(), caseSensitive, wholeWords, regex);

    if (!m_searchEngine) {
        SLOG_ERROR("SearchAdapter: SearchEngine is null");
        emit errorOccurred(tr("Search engine not initialized"));
        return;
    }

    if (query.isEmpty()) {
        SLOG_ERROR("SearchAdapter: Search query is empty");
        emit errorOccurred(tr("Search query is empty"));
        return;
    }

    // 创建搜索选项
    SearchOptions options;
    options.caseSensitive = caseSensitive;
    options.wholeWords = wholeWords;
    options.useRegex = regex;

    // 使用搜索引擎执行搜索
    m_searchEngine->search(query, options);
}

void SearchAdapter::stopSearch() {
    SLOG_INFO("SearchAdapter: Stopping search");

    if (!m_searchEngine) {
        SLOG_ERROR("SearchAdapter: SearchEngine is null");
        return;
    }

    m_searchEngine->cancelSearch();
}

void SearchAdapter::clearResults() {
    SLOG_INFO("SearchAdapter: Clearing search results");

    m_results.clear();
    m_currentResultIndex = -1;

    emit searchFinished(0);
}

void SearchAdapter::goToNextResult() {
    SLOG_INFO("SearchAdapter: Going to next result");

    if (m_results.isEmpty()) {
        SLOG_WARNING("SearchAdapter: No search results available");
        return;
    }

    m_currentResultIndex = (m_currentResultIndex + 1) % m_results.size();
    updateCurrentResult();
}

void SearchAdapter::goToPreviousResult() {
    SLOG_INFO("SearchAdapter: Going to previous result");

    if (m_results.isEmpty()) {
        SLOG_WARNING("SearchAdapter: No search results available");
        return;
    }

    m_currentResultIndex =
        (m_currentResultIndex - 1 + m_results.size()) % m_results.size();
    updateCurrentResult();
}

void SearchAdapter::goToResult(int index) {
    SLOG_INFO_F("SearchAdapter: Going to result: {}", index);

    if (m_results.isEmpty()) {
        SLOG_WARNING("SearchAdapter: No search results available");
        return;
    }

    if (index < 0 || index >= m_results.size()) {
        SLOG_ERROR_F("SearchAdapter: Invalid result index: {}", index);
        return;
    }

    m_currentResultIndex = index;
    updateCurrentResult();
}

void SearchAdapter::updateCurrentResult() {
    if (m_currentResultIndex < 0 || m_currentResultIndex >= m_results.size()) {
        return;
    }

    const SearchResult& result = m_results[m_currentResultIndex];

    SLOG_INFO_F("SearchAdapter: Current result: {}/{} on page {}",
                m_currentResultIndex + 1, m_results.size(), result.pageNumber);

    // 发送当前结果信号
    emit currentResultChanged(m_currentResultIndex, m_results.size());

    // 创建高亮区域列表（使用 boundingRect）
    QList<QRectF> highlights;
    highlights.append(result.boundingRect);
    emit resultFound(result.pageNumber, highlights);

    // 如果有 PDFViewerPage，跳转到结果页面
    if (m_pdfViewerPage) {
        m_pdfViewerPage->goToPage(result.pageNumber);
    }
}
