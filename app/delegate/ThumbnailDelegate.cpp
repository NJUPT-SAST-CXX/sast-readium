#include "ThumbnailDelegate.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QDateTime>
#include <QDebug>
#include <QElapsedTimer>
#include <QFont>
#include <QHash>
#include <QModelIndex>
#include <QMouseEvent>
#include <QMutex>
#include <QPainter>
#include <QPersistentModelIndex>
#include <QPixmap>
#include <QPropertyAnimation>
#include <QSettings>
#include <QStyleOptionViewItem>
#include <QTimer>
#include <atomic>
#include "../managers/StyleManager.h"
#include "../model/ThumbnailModel.h"

// Implementation class definition
class ThumbnailDelegate::Implementation {
public:
    // Constants
    static constexpr int DEFAULT_THUMBNAIL_WIDTH = 120;
    static constexpr int DEFAULT_THUMBNAIL_HEIGHT = 160;
    static constexpr int DEFAULT_MARGIN = 8;
    static constexpr int DEFAULT_BORDER_RADIUS = 8;
    static constexpr int DEFAULT_PAGE_NUMBER_HEIGHT = 24;
    static constexpr int DEFAULT_SHADOW_BLUR_RADIUS = 12;
    static constexpr int DEFAULT_SHADOW_OFFSET = 2;
    static constexpr int DEFAULT_BORDER_WIDTH = 2;
    static constexpr int LOADING_SPINNER_SIZE = 24;
    static constexpr int LOADING_ANIMATION_INTERVAL = 50;     // ms
    static constexpr int HOVER_ANIMATION_DURATION = 200;      // ms
    static constexpr int SELECTION_ANIMATION_DURATION = 300;  // ms
    static constexpr int DEFAULT_MAX_CACHE_SIZE = 50;
    static constexpr qint64 CACHE_EXPIRY_TIME = 300000;      // 5分钟
    static constexpr qint64 CACHE_CLEANUP_INTERVAL = 60000;  // 1分钟
    static constexpr double PERFORMANCE_SAMPLE_RATE = 0.1;   // 10%采样率

    // Chrome风格颜色常量
    static const QColor GOOGLE_BLUE;
    static const QColor GOOGLE_BLUE_DARK;
    static const QColor GOOGLE_RED;
    static const QColor LIGHT_BACKGROUND;
    static const QColor LIGHT_BORDER;
    static const QColor LIGHT_TEXT;
    static const QColor DARK_BACKGROUND;
    static const QColor DARK_BORDER;
    static const QColor DARK_TEXT;

