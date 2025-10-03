#include <QDebug>
#include <QRegularExpression>
#include <algorithm>
#include <cmath>
#include "SearchFeatures.h"

// SearchSuggestionEngine Implementation class
class SearchSuggestionEngine::Implementation {
public:
    struct TrieNode {
        QHash<QChar, std::shared_ptr<TrieNode>> children;
        bool isEndOfWord;
        int frequency;
        QString word;
    };

    Implementation() : root(std::make_shared<TrieNode>()) {
        root->isEndOfWord = false;
        root->frequency = 0;
    }

    std::shared_ptr<TrieNode> root;
    QHash<QString, int> queryFrequency;

    void insertWord(const QString& word, int frequency);
    void collectSuggestions(std::shared_ptr<TrieNode> node,
                            const QString& prefix, QStringList& suggestions,
                            int maxSuggestions);
};

// Implementation methods
void SearchSuggestionEngine::Implementation::insertWord(const QString& word,
                                                        int frequency) {
    std::shared_ptr<TrieNode> current = root;

    for (const QChar& ch : word) {
        if (!current->children.contains(ch)) {
            current->children[ch] = std::make_shared<TrieNode>();
            current->children[ch]->isEndOfWord = false;
            current->children[ch]->frequency = 0;
        }
        current = current->children[ch];
    }

    current->isEndOfWord = true;
    current->frequency += frequency;
    current->word = word;
}

void SearchSuggestionEngine::Implementation::collectSuggestions(
    std::shared_ptr<TrieNode> node, const QString& prefix,
    QStringList& suggestions, int maxSuggestions) {
    if (suggestions.size() >= maxSuggestions) {
        return;
    }

    if (node->isEndOfWord) {
        suggestions.append(prefix);
    }

    // Collect from children (sorted by frequency)
    QList<QPair<QChar, std::shared_ptr<TrieNode>>> children;
    for (auto it = node->children.begin(); it != node->children.end(); ++it) {
        children.append(qMakePair(it.key(), it.value()));
    }

    std::sort(children.begin(), children.end(),
              [](const QPair<QChar, std::shared_ptr<TrieNode>>& a,
                 const QPair<QChar, std::shared_ptr<TrieNode>>& b) {
                  return a.second->frequency > b.second->frequency;
              });

    for (const auto& child : children) {
        collectSuggestions(child.second, prefix + child.first, suggestions,
                           maxSuggestions);
        if (suggestions.size() >= maxSuggestions) {
            break;
        }
    }
};

// SearchSuggestionEngine implementation
SearchSuggestionEngine::SearchSuggestionEngine()
    : d(std::make_unique<Implementation>()) {}

SearchSuggestionEngine::~SearchSuggestionEngine() = default;

void SearchSuggestionEngine::trainModel(const QStringList& queries,
                                        const QList<int>& frequencies) {
    if (queries.size() != frequencies.size()) {
        qWarning()
            << "SearchSuggestionEngine: queries and frequencies size mismatch";
        return;
    }

    for (int i = 0; i < queries.size(); ++i) {
        d->insertWord(queries[i], frequencies[i]);
        d->queryFrequency[queries[i]] = frequencies[i];
    }
}

QStringList SearchSuggestionEngine::generateSuggestions(
    const QString& partialQuery, int maxSuggestions) {
    QStringList suggestions;

    // Combine different suggestion methods
    QStringList ngramSugs =
        ngramSuggestions(partialQuery, 3, maxSuggestions / 2);
    QStringList fuzzySugs =
        fuzzySuggestions(partialQuery, 2, maxSuggestions / 2);

    suggestions.append(ngramSugs);
    suggestions.append(fuzzySugs);

    // Remove duplicates and sort by frequency
    QSet<QString> uniqueSuggestions;
    QList<QPair<QString, int>> suggestionPairs;

    for (const QString& suggestion : suggestions) {
        if (!uniqueSuggestions.contains(suggestion)) {
            uniqueSuggestions.insert(suggestion);
            int frequency = d->queryFrequency.value(suggestion, 0);
            suggestionPairs.append(qMakePair(suggestion, frequency));
        }
    }

    // Sort by frequency (descending)
    std::sort(suggestionPairs.begin(), suggestionPairs.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });

    QStringList finalSuggestions;
    for (int i = 0; i < qMin(maxSuggestions, suggestionPairs.size()); ++i) {
        finalSuggestions.append(suggestionPairs[i].first);
    }

    return finalSuggestions;
}

void SearchSuggestionEngine::addQueryToModel(const QString& query,
                                             int frequency) {
    d->insertWord(query, frequency);
    d->queryFrequency[query] += frequency;
}

void SearchSuggestionEngine::updateQueryFrequency(const QString& query,
                                                  int frequency) {
    d->queryFrequency[query] = frequency;
}

int SearchSuggestionEngine::getQueryFrequency(const QString& query) const {
    return d->queryFrequency.value(query, 0);
}

