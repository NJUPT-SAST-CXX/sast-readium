# Smart Search Plugin

This plugin demonstrates the `ISearchPlugin` interface by providing enhanced search capabilities.

## Features

- **Fuzzy Search**: Levenshtein distance-based approximate matching
- **Relevance Ranking**: Score results based on multiple factors
- **Search Index**: Build and manage search indexes for faster queries
- **Multiple Ranking Strategies**: Frequency, position, and relevance-based ranking

## ISearchPlugin Interface

```cpp
class ISearchPlugin {
    QString algorithmName() const;
    bool canHandleQuery(const QString& query, const QJsonObject& options) const;
    QList<PluginSearchResult> executeSearch(const QString& query,
        const QString& documentPath, const QJsonObject& options);
    QList<PluginSearchResult> postProcessResults(
        const QList<PluginSearchResult>& results, const QString& query,
        SearchRankingStrategy strategy);
    bool buildSearchIndex(const QString& documentPath, const QJsonObject& options);
    qint64 getIndexSize(const QString& documentPath) const;
    void clearIndex(const QString& documentPath);
};
```

## Ranking Strategies

| Strategy | Description |
|----------|-------------|
| Frequency | Rank by term frequency in document |
| Position | Rank by position in document |
| Relevance | Combined scoring (default) |

## Configuration

```json
{
    "enableFuzzySearch": true,
    "fuzzyThreshold": 2,
    "caseSensitive": false,
    "maxResults": 100,
    "defaultStrategy": "relevance",
    "autoIndex": false
}
```

## Hook Registration

- `search.pre_execute`: Called before search execution
- `search.post_execute`: Called after search completes
- `search.results_rank`: Called when ranking results

## Fuzzy Matching

Uses Levenshtein distance algorithm to find approximate matches:

- `fuzzyThreshold`: Maximum edit distance allowed (default: 2)
- Handles typos, spelling variations, and similar terms

## Building

```bash
mkdir build && cd build
cmake .. && cmake --build .
```
