#ifndef ABOUTPAGE_H
#define ABOUTPAGE_H

#include "ElaDialog.h"

// Forward declarations
class ElaText;
class ElaPushButton;
class ElaImageCard;
class ElaAcrylicUrlCard;
class ElaScrollPageArea;

/**
 * @brief AboutPage - 关于页面
 *
 * 显示应用程序信息：
 * - 应用名称和版本
 * - 版权信息
 * - 许可证信息
 * - 第三方库信息
 * - 开发团队信息
 *
 * 继承自 ElaDialog 以提供模态对话框功能
 */
class AboutPage : public ElaDialog {
    Q_OBJECT

public:
    explicit AboutPage(QWidget* parent = nullptr);
    ~AboutPage() override;

protected:
    void changeEvent(QEvent* event) override;

private:
    ElaText* m_appNameLabel;
    ElaText* m_versionLabel;
    ElaText* m_copyrightLabel;
    ElaText* m_descriptionLabel;
    ElaPushButton* m_licenseBtn;
    ElaPushButton* m_creditsBtn;
    ElaPushButton* m_websiteBtn;

    // Enhanced UI components
    ElaImageCard* m_logoCard;
    ElaAcrylicUrlCard* m_githubCard;
    ElaAcrylicUrlCard* m_docsCard;
    ElaAcrylicUrlCard* m_issuesCard;
    ElaScrollPageArea* m_infoContainer;
    ElaScrollPageArea* m_linksContainer;

    void setupUi();
    void connectSignals();
    void retranslateUi();
    void showLicense();
    void showCredits();
    void openWebsite();
};

#endif  // ABOUTPAGE_H
