#include "UIRecoveryManager.h"
#include <QApplication>
#include <QDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QMetaProperty>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <algorithm>
#include "../../managers/StyleManager.h"
#include "UIErrorHandler.h"

UIRecoveryManager::UIRecoveryManager()
    : QObject(nullptr), m_logger("UIRecoveryManager") {
    // Register default recovery actions
    registerRecoveryAction(
        UIErrorType::WidgetCreationFailed,
        RecoveryAction(
            RecoveryStrategy::FallbackMethod, tr("Use fallback widget"),
            [this](QWidget* parent, const ErrorHandling::ErrorInfo& error) {
                return tryFallbackMethod(parent, error);
            },
            10));

    registerRecoveryAction(
        UIErrorType::StyleApplicationFailed,
        RecoveryAction(
            RecoveryStrategy::ResetToDefault, tr("Reset to default style"),
            [this](QWidget* widget, const ErrorHandling::ErrorInfo& error) {
                return tryResetToDefault(widget, error);
            },
            8));

    registerRecoveryAction(
        UIErrorType::LayoutError,
        RecoveryAction(
            RecoveryStrategy::GracefulDegradation, tr("Simplify layout"),
            [this](QWidget* widget, const ErrorHandling::ErrorInfo& error) {
                return tryGracefulDegradation(widget, error);
            },
            7));

    registerRecoveryAction(
        UIErrorType::ValidationError,
        RecoveryAction(
            RecoveryStrategy::UserGuidance, tr("Show validation help"),
            [this](QWidget* widget, const ErrorHandling::ErrorInfo& error) {
                showRecoveryGuidance(
                    widget, error.message,
                    tr("Please check your input and try again"));
                return true;
            },
            5));

    m_logger.info(
        "UIRecoveryManager initialized with default recovery actions");
}

UIRecoveryManager& UIRecoveryManager::instance() {
    static UIRecoveryManager instance;
    return instance;
}

void UIRecoveryManager::registerRecoveryAction(UIErrorType errorType,
                                               const RecoveryAction& action) {
    m_recoveryActions[errorType].append(action);

    // Sort by priority (highest first)
    std::sort(m_recoveryActions[errorType].begin(),
              m_recoveryActions[errorType].end(),
              [](const RecoveryAction& a, const RecoveryAction& b) {
                  return a.priority > b.priority;
              });

    m_logger.info(
        QString("Registered recovery action for %1: %2 (priority: %3)")
            .arg(getErrorTypeString(errorType), action.description)
            .arg(action.priority));
}

void UIRecoveryManager::registerFallbackWidget(
    const QString& widgetType, std::function<QWidget*(QWidget*)> factory) {
    m_fallbackWidgets[widgetType] = factory;
    m_logger.info(
        QString("Registered fallback widget factory for: %1").arg(widgetType));
}

bool UIRecoveryManager::attemptRecovery(UIErrorType errorType, QWidget* context,
                                        const ErrorHandling::ErrorInfo& error) {
    if (!m_autoRecoveryEnabled) {
        m_logger.info("Auto-recovery disabled, skipping recovery attempt");
        return false;
    }

    m_logger.info(QString("Attempting recovery for error type: %1")
                      .arg(getErrorTypeString(errorType)));

    // Save current state before attempting recovery
    if (context) {
        saveWidgetState(context);
    }

    auto actions = m_recoveryActions.value(errorType);
    for (const auto& action : actions) {
        try {
            m_logger.info(
                QString("Trying recovery action: %1").arg(action.description));

            bool success = action.action(context, error);
            logRecoveryAttempt(errorType, action.description, success);

            if (success) {
                emit recoveryAttempted(errorType, true, action.description);
                return true;
            }
        } catch (const std::exception& e) {
            m_logger.error(QString("Recovery action failed with exception: %1")
                               .arg(e.what()));
        }
    }

    // If all automatic recovery failed, try user prompt
    if (context && errorType != UIErrorType::ValidationError) {
        QStringList options = getRecoveryOptions(errorType);
        if (!options.isEmpty()) {
            bool userChoice = promptUserForRecovery(context, error, options);
            if (userChoice) {
                emit recoveryAttempted(errorType, true,
                                       tr("User intervention"));
                return true;
            }
        }
    }

    emit recoveryFailed(errorType, tr("All recovery attempts failed"));
    return false;
}

