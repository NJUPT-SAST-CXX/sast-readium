#pragma once

#include "ToastNotification.h"
#include <QWidget>
#include <QString>
#include <functional>

/**
 * @brief NotificationHelper - Utility class for showing notifications
 * 
 * Provides convenient static methods to replace QMessageBox with modern
 * toast notifications throughout the application.
 * 
 * Usage:
 * @code
 * // Instead of: QMessageBox::information(parent, "Title", "Message");
 * NotificationHelper::showInfo(parent, "Message");
 * 
 * // Instead of: QMessageBox::warning(parent, "Title", "Message");
 * NotificationHelper::showWarning(parent, "Message");
 * 
 * // Instead of: QMessageBox::critical(parent, "Title", "Message");
 * NotificationHelper::showError(parent, "Message");
 * 
 * // With action button
 * NotificationHelper::showSuccess(parent, "File saved", "Open", []() {
 *     // Open file action
 * });
 * @endcode
 */
class NotificationHelper {
public:
    /**
     * @brief Show an informational notification
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showInfo(QWidget* parent, const QString& message, int duration = 3000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Info, duration);
    }

    /**
     * @brief Show a success notification
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showSuccess(QWidget* parent, const QString& message, int duration = 3000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Success, duration);
    }

    /**
     * @brief Show a warning notification
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showWarning(QWidget* parent, const QString& message, int duration = 4000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Warning, duration);
    }

    /**
     * @brief Show an error notification
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showError(QWidget* parent, const QString& message, int duration = 5000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Error, duration);
    }

    /**
     * @brief Show a success notification with an action button
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param actionText Text for the action button
     * @param actionCallback Callback function when action is clicked
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showSuccess(QWidget* parent, const QString& message,
                           const QString& actionText, std::function<void()> actionCallback,
                           int duration = 4000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Success,
                               duration, actionText, actionCallback);
    }

    /**
     * @brief Show an info notification with an action button
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param actionText Text for the action button
     * @param actionCallback Callback function when action is clicked
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showInfo(QWidget* parent, const QString& message,
                        const QString& actionText, std::function<void()> actionCallback,
                        int duration = 4000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Info,
                               duration, actionText, actionCallback);
    }

    /**
     * @brief Show a warning notification with an action button
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param actionText Text for the action button
     * @param actionCallback Callback function when action is clicked
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showWarning(QWidget* parent, const QString& message,
                           const QString& actionText, std::function<void()> actionCallback,
                           int duration = 5000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Warning,
                               duration, actionText, actionCallback);
    }

    /**
     * @brief Show an error notification with an action button
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @param actionText Text for the action button
     * @param actionCallback Callback function when action is clicked
     * @param duration Duration in milliseconds (0 = no auto-dismiss)
     */
    static void showError(QWidget* parent, const QString& message,
                         const QString& actionText, std::function<void()> actionCallback,
                         int duration = 6000) {
        ToastNotification::show(parent, message, ToastNotification::Type::Error,
                               duration, actionText, actionCallback);
    }

    /**
     * @brief Show a loading notification (info type with no auto-dismiss)
     * @param parent Parent widget for positioning
     * @param message Message to display
     * @return Pointer to the toast notification (can be used to dismiss manually)
     */
    static void showLoading(QWidget* parent, const QString& message) {
        ToastNotification::show(parent, message, ToastNotification::Type::Info, 0);
    }
};

// Convenience macros for quick notifications
#define NOTIFY_INFO(parent, message) NotificationHelper::showInfo(parent, message)
#define NOTIFY_SUCCESS(parent, message) NotificationHelper::showSuccess(parent, message)
#define NOTIFY_WARNING(parent, message) NotificationHelper::showWarning(parent, message)
#define NOTIFY_ERROR(parent, message) NotificationHelper::showError(parent, message)

