#pragma once

#include <QAction>
#include <QList>
#include <QObject>
#include <QString>
#include <QWidget>
#include "../../../app/plugin/PluginInterface.h"

/**
 * @brief HelloPlugin - Example plugin demonstrating plugin system usage
 *
 * This plugin demonstrates:
 * - Basic plugin structure and lifecycle
 * - Plugin metadata
 * - Event subscription
 * - Service access
 * - Configuration management
 * - **UI Extension**: Menu items, toolbar buttons, context menu, status bar
 * - **Hook Registration**: Document workflow hooks
 * - **Plugin Communication**: Inter-plugin messaging
 */
class HelloPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "hello_plugin.json")
    Q_INTERFACES(IPluginInterface)

public:
    explicit HelloPlugin(QObject* parent = nullptr);
    ~HelloPlugin() override;

    // IPluginInterface override for inter-plugin communication
    void handleMessage(const QString& from, const QVariant& message) override;

protected:
    // PluginBase overrides
    bool onInitialize() override;
    void onShutdown() override;

    // IUIExtension interface implementation
    QList<QAction*> menuActions() const override;
    QString menuPath() const override;
    QList<QAction*> toolbarActions() const override;
    QString toolbarName() const override;
    QList<QAction*> contextMenuActions(const QString& contextId) const override;
    QString statusBarMessage() const override;
    int statusBarTimeout() const override;

private slots:
    // Event handlers
    void onDocumentOpened(const QVariant& data);
    void onDocumentClosed(const QVariant& data);

    // UI Action slots
    void onShowStatistics();
    void onResetCounter();
    void onShowAbout();
    void onCopyDocumentPath();

private:
    void setupEventSubscriptions();
    void removeEventSubscriptions();
    void setupHooks();
    void removeHooks();
    void createUIActions();
    void destroyUIActions();
    void updateStatusBar();
    void logMessage(const QString& message);

    // Hook callbacks
    QVariant onDocumentPreLoad(const QVariantMap& context);
    QVariant onDocumentPostLoad(const QVariantMap& context);

    // Plugin state
    int m_documentOpenCount;
    int m_totalPagesViewed;
    QString m_lastOpenedDocument;
    QString m_statusMessage;

    // UI Actions
    QAction* m_showStatsAction;
    QAction* m_resetCounterAction;
    QAction* m_aboutAction;
    QAction* m_toolbarAction;
    QAction* m_copyPathAction;
};