QStringList SearchSuggestionEngine::getMostFrequentQueries(int count) const {
    // Get all queries and sort by frequency
    QList<QPair<QString, int>> sortedQueries;
    for (auto it = d->queryFrequency.begin(); it != d->queryFrequency.end();
         ++it) {
        sortedQueries.append(qMakePair(it.key(), it.value()));
    }

    // Sort by frequency (descending)
    std::sort(sortedQueries.begin(), sortedQueries.end(),
              [](const QPair<QString, int>& a, const QPair<QString, int>& b) {
                  return a.second > b.second;
              });

    // Extract top queries
    QStringList result;
    for (int i = 0; i < qMin(count, sortedQueries.size()); ++i) {
        result.append(sortedQueries[i].first);
    }

    return result;
}

QStringList SearchSuggestionEngine::ngramSuggestions(
    const QString& partialQuery, int n, int maxSuggestions) {
    QStringList suggestions;

    if (partialQuery.length() < n) {
        return suggestions;
    }

    // Find all words that start with the partial query
    std::shared_ptr<Implementation::TrieNode> current = d->root;

    for (const QChar& ch : partialQuery) {
        if (!current->children.contains(ch)) {
            return suggestions;  // No suggestions found
        }
        current = current->children[ch];
    }

    // Collect suggestions from this node
    d->collectSuggestions(current, partialQuery, suggestions, maxSuggestions);

    return suggestions;
}

QStringList SearchSuggestionEngine::fuzzySuggestions(
    const QString& partialQuery, int maxDistance, int maxSuggestions) {
    QStringList suggestions;

    for (auto it = d->queryFrequency.begin(); it != d->queryFrequency.end();
         ++it) {
        const QString& query = it.key();

        if (query.startsWith(partialQuery, Qt::CaseInsensitive)) {
            suggestions.append(query);
        } else {
            int distance = FuzzySearchAlgorithms::levenshteinDistanceOptimized(
                partialQuery, query, maxDistance);
            if (distance <= maxDistance) {
                suggestions.append(query);
            }
        }

        if (suggestions.size() >= maxSuggestions) {
            break;
        }
    }

    return suggestions;
}

QStringList SearchSuggestionEngine::contextualSuggestions(
    const QString& partialQuery, const QStringList& context,
    int maxSuggestions) {
    QStringList suggestions =
        generateSuggestions(partialQuery, maxSuggestions * 2);
    QStringList contextualSugs;

    // Score suggestions based on context
    QHash<QString, double> scores;

    for (const QString& suggestion : suggestions) {
        double score = 0.0;

        for (const QString& contextWord : context) {
            if (suggestion.contains(contextWord, Qt::CaseInsensitive)) {
                score += 1.0;
            }

            // Add similarity bonus
            double similarity = FuzzySearchAlgorithms::jaroWinklerSimilarity(
                suggestion, contextWord);
            score += similarity * 0.5;
        }

        scores[suggestion] = score;
    }

    // Sort by score
    QList<QPair<QString, double>> scorePairs;
    for (auto it = scores.begin(); it != scores.end(); ++it) {
        scorePairs.append(qMakePair(it.key(), it.value()));
    }

    std::sort(
        scorePairs.begin(), scorePairs.end(),
        [](const QPair<QString, double>& a, const QPair<QString, double>& b) {
            return a.second > b.second;
        });

    for (int i = 0; i < qMin(maxSuggestions, scorePairs.size()); ++i) {
        contextualSugs.append(scorePairs[i].first);
    }

    return contextualSugs;
}

// BooleanSearchParser implementation
std::shared_ptr<BooleanSearchParser::QueryNode> BooleanSearchParser::parseQuery(
    const QString& query) {
    QStringList tokens = tokenize(query);
    if (tokens.isEmpty()) {
        return nullptr;
    }

    int index = 0;
    return parseExpression(tokens, index);
}

QList<SearchResult> BooleanSearchParser::executeQuery(
    std::shared_ptr<QueryNode> root, const QString& text, int pageNumber) {
    if (!root) {
        return QList<SearchResult>();
    }

    return evaluateNode(root, text, pageNumber);
}

QStringList BooleanSearchParser::tokenize(const QString& query) {
    QStringList tokens;
    QRegularExpression tokenRegex(
        R"(\s*(AND|OR|NOT|NEAR|\(|\)|"[^"]*"|[^\s\(\)]+)\s*)");

    QRegularExpressionMatchIterator iterator = tokenRegex.globalMatch(query);
    while (iterator.hasNext()) {
        QRegularExpressionMatch match = iterator.next();
        QString token = match.captured(1).trimmed();
        if (!token.isEmpty()) {
            tokens.append(token);
        }
    }

    return tokens;
}

