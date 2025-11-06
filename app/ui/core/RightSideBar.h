#ifndef RIGHTSIDEBAR_H
#define RIGHTSIDEBAR_H

#include <poppler/qt6/poppler-qt6.h>
#include <QString>
#include "ElaDockWidget.h"

// Forward declarations
class ElaTabWidget;
class QWidget;
class PropertiesPanel;
class AnnotationsPanel;
class LayersPanel;
class SearchPanel;
class DebugLogPanel;
class QPropertyAnimation;
class QSettings;

/**
 * @brief ElaRightSideBar - 右侧边栏组件
 *
 * 包含：
 * 1. 属性面板 - 显示选中对象的属性
 * 2. 注释面板 - 显示和管理注释
 * 3. 图层面板 - 显示和管理图层（如果支持）
 * 4. 搜索面板 - 全文搜索功能
 * 5. 调试面板 - 调试日志显示
 */
class RightSideBar : public ElaDockWidget {
    Q_OBJECT

public:
    enum TabIndex {
        PropertiesTab = 0,
        AnnotationsTab = 1,
        LayersTab = 2,
        SearchTab = 3,
        DebugTab = 4
    };
    Q_ENUM(TabIndex)

    explicit RightSideBar(QWidget* parent = nullptr);
    ~RightSideBar() override;

    // Tab management
    void switchToTab(TabIndex index);
    TabIndex currentTab() const;

    // Document management
    void setDocument(Poppler::Document* document, const QString& filePath);
    void clearDocument();
    bool hasDocument() const;

    // Panel access
    PropertiesPanel* propertiesPanel() const { return m_propertiesPanel; }
    AnnotationsPanel* annotationsPanel() const { return m_annotationsPanel; }
    LayersPanel* layersPanel() const { return m_layersPanel; }
    SearchPanel* searchPanel() const { return m_searchPanel; }
    DebugLogPanel* debugPanel() const { return m_debugPanel; }

    // ========================================================================
    // 可见性和宽度管理
    // ========================================================================

    /**
     * @brief 显示侧边栏（带动画）
     * @param animated 是否使用动画
     */
    void show(bool animated = true);

    /**
     * @brief 隐藏侧边栏（带动画）
     * @param animated 是否使用动画
     */
    void hide(bool animated = true);

    /**
     * @brief 切换可见性
     * @param animated 是否使用动画
     */
    void toggleVisibility(bool animated = true);

    /**
     * @brief 设置可见性（测试兼容重载）
     * @param visible 目标可见状态
     * @param animated 是否使用动画
     */
    void setVisible(bool visible, bool animated);

    /**
     * @brief QWidget-compatible visibility setter
     * Convenience wrapper to preserve existing call sites; non-animated by
     * default
     */
    void setVisible(bool visible) override;

    /**
     * @brief 设置首选宽度
     */
    void setPreferredWidth(int width);

    /**
     * @brief 获取首选宽度
     */
    int getPreferredWidth() const { return m_preferredWidth; }

    /**
     * @brief 获取宽度约束（测试兼容）
     */
    int getMinimumWidth() const { return minimumWidth; }
    int getMaximumWidth() const { return maximumWidth; }

    /**
     * @brief 是否可见
     */
    bool isRightSideBarVisible() const { return m_isCurrentlyVisible; }

    /**
     * @brief 保存状态到 QSettings
     */
    void saveState();

    /**
     * @brief 从 QSettings 恢复状态
     */
    void restoreState();

signals:
    void tabChanged(TabIndex index);
    void navigateToPage(int pageNumber);

    /**
     * @brief 可见性改变
     */
    void visibilityChanged(bool visible);

    /**
     * @brief 宽度改变
     */
    void widthChanged(int width);

protected:
    void changeEvent(QEvent* event) override;

private slots:
    void onAnimationFinished();

private:
    ElaTabWidget* m_tabWidget;
    PropertiesPanel* m_propertiesPanel;
    AnnotationsPanel* m_annotationsPanel;
    LayersPanel* m_layersPanel;
    SearchPanel* m_searchPanel;
    DebugLogPanel* m_debugPanel;

    // 可见性和宽度管理
    QPropertyAnimation* m_animation;
    QSettings* m_settings;
    bool m_isCurrentlyVisible;
    int m_preferredWidth;
    int m_lastWidth;

    // 常量
    static const int minimumWidth = 200;
    static const int maximumWidth = 400;
    static const int defaultWidth = 250;
    static const int animationDuration = 300;

    void setupUi();
    void retranslateUi();
    void initAnimation();
    void initSettings();
};

#endif  // RIGHTSIDEBAR_H