    explicit Implementation(ThumbnailDelegate* q)
        : q_ptr(q),
          thumbnailSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT),
          margin(DEFAULT_MARGIN),
          borderRadius(DEFAULT_BORDER_RADIUS),
          pageNumberHeight(DEFAULT_PAGE_NUMBER_HEIGHT),
          shadowEnabled(true),
          animationEnabled(true),
          shadowBlurRadius(DEFAULT_SHADOW_BLUR_RADIUS),
          shadowOffset(DEFAULT_SHADOW_OFFSET),
          borderWidth(DEFAULT_BORDER_WIDTH),
          renderCacheEnabled(true),
          maxCacheSize(DEFAULT_MAX_CACHE_SIZE),
          highQualityRendering(true),
          antiAliasingEnabled(true),
          smoothPixmapTransform(true),
          loadingTimer(new QTimer(q)) {
        // 设置默认颜色主题
        setLightTheme();

        // 设置字体
        pageNumberFont = QFont("Arial", 9);
        errorFont = QFont("Arial", 8);

        // 设置加载动画定时器
        loadingTimer->setInterval(LOADING_ANIMATION_INTERVAL);
        QObject::connect(loadingTimer, &QTimer::timeout, q,
                         &ThumbnailDelegate::onLoadingAnimationTimer);

        // 设置缓存清理定时器
        QTimer* cacheCleanupTimer = new QTimer(q);
        cacheCleanupTimer->setInterval(CACHE_CLEANUP_INTERVAL);
        QObject::connect(cacheCleanupTimer, &QTimer::timeout, q,
                         [this]() { cleanupExpiredCache(); });
        cacheCleanupTimer->start();
    }

    ~Implementation() {
        cleanupAnimations();
        clearRenderCache();
    }

    // Animation state structure
    struct AnimationState {
        qreal hoverOpacity = 0.0;
        qreal selectionOpacity = 0.0;
        int loadingAngle = 0;
        QPropertyAnimation* hoverAnimation = nullptr;
        QPropertyAnimation* selectionAnimation = nullptr;
        qint64 lastUpdate = 0;
        bool needsUpdate = true;

        AnimationState() = default;
        ~AnimationState() {
            delete hoverAnimation;
            delete selectionAnimation;
        }
    };

    // Render cache structure
    struct RenderCache {
        QPixmap cachedBackground;
        QPixmap cachedBorder;
        QPixmap cachedShadow;
        QSize cacheSize;
        int cacheState;
        qint64 timestamp;
        bool isValid;

        RenderCache() : cacheState(0), timestamp(0), isValid(false) {}
    };

    // Performance statistics structure
    struct PerformanceStats {
        std::atomic<int> paintCalls{0};
        std::atomic<qint64> totalPaintTime{0};
        std::atomic<int> cacheHits{0};
        std::atomic<int> cacheMisses{0};
        QElapsedTimer sessionTimer;

        PerformanceStats() { sessionTimer.start(); }

        double averagePaintTime() const {
            int calls = paintCalls.load();
            return calls > 0
                       ? static_cast<double>(totalPaintTime.load()) / calls
                       : 0.0;
        }

        double cacheHitRate() const {
            int hits = cacheHits.load();
            int misses = cacheMisses.load();
            int total = hits + misses;
            return total > 0 ? static_cast<double>(hits) / total : 0.0;
        }
    };

    // Member variables
    ThumbnailDelegate* q_ptr;

    // 尺寸设置
    QSize thumbnailSize;
    int margin;
    int borderRadius;
    int pageNumberHeight;

    // 视觉效果设置
    bool shadowEnabled;
    bool animationEnabled;
    int shadowBlurRadius;
    int shadowOffset;
    int borderWidth;

    // 颜色主题
    QColor backgroundColor;
    QColor borderColorNormal;
    QColor borderColorHovered;
    QColor borderColorSelected;
    QColor shadowColor;
    QColor pageNumberBgColor;
    QColor pageNumberTextColor;
    QColor loadingColor;
    QColor errorColor;
    QColor placeholderColor;

    // 动画管理
    mutable QHash<QPersistentModelIndex, AnimationState*> animationStates;
    QTimer* loadingTimer;

    // 渲染缓存
    mutable QHash<QString, RenderCache*> renderCache;
    mutable QMutex cacheMutex;
    bool renderCacheEnabled;
    int maxCacheSize;

    // 渲染选项
    bool highQualityRendering;
    bool antiAliasingEnabled;
    bool smoothPixmapTransform;

    // 性能统计
    mutable PerformanceStats performanceStats;

    // 字体
    QFont pageNumberFont;
    QFont errorFont;

    // Theme methods
    void setLightTheme();
    void setDarkTheme();

    // Helper methods
    void paintThumbnail(QPainter* painter, const QRect& rect,
                        const QPixmap& pixmap,
                        const QStyleOptionViewItem& option) const;
    void paintBackground(QPainter* painter, const QRect& rect,
                         const QStyleOptionViewItem& option) const;
    void paintBorder(QPainter* painter, const QRect& rect,
                     const QStyleOptionViewItem& option) const;
    void paintShadow(QPainter* painter, const QRect& rect,
                     const QStyleOptionViewItem& option) const;
    void paintPageNumber(QPainter* painter, const QRect& rect, int pageNumber,
                         const QStyleOptionViewItem& option) const;
    void paintLoadingIndicator(QPainter* painter, const QRect& rect,
                               const QStyleOptionViewItem& option) const;
    void paintErrorIndicator(QPainter* painter, const QRect& rect,
                             const QString& errorMessage,
                             const QStyleOptionViewItem& option) const;

    QRect getThumbnailRect(const QRect& itemRect) const;
    QRect getPageNumberRect(const QRect& thumbnailRect) const;

    AnimationState* getAnimationState(const QModelIndex& index) const;
    void updateHoverState(const QModelIndex& index, bool hovered);
    void updateSelectionState(const QModelIndex& index, bool selected);

    void setupAnimations(AnimationState* state, const QModelIndex& index) const;
    void cleanupAnimations();

    // Performance optimization methods
    Qt::TransformationMode getOptimalTransformationMode(
        const QSize& sourceSize, const QSize& targetSize) const;

    // Cache management methods
    QString generateCacheKey(const QModelIndex& index,
                             const QStyleOptionViewItem& option) const;
    RenderCache* getRenderCache(const QString& key) const;
    void insertRenderCache(const QString& key, RenderCache* cache);
    void cleanupExpiredCache();

    // Optimized rendering methods
    void paintOptimized(QPainter* painter, const QStyleOptionViewItem& option,
                        const QModelIndex& index) const;
    QPixmap renderToCache(const QStyleOptionViewItem& option,
                          const QModelIndex& index) const;
    void paintFromCache(QPainter* painter, const QRect& rect,
                        const QPixmap& cached) const;

    // Performance monitoring methods
    void recordPaintTime(qint64 time) const;
    void updatePerformanceStats() const;
    void clearRenderCache();
};

// Chrome风格颜色常量定义
const QColor ThumbnailDelegate::Implementation::GOOGLE_BLUE =
    QColor(66, 133, 244);
const QColor ThumbnailDelegate::Implementation::GOOGLE_BLUE_DARK =
    QColor(26, 115, 232);
