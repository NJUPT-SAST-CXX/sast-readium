#pragma once

#include <poppler/qt6/poppler-qt6.h>
#include <QLabel>
#include <QPropertyAnimation>
#include <QSettings>
#include <QTabWidget>
#include <QVBoxLayout>
#include <QWidget>
#include <memory>

// Forward declarations
class DebugLogPanel;
class DocumentPropertiesPanel;
class AnnotationToolbar;
class SearchWidget;

/**
 * @brief Right sidebar with properties, tools, and debug panels
 *
 * @details Provides auxiliary functionality via:
 * - Properties panel (document metadata)
 * - Tools panel (annotations, highlights)
 * - Debug log panel with filtering and export
 * - Animated show/hide with state persistence
 * - Theme-aware styling
 *
 * @note State (visibility, width, active tab) is persisted via QSettings
 */
class RightSideBar : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief Construct a new Right Side Bar object
     * @param parent Parent widget (optional)
     */
    RightSideBar(QWidget* parent = nullptr);

    /**
     * @brief Destroy the Right Side Bar object and clean up resources
     */
    ~RightSideBar();

    // Document management
    /**
     * @brief Update the properties panel with document information
     * @param document Poppler document pointer
     * @param filePath Full path to the PDF file
     */
    void setDocument(Poppler::Document* document, const QString& filePath);

    /**
     * @brief Clear document properties
     */
    void clearDocument();

    // Search widget access
    SearchWidget* getSearchWidget() const { return m_searchWidget; }

    // 显示/隐藏控制
    bool isVisible() const;
    using QWidget::setVisible;  // Bring base class method into scope
    void setVisible(bool visible, bool animated = true);
    void toggleVisibility(bool animated = true);

    // 宽度管理
    int getPreferredWidth() const;
    void setPreferredWidth(int width);
    int getMinimumWidth() const { return minimumWidth; }
    int getMaximumWidth() const { return maximumWidth; }

    // 状态持久化
    void saveState();
    void restoreState();

public slots:
    void show(bool animated = true);
    void hide(bool animated = true);

signals:
    void visibilityChanged(bool visible);
    void widthChanged(int width);
    void viewFullDetailsRequested(Poppler::Document* document,
                                  const QString& filePath);

private slots:
    void onAnimationFinished();
    void onViewFullDetailsRequested(Poppler::Document* document,
                                    const QString& filePath);

private:
    QTabWidget* tabWidget;
    QPropertyAnimation* animation;
    QSettings* settings;
    DebugLogPanel* debugLogPanel;
    DocumentPropertiesPanel* m_propertiesPanel;
    AnnotationToolbar* m_annotationToolbar;
    class SearchWidget* m_searchWidget;

    bool isCurrentlyVisible;
    int preferredWidth;
    int lastWidth;

    static const int minimumWidth = 200;
    static const int maximumWidth = 400;
    static const int defaultWidth = 250;
    static const int animationDuration = 300;

    void initWindow();
    void initContent();
    void initAnimation();
    void initSettings();
    void applyTheme();

    QWidget* createPropertiesTab();
    QWidget* createToolsTab();
    QWidget* createSearchTab();
    QWidget* createDebugTab();
};
