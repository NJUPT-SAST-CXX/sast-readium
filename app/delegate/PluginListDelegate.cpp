#include "PluginListDelegate.h"

#include <QApplication>
#include <QFont>
#include <QFontMetrics>
#include <QPainterPath>

#include "../model/PluginModel.h"

PluginListDelegate::PluginListDelegate(QObject* parent)
    : QStyledItemDelegate(parent),
      m_displayMode(NormalMode),
      m_showIcons(true),
      m_showStatus(true),
      m_highlightErrors(true),
      m_loadedColor(34, 139, 34),      // Forest green
      m_disabledColor(128, 128, 128),  // Gray
      m_errorColor(220, 20, 60),       // Crimson
      m_logger("PluginListDelegate") {}

void PluginListDelegate::paint(QPainter* painter,
                               const QStyleOptionViewItem& option,
                               const QModelIndex& index) const {
    if (!index.isValid()) {
        QStyledItemDelegate::paint(painter, option, index);
        return;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    switch (m_displayMode) {
        case CompactMode:
            paintCompact(painter, option, index);
            break;
        case NormalMode:
            paintNormal(painter, option, index);
            break;
        case DetailedMode:
            paintDetailed(painter, option, index);
            break;
    }

    painter->restore();
}

QSize PluginListDelegate::sizeHint(const QStyleOptionViewItem& option,
                                   const QModelIndex& index) const {
    Q_UNUSED(index)

    QFontMetrics fm(option.font);
    int height;

    switch (m_displayMode) {
        case CompactMode:
            height = fm.height() + 2 * MARGIN;
            break;
        case NormalMode:
            height = 2 * fm.height() + LINE_SPACING + 2 * MARGIN;
            break;
        case DetailedMode:
            height = 4 * fm.height() + 3 * LINE_SPACING + 2 * MARGIN;
            break;
        default:
            height = fm.height() + 2 * MARGIN;
            break;
    }

    if (m_showIcons && m_displayMode != CompactMode) {
        height = qMax(height, ICON_SIZE + 2 * MARGIN);
    }

    return QSize(option.rect.width(), height);
}

void PluginListDelegate::paintCompact(QPainter* painter,
                                      const QStyleOptionViewItem& option,
                                      const QModelIndex& index) const {
    // Draw background
    bool isLoaded = index.data(PluginModel::IsLoadedRole).toBool();
    bool isEnabled = index.data(PluginModel::IsEnabledRole).toBool();
    QStringList errors = index.data(PluginModel::ErrorsRole).toStringList();
    bool hasErrors = !errors.isEmpty();

    QColor bgColor = getBackgroundColor(option, isLoaded, isEnabled, hasErrors);
    painter->fillRect(option.rect, bgColor);

    // Draw selection/hover
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect,
                          option.palette.highlight().color().lighter(120));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect,
                          option.palette.highlight().color().lighter(160));
    }

    QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    int x = contentRect.left();

    // Draw status indicator
    if (m_showStatus) {
        QRect statusRect(x,
                         contentRect.top() +
                             (contentRect.height() - STATUS_INDICATOR_SIZE) / 2,
                         STATUS_INDICATOR_SIZE, STATUS_INDICATOR_SIZE);
        drawStatusIndicator(painter, statusRect, isLoaded, isEnabled,
                            hasErrors);
        x += STATUS_INDICATOR_SIZE + SPACING;
    }

    // Draw icon
    if (m_showIcons) {
        QRect iconRect(
            x,
            contentRect.top() + (contentRect.height() - COMPACT_ICON_SIZE) / 2,
            COMPACT_ICON_SIZE, COMPACT_ICON_SIZE);
        drawPluginIcon(painter, iconRect, index);
        x += COMPACT_ICON_SIZE + SPACING;
    }

    // Draw text
    QString name = index.data(PluginModel::NameRole).toString();
    QString version = index.data(PluginModel::VersionRole).toString();
    QString text = QString("%1 %2").arg(name, version);

    QColor textColor = getTextColor(option, isLoaded, isEnabled, hasErrors);
    painter->setPen(textColor);

    QRect textRect(x, contentRect.top(), contentRect.right() - x,
                   contentRect.height());
    painter->drawText(textRect, Qt::AlignLeft | Qt::AlignVCenter, text);
}