const QColor ThumbnailDelegate::Implementation::GOOGLE_RED =
    QColor(234, 67, 53);
const QColor ThumbnailDelegate::Implementation::LIGHT_BACKGROUND =
    QColor(255, 255, 255);
const QColor ThumbnailDelegate::Implementation::LIGHT_BORDER =
    QColor(200, 200, 200);
const QColor ThumbnailDelegate::Implementation::LIGHT_TEXT = QColor(60, 60, 60);
const QColor ThumbnailDelegate::Implementation::DARK_BACKGROUND =
    QColor(32, 33, 36);
const QColor ThumbnailDelegate::Implementation::DARK_BORDER =
    QColor(95, 99, 104);
const QColor ThumbnailDelegate::Implementation::DARK_TEXT =
    QColor(232, 234, 237);

// Implementation class method definitions
void ThumbnailDelegate::Implementation::setLightTheme() {
    backgroundColor = LIGHT_BACKGROUND;
    borderColorNormal = LIGHT_BORDER;
    borderColorHovered = GOOGLE_BLUE.lighter(150);
    borderColorSelected = GOOGLE_BLUE;
    shadowColor = QColor(0, 0, 0, 50);
    pageNumberBgColor = QColor(240, 240, 240);
    pageNumberTextColor = LIGHT_TEXT;
    loadingColor = GOOGLE_BLUE;
    errorColor = GOOGLE_RED;
    placeholderColor = QColor(200, 200, 200);
}

void ThumbnailDelegate::Implementation::setDarkTheme() {
    backgroundColor = DARK_BACKGROUND;
    borderColorNormal = DARK_BORDER;
    borderColorHovered = GOOGLE_BLUE.lighter(150);
    borderColorSelected = GOOGLE_BLUE;
    shadowColor = QColor(0, 0, 0, 100);
    pageNumberBgColor = QColor(60, 60, 60);
    pageNumberTextColor = DARK_TEXT;
    loadingColor = GOOGLE_BLUE;
    errorColor = GOOGLE_RED;
    placeholderColor = QColor(100, 100, 100);
}

ThumbnailDelegate::ThumbnailDelegate(QObject* parent)
    : QStyledItemDelegate(parent), d(std::make_unique<Implementation>(this)) {}

ThumbnailDelegate::~ThumbnailDelegate() = default;

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const {
    Q_UNUSED(option)
    Q_UNUSED(index)

    return QSize(
        d->thumbnailSize.width() + 2 * d->margin,
        d->thumbnailSize.height() + d->pageNumberHeight + 2 * d->margin);
}

void ThumbnailDelegate::paint(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }

    // 性能监控
    QElapsedTimer paintTimer;
    paintTimer.start();

    // 使用优化渲染
    if (d->renderCacheEnabled) {
        d->paintOptimized(painter, option, index);
    } else {
        // 传统渲染路径
        painter->save();

        if (d->antiAliasingEnabled) {
            painter->setRenderHint(QPainter::Antialiasing);
        }
        if (d->smoothPixmapTransform) {
            painter->setRenderHint(QPainter::SmoothPixmapTransform);
        }
        if (d->highQualityRendering) {
            painter->setRenderHint(QPainter::TextAntialiasing);
        }

        // 获取数据
        QPixmap thumbnail =
            index.data(ThumbnailModel::PixmapRole).value<QPixmap>();
        bool isLoading = index.data(ThumbnailModel::LoadingRole).toBool();
        bool hasError = index.data(ThumbnailModel::ErrorRole).toBool();
        QString errorMessage =
            index.data(ThumbnailModel::ErrorMessageRole).toString();
        int pageNumber = index.data(ThumbnailModel::PageNumberRole).toInt();

        // 计算矩形
        QRect thumbnailRect = d->getThumbnailRect(option.rect);
        QRect pageNumberRect = d->getPageNumberRect(thumbnailRect);

        // 绘制背景
        d->paintBackground(painter, option.rect, option);

        // 绘制阴影
        if (d->shadowEnabled) {
            d->paintShadow(painter, thumbnailRect, option);
        }

        // 绘制边框
        d->paintBorder(painter, thumbnailRect, option);

        // 绘制缩略图内容
        if (hasError) {
            d->paintErrorIndicator(painter, thumbnailRect, errorMessage,
                                   option);
        } else if (isLoading) {
            d->paintLoadingIndicator(painter, thumbnailRect, option);
        } else if (!thumbnail.isNull()) {
            d->paintThumbnail(painter, thumbnailRect, thumbnail, option);
        }

        // 绘制页码
        d->paintPageNumber(painter, pageNumberRect, pageNumber, option);

        painter->restore();
    }

    // 记录性能数据
    qint64 paintTime = paintTimer.elapsed();
    d->recordPaintTime(paintTime);
}

void ThumbnailDelegate::setThumbnailSize(const QSize& size) {
    if (d->thumbnailSize != size) {
        d->thumbnailSize = size;
        emit sizeHintChanged(QModelIndex());
    }
}

