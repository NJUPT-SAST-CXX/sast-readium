#include "RenderFilterPlugin.h"
#include <QDebug>
#include <QFont>
#include <QFontMetrics>
#include <QtMath>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

RenderFilterPlugin::RenderFilterPlugin(QObject* parent)
    : PluginBase(parent),
      m_activeFilter("none"),
      m_brightness(0),
      m_contrast(0),
      m_enableWatermark(false),
      m_watermarkText("SAMPLE"),
      m_watermarkColor(Qt::gray),
      m_watermarkOpacity(30),
      m_watermarkSize(48),
      m_pagesProcessed(0),
      m_filtersApplied(0) {
    // Set plugin metadata
    m_metadata.name = "Render Filter";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Provides page rendering filters including night mode, sepia, "
        "grayscale, and watermark overlay";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();

    // Declare capabilities
    m_capabilities.provides = QStringList()
                              << "render.filter" << "render.night_mode"
                              << "render.sepia" << "render.watermark"
                              << "render.overlay";
}

RenderFilterPlugin::~RenderFilterPlugin() {
    // Cleanup handled in onShutdown()
}

bool RenderFilterPlugin::onInitialize() {
    m_logger.info("RenderFilterPlugin: Initializing...");

    // Load configuration
    m_activeFilter = m_configuration.value("activeFilter").toString("none");
    m_brightness = m_configuration.value("brightness").toInt(0);
    m_contrast = m_configuration.value("contrast").toInt(0);
    m_enableWatermark = m_configuration.value("enableWatermark").toBool(false);
    m_watermarkText = m_configuration.value("watermarkText").toString("SAMPLE");
    m_watermarkOpacity = m_configuration.value("watermarkOpacity").toInt(30);
    m_watermarkSize = m_configuration.value("watermarkSize").toInt(48);

    // Register hooks
    registerHooks();

    // Setup event subscriptions
    setupEventSubscriptions();

    m_logger.info("RenderFilterPlugin: Active filter: {}",
                  m_activeFilter.toStdString());
    m_logger.info("RenderFilterPlugin: Initialized successfully");
    return true;
}

void RenderFilterPlugin::onShutdown() {
    m_logger.info("RenderFilterPlugin: Shutting down...");

    removeEventSubscriptions();
    unregisterHooks();

    m_logger.info(
        "RenderFilterPlugin: Statistics - Pages: {}, Filters applied: {}",
        m_pagesProcessed, m_filtersApplied);
}

// ============================================================================
// Inter-plugin Communication
// ============================================================================

void RenderFilterPlugin::handleMessage(const QString& from,
                                       const QVariant& message) {
    m_logger.info("RenderFilterPlugin: Received message from '{}'",
                  from.toStdString());

    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "set_filter") {
        // Set active filter
        QString filter = msgMap.value("filter").toString();
        if (filter == "none" || filter == "night" || filter == "sepia" ||
            filter == "grayscale") {
            m_activeFilter = filter;
            m_logger.info("RenderFilterPlugin: Filter set to '{}'",
                          filter.toStdString());
        }
    } else if (action == "set_watermark") {
        // Configure watermark
        m_enableWatermark = msgMap.value("enabled").toBool(m_enableWatermark);
        if (msgMap.contains("text"))
            m_watermarkText = msgMap.value("text").toString();
        if (msgMap.contains("opacity"))
            m_watermarkOpacity = msgMap.value("opacity").toInt();
    } else if (action == "get_status") {
        // Return current status
        Event* responseEvent = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["activeFilter"] = m_activeFilter;
        data["watermarkEnabled"] = m_enableWatermark;
        data["pagesProcessed"] = m_pagesProcessed;
        responseEvent->setData(QVariant::fromValue(data));
        eventBus()->publish(responseEvent);
    }
}

// ============================================================================
// Event Subscriptions
// ============================================================================

void RenderFilterPlugin::setupEventSubscriptions() {
    m_logger.debug("RenderFilterPlugin: Setting up event subscriptions");

    eventBus()->subscribe("settings.changed", this, [this](Event* event) {
        QVariantMap data = event->data().toMap();
        QString key = data.value("key").toString();

        if (key == "renderFilter") {
            m_activeFilter = data.value("value").toString();
            m_logger.info("RenderFilterPlugin: Filter changed to '{}'",
                          m_activeFilter.toStdString());
        } else if (key == "brightness") {
            m_brightness = data.value("value").toInt();
        } else if (key == "contrast") {
            m_contrast = data.value("value").toInt();
        }
    });

    m_logger.debug("RenderFilterPlugin: Event subscriptions set up");
}

