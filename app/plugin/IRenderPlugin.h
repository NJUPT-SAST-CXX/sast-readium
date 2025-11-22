#pragma once

#include <QImage>
#include <QJsonObject>
#include <QObject>
#include <QPainter>
#include <QRect>
#include <QString>

/**
 * @brief Render Filter Type
 */
enum class RenderFilterType {
    ColorAdjustment,   // Adjust colors (brightness, contrast, saturation)
    ImageEnhancement,  // Enhance image quality (sharpen, denoise)
    Overlay,           // Add overlays (watermarks, annotations)
    Transform,         // Geometric transforms (rotate, scale, crop)
    Custom             // Custom filter type
};

/**
 * @brief IRenderPlugin - Interface for custom rendering plugins
 *
 * Plugins implementing this interface can customize page rendering by applying
 * filters, overlays, or providing alternative rendering backends.
 */
class IRenderPlugin {
public:
    virtual ~IRenderPlugin() = default;

    /**
     * @brief Get the render filter type
     */
    virtual RenderFilterType filterType() const = 0;

    /**
     * @brief Check if plugin should process this page
     * @param documentPath Document path
     * @param pageNumber Page number (0-based)
     * @return True if plugin should process this page
     */
    virtual bool shouldProcessPage(const QString& documentPath,
                                   int pageNumber) const = 0;

    /**
     * @brief Apply rendering filter to page image
     * @param image The rendered page image (input/output)
     * @param pageNumber Page number (0-based)
     * @param options Filter options
     * @return True if filter was applied successfully
     */
    virtual bool applyFilter(QImage& image, int pageNumber,
                             const QJsonObject& options) = 0;

    /**
     * @brief Render overlay on top of page
     * @param painter QPainter to draw with
     * @param rect The page rectangle
     * @param pageNumber Page number (0-based)
     * @param options Overlay options
     */
    virtual void renderOverlay(QPainter* painter, const QRect& rect,
                               int pageNumber, const QJsonObject& options) = 0;

    /**
     * @brief Get filter priority (higher = applied first)
     * @return Priority value (0-100)
     */
    virtual int filterPriority() const { return 50; }

    /**
     * @brief Check if filter can be applied in parallel
     * @return True if filter is thread-safe
     */
    virtual bool isThreadSafe() const { return false; }
};

Q_DECLARE_INTERFACE(IRenderPlugin, "com.sast.readium.IRenderPlugin/1.0")