void ThumbnailDelegate::setMargins(int margin) {
    if (d->margin != margin) {
        d->margin = margin;
        emit sizeHintChanged(QModelIndex());
    }
}

void ThumbnailDelegate::setBorderRadius(int radius) {
    d->borderRadius = radius;
}

void ThumbnailDelegate::setShadowEnabled(bool enabled) {
    d->shadowEnabled = enabled;
}

void ThumbnailDelegate::setAnimationEnabled(bool enabled) {
    d->animationEnabled = enabled;
    if (!enabled) {
        d->cleanupAnimations();
    }
}

void ThumbnailDelegate::setLightTheme() { d->setLightTheme(); }

void ThumbnailDelegate::setDarkTheme() { d->setDarkTheme(); }

void ThumbnailDelegate::setCustomColors(const QColor& background,
                                        const QColor& border,
                                        const QColor& text,
                                        const QColor& accent) {
    d->backgroundColor = background;
    d->borderColorNormal = border;
    d->borderColorHovered = accent.lighter(150);
    d->borderColorSelected = accent;
    d->pageNumberTextColor = text;
    d->loadingColor = accent;
}

// Getter methods
QSize ThumbnailDelegate::thumbnailSize() const { return d->thumbnailSize; }

int ThumbnailDelegate::margins() const { return d->margin; }

int ThumbnailDelegate::borderRadius() const { return d->borderRadius; }

bool ThumbnailDelegate::shadowEnabled() const { return d->shadowEnabled; }

bool ThumbnailDelegate::animationEnabled() const { return d->animationEnabled; }

bool ThumbnailDelegate::isRenderCacheEnabled() const {
    return d->renderCacheEnabled;
}

bool ThumbnailDelegate::isHighQualityRenderingEnabled() const {
    return d->highQualityRendering;
}

bool ThumbnailDelegate::isAntiAliasingEnabled() const {
    return d->antiAliasingEnabled;
}

int ThumbnailDelegate::maxCacheSize() const { return d->maxCacheSize; }

double ThumbnailDelegate::averagePaintTime() const {
    return d->performanceStats.averagePaintTime();
}

double ThumbnailDelegate::cacheHitRate() const {
    return d->performanceStats.cacheHitRate();
}

int ThumbnailDelegate::totalPaintCalls() const {
    return d->performanceStats.paintCalls.load();
}

// Performance and cache management methods
void ThumbnailDelegate::setRenderCacheEnabled(bool enabled) {
    if (d->renderCacheEnabled != enabled) {
        d->renderCacheEnabled = enabled;
        if (!enabled) {
            d->clearRenderCache();
        }
    }
}

void ThumbnailDelegate::setHighQualityRenderingEnabled(bool enabled) {
    d->highQualityRendering = enabled;
}

void ThumbnailDelegate::setAntiAliasingEnabled(bool enabled) {
    d->antiAliasingEnabled = enabled;
}

void ThumbnailDelegate::clearRenderCache() { d->clearRenderCache(); }

void ThumbnailDelegate::setMaxCacheSize(int size) {
    d->maxCacheSize = qBound(10, size, 200);

    // 如果当前缓存超过新的限制，清理一些条目
    if (d->renderCache.size() > d->maxCacheSize) {
        d->cleanupExpiredCache();
    }
}

void ThumbnailDelegate::resetPerformanceStats() {
    d->performanceStats.paintCalls = 0;
    d->performanceStats.totalPaintTime = 0;
    d->performanceStats.cacheHits = 0;
    d->performanceStats.cacheMisses = 0;
    d->performanceStats.sessionTimer.restart();
}

bool ThumbnailDelegate::eventFilter(QObject* object, QEvent* event) {
    if (!d->animationEnabled) {
        return QStyledItemDelegate::eventFilter(object, event);
    }

    auto* view = qobject_cast<QAbstractItemView*>(object);
    if (!view) {
        return QStyledItemDelegate::eventFilter(object, event);
    }

    if (event->type() == QEvent::MouseMove) {
        auto* mouseEvent = dynamic_cast<QMouseEvent*>(event);
        if (!mouseEvent) {
            return QStyledItemDelegate::eventFilter(object, event);
        }
        QModelIndex index = view->indexAt(mouseEvent->pos());

        // Update hover states for all items
        for (auto it = d->animationStates.begin();
             it != d->animationStates.end(); ++it) {
            bool isHovered = (it.key() == index && index.isValid());
            d->updateHoverState(it.key(), isHovered);
        }

        // Create animation state for newly hovered item if needed
        if (index.isValid()) {
            d->updateHoverState(index, true);
        }

        view->viewport()->update();
    }

    return QStyledItemDelegate::eventFilter(object, event);
}

void ThumbnailDelegate::onAnimationValueChanged() {
    // 触发重绘
    if (parent()) {
        if (auto* view = qobject_cast<QAbstractItemView*>(parent())) {
            view->viewport()->update();
        }
    }
}