void RenderFilterPlugin::removeEventSubscriptions() {
    eventBus()->unsubscribeAll(this);
}

// ============================================================================
// Hook Registration
// ============================================================================

void RenderFilterPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();

    registry.registerCallback(StandardHooks::RENDER_PRE_PAGE, name(),
                              [this](const QVariantMap& context) {
                                  return onRenderPrePage(context);
                              });

    registry.registerCallback(StandardHooks::RENDER_POST_PAGE, name(),
                              [this](const QVariantMap& context) {
                                  return onRenderPostPage(context);
                              });

    registry.registerCallback(StandardHooks::RENDER_APPLY_FILTER, name(),
                              [this](const QVariantMap& context) {
                                  return onRenderApplyFilter(context);
                              });

    m_logger.debug("RenderFilterPlugin: Registered 3 hook callbacks");
}

void RenderFilterPlugin::unregisterHooks() {
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
}

QVariant RenderFilterPlugin::onRenderPrePage(const QVariantMap& context) {
    int pageNumber = context.value("pageNumber").toInt();
    m_logger.debug("RenderFilterPlugin: [HOOK] Pre-render page {}", pageNumber);

    QVariantMap result;
    result["filterEnabled"] = (m_activeFilter != "none");
    result["filterType"] = m_activeFilter;
    return result;
}

QVariant RenderFilterPlugin::onRenderPostPage(const QVariantMap& context) {
    int pageNumber = context.value("pageNumber").toInt();
    m_pagesProcessed++;

    m_logger.debug("RenderFilterPlugin: [HOOK] Post-render page {} (total: {})",
                   pageNumber, m_pagesProcessed);

    QVariantMap result;
    result["processed"] = true;
    result["totalPages"] = m_pagesProcessed;
    return result;
}

QVariant RenderFilterPlugin::onRenderApplyFilter(const QVariantMap& context) {
    QString filterType = context.value("filterType").toString();
    m_logger.debug("RenderFilterPlugin: [HOOK] Apply filter request: {}",
                   filterType.toStdString());

    QVariantMap result;
    result["handled"] = (filterType == m_activeFilter);
    result["pluginName"] = name();
    return result;
}

// ============================================================================
// IRenderPlugin Implementation
// ============================================================================

RenderFilterType RenderFilterPlugin::filterType() const {
    if (m_activeFilter == "night" || m_activeFilter == "sepia" ||
        m_activeFilter == "grayscale") {
        return RenderFilterType::ColorAdjustment;
    }
    if (m_enableWatermark) {
        return RenderFilterType::Overlay;
    }
    return RenderFilterType::Custom;
}

bool RenderFilterPlugin::shouldProcessPage(const QString& documentPath,
                                           int pageNumber) const {
    Q_UNUSED(documentPath)
    Q_UNUSED(pageNumber)

    // Process all pages if any filter is active
    return (m_activeFilter != "none") || m_enableWatermark ||
           (m_brightness != 0) || (m_contrast != 0);
}

bool RenderFilterPlugin::applyFilter(QImage& image, int pageNumber,
                                     const QJsonObject& options) {
    if (image.isNull()) {
        return false;
    }

    m_logger.debug("RenderFilterPlugin: Applying filter to page {}",
                   pageNumber);

    // Apply color filter
    if (m_activeFilter == "night") {
        applyNightMode(image);
        m_filtersApplied++;
    } else if (m_activeFilter == "sepia") {
        applySepiaFilter(image);
        m_filtersApplied++;
    } else if (m_activeFilter == "grayscale") {
        applyGrayscale(image);
        m_filtersApplied++;
    }

    // Apply brightness/contrast adjustments
    if (m_brightness != 0 || m_contrast != 0) {
        applyBrightnessContrast(image, m_brightness, m_contrast);
        m_filtersApplied++;
    }

    return true;
}

