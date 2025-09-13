#pragma once

#include <QWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QCheckBox>
#include <QLabel>
#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QProgressBar>
#include <QTimer>
#include <QSpinBox>
#include <QComboBox>
#include <QColorDialog>
#include <QRegularExpression>
#include "../../model/SearchModel.h"

class QShortcut;

/**
 * Comprehensive search widget with options and results display
 */
class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);
    ~SearchWidget() = default;

    // Search operations
    void setDocument(Poppler::Document* document);
    void focusSearchInput();
    void clearSearch();
    void showSearchOptions(bool show);

    // Search model access
    SearchModel* getSearchModel() const { return m_searchModel; }

    // Current search state
    bool hasResults() const;
    int getResultCount() const;
    SearchResult getCurrentResult() const;

    // Advanced search features
    void setFuzzySearchEnabled(bool enabled);
    void setPageRangeEnabled(bool enabled);
    void setPageRange(int startPage, int endPage);
    void updateSearchHistory();
    void loadSearchHistory();

    // Enhanced UI features
    void setHighlightColors(const QColor& normalColor, const QColor& currentColor);
    void showSearchProgress(bool show);
    void updateSearchProgress(int current, int total);
    void setSearchResultInfo(int currentResult, int totalResults);

    // Color accessors
    QColor getNormalHighlightColor() const;
    QColor getCurrentHighlightColor() const;

protected:
    void changeEvent(QEvent* event) override;

signals:
    void searchRequested(const QString& query, const SearchOptions& options);
    void resultSelected(const SearchResult& result);
    void navigateToResult(int pageNumber, const QRectF& rect);
    void searchClosed();
    void searchCleared();
    void highlightColorsChanged(const QColor& normalColor, const QColor& currentColor);

public slots:
    void performSearch();
    void performRealTimeSearch();
    void nextResult();
    void previousResult();
    void onResultClicked(const QModelIndex& index);

private slots:
    void onSearchTextChanged();
    void onSearchStarted();
    void onSearchFinished(int resultCount);
    void onSearchError(const QString& error);
    void onCurrentResultChanged(int index);
    void toggleSearchOptions();

    // Real-time search slots
    void onRealTimeSearchStarted();
    void onRealTimeResultsUpdated(const QList<SearchResult>& results);
    void onRealTimeSearchProgress(int currentPage, int totalPages);

    // Navigation helper methods
    void navigateToCurrentResult();

    // Advanced search slots
    void onFuzzySearchToggled(bool enabled);
    void onPageRangeToggled(bool enabled);
    void onPageRangeChanged();
    void onSearchHistorySelected(const QString& query);
    void onClearHistoryClicked();

    // UI enhancement slots
    void onHighlightColorClicked();
    void onCurrentHighlightColorClicked();

private:
    void setupUI();
    void setupConnections();
    void setupShortcuts();
    void retranslateUi();
    void updateNavigationButtons();
    void updateResultsInfo();
    SearchOptions getSearchOptions() const;
    void setSearchInProgress(bool inProgress);

    // UI Components
    QVBoxLayout* m_mainLayout;
    QHBoxLayout* m_searchLayout;
    QHBoxLayout* m_navigationLayout;
    QHBoxLayout* m_infoLayout;

    // Search input and controls
    QLineEdit* m_searchInput;
    QPushButton* m_searchButton;
    QPushButton* m_clearButton;
    QPushButton* m_optionsButton;
    QPushButton* m_closeButton;

    // Navigation controls
    QPushButton* m_previousButton;
    QPushButton* m_nextButton;
    QLabel* m_resultInfoLabel;

    // Search options
    QGroupBox* m_optionsGroup;
    QCheckBox* m_caseSensitiveCheck;
    QCheckBox* m_wholeWordsCheck;
    QCheckBox* m_regexCheck;
    QCheckBox* m_searchBackwardCheck;

    // Advanced search options
    QCheckBox* m_fuzzySearchCheck;
    QSpinBox* m_fuzzyThresholdSpin;
    QLabel* m_fuzzyThresholdLabel;

    // Page range search
    QGroupBox* m_pageRangeGroup;
    QCheckBox* m_pageRangeCheck;
    QSpinBox* m_startPageSpin;
    QSpinBox* m_endPageSpin;
    QLabel* m_pageRangeLabel;

    // Search history
    QComboBox* m_searchHistoryCombo;
    QPushButton* m_clearHistoryButton;

    // Results display
    QListView* m_resultsView;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;

    // Enhanced UI elements
    QLabel* m_searchProgressLabel;
    QProgressBar* m_searchProgressBar;
    QPushButton* m_highlightColorButton;
    QPushButton* m_currentHighlightColorButton;

    // Data and state
    SearchModel* m_searchModel;
    Poppler::Document* m_document;
    QTimer* m_searchTimer; // For debounced search
    bool m_optionsVisible;

    // Shortcuts
    QShortcut* m_findShortcut;
    QShortcut* m_findNextShortcut;
    QShortcut* m_findPreviousShortcut;
    QShortcut* m_escapeShortcut;
};