void PluginListDelegate::paintNormal(QPainter* painter,
                                     const QStyleOptionViewItem& option,
                                     const QModelIndex& index) const {
    // Draw background
    bool isLoaded = index.data(PluginModel::IsLoadedRole).toBool();
    bool isEnabled = index.data(PluginModel::IsEnabledRole).toBool();
    QStringList errors = index.data(PluginModel::ErrorsRole).toStringList();
    bool hasErrors = !errors.isEmpty();

    QColor bgColor = getBackgroundColor(option, isLoaded, isEnabled, hasErrors);
    painter->fillRect(option.rect, bgColor);

    // Draw selection/hover
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect,
                          option.palette.highlight().color().lighter(120));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect,
                          option.palette.highlight().color().lighter(160));
    }

    QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    int x = contentRect.left();

    // Draw icon
    if (m_showIcons) {
        QRect iconRect(x, contentRect.top(), ICON_SIZE, ICON_SIZE);
        drawPluginIcon(painter, iconRect, index);
        x += ICON_SIZE + SPACING;
    }

    // Calculate text area
    QRect textRect(x, contentRect.top(), contentRect.right() - x,
                   contentRect.height());

    QFontMetrics fm(option.font);
    QColor textColor = getTextColor(option, isLoaded, isEnabled, hasErrors);

    // Draw plugin name (line 1)
    QString name = index.data(PluginModel::NameRole).toString();
    QFont nameFont = option.font;
    nameFont.setBold(true);
    painter->setFont(nameFont);
    painter->setPen(textColor);

    QRect nameRect = textRect;
    nameRect.setHeight(fm.height());
    painter->drawText(nameRect, Qt::AlignLeft | Qt::AlignTop, name);

    // Draw version and status (line 2)
    QString version = index.data(PluginModel::VersionRole).toString();
    QString status = index.data(PluginModel::StatusTextRole).toString();
    QString line2 = QString("v%1 - %2").arg(version, status);

    QFont normalFont = option.font;
    normalFont.setPointSize(normalFont.pointSize() - 1);
    painter->setFont(normalFont);
    painter->setPen(textColor.lighter(120));

    QRect line2Rect = textRect;
    line2Rect.setTop(nameRect.bottom() + LINE_SPACING);
    line2Rect.setHeight(fm.height());
    painter->drawText(line2Rect, Qt::AlignLeft | Qt::AlignTop, line2);

    // Draw status indicator
    if (m_showStatus) {
        QRect statusRect(contentRect.right() - STATUS_INDICATOR_SIZE,
                         contentRect.top() +
                             (contentRect.height() - STATUS_INDICATOR_SIZE) / 2,
                         STATUS_INDICATOR_SIZE, STATUS_INDICATOR_SIZE);
        drawStatusIndicator(painter, statusRect, isLoaded, isEnabled,
                            hasErrors);
    }
}

void PluginListDelegate::paintDetailed(QPainter* painter,
                                       const QStyleOptionViewItem& option,
                                       const QModelIndex& index) const {
    // Draw background
    bool isLoaded = index.data(PluginModel::IsLoadedRole).toBool();
    bool isEnabled = index.data(PluginModel::IsEnabledRole).toBool();
    QStringList errors = index.data(PluginModel::ErrorsRole).toStringList();
    bool hasErrors = !errors.isEmpty();

    QColor bgColor = getBackgroundColor(option, isLoaded, isEnabled, hasErrors);
    painter->fillRect(option.rect, bgColor);

    // Draw selection/hover
    if (option.state & QStyle::State_Selected) {
        painter->fillRect(option.rect,
                          option.palette.highlight().color().lighter(120));
    } else if (option.state & QStyle::State_MouseOver) {
        painter->fillRect(option.rect,
                          option.palette.highlight().color().lighter(160));
    }

    QRect contentRect = option.rect.adjusted(MARGIN, MARGIN, -MARGIN, -MARGIN);
    int x = contentRect.left();

    // Draw icon
    if (m_showIcons) {
        QRect iconRect(x, contentRect.top(), ICON_SIZE, ICON_SIZE);
        drawPluginIcon(painter, iconRect, index);
        x += ICON_SIZE + SPACING;
    }

    // Calculate text area
    QRect textRect(x, contentRect.top(), contentRect.right() - x,
                   contentRect.height());

    QFontMetrics fm(option.font);
    QColor textColor = getTextColor(option, isLoaded, isEnabled, hasErrors);
    int lineHeight = fm.height();
    int y = textRect.top();

    // Line 1: Plugin name
    QString name = index.data(PluginModel::NameRole).toString();
    QFont nameFont = option.font;
    nameFont.setBold(true);
    painter->setFont(nameFont);
    painter->setPen(textColor);
    painter->drawText(QRect(textRect.left(), y, textRect.width(), lineHeight),
                      Qt::AlignLeft | Qt::AlignTop, name);
    y += lineHeight + LINE_SPACING;

    // Line 2: Version and Author
    QString version = index.data(PluginModel::VersionRole).toString();
    QString author = index.data(PluginModel::AuthorRole).toString();
    QString line2 = tr("Version %1 by %2").arg(version, author);

    QFont normalFont = option.font;
    normalFont.setPointSize(normalFont.pointSize() - 1);
    painter->setFont(normalFont);
    painter->setPen(textColor.lighter(120));
    painter->drawText(QRect(textRect.left(), y, textRect.width(), lineHeight),
                      Qt::AlignLeft | Qt::AlignTop, line2);
    y += lineHeight + LINE_SPACING;

    // Line 3: Description
    QString description = index.data(PluginModel::DescriptionRole).toString();
    painter->drawText(
        QRect(textRect.left(), y, textRect.width(), lineHeight),
        Qt::AlignLeft | Qt::AlignTop,
        fm.elidedText(description, Qt::ElideRight, textRect.width()));
    y += lineHeight + LINE_SPACING;

    // Line 4: Status
    QString status = index.data(PluginModel::StatusTextRole).toString();
    QString pluginType = index.data(PluginModel::PluginTypeRole).toString();
    QString line4 = tr("Status: %1 | Type: %2").arg(status, pluginType);
    painter->drawText(QRect(textRect.left(), y, textRect.width(), lineHeight),
                      Qt::AlignLeft | Qt::AlignTop, line4);

    // Draw status indicator
    if (m_showStatus) {
        QRect statusRect(contentRect.right() - STATUS_INDICATOR_SIZE,
                         contentRect.top() +
                             (contentRect.height() - STATUS_INDICATOR_SIZE) / 2,
                         STATUS_INDICATOR_SIZE, STATUS_INDICATOR_SIZE);
        drawStatusIndicator(painter, statusRect, isLoaded, isEnabled,
                            hasErrors);
    }
}

