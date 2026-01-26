#include "ThumbnailDelegate.h"
#include <QAbstractItemView>
#include <QApplication>
#include <QDebug>
#include <QModelIndex>
#include <QPainter>
#include <QPropertyAnimation>
#include <QStyleOptionViewItem>
#include <QTimer>
#include "model/ThumbnailModel.h"

// Chrome风格颜色常量
const QColor ThumbnailDelegate::GOOGLE_BLUE = QColor(66, 133, 244);
const QColor ThumbnailDelegate::GOOGLE_BLUE_DARK = QColor(26, 115, 232);
const QColor ThumbnailDelegate::GOOGLE_RED = QColor(234, 67, 53);
const QColor ThumbnailDelegate::LIGHT_BACKGROUND = QColor(255, 255, 255);
const QColor ThumbnailDelegate::LIGHT_BORDER = QColor(200, 200, 200);
const QColor ThumbnailDelegate::LIGHT_TEXT = QColor(60, 60, 60);
const QColor ThumbnailDelegate::DARK_BACKGROUND = QColor(0, 0, 0);
const QColor ThumbnailDelegate::DARK_BORDER = QColor(95, 99, 104);
const QColor ThumbnailDelegate::DARK_TEXT = QColor(232, 234, 237);

ThumbnailDelegate::ThumbnailDelegate(QObject* parent)
    : QStyledItemDelegate(parent),
      m_thumbnailSize(DEFAULT_THUMBNAIL_WIDTH, DEFAULT_THUMBNAIL_HEIGHT),
      m_margin(DEFAULT_MARGIN),
      m_borderRadius(0),  // 设置为0去除圆角
      m_pageNumberHeight(DEFAULT_PAGE_NUMBER_HEIGHT),
      m_shadowEnabled(true),
      m_animationEnabled(true),
      m_shadowBlurRadius(DEFAULT_SHADOW_BLUR_RADIUS),
      m_shadowOffset(DEFAULT_SHADOW_OFFSET),
      m_borderWidth(DEFAULT_BORDER_WIDTH),
      m_loadingTimer(new QTimer(this)) {
    // 设置默认颜色主题
    setLightTheme();

    // 设置字体
    m_pageNumberFont = QFont("Arial", 9);
    m_errorFont = QFont("Arial", 8);

    // 设置加载动画定时器
    m_loadingTimer->setInterval(LOADING_ANIMATION_INTERVAL);
    connect(m_loadingTimer, &QTimer::timeout, this,
            &ThumbnailDelegate::onLoadingAnimationTimer);
}

ThumbnailDelegate::~ThumbnailDelegate() { cleanupAnimations(); }

QSize ThumbnailDelegate::sizeHint(const QStyleOptionViewItem& option,
                                  const QModelIndex& index) const {
    Q_UNUSED(option)
    Q_UNUSED(index)

    return QSize(m_thumbnailSize.width() + 2 * m_margin,
                 m_thumbnailSize.height() + m_pageNumberHeight + 2 * m_margin);
}

