#include "UIConsistencyManager.h"
#include <QApplication>
#include <QFile>
#include <QLabel>
#include <QLayout>
#include <QLineEdit>
#include <QMenuBar>
#include <QPushButton>
#include <QStatusBar>
#include <QTextStream>
#include <QToolBar>
#include "../../logging/LoggingMacros.h"

UIConsistencyManager::UIConsistencyManager()
    : QObject(nullptr),
      m_validationTimer(new QTimer(this)),
      m_consistencyLevel(ConsistencyLevel::Moderate),
      m_autoCorrection(true),
      m_continuousValidation(false),
      m_logger("UIConsistencyManager") {
    // Setup validation timer
    m_validationTimer->setSingleShot(false);
    connect(m_validationTimer, &QTimer::timeout, this,
            &UIConsistencyManager::onValidationTimer);

    // Connect to style manager for theme changes
    connect(&STYLE, &StyleManager::themeChanged, this,
            &UIConsistencyManager::onThemeChanged);

    m_logger.info("UIConsistencyManager initialized");
}

UIConsistencyManager& UIConsistencyManager::instance() {
    static UIConsistencyManager instance;
    return instance;
}

void UIConsistencyManager::registerComponent(QWidget* widget,
                                             const QString& componentType) {
    if (!widget) {
        m_logger.warning("Cannot register null widget");
        return;
    }

    m_registeredComponents[widget] = componentType;

    // Connect to destroyed signal for cleanup
    connect(widget, &QObject::destroyed, this,
            &UIConsistencyManager::onComponentDestroyed);

    // Apply design system styles immediately
    applyDesignSystemStyles(widget, componentType);

    emit componentRegistered(widget, componentType);

    m_logger.debug(QString("Component registered for consistency: %1 (%2)")
                       .arg(componentType, widget->objectName()));
}

void UIConsistencyManager::unregisterComponent(QWidget* widget) {
    if (!widget)
        return;

    auto it = m_registeredComponents.find(widget);
    if (it != m_registeredComponents.end()) {
        QString componentType = it.value();
        m_registeredComponents.erase(it);

        disconnect(widget, &QObject::destroyed, this,
                   &UIConsistencyManager::onComponentDestroyed);

        emit componentUnregistered(widget);

        m_logger.debug(
            QString("Component unregistered: %1").arg(componentType));
    }
}

UIConsistencyManager::ValidationResult UIConsistencyManager::validateComponent(
    QWidget* widget) {
    if (!widget)
        return ValidationResult::NonCompliant;

    QList<StyleIssue> issues;

    // Validate different aspects
    issues.append(validateWidgetColors(widget));
    issues.append(validateWidgetFonts(widget));
    issues.append(validateWidgetSpacing(widget));
    issues.append(validateWidgetSizes(widget));
    issues.append(validateWidgetBehavior(widget));

    // Component-specific validation
    QString componentType = getComponentType(widget);
    if (componentType == "QPushButton") {
        issues.append(validateButton(widget));
    } else if (componentType == "QLineEdit") {
        issues.append(validateLineEdit(widget));
    } else if (componentType == "QLabel") {
        issues.append(validateLabel(widget));
    } else if (componentType == "QToolBar") {
        issues.append(validateToolBar(widget));
    } else if (componentType == "QStatusBar") {
        issues.append(validateStatusBar(widget));
    } else if (componentType == "QMenuBar") {
        issues.append(validateMenuBar(widget));
    }

    // Store issues for reporting
    for (const auto& issue : issues) {
        m_validationIssues.append(issue);
        emit consistencyIssueFound(issue);
    }

    // Determine validation result
    ValidationResult result = ValidationResult::Compliant;
    int criticalIssues = 0;
    int majorIssues = 0;

    for (const auto& issue : issues) {
        if (issue.severity == "Critical") {
            criticalIssues++;
        } else if (issue.severity == "Major") {
            majorIssues++;
        }
    }

    if (criticalIssues > 0) {
        result = ValidationResult::NonCompliant;
    } else if (majorIssues > 2) {
        result = ValidationResult::MajorIssues;
    } else if (issues.size() > 0) {
        result = ValidationResult::MinorIssues;
    }

    // Auto-correct if enabled
    if (m_autoCorrection && result != ValidationResult::Compliant) {
        enforceConsistency(widget);
    }

    return result;
}

