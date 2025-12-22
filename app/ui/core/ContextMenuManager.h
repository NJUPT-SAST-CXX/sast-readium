#pragma once

#include <QAction>
#include "ElaMenu.h"
class QMenu;
#include <QObject>
#include <QPoint>
#include <QWidget>
#include "../../controller/tool.hpp"

/**
 * @brief Centralized context menu management for all UI components
 *
 * @details This class provides a unified approach to context menu creation
 * and management across all UI components in the SAST Readium application.
 * It ensures consistent behavior, proper action implementations, and
 * context-sensitive menu item enabling/disabling.
 *
 * **Features:**
 * - Document content context menus with PDF-specific actions
 * - UI element context menus (tabs, sidebars, toolbars)
 * - Nested submenu support with proper navigation
 * - Context-sensitive action enabling/disabling
 * - Error handling for all context menu operations
 * - Consistent styling and behavior across components
 *
 * **Context Menu Types:**
 * - Document viewer context menus (copy, zoom, rotate, etc.)
 * - Tab context menus (close, close others, reopen, etc.)
 * - Sidebar context menus (bookmark management, thumbnail options)
 * - Toolbar context menus (customize, reset, etc.)
 * - Search widget context menus (search options, history)
 *
 * @note All context menus are created on-demand and cached for performance
 */
class ContextMenuManager : public QObject {
    Q_OBJECT

public:
    /**
     * @brief Get the singleton instance
     */
    static ContextMenuManager& instance() {
        static ContextMenuManager instance;
        return instance;
    }

    /**
     * @brief Context menu types for different UI components
     */
    enum class MenuType {
        DocumentViewer,    // PDF document content area
        DocumentTab,       // Document tab widget
        SidebarThumbnail,  // Sidebar thumbnail view
        SidebarBookmark,   // Sidebar bookmark view
        ToolbarArea,       // Toolbar customization
        SearchWidget,      // Search widget options
        StatusBar,         // Status bar information
        RightSidebar       // Right sidebar panels
    };

    /**
     * @brief Document context for context-sensitive menus
     */
    struct DocumentContext {
        bool hasDocument = false;
        bool hasSelection = false;
        bool canCopy = false;
        bool canZoom = false;
        bool canRotate = false;
        int currentPage = 0;
        int totalPages = 0;
        double zoomLevel = 1.0;
        QString documentPath;
        QString selectedText;
    };

    /**
     * @brief UI element context for element-specific menus
     */
    struct UIElementContext {
        QWidget* targetWidget = nullptr;
        int elementIndex = -1;
        QString elementId;
        QVariantMap properties;
        bool isEnabled = true;
        bool isVisible = true;
    };

    /**
     * @brief Construct a new Context Menu Manager
     * @param parent Parent object (optional)
     */
    explicit ContextMenuManager(QObject* parent = nullptr);

    /**
     * @brief Destroy the Context Menu Manager and clean up resources
     */
    ~ContextMenuManager();

    /**
     * @brief Show context menu for document viewer
     * @param position Global position where menu should appear
     * @param context Document context information
     * @param parent Parent widget for the menu
     */
    void showDocumentViewerMenu(const QPoint& position,
                                const DocumentContext& context,
                                QWidget* parent);

    /**
     * @brief Show context menu for document tab
     * @param position Global position where menu should appear
     * @param tabIndex Index of the tab
     * @param context UI element context
     * @param parent Parent widget for the menu
     */
    void showDocumentTabMenu(const QPoint& position, int tabIndex,
                             const UIElementContext& context, QWidget* parent);

    /**
     * @brief Show context menu for sidebar elements
     * @param position Global position where menu should appear
     * @param menuType Type of sidebar menu (thumbnail or bookmark)
     * @param context UI element context
     * @param parent Parent widget for the menu
     */
    void showSidebarMenu(const QPoint& position, MenuType menuType,
                         const UIElementContext& context, QWidget* parent);

    /**
     * @brief Show context menu for toolbar area
     * @param position Global position where menu should appear
     * @param context UI element context
     * @param parent Parent widget for the menu
     */
    void showToolbarMenu(const QPoint& position,
                         const UIElementContext& context, QWidget* parent);

    /**
     * @brief Show context menu for search widget
     * @param position Global position where menu should appear
     * @param context UI element context
     * @param parent Parent widget for the menu
     */
    void showSearchMenu(const QPoint& position, const UIElementContext& context,
                        QWidget* parent);

    /**
     * @brief Show context menu for status bar
     * @param position Global position where menu should appear
     * @param context UI element context
     * @param parent Parent widget for the menu
     */
    void showStatusBarMenu(const QPoint& position,
                           const UIElementContext& context, QWidget* parent);

    /**
     * @brief Show context menu for right sidebar
     * @param position Global position where menu should appear
     * @param context UI element context
     * @param parent Parent widget for the menu
     */
    void showRightSidebarMenu(const QPoint& position,
                              const UIElementContext& context, QWidget* parent);

    /**
     * @brief Update context menu states based on application state
     * @param documentContext Current document context
     */
    void updateMenuStates(const DocumentContext& documentContext);