void ThumbnailDelegate::onLoadingAnimationTimer() {
    // 更新加载动画状态
    for (auto it = d->animationStates.begin(); it != d->animationStates.end();
         ++it) {
        Implementation::AnimationState* state = it.value();
        state->loadingAngle = (state->loadingAngle + 15) % 360;
    }

    // 触发重绘
    if (parent()) {
        if (auto* view = qobject_cast<QAbstractItemView*>(parent())) {
            view->viewport()->update();
        }
    }
}

// Implementation class method definitions for helper methods
QRect ThumbnailDelegate::Implementation::getThumbnailRect(
    const QRect& itemRect) const {
    int x = itemRect.x() + margin;
    int y = itemRect.y() + margin;
    return QRect(x, y, thumbnailSize.width(), thumbnailSize.height());
}

QRect ThumbnailDelegate::Implementation::getPageNumberRect(
    const QRect& thumbnailRect) const {
    int x = thumbnailRect.x();
    int y = thumbnailRect.bottom() + 2;
    int width = thumbnailRect.width();
    return QRect(x, y, width, pageNumberHeight);
}

// Essential Implementation class methods for basic functionality
void ThumbnailDelegate::Implementation::clearRenderCache() {
    QMutexLocker locker(&cacheMutex);

    for (auto it = renderCache.begin(); it != renderCache.end(); ++it) {
        delete it.value();
    }
    renderCache.clear();
}

void ThumbnailDelegate::Implementation::cleanupAnimations() {
    for (auto it = animationStates.begin(); it != animationStates.end(); ++it) {
        delete it.value();
    }
    animationStates.clear();
}

void ThumbnailDelegate::Implementation::recordPaintTime(qint64 time) const {
    performanceStats.paintCalls.fetch_add(1);
    performanceStats.totalPaintTime.fetch_add(time);
}

void ThumbnailDelegate::Implementation::cleanupExpiredCache() {
    QMutexLocker locker(&cacheMutex);

    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

    auto it = renderCache.begin();
    while (it != renderCache.end()) {
        if (!it.value()->isValid ||
            (currentTime - it.value()->timestamp) > CACHE_EXPIRY_TIME) {
            delete it.value();
            it = renderCache.erase(it);
        } else {
            ++it;
        }
    }
}

// Paint method implementations
void ThumbnailDelegate::Implementation::paintThumbnail(
    QPainter* painter, const QRect& rect, const QPixmap& pixmap,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)
    painter->drawPixmap(rect, pixmap, pixmap.rect());
}

void ThumbnailDelegate::Implementation::paintBackground(
    QPainter* painter, const QRect& rect,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)
    painter->fillRect(rect, backgroundColor);
}

void ThumbnailDelegate::Implementation::paintBorder(
    QPainter* painter, const QRect& rect,
    const QStyleOptionViewItem& option) const {
    QColor borderColor = borderColorNormal;

    if (option.state & QStyle::State_Selected) {
        borderColor = borderColorSelected;
    } else if (option.state & QStyle::State_MouseOver) {
        borderColor = borderColorHovered;
    }

    QPen borderPen(borderColor, borderWidth);
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);

    if (borderRadius > 0) {
        painter->drawRoundedRect(rect, borderRadius, borderRadius);
    } else {
        painter->drawRect(rect);
    }
}

void ThumbnailDelegate::Implementation::paintShadow(
    QPainter* painter, const QRect& rect,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)
    QRect shadowRect =
        rect.adjusted(-shadowOffset, -shadowOffset, shadowOffset, shadowOffset);
    painter->fillRect(shadowRect, shadowColor);
}

void ThumbnailDelegate::Implementation::paintPageNumber(
    QPainter* painter, const QRect& rect, int pageNumber,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)
    if (rect.height() <= 0)
        return;

    painter->fillRect(rect, pageNumberBgColor);
    painter->setPen(pageNumberTextColor);
    painter->setFont(pageNumberFont);

    QString pageText = QString::number(pageNumber + 1);
    painter->drawText(rect, Qt::AlignCenter, pageText);
}