UIConsistencyManager::ValidationResult
UIConsistencyManager::validateAllComponents() {
    m_validationIssues.clear();

    ValidationResult overallResult = ValidationResult::Compliant;
    int totalIssues = 0;

    for (auto it = m_registeredComponents.begin();
         it != m_registeredComponents.end(); ++it) {
        if (it.key()) {  // Check if widget still exists
            ValidationResult componentResult = validateComponent(it.key());

            // Update overall result to worst case
            if (componentResult > overallResult) {
                overallResult = componentResult;
            }
        }
    }

    totalIssues = m_validationIssues.size();

    emit validationCompleted(overallResult, totalIssues);

    m_logger.info(QString("Validation completed: %1 issues found (result: %2)")
                      .arg(totalIssues)
                      .arg(static_cast<int>(overallResult)));

    return overallResult;
}

void UIConsistencyManager::enforceConsistency(QWidget* widget) {
    if (!widget)
        return;

    int correctionCount = 0;

    // Apply corrections
    correctWidgetColors(widget);
    correctWidgetFonts(widget);
    correctWidgetSpacing(widget);
    correctWidgetSizes(widget);

    // Apply design system styles
    QString componentType =
        m_registeredComponents.value(widget, getComponentType(widget));
    applyDesignSystemStyles(widget, componentType);

    emit consistencyEnforced(widget, correctionCount);

    m_logger.debug(QString("Consistency enforced for widget: %1")
                       .arg(widget->objectName()));
}

void UIConsistencyManager::enforceGlobalConsistency() {
    m_logger.info("Enforcing global consistency...");

    int totalCorrections = 0;

    for (auto it = m_registeredComponents.begin();
         it != m_registeredComponents.end(); ++it) {
        if (it.key()) {
            enforceConsistency(it.key());
            totalCorrections++;
        }
    }

    m_logger.info(
        QString("Global consistency enforced: %1 components corrected")
            .arg(totalCorrections));
}

void UIConsistencyManager::applyDesignSystemStyles(
    QWidget* widget, const QString& componentType) {
    if (!widget)
        return;

    // Apply component-specific design system styles
    if (componentType == "QPushButton") {
        QPushButton* button = qobject_cast<QPushButton*>(widget);
        if (button) {
            button->setMinimumHeight(DesignSystem::getStandardButtonHeight());
            button->setFont(DesignSystem::getStandardFont("button"));
        }
    } else if (componentType == "QLineEdit") {
        QLineEdit* lineEdit = qobject_cast<QLineEdit*>(widget);
        if (lineEdit) {
            lineEdit->setMinimumHeight(DesignSystem::getStandardButtonHeight());
            lineEdit->setFont(DesignSystem::getStandardFont("input"));
        }
    } else if (componentType == "QLabel") {
        QLabel* label = qobject_cast<QLabel*>(widget);
        if (label) {
            label->setFont(DesignSystem::getStandardFont("body"));
        }
    }

    // Apply consistent spacing
    if (widget->layout()) {
        widget->layout()->setSpacing(DesignSystem::getStandardSpacing());
        widget->layout()->setContentsMargins(
            DesignSystem::getStandardSpacing(),
            DesignSystem::getStandardSpacing() / 2,
            DesignSystem::getStandardSpacing(),
            DesignSystem::getStandardSpacing() / 2);
    }
}

// Validation methods implementation

QList<UIConsistencyManager::StyleIssue>
UIConsistencyManager::validateWidgetColors(QWidget* widget) {
    QList<StyleIssue> issues;

    if (!widget)
        return issues;

    // Check background color
    QPalette palette = widget->palette();
    QColor bgColor = palette.color(QPalette::Window);

    if (!isColorCompliant(bgColor, "background")) {
        QColor correctedColor = correctColor(bgColor, "background");
        issues.append(StyleIssue(widget->objectName(), "backgroundColor",
                                 colorToString(correctedColor),
                                 colorToString(bgColor), "Minor",
                                 "Use design system background colors"));
    }

    return issues;
}

QList<UIConsistencyManager::StyleIssue>
UIConsistencyManager::validateWidgetFonts(QWidget* widget) {
    QList<StyleIssue> issues;

    if (!widget)
        return issues;

    QFont font = widget->font();
    QString componentType = getComponentType(widget);

    if (!isFontCompliant(font, componentType)) {
        QFont correctedFont = correctFont(font, componentType);
        issues.append(StyleIssue(
            widget->objectName(), "font", fontToString(correctedFont),
            fontToString(font), "Minor", "Use design system typography"));
    }

    return issues;
}