void ThumbnailDelegate::paint(QPainter* painter,
                              const QStyleOptionViewItem& option,
                              const QModelIndex& index) const {
    if (!index.isValid()) {
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // 获取数据
    QPixmap thumbnail = index.data(ThumbnailModel::PixmapRole).value<QPixmap>();
    bool isLoading = index.data(ThumbnailModel::LoadingRole).toBool();
    bool hasError = index.data(ThumbnailModel::ErrorRole).toBool();
    QString errorMessage =
        index.data(ThumbnailModel::ErrorMessageRole).toString();
    int pageNumber = index.data(ThumbnailModel::PageNumberRole).toInt();

    // 计算矩形
    QRect thumbnailRect = getThumbnailRect(option.rect);
    QRect pageNumberRect = getPageNumberRect(thumbnailRect);

    // 绘制背景
    paintBackground(painter, option.rect, option);

    // 绘制阴影
    if (m_shadowEnabled) {
        paintShadow(painter, thumbnailRect, option);
    }

    // 绘制边框
    paintBorder(painter, thumbnailRect, option);

    // 绘制缩略图内容
    if (hasError) {
        paintErrorIndicator(painter, thumbnailRect, errorMessage, option);
    } else if (isLoading) {
        paintLoadingIndicator(painter, thumbnailRect, option);
    } else if (!thumbnail.isNull()) {
        paintThumbnail(painter, thumbnailRect, thumbnail, option);
    }

    // 绘制页码
    paintPageNumber(painter, pageNumberRect, pageNumber, option);

    painter->restore();
}

void ThumbnailDelegate::setThumbnailSize(const QSize& size) {
    if (m_thumbnailSize != size) {
        m_thumbnailSize = size;
        emit sizeHintChanged(QModelIndex());
    }
}

void ThumbnailDelegate::setMargins(int margin) {
    if (m_margin != margin) {
        m_margin = margin;
        emit sizeHintChanged(QModelIndex());
    }
}

void ThumbnailDelegate::setBorderRadius(int radius) { m_borderRadius = radius; }

void ThumbnailDelegate::setShadowEnabled(bool enabled) {
    m_shadowEnabled = enabled;
}

void ThumbnailDelegate::setAnimationEnabled(bool enabled) {
    m_animationEnabled = enabled;
    if (!enabled) {
        cleanupAnimations();
    }
}

void ThumbnailDelegate::setLightTheme() {
    m_backgroundColor = LIGHT_BACKGROUND;
    m_borderColorNormal = LIGHT_BORDER;
    m_borderColorHovered = GOOGLE_BLUE.lighter(150);
    m_borderColorSelected = GOOGLE_BLUE;
    m_shadowColor = QColor(0, 0, 0, 50);
    m_pageNumberBgColor = QColor(0, 0, 0, 0);
    m_pageNumberTextColor = QColor(60, 60, 60);
    m_loadingColor = GOOGLE_BLUE;
    m_errorColor = GOOGLE_RED;
    m_placeholderColor = QColor(200, 200, 200);
}

void ThumbnailDelegate::setDarkTheme() {
    m_backgroundColor = QColor(0, 0, 0, 0);  // 透明背景
    m_borderColorNormal = DARK_BORDER;
    m_borderColorHovered = GOOGLE_BLUE.lighter(150);
    m_borderColorSelected = GOOGLE_BLUE;
    m_shadowColor = QColor(0, 0, 0, 100);
    m_pageNumberBgColor = QColor(0, 0, 0, 0);  // 页码背景也设为透明
    m_pageNumberTextColor = DARK_TEXT;
    m_loadingColor = GOOGLE_BLUE;
    m_errorColor = GOOGLE_RED;
    m_placeholderColor = QColor(100, 100, 100);
}

void ThumbnailDelegate::setCustomColors(const QColor& background,
                                        const QColor& border,
                                        const QColor& text,
                                        const QColor& accent) {
    m_backgroundColor = background;
    m_borderColorNormal = border;
    m_borderColorHovered = accent.lighter(150);
    m_borderColorSelected = accent;
    m_pageNumberTextColor = text;
    m_loadingColor = accent;
}

bool ThumbnailDelegate::eventFilter(QObject* object, QEvent* event) {
    // 处理鼠标事件以触发动画
    if (auto* view = qobject_cast<QAbstractItemView*>(object)) {
        if (event->type() == QEvent::MouseMove) {
            // 处理悬停状态
        }
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
    for (auto it = m_animationStates.begin(); it != m_animationStates.end();
         ++it) {
        AnimationState* state = it.value();
        state->loadingAngle = (state->loadingAngle + 15) % 360;
    }

    // 触发重绘
    if (parent()) {
        if (auto* view = qobject_cast<QAbstractItemView*>(parent())) {
            view->viewport()->update();
        }
    }
}

QRect ThumbnailDelegate::getThumbnailRect(const QRect& itemRect) const {
    int x = itemRect.x() + m_margin;
    int y = itemRect.y() + m_margin;
    return QRect(x, y, m_thumbnailSize.width(), m_thumbnailSize.height());
}

QRect ThumbnailDelegate::getPageNumberRect(const QRect& thumbnailRect) const {
    int x = thumbnailRect.x();
    int y = thumbnailRect.bottom() + 2;
    int width = thumbnailRect.width();
    return QRect(x, y, width, m_pageNumberHeight);
}

void ThumbnailDelegate::paintThumbnail(
    QPainter* painter, const QRect& rect, const QPixmap& pixmap,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    if (!pixmap.isNull()) {
        // 缩放图片填满整个rect，不保持宽高比
        QPixmap displayPixmap = pixmap.scaled(rect.size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        // 直接绘制填满整个rect
        painter->drawPixmap(rect, displayPixmap);
    } else {
        // 没有缩略图时，不绘制背景（保持透明）
        
        // 绘制占位符图标
        painter->setPen(QColor(120, 120, 120));
        QFont font = painter->font();
        font.setPixelSize(24);
        painter->setFont(font);
        painter->drawText(rect, Qt::AlignCenter, "📄");
    }
}

void ThumbnailDelegate::paintBackground(
    QPainter* painter, const QRect& rect,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    painter->fillRect(rect, m_backgroundColor);
}

void ThumbnailDelegate::paintBorder(QPainter* painter, const QRect& rect,
                                    const QStyleOptionViewItem& option) const {
    QColor borderColor = m_borderColorNormal;

    if (option.state & QStyle::State_Selected) {
        borderColor = m_borderColorSelected;
    } else if (option.state & QStyle::State_MouseOver) {
        borderColor = m_borderColorHovered;
    }

    QPen borderPen(borderColor, m_borderWidth);
    painter->setPen(borderPen);
    painter->setBrush(Qt::NoBrush);

    if (m_borderRadius > 0) {
        painter->drawRoundedRect(rect, m_borderRadius, m_borderRadius);
    } else {
        painter->drawRect(rect);
    }
}

void ThumbnailDelegate::paintShadow(QPainter* painter, const QRect& rect,
                                    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    // 简化的阴影绘制
    QRect shadowRect = rect.adjusted(-m_shadowOffset, -m_shadowOffset,
                                     m_shadowOffset, m_shadowOffset);

    painter->fillRect(shadowRect, m_shadowColor);
}

void ThumbnailDelegate::paintPageNumber(
    QPainter* painter, const QRect& rect, int pageNumber,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    if (rect.height() <= 0)
        return;

    // 绘制页码背景
    painter->fillRect(rect, m_pageNumberBgColor);

    // 绘制页码文字
    painter->setPen(m_pageNumberTextColor);
    painter->setFont(m_pageNumberFont);

    QString pageText = QString::number(pageNumber + 1);  // 页码从1开始显示
    painter->drawText(rect, Qt::AlignCenter, pageText);
}

void ThumbnailDelegate::paintLoadingIndicator(
    QPainter* painter, const QRect& rect,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    // 绘制半透明遮罩
    painter->fillRect(rect, QColor(255, 255, 255, 200));

    // 获取动画状态
    AnimationState* state = getAnimationState(option.index);
    int angle = state ? state->loadingAngle : 0;

    // 绘制旋转的加载指示器
    QRect spinnerRect(rect.center().x() - LOADING_SPINNER_SIZE / 2,
                      rect.center().y() - LOADING_SPINNER_SIZE / 2,
                      LOADING_SPINNER_SIZE, LOADING_SPINNER_SIZE);

    painter->save();
    painter->translate(spinnerRect.center());
    painter->rotate(angle);

    painter->setPen(QPen(m_loadingColor, 3, Qt::SolidLine, Qt::RoundCap));
    painter->drawArc(-LOADING_SPINNER_SIZE / 2, -LOADING_SPINNER_SIZE / 2,
                     LOADING_SPINNER_SIZE, LOADING_SPINNER_SIZE, 0,
                     270 * 16);  // 3/4 圆弧

    painter->restore();
}

void ThumbnailDelegate::paintErrorIndicator(
    QPainter* painter, const QRect& rect, const QString& errorMessage,
    const QStyleOptionViewItem& option) const {
    Q_UNUSED(option)

    // 绘制半透明遮罩
    painter->fillRect(rect, QColor(255, 255, 255, 200));

    // 绘制错误图标
    painter->setPen(QPen(m_errorColor, 2));
    painter->setBrush(Qt::NoBrush);

    QRect iconRect(rect.center().x() - 12, rect.center().y() - 12, 24, 24);
    painter->drawEllipse(iconRect);

    // 绘制感叹号
    painter->setPen(QPen(m_errorColor, 3, Qt::SolidLine, Qt::RoundCap));
    painter->drawLine(iconRect.center().x(), iconRect.top() + 6,
                      iconRect.center().x(), iconRect.center().y() + 2);
    painter->drawPoint(iconRect.center().x(), iconRect.bottom() - 4);

    // 绘制错误消息（如果有空间）
    if (!errorMessage.isEmpty() && rect.height() > 60) {
        painter->setPen(m_errorColor);
        painter->setFont(m_errorFont);
        QRect textRect = rect.adjusted(4, iconRect.bottom() + 4, -4, -4);
        painter->drawText(textRect, Qt::AlignCenter | Qt::TextWordWrap,
                          errorMessage);
    }
}

ThumbnailDelegate::AnimationState* ThumbnailDelegate::getAnimationState(
    const QModelIndex& index) const {
    if (!m_animationEnabled || !index.isValid()) {
        return nullptr;
    }

    QPersistentModelIndex persistentIndex(index);
    auto it = m_animationStates.find(persistentIndex);

    if (it == m_animationStates.end()) {
        AnimationState* state = new AnimationState();
        setupAnimations(state, index);
        m_animationStates[persistentIndex] = state;
        return state;
    }

    return it.value();
}

void ThumbnailDelegate::setupAnimations(AnimationState* state,
                                        const QModelIndex& index) const {
    Q_UNUSED(index)

    // 悬停动画
    state->hoverAnimation = new QPropertyAnimation();
    state->hoverAnimation->setTargetObject(
        const_cast<ThumbnailDelegate*>(this));
    state->hoverAnimation->setPropertyName("animationValue");
    state->hoverAnimation->setDuration(HOVER_ANIMATION_DURATION);

    connect(state->hoverAnimation, &QPropertyAnimation::valueChanged, this,
            &ThumbnailDelegate::onAnimationValueChanged);

    // 选中动画
    state->selectionAnimation = new QPropertyAnimation();
    state->selectionAnimation->setTargetObject(
        const_cast<ThumbnailDelegate*>(this));
    state->selectionAnimation->setPropertyName("animationValue");
    state->selectionAnimation->setDuration(SELECTION_ANIMATION_DURATION);

    connect(state->selectionAnimation, &QPropertyAnimation::valueChanged, this,
            &ThumbnailDelegate::onAnimationValueChanged);
}

void ThumbnailDelegate::cleanupAnimations() {
    for (auto it = m_animationStates.begin(); it != m_animationStates.end();
         ++it) {
        delete it.value();
    }
    m_animationStates.clear();
}

Qt::TransformationMode ThumbnailDelegate::getOptimalTransformationMode(
    const QSize& sourceSize, const QSize& targetSize) const {
    // 计算缩放比例
    double scaleRatio =
        qMin(static_cast<double>(targetSize.width()) / sourceSize.width(),
             static_cast<double>(targetSize.height()) / sourceSize.height());

    // 对于小幅缩放或小尺寸目标，使用快速变换
    if (scaleRatio > 0.75 || targetSize.width() <= 150 ||
        targetSize.height() <= 200) {
        return Qt::FastTransformation;
    }

    // 对于大幅缩放，使用平滑变换以保持质量
    return Qt::SmoothTransformation;
}

void ThumbnailDelegate::updateHoverState(const QModelIndex& index,
                                         bool hovered) {
    if (!m_animationEnabled)
        return;

    AnimationState* state = getAnimationState(index);
    if (!state || !state->hoverAnimation)
        return;

    qreal targetOpacity = hovered ? 1.0 : 0.0;

    if (qAbs(state->hoverOpacity - targetOpacity) > 0.001) {
        state->hoverAnimation->setStartValue(state->hoverOpacity);
        state->hoverAnimation->setEndValue(targetOpacity);
        state->hoverAnimation->start();
    }
}

void ThumbnailDelegate::updateSelectionState(const QModelIndex& index,
                                             bool selected) {
    if (!m_animationEnabled)
        return;

    AnimationState* state = getAnimationState(index);
    if (!state || !state->selectionAnimation)
        return;

    qreal targetOpacity = selected ? 1.0 : 0.0;

    if (qAbs(state->selectionOpacity - targetOpacity) > 0.001) {
        state->selectionAnimation->setStartValue(state->selectionOpacity);
        state->selectionAnimation->setEndValue(targetOpacity);
        state->selectionAnimation->start();
    }
}