bool UIRecoveryManager::recoverWidgetCreation(
    QWidget* parent, const QString& widgetType,
    const ErrorHandling::ErrorInfo& error) {
    m_logger.info(QString("Attempting widget creation recovery for type: %1")
                      .arg(widgetType));

    // Try fallback widget factory
    auto it = m_fallbackWidgets.find(widgetType);
    if (it != m_fallbackWidgets.end()) {
        try {
            QWidget* fallbackWidget = it.value()(parent);
            if (fallbackWidget) {
                m_logger.info(
                    QString("Successfully created fallback widget for: %1")
                        .arg(widgetType));
                return true;
            }
        } catch (const std::exception& e) {
            m_logger.error(
                QString("Fallback widget creation failed: %1").arg(e.what()));
        }
    }

    // Try creating a simple placeholder widget
    try {
        QLabel* placeholder = new QLabel(tr("Widget unavailable"), parent);
        placeholder->setStyleSheet(
            "QLabel { color: gray; font-style: italic; }");
        placeholder->setAlignment(Qt::AlignCenter);
        m_logger.info(
            QString("Created placeholder widget for: %1").arg(widgetType));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Placeholder widget creation failed: %1").arg(e.what()));
    }

    return false;
}

bool UIRecoveryManager::recoverLayoutError(
    QWidget* widget, const ErrorHandling::ErrorInfo& error) {
    if (!widget)
        return false;

    m_logger.info(QString("Attempting layout error recovery for widget: %1")
                      .arg(widget->objectName()));

    try {
        // Try to reset layout to a simple vertical layout
        QLayout* currentLayout = widget->layout();
        if (currentLayout) {
            // Save child widgets
            QList<QWidget*> children;
            for (int i = 0; i < currentLayout->count(); ++i) {
                QLayoutItem* item = currentLayout->itemAt(i);
                if (item && item->widget()) {
                    children.append(item->widget());
                }
            }

            // Delete current layout
            delete currentLayout;

            // Create simple vertical layout
            QVBoxLayout* simpleLayout = new QVBoxLayout(widget);
            for (QWidget* child : children) {
                simpleLayout->addWidget(child);
            }

            m_logger.info(
                "Successfully recovered layout with simple vertical layout");
            return true;
        }
    } catch (const std::exception& e) {
        m_logger.error(QString("Layout recovery failed: %1").arg(e.what()));
    }

    return false;
}

bool UIRecoveryManager::recoverStyleError(
    QWidget* widget, const ErrorHandling::ErrorInfo& error) {
    if (!widget)
        return false;

    m_logger.info(QString("Attempting style error recovery for widget: %1")
                      .arg(widget->objectName()));

    try {
        // Clear custom stylesheet and use default
        widget->setStyleSheet("");

        // Apply basic safe styling
        QString safeStyle =
            QString("QWidget { background-color: %1; color: %2; }")
                .arg(STYLE.backgroundColor().name(), STYLE.textColor().name());

        widget->setStyleSheet(safeStyle);

        m_logger.info(
            "Successfully recovered from style error with safe styling");
        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Style recovery failed: %1").arg(e.what()));
    }

    return false;
}

bool UIRecoveryManager::recoverDataBinding(
    QWidget* widget, const ErrorHandling::ErrorInfo& error) {
    if (!widget)
        return false;

    m_logger.info(QString("Attempting data binding recovery for widget: %1")
                      .arg(widget->objectName()));

    try {
        // Disconnect all signals to prevent further errors
        widget->disconnect();

        // Set widget to a safe state
        widget->setEnabled(false);

        // Show error indicator
        widget->setToolTip(tr("Data binding error: %1").arg(error.message));

        m_logger.info("Successfully isolated widget with data binding error");
        return true;
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Data binding recovery failed: %1").arg(e.what()));
    }

    return false;
}

// State management methods

void UIRecoveryManager::saveWidgetState(QWidget* widget) {
    if (!widget)
        return;

    WidgetState state = captureWidgetState(widget);
    m_savedStates[widget] = state;

    // Clean up when widget is destroyed
    connect(widget, &QObject::destroyed, this,
            [this, widget]() { m_savedStates.remove(widget); });

    m_logger.debug(
        QString("Saved state for widget: %1").arg(widget->objectName()));
}

bool UIRecoveryManager::restoreWidgetState(QWidget* widget) {
    if (!widget || !m_savedStates.contains(widget)) {
        return false;
    }

    try {
        WidgetState state = m_savedStates[widget];
        applyWidgetState(widget, state);
        m_logger.info(QString("Successfully restored widget state: %1")
                          .arg(widget->objectName()));
        return true;
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Failed to restore widget state: %1").arg(e.what()));
        return false;
    }
}

void UIRecoveryManager::clearSavedState(QWidget* widget) {
    if (widget) {
        m_savedStates.remove(widget);
    }
}

// User interaction methods

