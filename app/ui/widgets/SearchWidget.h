#pragma once

#include <QCheckBox>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QProgressBar>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
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

signals:
    void searchRequested(const QString& query, const SearchOptions& options);
    void resultSelected(const SearchResult& result);
    void navigateToResult(int pageNumber, const QRectF& rect);
    void searchClosed();
    void searchCleared();

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

private:
    void setupUI();
    void setupConnections();
    void setupShortcuts();
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

    // Results display
    QListView* m_resultsView;
    QLabel* m_statusLabel;
    QProgressBar* m_progressBar;

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
};