void PluginListDelegate::drawStatusIndicator(QPainter* painter,
                                             const QRect& rect, bool isLoaded,
                                             bool isEnabled,
                                             bool hasErrors) const {
    QColor color;

    if (hasErrors && m_highlightErrors) {
        color = m_errorColor;
    } else if (!isEnabled) {
        color = m_disabledColor;
    } else if (isLoaded) {
        color = m_loadedColor;
    } else {
        color = Qt::gray;
    }

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);
    painter->setPen(Qt::NoPen);
    painter->setBrush(color);
    painter->drawEllipse(rect);
    painter->restore();
}

void PluginListDelegate::drawPluginIcon(QPainter* painter, const QRect& rect,
                                        const QModelIndex& index) const {
    Q_UNUSED(index)

    // Draw a default plugin icon
    // TODO: Use actual plugin icon from metadata if available

    painter->save();
    painter->setRenderHint(QPainter::Antialiasing);

    // Draw rounded rectangle background
    QPainterPath path;
    path.addRoundedRect(rect, 4, 4);
    painter->fillPath(path, QColor(100, 100, 200));

    // Draw "P" symbol
    painter->setPen(Qt::white);
    QFont iconFont = painter->font();
    iconFont.setPixelSize(rect.height() * 0.6);
    iconFont.setBold(true);
    painter->setFont(iconFont);
    painter->drawText(rect, Qt::AlignCenter, "P");

    painter->restore();
}

QColor PluginListDelegate::getBackgroundColor(
    const QStyleOptionViewItem& option, bool isLoaded, bool isEnabled,
    bool hasErrors) const {
    Q_UNUSED(isLoaded)
    Q_UNUSED(isEnabled)

    if (hasErrors && m_highlightErrors) {
        return m_errorColor.lighter(180);
    }

    return option.palette.base().color();
}

QColor PluginListDelegate::getTextColor(const QStyleOptionViewItem& option,
                                        bool isLoaded, bool isEnabled,
                                        bool hasErrors) const {
    if (hasErrors && m_highlightErrors) {
        return m_errorColor.darker(120);
    }

    if (!isEnabled) {
        return m_disabledColor;
    }

    if (isLoaded) {
        return option.palette.text().color();
    }

    return option.palette.text().color().lighter(140);
}

QString PluginListDelegate::getStatusText(bool isLoaded, bool isEnabled,
                                          bool hasErrors) const {
    if (hasErrors) {
        return tr("Error");
    }
    if (!isEnabled) {
        return tr("Disabled");
    }
    if (!isLoaded) {
        return tr("Not Loaded");
    }
    return tr("Active");
}