bool UIRecoveryManager::promptUserForRecovery(
    QWidget* parent, const ErrorHandling::ErrorInfo& error,
    const QStringList& options) {
    QMessageBox msgBox(parent);
    msgBox.setWindowTitle(tr("Error Recovery"));
    msgBox.setText(tr("An error occurred: %1\n\nHow would you like to proceed?")
                       .arg(error.message));
    msgBox.setIcon(QMessageBox::Question);

    QPushButton* retryButton =
        msgBox.addButton(tr("Retry"), QMessageBox::ActionRole);
    QPushButton* ignoreButton =
        msgBox.addButton(tr("Ignore"), QMessageBox::RejectRole);
    QPushButton* resetButton =
        msgBox.addButton(tr("Reset"), QMessageBox::ResetRole);

    msgBox.setDefaultButton(retryButton);

    int result = msgBox.exec();

    if (msgBox.clickedButton() == retryButton) {
        return tryAutomaticRetry(parent, error);
    } else if (msgBox.clickedButton() == resetButton) {
        return tryResetToDefault(parent, error);
    }

    return false;  // User chose to ignore
}

void UIRecoveryManager::showRecoveryGuidance(QWidget* parent,
                                             const QString& problem,
                                             const QString& solution) {
    QString guidance =
        tr("Problem: %1\n\nSuggested solution: %2").arg(problem, solution);

    UIErrorHandler::instance().showFeedback(
        parent, guidance, UIErrorHandler::FeedbackType::Info, 8000);

    emit userGuidanceShown(problem, solution);
    m_logger.info(
        QString("Showed recovery guidance - Problem: %1, Solution: %2")
            .arg(problem, solution));
}

// Recovery implementation methods

bool UIRecoveryManager::tryAutomaticRetry(
    QWidget* context, const ErrorHandling::ErrorInfo& error) {
    m_logger.info(
        QString("Attempting automatic retry for error: %1").arg(error.message));

    // Simple retry - restore saved state and hope for the best
    if (context && restoreWidgetState(context)) {
        return true;
    }

    return false;
}

bool UIRecoveryManager::tryFallbackMethod(
    QWidget* context, const ErrorHandling::ErrorInfo& error) {
    m_logger.info(
        QString("Attempting fallback method for error: %1").arg(error.message));

    // This would be implemented based on specific error context
    // For now, just try to create a simple replacement
    if (context && context->parent()) {
        try {
            QLabel* fallback =
                new QLabel(tr("Feature temporarily unavailable"),
                           qobject_cast<QWidget*>(context->parent()));
            fallback->setStyleSheet(
                "QLabel { color: orange; font-style: italic; }");
            fallback->setAlignment(Qt::AlignCenter);

            // Replace the problematic widget
            if (context->layout()) {
                context->layout()->replaceWidget(context, fallback);
            }

            return true;
        } catch (const std::exception& e) {
            m_logger.error(QString("Fallback method failed: %1").arg(e.what()));
        }
    }

    return false;
}

bool UIRecoveryManager::tryGracefulDegradation(
    QWidget* context, const ErrorHandling::ErrorInfo& error) {
    m_logger.info(QString("Attempting graceful degradation for error: %1")
                      .arg(error.message));

    if (!context)
        return false;

    try {
        // Disable advanced features and use basic functionality
        context->setEnabled(true);

        // Remove complex child widgets that might be causing issues
        auto children = context->findChildren<QWidget*>(
            QString(), Qt::FindDirectChildrenOnly);
        for (auto* child : children) {
            if (child->objectName().contains("advanced") ||
                child->objectName().contains("complex")) {
                child->setVisible(false);
            }
        }

        // Add a simple status indicator
        QLabel* statusLabel = new QLabel(tr("Running in safe mode"), context);
        statusLabel->setStyleSheet(
            "QLabel { color: orange; font-size: 10px; }");

        if (context->layout()) {
            context->layout()->addWidget(statusLabel);
        }

        return true;
    } catch (const std::exception& e) {
        m_logger.error(
            QString("Graceful degradation failed: %1").arg(e.what()));
    }

    return false;
}

bool UIRecoveryManager::tryResetToDefault(
    QWidget* context, const ErrorHandling::ErrorInfo& error) {
    m_logger.info(QString("Attempting reset to default for error: %1")
                      .arg(error.message));

    if (!context)
        return false;

    try {
        // Reset widget properties to defaults
        context->setStyleSheet("");
        context->setEnabled(true);
        context->setVisible(true);

        // Reset geometry if it seems problematic
        if (context->size().width() <= 0 || context->size().height() <= 0) {
            context->resize(200, 100);
        }

        // Clear any custom properties that might be causing issues
        const QMetaObject* metaObj = context->metaObject();
        for (int i = 0; i < metaObj->propertyCount(); ++i) {
            QMetaProperty prop = metaObj->property(i);
            if (prop.isWritable() && prop.isResettable()) {
                prop.reset(context);
            }
        }

        return true;
    } catch (const std::exception& e) {
        m_logger.error(QString("Reset to default failed: %1").arg(e.what()));
    }

    return false;
}

