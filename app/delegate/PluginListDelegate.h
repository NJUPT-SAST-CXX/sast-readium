#pragma once

#include <QModelIndex>
#include <QPainter>
#include <QStyleOptionViewItem>
#include <QStyledItemDelegate>

#include "../logging/SimpleLogging.h"

/**
 * @brief Custom delegate for rendering plugin items in lists and tables
 *
 * This delegate provides rich visual representation of plugins with:
 * - Plugin icon and name
 * - Version and author information
 * - Load/enable status indicators
 * - Visual feedback for errors
 * - Hover effects
 * - Custom colors based on state
 *
 * The delegate supports both list views and table views with automatic
 * layout adjustment based on available space.
 */
class PluginListDelegate : public QStyledItemDelegate {
    Q_OBJECT

public:
    /**
     * Display mode for the delegate
     */
    enum DisplayMode {
        CompactMode,  // Single line with minimal info
        NormalMode,   // Two lines with name, version, and status
        DetailedMode  // Multi-line with full details
    };
    Q_ENUM(DisplayMode)

    explicit PluginListDelegate(QObject* parent = nullptr);
    ~PluginListDelegate() override = default;

    Q_DISABLE_COPY_MOVE(PluginListDelegate)

    // QStyledItemDelegate interface
    void paint(QPainter* painter, const QStyleOptionViewItem& option,
               const QModelIndex& index) const override;
    [[nodiscard]] QSize sizeHint(const QStyleOptionViewItem& option,
                                 const QModelIndex& index) const override;

    // Display mode
    void setDisplayMode(DisplayMode mode) { m_displayMode = mode; }
    [[nodiscard]] DisplayMode displayMode() const { return m_displayMode; }

    // Visual customization
    void setShowIcons(bool show) { m_showIcons = show; }
    [[nodiscard]] bool showIcons() const { return m_showIcons; }

    void setShowStatus(bool show) { m_showStatus = show; }
    [[nodiscard]] bool showStatus() const { return m_showStatus; }

    void setHighlightErrors(bool highlight) { m_highlightErrors = highlight; }
    [[nodiscard]] bool highlightErrors() const { return m_highlightErrors; }

    // Colors
    void setLoadedColor(const QColor& color) { m_loadedColor = color; }
    [[nodiscard]] QColor loadedColor() const { return m_loadedColor; }

    void setDisabledColor(const QColor& color) { m_disabledColor = color; }
    [[nodiscard]] QColor disabledColor() const { return m_disabledColor; }

    void setErrorColor(const QColor& color) { m_errorColor = color; }
    [[nodiscard]] QColor errorColor() const { return m_errorColor; }

protected:
    void paintCompact(QPainter* painter, const QStyleOptionViewItem& option,
                      const QModelIndex& index) const;
    void paintNormal(QPainter* painter, const QStyleOptionViewItem& option,
                     const QModelIndex& index) const;
    void paintDetailed(QPainter* painter, const QStyleOptionViewItem& option,
                       const QModelIndex& index) const;

    void drawStatusIndicator(QPainter* painter, const QRect& rect,
                             bool isLoaded, bool isEnabled,
                             bool hasErrors) const;
    void drawPluginIcon(QPainter* painter, const QRect& rect,
                        const QModelIndex& index) const;

    [[nodiscard]] QColor getBackgroundColor(const QStyleOptionViewItem& option,
                                            bool isLoaded, bool isEnabled,
                                            bool hasErrors) const;
    [[nodiscard]] QColor getTextColor(const QStyleOptionViewItem& option,
                                      bool isLoaded, bool isEnabled,
                                      bool hasErrors) const;

    [[nodiscard]] QString getStatusText(bool isLoaded, bool isEnabled,
                                        bool hasErrors) const;

private:
    DisplayMode m_displayMode;
    bool m_showIcons;
    bool m_showStatus;
    bool m_highlightErrors;

    // Colors
    QColor m_loadedColor;
    QColor m_disabledColor;
    QColor m_errorColor;

    // Spacing constants
    static constexpr int ICON_SIZE = 32;
    static constexpr int COMPACT_ICON_SIZE = 16;
    static constexpr int STATUS_INDICATOR_SIZE = 8;
    static constexpr int MARGIN = 4;
    static constexpr int SPACING = 6;
    static constexpr int LINE_SPACING = 2;

    // Logging
    SastLogging::CategoryLogger m_logger{"PluginListDelegate"};
};