void ThumbnailDelegate::Implementation::paintLoadingIndicator(
    QPainter* painter, const QRect& rect,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    StyleManager* styleManager = &StyleManager::instance();

    // Get theme-aware colors
    QColor baseColor;
    QColor shimmerColor;
    if (styleManager->currentTheme() == Theme::Light) {
        baseColor = QColor(240, 240, 240);
        shimmerColor = QColor(250, 250, 250);
    } else {
        baseColor = QColor(45, 45, 45);
        shimmerColor = QColor(60, 60, 60);
    }

    // Fill background
    painter->fillRect(rect, baseColor);

    // Draw skeleton shapes to mimic thumbnail content
    painter->setRenderHint(QPainter::Antialiasing);

    // Calculate layout
    int margin = styleManager->spacingSM();
    int spacing = styleManager->spacingXS();
    QRect contentRect = rect.adjusted(margin, margin, -margin, -margin);

    // Draw main content rectangle (represents the page content)
    int mainHeight = contentRect.height() * 0.7;
    QRect mainRect(contentRect.x(), contentRect.y(), contentRect.width(),
                   mainHeight);
    painter->fillRect(mainRect, shimmerColor);

    // Draw text line placeholders (represents page text)
    int lineHeight = 4;
    int lineY = mainRect.bottom() + spacing * 2;
    int numLines = 3;

    for (int i = 0; i < numLines && lineY + lineHeight < contentRect.bottom();
         ++i) {
        int lineWidth = contentRect.width();
        if (i == numLines - 1) {
            lineWidth = lineWidth * 0.6;  // Last line shorter
        }

        QRect lineRect(contentRect.x(), lineY, lineWidth, lineHeight);
        painter->fillRect(lineRect, shimmerColor);

        lineY += lineHeight + spacing;
    }

    // Draw a subtle shimmer effect using a gradient
    // This creates a moving shimmer animation effect
    static qreal shimmerPosition = 0.0;
    shimmerPosition += 0.02;
    if (shimmerPosition > 1.0) {
        shimmerPosition = 0.0;
    }

    QLinearGradient gradient;
    gradient.setStart(rect.left(), rect.center().y());
    gradient.setFinalStop(rect.right(), rect.center().y());

    qreal shimmerWidth = 0.3;
    qreal shimmerStart = shimmerPosition - shimmerWidth / 2;
    qreal shimmerEnd = shimmerPosition + shimmerWidth / 2;

    gradient.setColorAt(qMax(0.0, shimmerStart), QColor(255, 255, 255, 0));
    gradient.setColorAt(shimmerPosition, QColor(255, 255, 255, 30));
    gradient.setColorAt(qMin(1.0, shimmerEnd), QColor(255, 255, 255, 0));

    painter->fillRect(rect, gradient);
}

void ThumbnailDelegate::Implementation::paintErrorIndicator(
    QPainter* painter, const QRect& rect, const QString& errorMessage,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    // Fill background with error color
    painter->fillRect(rect, errorColor.lighter(150));

    // Draw error icon (X mark)
    painter->setRenderHint(QPainter::Antialiasing);
    QPen errorPen(errorColor, 3);
    painter->setPen(errorPen);

    int iconSize = qMin(rect.width(), rect.height()) / 3;
    QRect iconRect(rect.center().x() - iconSize / 2,
                   rect.center().y() - iconSize / 2, iconSize, iconSize);

    // Draw X
    painter->drawLine(iconRect.topLeft(), iconRect.bottomRight());
    painter->drawLine(iconRect.topRight(), iconRect.bottomLeft());

    // Draw error message text
    if (!errorMessage.isEmpty()) {
        painter->setFont(errorFont);
        painter->setPen(errorColor.darker(150));

        QRect textRect =
            rect.adjusted(margin, iconRect.bottom() + 5, -margin, -margin);
        QString displayText = errorMessage;

        // Truncate if too long
        QFontMetrics fm(errorFont);
        if (fm.horizontalAdvance(displayText) > textRect.width()) {
            displayText =
                fm.elidedText(displayText, Qt::ElideRight, textRect.width());
        }

        painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap,
                          displayText);
    }
}

// Animation helper methods
void ThumbnailDelegate::Implementation::updateHoverState(
    const QModelIndex& index, bool hovered) {
    if (!animationEnabled || !index.isValid()) {
        return;
    }

    AnimationState* state = getAnimationState(index);
    if (!state || !state->hoverAnimation) {
        return;
    }

    qreal targetOpacity = hovered ? 1.0 : 0.0;
    if (qAbs(state->hoverOpacity - targetOpacity) < 0.01) {
        return;  // Already at target
    }

    state->hoverAnimation->stop();
    state->hoverAnimation->setStartValue(state->hoverOpacity);
    state->hoverAnimation->setEndValue(targetOpacity);
    state->hoverAnimation->start();
}

void ThumbnailDelegate::Implementation::updateSelectionState(
    const QModelIndex& index, bool selected) {
    if (!animationEnabled || !index.isValid()) {
        return;
    }

    AnimationState* state = getAnimationState(index);
    if (!state || !state->selectionAnimation) {
        return;
    }

    qreal targetOpacity = selected ? 1.0 : 0.0;
    if (qAbs(state->selectionOpacity - targetOpacity) < 0.01) {
        return;  // Already at target
    }

    state->selectionAnimation->stop();
    state->selectionAnimation->setStartValue(state->selectionOpacity);
    state->selectionAnimation->setEndValue(targetOpacity);
    state->selectionAnimation->start();
}

