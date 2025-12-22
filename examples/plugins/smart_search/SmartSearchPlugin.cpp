#include "SmartSearchPlugin.h"
#include <QDebug>
#include <QFile>
#include <QJsonArray>
#include <QRegularExpression>
#include <algorithm>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

SmartSearchPlugin::SmartSearchPlugin(QObject* parent)
    : PluginBase(parent),
      m_enableFuzzySearch(true),
      m_fuzzyThreshold(2),
      m_caseSensitive(false),
      m_maxResults(100),
      m_defaultStrategy(SearchRankingStrategy::Relevance),
      m_searchesPerformed(0),
      m_indexesBuilt(0) {
    m_metadata.name = "Smart Search";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Enhanced search with fuzzy matching, relevance ranking, and indexing";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "search.plugin" << "search.fuzzy"
                              << "search.ranking" << "search.index";
}

SmartSearchPlugin::~SmartSearchPlugin() {}

bool SmartSearchPlugin::onInitialize() {
    m_logger.info("SmartSearchPlugin: Initializing...");
    m_enableFuzzySearch =
        m_configuration.value("enableFuzzySearch").toBool(true);
    m_fuzzyThreshold = m_configuration.value("fuzzyThreshold").toInt(2);
    m_caseSensitive = m_configuration.value("caseSensitive").toBool(false);
    m_maxResults = m_configuration.value("maxResults").toInt(100);
    registerHooks();
    setupEventSubscriptions();
    m_logger.info("SmartSearchPlugin: Initialized successfully");
    return true;
}

void SmartSearchPlugin::onShutdown() {
    m_logger.info("SmartSearchPlugin: Shutting down...");
    removeEventSubscriptions();
    unregisterHooks();
    m_searchIndex.clear();
    m_indexSizes.clear();
    m_logger.info(QString("SmartSearchPlugin: Searches: %1, Indexes: %2")
                      .arg(m_searchesPerformed)
                      .arg(m_indexesBuilt));
}

void SmartSearchPlugin::handleMessage(const QString& from,
                                      const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();
    if (action == "search") {
        QString query = msgMap.value("query").toString();
        QString docPath = msgMap.value("documentPath").toString();
        auto results = executeSearch(query, docPath, QJsonObject());
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["resultCount"] = results.size();
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

void SmartSearchPlugin::setupEventSubscriptions() {
    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        if (m_configuration.value("autoIndex").toBool(false)) {
            buildSearchIndex(event->data().toString(), QJsonObject());
        }
    });
}

void SmartSearchPlugin::removeEventSubscriptions() {
    eventBus()->unsubscribeAll(this);
}

void SmartSearchPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        StandardHooks::SEARCH_PRE_EXECUTE, name(),
        [this](const QVariantMap& ctx) { return onSearchPreExecute(ctx); });
    registry.registerCallback(
        StandardHooks::SEARCH_POST_EXECUTE, name(),
        [this](const QVariantMap& ctx) { return onSearchPostExecute(ctx); });
    registry.registerCallback(
        StandardHooks::SEARCH_RESULTS_RANK, name(),
        [this](const QVariantMap& ctx) { return onSearchResultsRank(ctx); });
}

void SmartSearchPlugin::unregisterHooks() {
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
}

QVariant SmartSearchPlugin::onSearchPreExecute(const QVariantMap& context) {
    QVariantMap result;
    result["pluginHandles"] =
        canHandleQuery(context.value("query").toString(), QJsonObject());
    result["algorithmName"] = algorithmName();
    return result;
}

QVariant SmartSearchPlugin::onSearchPostExecute(const QVariantMap& context) {
    m_searchesPerformed++;
    QVariantMap result;
    result["processed"] = true;
    result["totalSearches"] = m_searchesPerformed;
    return result;
}

QVariant SmartSearchPlugin::onSearchResultsRank(const QVariantMap& context) {
    QVariantMap result;
    result["canRank"] = true;
    result["supportedStrategies"] = QStringList()
                                    << "frequency" << "position" << "relevance";
    return result;
}

QString SmartSearchPlugin::algorithmName() const {
    return m_enableFuzzySearch ? "SmartSearch-Fuzzy" : "SmartSearch-Exact";
}

bool SmartSearchPlugin::canHandleQuery(const QString& query,
                                       const QJsonObject&) const {
    return !query.trimmed().isEmpty();
}