void RenderFilterPlugin::renderOverlay(QPainter* painter, const QRect& rect,
                                       int pageNumber,
                                       const QJsonObject& options) {
    if (!m_enableWatermark || !painter) {
        return;
    }

    m_logger.debug("RenderFilterPlugin: Rendering watermark on page {}",
                   pageNumber);

    painter->save();

    // Set up watermark appearance
    QFont font("Arial", m_watermarkSize);
    font.setBold(true);
    painter->setFont(font);

    QColor color = m_watermarkColor;
    color.setAlpha(static_cast<int>(255 * m_watermarkOpacity / 100.0));
    painter->setPen(color);

    // Calculate text metrics
    QFontMetrics fm(font);
    int textWidth = fm.horizontalAdvance(m_watermarkText);
    int textHeight = fm.height();

    // Draw repeating watermark pattern
    int spacingX = textWidth + 100;
    int spacingY = textHeight + 100;

    painter->rotate(-30);  // Diagonal watermark

    for (int y = -rect.height(); y < rect.height() * 2; y += spacingY) {
        for (int x = -rect.width(); x < rect.width() * 2; x += spacingX) {
            painter->drawText(x, y, m_watermarkText);
        }
    }

    painter->restore();
}

int RenderFilterPlugin::filterPriority() const {
    // Higher priority = applied first
    // Color adjustments should be applied before overlays
    if (m_activeFilter != "none") {
        return 80;  // High priority for color filters
    }
    return 50;  // Default priority
}

bool RenderFilterPlugin::isThreadSafe() const {
    // Our filter implementations are stateless and can run in parallel
    return true;
}

// ============================================================================
// Filter Implementations
// ============================================================================

void RenderFilterPlugin::applyNightMode(QImage& image) {
    // Invert colors for night mode
    image.invertPixels();

    // Optionally reduce blue light
    for (int y = 0; y < image.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            int r = qRed(line[x]);
            int g = qGreen(line[x]);
            int b = qBlue(line[x]);
            int a = qAlpha(line[x]);

            // Warm up colors slightly (reduce blue)
            b = qBound(0, b - 20, 255);
            r = qBound(0, r + 10, 255);

            line[x] = qRgba(r, g, b, a);
        }
    }
}

void RenderFilterPlugin::applySepiaFilter(QImage& image) {
    for (int y = 0; y < image.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            int r = qRed(line[x]);
            int g = qGreen(line[x]);
            int b = qBlue(line[x]);
            int a = qAlpha(line[x]);

            // Sepia transformation
            int newR = qBound(
                0, static_cast<int>(0.393 * r + 0.769 * g + 0.189 * b), 255);
            int newG = qBound(
                0, static_cast<int>(0.349 * r + 0.686 * g + 0.168 * b), 255);
            int newB = qBound(
                0, static_cast<int>(0.272 * r + 0.534 * g + 0.131 * b), 255);

            line[x] = qRgba(newR, newG, newB, a);
        }
    }
}

void RenderFilterPlugin::applyBrightnessContrast(QImage& image, int brightness,
                                                 int contrast) {
    // Brightness: -100 to +100
    // Contrast: -100 to +100

    double factor = (259.0 * (contrast + 255.0)) / (255.0 * (259.0 - contrast));

    for (int y = 0; y < image.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            int r = qRed(line[x]);
            int g = qGreen(line[x]);
            int b = qBlue(line[x]);
            int a = qAlpha(line[x]);

            // Apply brightness
            r = qBound(0, r + brightness, 255);
            g = qBound(0, g + brightness, 255);
            b = qBound(0, b + brightness, 255);

            // Apply contrast
            r = qBound(0, static_cast<int>(factor * (r - 128) + 128), 255);
            g = qBound(0, static_cast<int>(factor * (g - 128) + 128), 255);
            b = qBound(0, static_cast<int>(factor * (b - 128) + 128), 255);

            line[x] = qRgba(r, g, b, a);
        }
    }
}

void RenderFilterPlugin::applyGrayscale(QImage& image) {
    for (int y = 0; y < image.height(); ++y) {
        QRgb* line = reinterpret_cast<QRgb*>(image.scanLine(y));
        for (int x = 0; x < image.width(); ++x) {
            int r = qRed(line[x]);
            int g = qGreen(line[x]);
            int b = qBlue(line[x]);
            int a = qAlpha(line[x]);

            // Luminosity method for grayscale
            int gray = static_cast<int>(0.299 * r + 0.587 * g + 0.114 * b);

            line[x] = qRgba(gray, gray, gray, a);
        }
    }
}