// Widget state management

UIRecoveryManager::WidgetState UIRecoveryManager::captureWidgetState(
    QWidget* widget) {
    WidgetState state;

    if (!widget)
        return state;

    state.objectName = widget->objectName();
    state.styleSheet = widget->styleSheet();
    state.enabled = widget->isEnabled();
    state.visible = widget->isVisible();
    state.geometry = widget->geometry();

    // Capture dynamic properties
    const QMetaObject* metaObj = widget->metaObject();
    for (int i = 0; i < metaObj->propertyCount(); ++i) {
        QMetaProperty prop = metaObj->property(i);
        if (prop.isReadable()) {
            QString propName = prop.name();
            QVariant value = prop.read(widget);
            state.properties[propName] = value;
        }
    }

    return state;
}

void UIRecoveryManager::applyWidgetState(QWidget* widget,
                                         const WidgetState& state) {
    if (!widget)
        return;

    widget->setObjectName(state.objectName);
    widget->setStyleSheet(state.styleSheet);
    widget->setEnabled(state.enabled);
    widget->setVisible(state.visible);
    widget->setGeometry(state.geometry);

    // Restore dynamic properties
    const QMetaObject* metaObj = widget->metaObject();
    for (auto it = state.properties.begin(); it != state.properties.end();
         ++it) {
        int propIndex = metaObj->indexOfProperty(it.key().toUtf8().constData());
        if (propIndex >= 0) {
            QMetaProperty prop = metaObj->property(propIndex);
            if (prop.isWritable()) {
                prop.write(widget, it.value());
            }
        }
    }
}

// Helper methods

QStringList UIRecoveryManager::getRecoveryOptions(UIErrorType errorType) {
    QStringList options;

    switch (errorType) {
        case UIErrorType::WidgetCreationFailed:
            options << tr("Use simplified widget") << tr("Skip this component");
            break;
        case UIErrorType::StyleApplicationFailed:
            options << tr("Use default theme") << tr("Disable styling");
            break;
        case UIErrorType::LayoutError:
            options << tr("Use simple layout")
                    << tr("Hide problematic elements");
            break;
        case UIErrorType::DataBindingError:
            options << tr("Use cached data") << tr("Disable live updates");
            break;
        default:
            options << tr("Retry operation")
                    << tr("Continue without this feature");
            break;
    }

    return options;
}

QString UIRecoveryManager::getErrorTypeString(UIErrorType errorType) {
    switch (errorType) {
        case UIErrorType::WidgetCreationFailed:
            return "WidgetCreationFailed";
        case UIErrorType::LayoutError:
            return "LayoutError";
        case UIErrorType::StyleApplicationFailed:
            return "StyleApplicationFailed";
        case UIErrorType::EventHandlingError:
            return "EventHandlingError";
        case UIErrorType::DataBindingError:
            return "DataBindingError";
        case UIErrorType::ValidationError:
            return "ValidationError";
        case UIErrorType::ResourceLoadError:
            return "ResourceLoadError";
        case UIErrorType::PermissionError:
            return "PermissionError";
        case UIErrorType::StateCorruption:
            return "StateCorruption";
        case UIErrorType::MemoryPressure:
            return "MemoryPressure";
    }
    return "Unknown";
}

void UIRecoveryManager::logRecoveryAttempt(UIErrorType errorType,
                                           const QString& method,
                                           bool success) {
    QString result = success ? "SUCCESS" : "FAILED";
    m_logger.info(QString("Recovery attempt - Type: %1, Method: %2, Result: %3")
                      .arg(getErrorTypeString(errorType), method, result));
}

// UIOperationGuard implementation

UIOperationGuard::UIOperationGuard(QWidget* widget, const QString& operation)
    : m_widget(widget),
      m_operation(operation),
      m_errorType(UIRecoveryManager::UIErrorType::EventHandlingError),
      m_committed(false),
      m_failed(false) {
    if (m_widget) {
        UIRecoveryManager::instance().saveWidgetState(m_widget);
    }
}

UIOperationGuard::~UIOperationGuard() {
    if (!m_committed && m_failed && m_widget) {
        // Attempt recovery
        ErrorHandling::ErrorInfo error(
            ErrorHandling::ErrorCategory::UI,
            ErrorHandling::ErrorSeverity::Error,
            QString("Operation failed: %1").arg(m_operation), QString(),
            m_operation);

        UIRecoveryManager::instance().attemptRecovery(m_errorType, m_widget,
                                                      error);
    }
}

void UIOperationGuard::commit() {
    m_committed = true;
    if (m_widget) {
        UIRecoveryManager::instance().clearSavedState(m_widget);
    }
}

void UIOperationGuard::fail(const QString& error) {
    m_failed = true;
    // Recovery will be attempted in destructor
}