QList<PluginSearchResult> SmartSearchPlugin::executeSearch(
    const QString& query, const QString& documentPath, const QJsonObject&) {
    QList<PluginSearchResult> results;
    QString searchQuery = m_caseSensitive ? query : query.toLower();

    if (m_searchIndex.contains(documentPath)) {
        const auto& wordIndex = m_searchIndex[documentPath];
        for (auto it = wordIndex.begin(); it != wordIndex.end(); ++it) {
            bool matched =
                m_enableFuzzySearch
                    ? fuzzyMatch(it.key(), searchQuery, m_fuzzyThreshold)
                    : it.key().contains(searchQuery);
            if (matched) {
                PluginSearchResult r;
                r.text = it.key();
                r.pageNumber = 0;
                r.relevanceScore = static_cast<double>(it.value()) / 100.0;
                r.metadata["frequency"] = it.value();
                results.append(r);
            }
        }
    }
    results = postProcessResults(results, query, m_defaultStrategy);
    if (results.size() > m_maxResults)
        results = results.mid(0, m_maxResults);
    m_searchesPerformed++;
    return results;
}

QList<PluginSearchResult> SmartSearchPlugin::postProcessResults(
    const QList<PluginSearchResult>& results, const QString& query,
    SearchRankingStrategy strategy) {
    QList<PluginSearchResult> ranked = results;
    switch (strategy) {
        case SearchRankingStrategy::Frequency:
            rankByFrequency(ranked, query);
            break;
        case SearchRankingStrategy::Position:
            rankByPosition(ranked);
            break;
        case SearchRankingStrategy::Relevance:
            rankByRelevance(ranked, query);
            break;
        default:
            break;
    }
    return ranked;
}

bool SmartSearchPlugin::buildSearchIndex(const QString& documentPath,
                                         const QJsonObject&) {
    m_logger.info(QString("SmartSearchPlugin: Building index for '%1'")
                      .arg(documentPath));
    QHash<QString, int> wordFreq;
    // Simulated index building - in real impl, extract text from PDF
    wordFreq["example"] = 10;
    wordFreq["document"] = 5;
    wordFreq["search"] = 8;
    m_searchIndex[documentPath] = wordFreq;
    m_indexSizes[documentPath] = wordFreq.size() * 50;
    m_indexesBuilt++;
    return true;
}

qint64 SmartSearchPlugin::getIndexSize(const QString& documentPath) const {
    return m_indexSizes.value(documentPath, 0);
}

void SmartSearchPlugin::clearIndex(const QString& documentPath) {
    m_searchIndex.remove(documentPath);
    m_indexSizes.remove(documentPath);
}

int SmartSearchPlugin::levenshteinDistance(const QString& s1,
                                           const QString& s2) const {
    int m = s1.length(), n = s2.length();
    QVector<QVector<int>> dp(m + 1, QVector<int>(n + 1));
    for (int i = 0; i <= m; ++i)
        dp[i][0] = i;
    for (int j = 0; j <= n; ++j)
        dp[0][j] = j;
    for (int i = 1; i <= m; ++i) {
        for (int j = 1; j <= n; ++j) {
            int cost = (s1[i - 1] == s2[j - 1]) ? 0 : 1;
            dp[i][j] = std::min(
                {dp[i - 1][j] + 1, dp[i][j - 1] + 1, dp[i - 1][j - 1] + cost});
        }
    }
    return dp[m][n];
}

double SmartSearchPlugin::calculateRelevanceScore(const QString& text,
                                                  const QString& query,
                                                  int position,
                                                  int totalLength) const {
    double exactMatch = text.contains(query, Qt::CaseInsensitive) ? 0.5 : 0.0;
    double posScore = 1.0 - (static_cast<double>(position) / totalLength);
    double lenScore = 1.0 - qAbs(text.length() - query.length()) / 20.0;
    return qBound(0.0, exactMatch + posScore * 0.3 + lenScore * 0.2, 1.0);
}

bool SmartSearchPlugin::fuzzyMatch(const QString& text, const QString& pattern,
                                   int maxDist) const {
    QString t = m_caseSensitive ? text : text.toLower();
    QString p = m_caseSensitive ? pattern : pattern.toLower();
    return levenshteinDistance(t, p) <= maxDist;
}

void SmartSearchPlugin::rankByFrequency(QList<PluginSearchResult>& results,
                                        const QString&) const {
    std::sort(results.begin(), results.end(),
              [](const PluginSearchResult& a, const PluginSearchResult& b) {
                  return a.metadata.value("frequency").toInt() >
                         b.metadata.value("frequency").toInt();
              });
}

void SmartSearchPlugin::rankByPosition(
    QList<PluginSearchResult>& results) const {
    std::sort(results.begin(), results.end(),
              [](const PluginSearchResult& a, const PluginSearchResult& b) {
                  return a.pageNumber < b.pageNumber;
              });
}

void SmartSearchPlugin::rankByRelevance(QList<PluginSearchResult>& results,
                                        const QString&) const {
    std::sort(results.begin(), results.end(),
              [](const PluginSearchResult& a, const PluginSearchResult& b) {
                  return a.relevanceScore > b.relevanceScore;
              });
}
