#pragma once

#include <QColor>
#include <QHash>
#include <QObject>
#include "plugin/PluginInterface.h"

/**
 * @brief Theme definition structure
 */
struct ThemeDefinition {
    QString name;
    QString displayName;
    QColor backgroundColor;
    QColor textColor;
    QColor accentColor;
    QColor highlightColor;
    QColor borderColor;
    QString customStyleSheet;
    bool isDark;
};

/**
 * @brief ThemeProviderPlugin - Example theme customization plugin
 *
 * This plugin demonstrates:
 * - **Custom Themes**: Define and apply custom color schemes
 * - **Settings Persistence**: Save/load theme preferences
 * - **Dynamic Switching**: Real-time theme changes via EventBus
 * - **StyleSheet Generation**: Generate Qt stylesheets from themes
 * - **UI Extension**: Theme selector in toolbar/menu
 */
class ThemeProviderPlugin : public PluginBase, public IUIExtension {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.sast.readium.IPlugin/1.0" FILE
                          "theme_provider.json")
    Q_INTERFACES(IPluginInterface IUIExtension)

public:
    explicit ThemeProviderPlugin(QObject* parent = nullptr);
    ~ThemeProviderPlugin() override;

    void handleMessage(const QString& from, const QVariant& message) override;

    // IUIExtension interface
    QList<QAction*> menuActions() const override;
    QList<QAction*> toolbarActions() const override;
    QList<QAction*> contextMenuActions() const override { return {}; }
    QString statusBarMessage() const override;
    QWidget* createDockWidget() override { return nullptr; }
    QString menuPath() const override { return "View/Themes"; }
    QString toolbarId() const override { return "view_toolbar"; }

protected:
    bool onInitialize() override;
    void onShutdown() override;

private slots:
    void onThemeSelected(const QString& themeName);
    void onOpenThemeEditor();

private:
    void registerHooks();
    void unregisterHooks();
    void setupEventSubscriptions();
    void loadBuiltinThemes();
    void loadCustomThemes();
    void saveCustomThemes();

    void applyTheme(const QString& themeName);
    QString generateStyleSheet(const ThemeDefinition& theme) const;

    QVariant onSettingsChanged(const QVariantMap& context);

    // Theme storage
    QHash<QString, ThemeDefinition> m_themes;
    QString m_activeTheme;

    // UI Actions
    QList<QAction*> m_menuActions;
    QAction* m_themeEditorAction;

    // Statistics
    int m_themeChanges;
};
