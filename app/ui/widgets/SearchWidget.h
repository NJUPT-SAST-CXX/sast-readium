#pragma once

#include <QColorDialog>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QRegularExpression>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>

// ElaWidgetTools forward declarations
class ElaPushButton;
class ElaCheckBox;
class ElaListView;
class ElaProgressBar;
class ElaText;
class ElaComboBox;
class ElaSpinBox;
#include "../../model/SearchModel.h"
#include "../core/ContextMenuManager.h"

class QShortcut;

/**
 * Comprehensive search widget with options and results display
 */
class SearchWidget : public QWidget {
    Q_OBJECT

public:
    explicit SearchWidget(QWidget* parent = nullptr);
    ~SearchWidget();

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
    void setHighlightColors(const QColor& normalColor,
                            const QColor& currentColor);
    void showSearchProgress(bool show);
    void updateSearchProgress(int current, int total);
    void setSearchResultInfo(int currentResult, int totalResults);

    // Color accessors
    QColor getNormalHighlightColor() const;
    QColor getCurrentHighlightColor() const;

    // Search history persistence
    void saveSearchHistoryToSettings();
    void loadSearchHistoryFromSettings();

    // Search validation and error handling
    bool validateSearchInput(const QString& query) const;
    void showSearchError(const QString& error);

    // Performance optimization
    void cancelCurrentSearch();
    void optimizeSearchPerformance();

protected:
    void changeEvent(QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;

signals:
    void searchRequested(const QString& query, const SearchOptions& options);
    void resultSelected(const SearchResult& result);
    void navigateToResult(int pageNumber, const QRectF& rect);
    void searchClosed();
    void searchCleared();
    void highlightColorsChanged(const QColor& normalColor,
                                const QColor& currentColor);

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
    void onRealTimeSearchStarted();
    void onRealTimeResultsUpdated(const QList<SearchResult>& results);
    void onRealTimeSearchProgress(int currentPage, int totalPages);
    void navigateToCurrentResult();
    void onFuzzySearchToggled(bool enabled);
    void onPageRangeToggled(bool enabled);
    void onPageRangeChanged();
    void onSearchHistorySelected(const QString& query);
    void onClearHistoryClicked();
    void onHighlightColorClicked();
    void onCurrentHighlightColorClicked();

private:
    // Private helper methods
    void setupUI();
    void setupConnections();
    void setupShortcuts();
    void retranslateUi();
    void applyTheme();
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
    QLineEdit*
        m_searchInput;  // from ElaComboBox::lineEdit(); kept as QLineEdit
    ElaPushButton* m_searchButton;
    ElaPushButton* m_clearButton;
    ElaPushButton* m_optionsButton;
    ElaPushButton* m_closeButton;

    // Navigation controls
    ElaPushButton* m_previousButton;
    ElaPushButton* m_nextButton;
    ElaText* m_resultInfoLabel;

    // Search options
    QGroupBox* m_optionsGroup;
    ElaCheckBox* m_caseSensitiveCheck;
    ElaCheckBox* m_wholeWordsCheck;
    ElaCheckBox* m_regexCheck;
    ElaCheckBox* m_searchBackwardCheck;

    // Advanced search options
    ElaCheckBox* m_fuzzySearchCheck;
    ElaSpinBox* m_fuzzyThresholdSpin;
    ElaText* m_fuzzyThresholdLabel;

    // Page range search
    QGroupBox* m_pageRangeGroup;
    ElaCheckBox* m_pageRangeCheck;
    ElaSpinBox* m_startPageSpin;
    ElaSpinBox* m_endPageSpin;
    ElaText* m_pageRangeLabel;

    // Search history
    ElaComboBox* m_searchHistoryCombo;
    ElaPushButton* m_clearHistoryButton;

    // Results display
    ElaListView* m_resultsView;
    ElaText* m_statusLabel;
    ElaProgressBar* m_progressBar;

    // Enhanced UI elements
    ElaText* m_searchProgressLabel;
    ElaProgressBar* m_searchProgressBar;
    ElaPushButton* m_highlightColorButton;
    ElaPushButton* m_currentHighlightColorButton;

    // Data and state
    SearchModel* m_searchModel;
    Poppler::Document* m_document;
    QTimer* m_searchTimer;  // For debounced search
    bool m_optionsVisible;

    // Shortcuts
    QShortcut* m_findShortcut;
    QShortcut* m_findNextShortcut;
    QShortcut* m_findPreviousShortcut;
    QShortcut* m_escapeShortcut;

    // Context menu management
    ContextMenuManager* contextMenuManager;
};