QList<UIConsistencyManager::StyleIssue>
UIConsistencyManager::validateWidgetSpacing(QWidget* widget) {
    QList<StyleIssue> issues;

    if (!widget || !widget->layout())
        return issues;

    int spacing = widget->layout()->spacing();

    if (!isSpacingCompliant(spacing, "layout")) {
        int correctedSpacing = correctSpacing(spacing, "layout");
        issues.append(StyleIssue(widget->objectName(), "spacing",
                                 QString::number(correctedSpacing),
                                 QString::number(spacing), "Minor",
                                 "Use design system spacing values"));
    }

    return issues;
}

// Helper methods

QString UIConsistencyManager::getComponentType(QWidget* widget) {
    if (!widget)
        return QString();
    return widget->metaObject()->className();
}

QString UIConsistencyManager::colorToString(const QColor& color) {
    return color.name(QColor::HexArgb);
}

QString UIConsistencyManager::fontToString(const QFont& font) {
    return QString("%1, %2pt").arg(font.family()).arg(font.pointSize());
}

// Slots

void UIConsistencyManager::onComponentDestroyed(QObject* object) {
    QWidget* widget = qobject_cast<QWidget*>(object);
    if (widget) {
        unregisterComponent(widget);
    }
}

void UIConsistencyManager::onValidationTimer() {
    if (m_continuousValidation) {
        validateAllComponents();
    }
}

void UIConsistencyManager::onThemeChanged(Theme theme) {
    m_logger.info("Theme changed, updating component consistency");
    updateComponentsForTheme(theme);
    enforceGlobalConsistency();
}

void UIConsistencyManager::updateComponentsForTheme(Theme theme) {
    for (auto it = m_registeredComponents.begin();
         it != m_registeredComponents.end(); ++it) {
        if (it.key()) {
            applyDesignSystemStyles(it.key(), it.value());
        }
    }
}

// DesignSystem implementation

bool DesignSystem::isValidSpacing(int pixels) {
    // Valid spacing values: 4, 8, 12, 16, 20, 24, 32, 40, 48
    QList<int> validSpacing = {4, 8, 12, 16, 20, 24, 32, 40, 48};
    return validSpacing.contains(pixels);
}

int DesignSystem::getNearestValidSpacing(int pixels) {
    QList<int> validSpacing = {4, 8, 12, 16, 20, 24, 32, 40, 48};

    int nearest = validSpacing.first();
    int minDiff = qAbs(pixels - nearest);

    for (int spacing : validSpacing) {
        int diff = qAbs(pixels - spacing);
        if (diff < minDiff) {
            minDiff = diff;
            nearest = spacing;
        }
    }

    return nearest;
}

int DesignSystem::getStandardButtonHeight() {
    return 32;  // Standard button height
}

int DesignSystem::getStandardIconSize() {
    return 16;  // Standard icon size
}

int DesignSystem::getStandardSpacing() {
    return 8;  // Standard spacing unit
}

QFont DesignSystem::getStandardFont(const QString& type) {
    QFont font = QApplication::font();

    if (type == "heading") {
        font.setPointSize(font.pointSize() + 2);
        font.setBold(true);
    } else if (type == "button") {
        // Keep default size, medium weight
    } else if (type == "caption") {
        font.setPointSize(font.pointSize() - 1);
    }

    return font;
}

QColor DesignSystem::getStandardColor(const QString& role, Theme theme) {
    if (theme == Theme::Dark) {
        if (role == "background")
            return QColor("#2b2b2b");
        if (role == "text")
            return QColor("#ffffff");
        if (role == "accent")
            return QColor("#0078d4");
    } else {
        if (role == "background")
            return QColor("#ffffff");
        if (role == "text")
            return QColor("#000000");
        if (role == "accent")
            return QColor("#0078d4");
    }

    return QColor();
}

// Design system compliance methods

bool UIConsistencyManager::isColorCompliant(const QColor& color,
                                            const QString& context) {
    Q_UNUSED(context)
    // Stub implementation - accept all valid colors
    return color.isValid();
}

