#pragma once

#include <QImage>
#include <QObject>
#include <QPainter>
#include "plugin/IRenderPlugin.h"
#include "plugin/PluginInterface.h"

/**
 * @brief RenderFilterPlugin - Example render plugin
 *
 * This plugin demonstrates the IRenderPlugin interface by providing:
 * - **Night Mode Filter**: Inverts colors for comfortable dark reading
 * - **Sepia Filter**: Warm, vintage color adjustment
 * - **Watermark Overlay**: Customizable text/image overlay
 * - **Brightness/Contrast**: Basic image adjustments
 *
 * Features demonstrated:
 * - Multiple filter types with priority ordering
 * - Thread-safe parallel processing
 * - Configuration-driven filter parameters
 * - Hook registration for render workflow
 */
class RenderFilterPlugin : public PluginBase, public IRenderPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "render_filter.json")
    Q_INTERFACES(IPluginInterface IRenderPlugin)

public:
    explicit RenderFilterPlugin(QObject* parent = nullptr);
    ~RenderFilterPlugin() override;

    // IPluginInterface override
    void handleMessage(const QString& from, const QVariant& message) override;

protected:
    // PluginBase overrides
    bool onInitialize() override;
    void onShutdown() override;

    // IRenderPlugin interface
    RenderFilterType filterType() const override;
    bool shouldProcessPage(const QString& documentPath,
                           int pageNumber) const override;
    bool applyFilter(QImage& image, int pageNumber,
                     const QJsonObject& options) override;
    void renderOverlay(QPainter* painter, const QRect& rect, int pageNumber,
                       const QJsonObject& options) override;
    int filterPriority() const override;
    bool isThreadSafe() const override;

private:
    void registerHooks();
    void unregisterHooks();
    void setupEventSubscriptions();
    void removeEventSubscriptions();

    // Filter implementations
    void applyNightMode(QImage& image);
    void applySepiaFilter(QImage& image);
    void applyBrightnessContrast(QImage& image, int brightness, int contrast);
    void applyGrayscale(QImage& image);

    // Hook callbacks
    QVariant onRenderPrePage(const QVariantMap& context);
    QVariant onRenderPostPage(const QVariantMap& context);
    QVariant onRenderApplyFilter(const QVariantMap& context);

    // Configuration
    QString m_activeFilter;  // "none", "night", "sepia", "grayscale"
    int m_brightness;
    int m_contrast;
    bool m_enableWatermark;
    QString m_watermarkText;
    QColor m_watermarkColor;
    int m_watermarkOpacity;
    int m_watermarkSize;

    // Statistics
    int m_pagesProcessed;
    int m_filtersApplied;
};