void ThumbnailDelegate::Implementation::setupAnimations(
    AnimationState* state, const QModelIndex& index) const {
    Q_UNUSED(index)

    if (!state) {
        return;
    }

    // Setup hover animation
    state->hoverAnimation = new QPropertyAnimation();
    state->hoverAnimation->setDuration(HOVER_ANIMATION_DURATION);
    state->hoverAnimation->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(state->hoverAnimation, &QPropertyAnimation::valueChanged,
                     q_ptr, [this, state](const QVariant& value) {
                         state->hoverOpacity = value.toReal();
                         state->needsUpdate = true;
                     });

    // Setup selection animation
    state->selectionAnimation = new QPropertyAnimation();
    state->selectionAnimation->setDuration(SELECTION_ANIMATION_DURATION);
    state->selectionAnimation->setEasingCurve(QEasingCurve::OutCubic);
    QObject::connect(state->selectionAnimation,
                     &QPropertyAnimation::valueChanged, q_ptr,
                     [this, state](const QVariant& value) {
                         state->selectionOpacity = value.toReal();
                         state->needsUpdate = true;
                     });
}

// Additional method implementations
ThumbnailDelegate::Implementation::AnimationState*
ThumbnailDelegate::Implementation::getAnimationState(
    const QModelIndex& index) const {
    if (!index.isValid()) {
        return nullptr;
    }

    QPersistentModelIndex persistentIndex(index);

    // Check if animation state already exists
    auto it = animationStates.find(persistentIndex);
    if (it != animationStates.end()) {
        return it.value();
    }

    // Create new animation state
    auto* state = new AnimationState();
    animationStates.insert(persistentIndex, state);
    setupAnimations(state, index);

    return state;
}