bool UIConsistencyManager::isFontCompliant(const QFont& font,
                                           const QString& context) {
    Q_UNUSED(context)
    // Stub implementation - accept all fonts with reasonable size
    return font.pointSize() >= 6 && font.pointSize() <= 72;
}

bool UIConsistencyManager::isSpacingCompliant(int spacing,
                                              const QString& context) {
    Q_UNUSED(context)
    // Stub implementation - accept spacing in reasonable range
    return spacing >= 0 && spacing <= 100;
}

// Style correction methods

QColor UIConsistencyManager::correctColor(const QColor& color,
                                          const QString& context) {
    if (color.isValid()) {
        return color;
    }
    // Return default color based on context
    Theme currentTheme = STYLE.currentTheme();
    return DesignSystem::getStandardColor(context, currentTheme);
}

QFont UIConsistencyManager::correctFont(const QFont& font,
                                        const QString& context) {
    QFont corrected = font;

    // Ensure font size is in valid range
    if (corrected.pointSize() < 6) {
        corrected.setPointSize(6);
    } else if (corrected.pointSize() > 72) {
        corrected.setPointSize(72);
    }

    // Apply context-specific corrections
    if (context == "button" || context == "input") {
        if (corrected.pointSize() < 8) {
            corrected.setPointSize(8);
        }
    }

    return corrected;
}

int UIConsistencyManager::correctSpacing(int spacing, const QString& context) {
    Q_UNUSED(context)

    // Ensure spacing is non-negative
    if (spacing < 0) {
        return 0;
    }

    // Snap to standard spacing units (multiples of 4)
    int standardUnit = 4;
    return (spacing / standardUnit) * standardUnit;
}

// Widget correction methods

void UIConsistencyManager::correctWidgetColors(QWidget* widget) {
    if (!widget)
        return;

    // Stub implementation - apply standard palette
    QPalette palette = widget->palette();
    Theme currentTheme = STYLE.currentTheme();

    QColor bgColor = DesignSystem::getStandardColor("background", currentTheme);
    QColor textColor = DesignSystem::getStandardColor("text", currentTheme);

    if (bgColor.isValid()) {
        palette.setColor(QPalette::Window, bgColor);
        palette.setColor(QPalette::Base, bgColor);
    }
    if (textColor.isValid()) {
        palette.setColor(QPalette::WindowText, textColor);
        palette.setColor(QPalette::Text, textColor);
    }

    widget->setPalette(palette);
}

void UIConsistencyManager::correctWidgetFonts(QWidget* widget) {
    if (!widget)
        return;

    // Stub implementation - ensure font is in valid range
    QFont font = widget->font();
    QFont corrected = correctFont(font, "widget");
    widget->setFont(corrected);
}

void UIConsistencyManager::correctWidgetSpacing(QWidget* widget) {
    if (!widget || !widget->layout())
        return;

    // Stub implementation - apply standard spacing
    int standardSpacing = DesignSystem::getStandardSpacing();
    widget->layout()->setSpacing(standardSpacing);
    widget->layout()->setContentsMargins(standardSpacing, standardSpacing / 2,
                                         standardSpacing, standardSpacing / 2);
}

void UIConsistencyManager::correctWidgetSizes(QWidget* widget) {
    if (!widget)
        return;

    // Stub implementation - ensure minimum sizes for interactive widgets
    if (qobject_cast<QPushButton*>(widget)) {
        if (widget->minimumHeight() < 24) {
            widget->setMinimumHeight(24);
        }
    } else if (qobject_cast<QLineEdit*>(widget)) {
        if (widget->minimumHeight() < 24) {
            widget->setMinimumHeight(24);
        }
    }
}

// Validation methods

QList<UIConsistencyManager::StyleIssue>
UIConsistencyManager::validateWidgetSizes(QWidget* widget) {
    QList<StyleIssue> issues;

    if (!widget)
        return issues;

    // Stub implementation - check minimum sizes for interactive widgets
    if (qobject_cast<QPushButton*>(widget) ||
        qobject_cast<QLineEdit*>(widget)) {
        if (widget->minimumHeight() < 24) {
            issues.append(StyleIssue(
                widget->objectName(), "minimumHeight", QString::number(24),
                "Minor", "Widget height below recommended minimum"));
        }
    }

    return issues;
}

