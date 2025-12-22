#pragma once

#include <QHash>
#include <QObject>
#include "plugin/ISearchPlugin.h"
#include "plugin/PluginInterface.h"

/**
 * @brief SmartSearchPlugin - Example search enhancement plugin
 *
 * This plugin demonstrates the ISearchPlugin interface by providing:
 * - **Fuzzy Search**: Levenshtein distance-based approximate matching
 * - **Relevance Ranking**: Score results based on term frequency and position
 * - **Search Index**: Build and maintain search indexes for faster queries
 * - **Result Post-processing**: Filter, sort, and rank search results
 *
 * Features demonstrated:
 * - Custom search algorithm implementation
 * - Multiple ranking strategies
 * - Index building and management
 * - Hook registration for search workflow
 */
class SmartSearchPlugin : public PluginBase, public ISearchPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "smart_search.json")
    Q_INTERFACES(IPluginInterface ISearchPlugin)

public:
    explicit SmartSearchPlugin(QObject* parent = nullptr);
    ~SmartSearchPlugin() override;

    // IPluginInterface override
    void handleMessage(const QString& from, const QVariant& message) override;

protected:
    // PluginBase overrides
    bool onInitialize() override;
    void onShutdown() override;

    // ISearchPlugin interface
    QString algorithmName() const override;
    bool canHandleQuery(const QString& query,
                        const QJsonObject& options) const override;
    QList<PluginSearchResult> executeSearch(
        const QString& query, const QString& documentPath,
        const QJsonObject& options) override;
    QList<PluginSearchResult> postProcessResults(
        const QList<PluginSearchResult>& results, const QString& query,
        SearchRankingStrategy strategy) override;
    bool buildSearchIndex(const QString& documentPath,
                          const QJsonObject& options) override;
    qint64 getIndexSize(const QString& documentPath) const override;
    void clearIndex(const QString& documentPath) override;

private:
    void registerHooks();
    void unregisterHooks();
    void setupEventSubscriptions();
    void removeEventSubscriptions();

    // Search algorithms
    int levenshteinDistance(const QString& s1, const QString& s2) const;
    double calculateRelevanceScore(const QString& text, const QString& query,
                                   int position, int totalLength) const;
    bool fuzzyMatch(const QString& text, const QString& pattern,
                    int maxDistance) const;

    // Ranking implementations
    void rankByFrequency(QList<PluginSearchResult>& results,
                         const QString& query) const;
    void rankByPosition(QList<PluginSearchResult>& results) const;
    void rankByRelevance(QList<PluginSearchResult>& results,
                         const QString& query) const;

    // Hook callbacks
    QVariant onSearchPreExecute(const QVariantMap& context);
    QVariant onSearchPostExecute(const QVariantMap& context);
    QVariant onSearchResultsRank(const QVariantMap& context);

    // Configuration
    bool m_enableFuzzySearch;
    int m_fuzzyThreshold;  // Max Levenshtein distance
    bool m_caseSensitive;
    int m_maxResults;
    SearchRankingStrategy m_defaultStrategy;

    // Index storage (document path -> word frequency map)
    QHash<QString, QHash<QString, int>> m_searchIndex;
    QHash<QString, qint64> m_indexSizes;

    // Statistics
    int m_searchesPerformed;
    int m_indexesBuilt;
};
