#pragma once

#include <QColor>
#include <QFont>
#include <QHash>
#include <QObject>
#include <QString>
#include <QTimer>
#include <QWidget>
#include "../../logging/SimpleLogging.h"
#include "../../managers/StyleManager.h"

/**
 * @brief Ensures visual consistency across all UI components
 *
 * Manages consistent styling, spacing, colors, fonts, and visual behavior
 * across the entire application. Provides automatic style validation and
 * correction to maintain design system compliance.
 */
class UIConsistencyManager : public QObject {
    Q_OBJECT

public:
    enum class ConsistencyLevel {
        Strict,    // Enforce all design system rules
        Moderate,  // Allow minor deviations
        Relaxed    // Only enforce critical consistency
    };

    enum class ValidationResult {
        Compliant,    // Fully compliant with design system
        MinorIssues,  // Minor inconsistencies found
        MajorIssues,  // Major inconsistencies found
        NonCompliant  // Severely non-compliant
    };

    struct StyleIssue {
        QString component;
        QString property;
        QString expected;
        QString actual;
        QString severity;
        QString suggestion;

        StyleIssue() = default;
        StyleIssue(const QString& comp, const QString& prop, const QString& exp,
                   const QString& act, const QString& sev,
                   const QString& sug = QString())
            : component(comp),
              property(prop),
              expected(exp),
              actual(act),
              severity(sev),
              suggestion(sug) {}
    };

    static UIConsistencyManager& instance();

    // Component registration and validation
    void registerComponent(QWidget* widget, const QString& componentType);
    void unregisterComponent(QWidget* widget);
    ValidationResult validateComponent(QWidget* widget);
    ValidationResult validateAllComponents();

    // Style enforcement
    void enforceConsistency(QWidget* widget);
    void enforceGlobalConsistency();
    void applyDesignSystemStyles(QWidget* widget, const QString& componentType);

    // Consistency rules management
    void setConsistencyLevel(ConsistencyLevel level) {
        m_consistencyLevel = level;
    }
    void enableAutoCorrection(bool enabled) { m_autoCorrection = enabled; }
    void enableContinuousValidation(bool enabled, int intervalMs = 30000);

    // Design system compliance
    bool isColorCompliant(const QColor& color, const QString& context);
    bool isFontCompliant(const QFont& font, const QString& context);
    bool isSpacingCompliant(int spacing, const QString& context);
    bool isSizeCompliant(const QSize& size, const QString& context);

    // Style correction
    QColor correctColor(const QColor& color, const QString& context);
    QFont correctFont(const QFont& font, const QString& context);
    int correctSpacing(int spacing, const QString& context);
    QSize correctSize(const QSize& size, const QString& context);

    // Validation reporting
    QList<StyleIssue> getValidationIssues(QWidget* widget = nullptr);
    QString generateValidationReport();
    void exportValidationReport(const QString& filePath);

    // Theme consistency
    void validateThemeConsistency();
    void enforceThemeConsistency();
    void updateComponentsForTheme(Theme theme);

signals:
    void componentRegistered(QWidget* widget, const QString& type);
    void componentUnregistered(QWidget* widget);
    void validationCompleted(ValidationResult result, int issueCount);
    void consistencyIssueFound(const StyleIssue& issue);
    void consistencyEnforced(QWidget* widget, int correctionCount);

private slots:
    void onComponentDestroyed(QObject* object);
    void onValidationTimer();
    void onThemeChanged(Theme theme);

private:
    UIConsistencyManager();
    ~UIConsistencyManager() override = default;
    UIConsistencyManager(const UIConsistencyManager&) = delete;
    UIConsistencyManager& operator=(const UIConsistencyManager&) = delete;

    // Validation methods
    QList<StyleIssue> validateWidgetColors(QWidget* widget);
    QList<StyleIssue> validateWidgetFonts(QWidget* widget);
    QList<StyleIssue> validateWidgetSpacing(QWidget* widget);
    QList<StyleIssue> validateWidgetSizes(QWidget* widget);
    QList<StyleIssue> validateWidgetBehavior(QWidget* widget);

    // Correction methods
    void correctWidgetColors(QWidget* widget);
    void correctWidgetFonts(QWidget* widget);
    void correctWidgetSpacing(QWidget* widget);
    void correctWidgetSizes(QWidget* widget);

    // Component-specific validation
    QList<StyleIssue> validateButton(QWidget* button);
    QList<StyleIssue> validateLineEdit(QWidget* lineEdit);
    QList<StyleIssue> validateLabel(QWidget* label);
    QList<StyleIssue> validateToolBar(QWidget* toolBar);
    QList<StyleIssue> validateStatusBar(QWidget* statusBar);
    QList<StyleIssue> validateMenuBar(QWidget* menuBar);

    // Helper methods
    QString getComponentType(QWidget* widget);
    bool isSystemWidget(QWidget* widget);
    QString colorToString(const QColor& color);
    QString fontToString(const QFont& font);

    // Data members
    QHash<QWidget*, QString> m_registeredComponents;
    QList<StyleIssue> m_validationIssues;
    QTimer* m_validationTimer;

    ConsistencyLevel m_consistencyLevel;
    bool m_autoCorrection;
    bool m_continuousValidation;

    SastLogging::CategoryLogger m_logger;
};

/**
 * @brief Design system constants and utilities
 */
class DesignSystem {
public:
    // Color palette validation
    static bool isValidPrimaryColor(const QColor& color);
    static bool isValidSecondaryColor(const QColor& color);
    static bool isValidAccentColor(const QColor& color);
    static bool isValidNeutralColor(const QColor& color);

    // Typography validation
    static bool isValidHeadingFont(const QFont& font);
    static bool isValidBodyFont(const QFont& font);
    static bool isValidCaptionFont(const QFont& font);

    // Spacing validation
    static bool isValidSpacing(int pixels);
    static int getNearestValidSpacing(int pixels);

    // Size validation
    static bool isValidButtonSize(const QSize& size);
    static bool isValidIconSize(const QSize& size);
    static QSize getNearestValidSize(const QSize& size, const QString& type);

    // Component standards
    static int getStandardButtonHeight();
    static int getStandardIconSize();
    static int getStandardSpacing();
    static QFont getStandardFont(const QString& type);
    static QColor getStandardColor(const QString& role, Theme theme);
};

// Convenience macros
#define UI_CONSISTENCY_MANAGER UIConsistencyManager::instance()

#define REGISTER_UI_COMPONENT_CONSISTENCY(widget, type) \
    UI_CONSISTENCY_MANAGER.registerComponent(widget, type)

#define VALIDATE_UI_CONSISTENCY(widget) \
    UI_CONSISTENCY_MANAGER.validateComponent(widget)

#define ENFORCE_UI_CONSISTENCY(widget) \
    UI_CONSISTENCY_MANAGER.enforceConsistency(widget)