QList<UIConsistencyManager::StyleIssue>
UIConsistencyManager::validateWidgetBehavior(QWidget* widget) {
    QList<StyleIssue> issues;

    if (!widget)
        return issues;

    // Stub implementation - basic behavior validation
    // Check if widget is enabled but not visible (potential issue)
    if (widget->isEnabled() && !widget->isVisible() && widget->parent()) {
        issues.append(StyleIssue(widget->objectName(), "visibility", "visible",
                                 "Minor", "Widget is enabled but not visible"));
    }

    return issues;
}

// Component-specific validation methods

QList<UIConsistencyManager::StyleIssue> UIConsistencyManager::validateButton(
    QWidget* button) {
    QList<StyleIssue> issues;

    if (!button)
        return issues;

    QPushButton* pushButton = qobject_cast<QPushButton*>(button);
    if (!pushButton)
        return issues;

    // Stub implementation - basic button validation
    if (pushButton->text().isEmpty() && pushButton->icon().isNull()) {
        issues.append(StyleIssue(button->objectName(), "content",
                                 "text or icon", "Major",
                                 "Button has no text or icon"));
    }

    return issues;
}

QList<UIConsistencyManager::StyleIssue> UIConsistencyManager::validateLineEdit(
    QWidget* lineEdit) {
    QList<StyleIssue> issues;

    if (!lineEdit)
        return issues;

    QLineEdit* edit = qobject_cast<QLineEdit*>(lineEdit);
    if (!edit)
        return issues;

    // Stub implementation - basic line edit validation
    if (edit->placeholderText().isEmpty() && !edit->parent()) {
        issues.append(StyleIssue(lineEdit->objectName(), "placeholderText",
                                 "descriptive text", "Minor",
                                 "Line edit has no placeholder text"));
    }

    return issues;
}

QList<UIConsistencyManager::StyleIssue> UIConsistencyManager::validateLabel(
    QWidget* label) {
    QList<StyleIssue> issues;

    if (!label)
        return issues;

    QLabel* labelWidget = qobject_cast<QLabel*>(label);
    if (!labelWidget)
        return issues;

    // Stub implementation - basic label validation
    if (labelWidget->text().isEmpty() && labelWidget->pixmap().isNull()) {
        issues.append(StyleIssue(label->objectName(), "content",
                                 "text or pixmap", "Minor",
                                 "Label has no text or pixmap"));
    }

    return issues;
}

QList<UIConsistencyManager::StyleIssue> UIConsistencyManager::validateToolBar(
    QWidget* toolBar) {
    QList<StyleIssue> issues;

    if (!toolBar)
        return issues;

    QToolBar* toolbar = qobject_cast<QToolBar*>(toolBar);
    if (!toolbar)
        return issues;

    // Stub implementation - basic toolbar validation
    if (toolbar->actions().isEmpty()) {
        issues.append(StyleIssue(toolBar->objectName(), "actions",
                                 "at least one action", "Minor",
                                 "Toolbar has no actions"));
    }

    return issues;
}

QList<UIConsistencyManager::StyleIssue> UIConsistencyManager::validateStatusBar(
    QWidget* statusBar) {
    QList<StyleIssue> issues;

    if (!statusBar)
        return issues;

    QStatusBar* status = qobject_cast<QStatusBar*>(statusBar);
    if (!status)
        return issues;

    // Stub implementation - basic status bar validation
    // Status bars are typically valid as-is
    Q_UNUSED(status)

    return issues;
}

QList<UIConsistencyManager::StyleIssue> UIConsistencyManager::validateMenuBar(
    QWidget* menuBar) {
    QList<StyleIssue> issues;

    if (!menuBar)
        return issues;

    QMenuBar* menu = qobject_cast<QMenuBar*>(menuBar);
    if (!menu)
        return issues;

    // Stub implementation - basic menu bar validation
    if (menu->actions().isEmpty()) {
        issues.append(StyleIssue(menuBar->objectName(), "menus",
                                 "at least one menu", "Major",
                                 "Menu bar has no menus"));
    }

    return issues;
}

// Continuous validation

void UIConsistencyManager::enableContinuousValidation(bool enabled,
                                                      int intervalMs) {
    m_continuousValidation = enabled;

    if (enabled) {
        m_validationTimer->setInterval(intervalMs);
        m_validationTimer->start();
        m_logger.info(QString("Continuous validation enabled (interval: %1 ms)")
                          .arg(intervalMs));
    } else {
        m_validationTimer->stop();
        m_logger.info("Continuous validation disabled");
    }
}