void ThumbnailDelegate::Implementation::paintOptimized(
    QPainter* painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const {
    QString cacheKey = generateCacheKey(index, option);
    RenderCache* cache = getRenderCache(cacheKey);

    if (cache && cache->isValid && !cache->cachedBackground.isNull()) {
        // Paint from cache
        paintFromCache(painter, option.rect, cache->cachedBackground);
        performanceStats.cacheHits++;
        return;
    }

    // Cache miss - perform normal rendering
    performanceStats.cacheMisses++;

    painter->save();

    if (antiAliasingEnabled) {
        painter->setRenderHint(QPainter::Antialiasing);
    }
    if (smoothPixmapTransform) {
        painter->setRenderHint(QPainter::SmoothPixmapTransform);
    }
    if (highQualityRendering) {
        painter->setRenderHint(QPainter::TextAntialiasing);
    }

    // Get data
    QPixmap thumbnail = index.data(ThumbnailModel::PixmapRole).value<QPixmap>();
    bool isLoading = index.data(ThumbnailModel::LoadingRole).toBool();
    bool hasError = index.data(ThumbnailModel::ErrorRole).toBool();
    QString errorMessage =
        index.data(ThumbnailModel::ErrorMessageRole).toString();
    int pageNumber = index.data(ThumbnailModel::PageNumberRole).toInt();

    // Calculate rectangles
    QRect thumbnailRect = getThumbnailRect(option.rect);
    QRect pageNumberRect = getPageNumberRect(thumbnailRect);

    // Paint background
    paintBackground(painter, option.rect, option);

    // Paint shadow
    if (shadowEnabled) {
        paintShadow(painter, thumbnailRect, option);
    }

    // Paint border
    paintBorder(painter, thumbnailRect, option);

    // Paint thumbnail content
    if (hasError) {
        paintErrorIndicator(painter, thumbnailRect, errorMessage, option);
    } else if (isLoading) {
        paintLoadingIndicator(painter, thumbnailRect, option);
    } else if (!thumbnail.isNull()) {
        paintThumbnail(painter, thumbnailRect, thumbnail, option);
    }

    // Paint page number
    paintPageNumber(painter, pageNumberRect, pageNumber, option);

    painter->restore();

    // Optionally cache the result for future use
    // (Caching disabled for now as it would require rendering to QPixmap first)
}

Qt::TransformationMode
ThumbnailDelegate::Implementation::getOptimalTransformationMode(
    const QSize& sourceSize, const QSize& targetSize) const {
    if (!smoothPixmapTransform) {
        return Qt::FastTransformation;
    }

    // Use fast transformation for large downscaling (>2x)
    if (sourceSize.width() > targetSize.width() * 2 ||
        sourceSize.height() > targetSize.height() * 2) {
        return Qt::FastTransformation;
    }

    // Use smooth transformation for upscaling or moderate downscaling
    return Qt::SmoothTransformation;
}

// Cache management methods
QString ThumbnailDelegate::Implementation::generateCacheKey(
    const QModelIndex& index, const QStyleOptionViewItem& option) const {
    if (!index.isValid()) {
        return QString();
    }

    // Generate unique key based on index, size, and state
    int state = 0;
    if (option.state & QStyle::State_Selected) {
        state |= 1;
    }
    if (option.state & QStyle::State_MouseOver) {
        state |= 2;
    }

    return QString("%1_%2x%3_%4")
        .arg(index.row())
        .arg(option.rect.width())
        .arg(option.rect.height())
        .arg(state);
}

ThumbnailDelegate::Implementation::RenderCache*
ThumbnailDelegate::Implementation::getRenderCache(const QString& key) const {
    if (key.isEmpty()) {
        return nullptr;
    }

    QMutexLocker locker(&cacheMutex);
    auto iter = renderCache.find(key);
    if (iter != renderCache.end()) {
        RenderCache* cache = iter.value();
        // Check if cache is still valid (not expired)
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
        if (cache->isValid &&
            (currentTime - cache->timestamp) < CACHE_EXPIRY_TIME) {
            return cache;
        } else {
            // Cache expired
            cache->isValid = false;
        }
    }
    return nullptr;
}

void ThumbnailDelegate::Implementation::insertRenderCache(const QString& key,
                                                          RenderCache* cache) {
    if (key.isEmpty() || !cache) {
        return;
    }

    QMutexLocker locker(&cacheMutex);

    // Check cache size limit
    if (renderCache.size() >= maxCacheSize) {
        // Remove oldest entry (simple LRU)
        qint64 oldestTime = QDateTime::currentMSecsSinceEpoch();
        QString oldestKey;

        for (auto iter = renderCache.begin(); iter != renderCache.end();
             ++iter) {
            if (iter.value()->timestamp < oldestTime) {
                oldestTime = iter.value()->timestamp;
                oldestKey = iter.key();
            }
        }

        if (!oldestKey.isEmpty()) {
            delete renderCache.take(oldestKey);
        }
    }

    cache->timestamp = QDateTime::currentMSecsSinceEpoch();
    cache->isValid = true;
    renderCache.insert(key, cache);
}

QPixmap ThumbnailDelegate::Implementation::renderToCache(
    const QStyleOptionViewItem& option, const QModelIndex& index) const {
    // Create a pixmap to render into
    QPixmap pixmap(option.rect.size());
    pixmap.fill(Qt::transparent);

    QPainter pixmapPainter(&pixmap);
    pixmapPainter.setRenderHint(QPainter::Antialiasing, antiAliasingEnabled);
    pixmapPainter.setRenderHint(QPainter::SmoothPixmapTransform,
                                smoothPixmapTransform);
    pixmapPainter.setRenderHint(QPainter::TextAntialiasing,
                                highQualityRendering);

    // Get data
    QPixmap thumbnail = index.data(ThumbnailModel::PixmapRole).value<QPixmap>();
    bool isLoading = index.data(ThumbnailModel::LoadingRole).toBool();
    bool hasError = index.data(ThumbnailModel::ErrorRole).toBool();
    QString errorMessage =
        index.data(ThumbnailModel::ErrorMessageRole).toString();
    int pageNumber = index.data(ThumbnailModel::PageNumberRole).toInt();

    // Calculate rectangles (relative to pixmap origin)
    QRect localRect(0, 0, option.rect.width(), option.rect.height());
    QRect thumbnailRect = getThumbnailRect(localRect);
    QRect pageNumberRect = getPageNumberRect(thumbnailRect);

    // Paint all components
    paintBackground(&pixmapPainter, localRect, option);

    if (shadowEnabled) {
        paintShadow(&pixmapPainter, thumbnailRect, option);
    }

    paintBorder(&pixmapPainter, thumbnailRect, option);

    if (hasError) {
        paintErrorIndicator(&pixmapPainter, thumbnailRect, errorMessage,
                            option);
    } else if (isLoading) {
        paintLoadingIndicator(&pixmapPainter, thumbnailRect, option);
    } else if (!thumbnail.isNull()) {
        paintThumbnail(&pixmapPainter, thumbnailRect, thumbnail, option);
    }

    paintPageNumber(&pixmapPainter, pageNumberRect, pageNumber, option);

    return pixmap;
}

void ThumbnailDelegate::Implementation::paintFromCache(
    QPainter* painter, const QRect& rect, const QPixmap& cached) const {
    if (cached.isNull()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::SmoothPixmapTransform,
                           smoothPixmapTransform);
    painter->drawPixmap(rect, cached);
    painter->restore();
}

void ThumbnailDelegate::Implementation::updatePerformanceStats() const {
    // Sample performance stats periodically
    static int sampleCounter = 0;
    sampleCounter++;

    if (sampleCounter % static_cast<int>(1.0 / PERFORMANCE_SAMPLE_RATE) == 0) {
        qDebug() << "ThumbnailDelegate Performance Stats:";
        qDebug() << "  Paint calls:" << performanceStats.paintCalls.load();
        qDebug() << "  Avg paint time:" << performanceStats.averagePaintTime()
                 << "ms";
        qDebug() << "  Cache hit rate:"
                 << (performanceStats.cacheHitRate() * 100) << "%";
        qDebug() << "  Cache size:" << renderCache.size();
    }
}