std::shared_ptr<BooleanSearchParser::QueryNode>
BooleanSearchParser::parseExpression(const QStringList& tokens, int& index) {
    auto left = parseTerm(tokens, index);

    while (index < tokens.size()) {
        QString op = tokens[index];

        if (op == "AND" || op == "OR") {
            index++;
            auto right = parseTerm(tokens, index);

            auto node = std::make_shared<QueryNode>();
            node->op = (op == "AND") ? AND : OR;
            node->left = left;
            node->right = right;
            left = node;
        } else {
            break;
        }
    }

    return left;
}

std::shared_ptr<BooleanSearchParser::QueryNode> BooleanSearchParser::parseTerm(
    const QStringList& tokens, int& index) {
    if (index >= tokens.size()) {
        return nullptr;
    }

    QString token = tokens[index];

    if (token == "NOT") {
        index++;
        auto node = std::make_shared<QueryNode>();
        node->op = NOT;
        node->left = parseTerm(tokens, index);
        return node;
    } else if (token == "(") {
        index++;
        auto node = parseExpression(tokens, index);
        if (index < tokens.size() && tokens[index] == ")") {
            index++;
        }
        return node;
    } else {
        // Regular term or phrase
        auto node = std::make_shared<QueryNode>();
        node->term = token;

        // Remove quotes if present
        if (node->term.startsWith('"') && node->term.endsWith('"')) {
            node->term = node->term.mid(1, node->term.length() - 2);
            node->op = PHRASE;
        }

        index++;

        // Check for NEAR operator
        if (index < tokens.size() && tokens[index] == "NEAR") {
            index++;
            if (index < tokens.size()) {
                bool ok;
                int proximity = tokens[index].toInt(&ok);
                if (ok) {
                    node->proximity = proximity;
                    index++;
                } else {
                    node->proximity = 10;  // Default proximity
                }
            }
            node->op = NEAR;
        }

        return node;
    }
}

QList<SearchResult> BooleanSearchParser::evaluateNode(
    std::shared_ptr<QueryNode> node, const QString& text, int pageNumber) {
    if (!node) {
        return QList<SearchResult>();
    }

    if (!node->term.isEmpty()) {
        // Leaf node - search for term
        QList<SearchResult> results;

        QRegularExpression regex(QRegularExpression::escape(node->term),
                                 QRegularExpression::CaseInsensitiveOption);
        QRegularExpressionMatchIterator iterator = regex.globalMatch(text);

        while (iterator.hasNext()) {
            QRegularExpressionMatch match = iterator.next();

            SearchResult result;
            result.pageNumber = pageNumber;
            result.matchedText = match.captured();
            result.textPosition = match.capturedStart();
            result.textLength = match.capturedLength();

            int contextStart = qMax(0, result.textPosition - 50);
            int contextEnd = qMin(text.length(),
                                  result.textPosition + result.textLength + 50);
            result.contextText =
                text.mid(contextStart, contextEnd - contextStart);

            results.append(result);
        }

        return results;
    } else {
        // Operator node
        QList<SearchResult> leftResults =
            evaluateNode(node->left, text, pageNumber);
        QList<SearchResult> rightResults =
            evaluateNode(node->right, text, pageNumber);

        return combineResults(leftResults, rightResults, node->op);
    }
}

QList<SearchResult> BooleanSearchParser::combineResults(
    const QList<SearchResult>& left, const QList<SearchResult>& right,
    Operator op) {
    QList<SearchResult> combined;

    switch (op) {
        case AND:
            // Return results that appear in both sets (intersection)
            for (const SearchResult& leftResult : left) {
                for (const SearchResult& rightResult : right) {
                    if (qAbs(leftResult.textPosition -
                             rightResult.textPosition) <
                        100) {  // Proximity check
                        combined.append(leftResult);
                        break;
                    }
                }
            }
            break;

        case OR:
            // Return all results from both sets (union)
            combined = left;
            combined.append(right);
            break;

        case NOT:
            // Return left results that don't have corresponding right results
            for (const SearchResult& leftResult : left) {
                bool found = false;
                for (const SearchResult& rightResult : right) {
                    if (qAbs(leftResult.textPosition -
                             rightResult.textPosition) < 50) {
                        found = true;
                        break;
                    }
                }
                if (!found) {
                    combined.append(leftResult);
                }
            }
            break;

        case NEAR:
            // Return results that are near each other
            for (const SearchResult& leftResult : left) {
                for (const SearchResult& rightResult : right) {
                    int distance = qAbs(leftResult.textPosition -
                                        rightResult.textPosition);
                    if (distance < 200) {  // Proximity threshold
                        SearchResult nearResult = leftResult;
                        nearResult.textLength =
                            qMax(
                                leftResult.textPosition + leftResult.textLength,
                                rightResult.textPosition +
                                    rightResult.textLength) -
                            qMin(leftResult.textPosition,
                                 rightResult.textPosition);
                        nearResult.textPosition = qMin(
                            leftResult.textPosition, rightResult.textPosition);
                        combined.append(nearResult);
                    }
                }
            }
            break;

        default:
            combined = left;
            break;
    }

    return combined;
}