    /**
     * @brief Clear all cached menus (useful for theme changes)
     */
    void clearMenuCache();

signals:
    /**
     * @brief Emitted when a context menu action is triggered
     * @param action The action that was triggered
     * @param context Additional context data
     */
    void actionTriggered(ActionMap action,
                         const QVariantMap& context = QVariantMap());

    /**
     * @brief Emitted when a custom context menu action is triggered
     * @param actionId Custom action identifier
     * @param context Additional context data
     */
    void customActionTriggered(const QString& actionId,
                               const QVariantMap& context = QVariantMap());

private slots:
    /**
     * @brief Handle document viewer action triggers
     */
    void onDocumentViewerAction();

    /**
     * @brief Handle tab action triggers
     */
    void onTabAction();

    /**
     * @brief Handle sidebar action triggers
     */
    void onSidebarAction();

    /**
     * @brief Handle toolbar action triggers
     */
    void onToolbarAction();

    /**
     * @brief Handle search action triggers
     */
    void onSearchAction();

private:
    /**
     * @brief Create document viewer context menu
     * @param context Document context
     * @param parent Parent widget
     * @return Created menu
     */
    ElaMenu* createDocumentViewerMenu(const DocumentContext& context,
                                      QWidget* parent);

    /**
     * @brief Create document tab context menu
     * @param tabIndex Tab index
     * @param context UI element context
     * @param parent Parent widget
     * @return Created menu
     */
    ElaMenu* createDocumentTabMenu(int tabIndex,
                                   const UIElementContext& context,
                                   QWidget* parent);

    /**
     * @brief Create sidebar context menu
     * @param menuType Sidebar menu type
     * @param context UI element context
     * @param parent Parent widget
     * @return Created menu
     */
    ElaMenu* createSidebarMenu(MenuType menuType,
                               const UIElementContext& context,
                               QWidget* parent);

    /**
     * @brief Create toolbar context menu
     * @param context UI element context
     * @param parent Parent widget
     * @return Created menu
     */
    ElaMenu* createToolbarMenu(const UIElementContext& context,
                               QWidget* parent);

    /**
     * @brief Create search widget context menu
     * @param context UI element context
     * @param parent Parent widget
     * @return Created menu
     */
    ElaMenu* createSearchMenu(const UIElementContext& context, QWidget* parent);

    /**
     * @brief Create status bar context menu
     * @param context UI element context
     * @param parent Parent widget
     * @return Created menu
     */
    ElaMenu* createStatusBarMenu(const UIElementContext& context,
                                 QWidget* parent);

    /**
     * @brief Create right sidebar context menu
     * @param context UI element context
     * @param parent Parent widget
     * @return Created menu
     */
    ElaMenu* createRightSidebarMenu(const UIElementContext& context,
                                    QWidget* parent);

    /**
     * @brief Create submenu for zoom operations
     * @param parent Parent menu
     * @param context Document context
     * @return Created submenu
     */
    ElaMenu* createZoomSubmenu(ElaMenu* parent, const DocumentContext& context);

    /**
     * @brief Create submenu for page operations
     * @param parent Parent menu
     * @param context Document context
     * @return Created submenu
     */
    ElaMenu* createPageSubmenu(ElaMenu* parent, const DocumentContext& context);

    /**
     * @brief Create submenu for view operations
     * @param parent Parent menu
     * @param context Document context
     * @return Created submenu
     */
    ElaMenu* createViewSubmenu(ElaMenu* parent, const DocumentContext& context);

    /**
     * @brief Apply consistent styling to menu
     * @param menu Menu to style
     */
    void applyMenuStyling(ElaMenu* menu);
    void applyMenuStyling(QMenu* menu);

    /**
     * @brief Validate context before showing menu
     * @param context Context to validate
     * @return True if context is valid
     */
    bool validateContext(const DocumentContext& context) const;

    /**
     * @brief Validate UI element context
     * @param context UI element context to validate
     * @return True if context is valid
     */
    bool validateUIContext(const UIElementContext& context) const;

    /**
     * @brief Handle action execution with error handling
     * @param action Action to execute
     * @param context Additional context
     */
    void executeAction(ActionMap action,
                       const QVariantMap& context = QVariantMap());

    /**
     * @brief Handle custom action execution with error handling
     * @param actionId Custom action ID
     * @param context Additional context
     */
    void executeCustomAction(const QString& actionId,
                             const QVariantMap& context = QVariantMap());

private:
    // Menu caching for performance
    QHash<MenuType, ElaMenu*> m_menuCache;

    // Current context information
    DocumentContext m_currentDocumentContext;
    UIElementContext m_currentUIContext;

    // Action tracking for context
    QHash<QAction*, ActionMap> m_actionMap;
    QHash<QAction*, QString> m_customActionMap;
    QHash<QAction*, QVariantMap> m_actionContextMap;

    // Menu styling
    QString m_menuStyleSheet;

    // Error handling
    bool m_errorHandlingEnabled;
};
