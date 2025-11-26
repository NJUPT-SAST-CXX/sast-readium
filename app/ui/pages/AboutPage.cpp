#include "AboutPage.h"

// ElaWidgetTools
#include "ElaContentDialog.h"
#include "ElaPushButton.h"
#include "ElaText.h"

// Qt
#include <QHBoxLayout>
#include <QScrollArea>
#include <QVBoxLayout>

#include <QDesktopServices>
#include <QEvent>
#include <QIcon>
#include <QUrl>

// Logging
#include "logging/SimpleLogging.h"

// ============================================================================
// 构造和析构
// ============================================================================

AboutPage::AboutPage(QWidget* parent) : ElaDialog(parent) {
    SLOG_INFO("AboutPage: Constructor started");

    // Set dialog properties (following ElaWidgetTools example pattern)
    setFixedSize(500, 600);
    setWindowTitle(tr("About SAST Readium"));
    setWindowModality(Qt::ApplicationModal);
    setWindowButtonFlags(ElaAppBarType::CloseButtonHint);
    setIsFixedSize(true);

    setupUi();
    connectSignals();

    SLOG_INFO("AboutPage: Constructor completed");
}

AboutPage::~AboutPage() { SLOG_INFO("AboutPage: Destructor called"); }

// ============================================================================
// UI 初始化
// ============================================================================

void AboutPage::setupUi() {
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(20);
    mainLayout->setAlignment(Qt::AlignCenter);

    // 应用名称
    m_appNameLabel = new ElaText(tr("SAST Readium"), this);
    QFont nameFont = m_appNameLabel->font();
    nameFont.setPointSize(24);
    nameFont.setBold(true);
    m_appNameLabel->setFont(nameFont);
    m_appNameLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_appNameLabel);

    // 版本号
    m_versionLabel =
        new ElaText(tr("Version 2.0.0 (ElaWidgetTools Edition)"), this);
    QFont versionFont = m_versionLabel->font();
    versionFont.setPointSize(12);
    m_versionLabel->setFont(versionFont);
    m_versionLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_versionLabel);

    mainLayout->addSpacing(20);

    // 版权信息
    m_copyrightLabel = new ElaText(tr("Copyright © 2024 SAST Team"), this);
    m_copyrightLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(m_copyrightLabel);

    // 描述
    m_descriptionLabel = new ElaText(
        tr("A modern PDF reader built with Qt6 and ElaWidgetTools.\n"
           "Featuring a beautiful Fluent Design interface and powerful PDF "
           "viewing capabilities."),
        this);
    m_descriptionLabel->setAlignment(Qt::AlignCenter);
    m_descriptionLabel->setWordWrap(true);
    mainLayout->addWidget(m_descriptionLabel);

    mainLayout->addSpacing(30);

    // 按钮
    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setSpacing(10);

    m_licenseBtn = new ElaPushButton(tr("License"), this);
    buttonLayout->addWidget(m_licenseBtn);

    m_creditsBtn = new ElaPushButton(tr("Credits"), this);
    buttonLayout->addWidget(m_creditsBtn);

    m_websiteBtn = new ElaPushButton(tr("Website"), this);
    buttonLayout->addWidget(m_websiteBtn);

    mainLayout->addLayout(buttonLayout);

    mainLayout->addStretch();
}

void AboutPage::connectSignals() {
    connect(m_licenseBtn, &ElaPushButton::clicked, this,
            &AboutPage::showLicense);
    connect(m_creditsBtn, &ElaPushButton::clicked, this,
            &AboutPage::showCredits);
    connect(m_websiteBtn, &ElaPushButton::clicked, this,
            &AboutPage::openWebsite);
}

// ============================================================================
// 按钮处理
// ============================================================================

void AboutPage::showLicense() {
    SLOG_INFO("AboutPage: Showing license");

    QString licenseText =
        tr("MIT License\n\n"
           "Copyright (c) 2024 SAST Team\n\n"
           "Permission is hereby granted, free of charge, to any person "
           "obtaining a copy\n"
           "of this software and associated documentation files (the "
           "\"Software\"), to deal\n"
           "in the Software without restriction, including without limitation "
           "the rights\n"
           "to use, copy, modify, merge, publish, distribute, sublicense, "
           "and/or sell\n"
           "copies of the Software, and to permit persons to whom the Software "
           "is\n"
           "furnished to do so, subject to the following conditions:\n\n"
           "The above copyright notice and this permission notice shall be "
           "included in all\n"
           "copies or substantial portions of the Software.\n\n"
           "THE SOFTWARE IS PROVIDED \"AS IS\", WITHOUT WARRANTY OF ANY KIND, "
           "EXPRESS OR\n"
           "IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF "
           "MERCHANTABILITY,\n"
           "FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT "
           "SHALL THE\n"
           "AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR "
           "OTHER\n"
           "LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, "
           "ARISING FROM,\n"
           "OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER "
           "DEALINGS IN THE\n"
           "SOFTWARE.");

    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("License"));

    auto* centralWidget = new QWidget(dialog);
    auto* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(20, 10, 20, 10);

    auto* textLabel = new ElaText(licenseText, centralWidget);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel);

    dialog->setCentralWidget(centralWidget);
    dialog->setLeftButtonText(QString());
    dialog->setMiddleButtonText(QString());
    dialog->setRightButtonText(tr("OK"));

    connect(dialog, &ElaContentDialog::rightButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();
}

void AboutPage::showCredits() {
    SLOG_INFO("AboutPage: Showing credits");

    QString creditsText =
        tr("SAST Readium is built with the following open-source libraries:\n\n"
           "• Qt6 - Cross-platform application framework\n"
           "• ElaWidgetTools - Modern Fluent Design UI components\n"
           "• Poppler - PDF rendering library\n"
           "• spdlog - Fast C++ logging library\n\n"
           "Special thanks to:\n"
           "• SAST Team - Development and maintenance\n"
           "• All contributors and testers\n"
           "• The open-source community");

    auto* dialog = new ElaContentDialog(this);
    dialog->setWindowTitle(tr("Credits"));

    auto* centralWidget = new QWidget(dialog);
    auto* layout = new QVBoxLayout(centralWidget);
    layout->setContentsMargins(20, 10, 20, 10);

    auto* textLabel = new ElaText(creditsText, centralWidget);
    textLabel->setWordWrap(true);
    layout->addWidget(textLabel);

    dialog->setCentralWidget(centralWidget);
    dialog->setLeftButtonText(QString());
    dialog->setMiddleButtonText(QString());
    dialog->setRightButtonText(tr("OK"));

    connect(dialog, &ElaContentDialog::rightButtonClicked, dialog,
            &ElaContentDialog::close);
    dialog->exec();
    dialog->deleteLater();
}

void AboutPage::openWebsite() {
    SLOG_INFO("AboutPage: Opening website");

    QDesktopServices::openUrl(
        QUrl("https://github.com/NJUPT-SAST/sast-readium"));
}

// ============================================================================
// 事件处理
// ============================================================================

void AboutPage::changeEvent(QEvent* event) {
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    ElaDialog::changeEvent(event);
}

void AboutPage::retranslateUi() {
    SLOG_INFO("AboutPage: Retranslating UI");

    m_appNameLabel->setText(tr("SAST Readium"));
    m_versionLabel->setText(tr("Version 2.0.0 (ElaWidgetTools Edition)"));
    m_copyrightLabel->setText(tr("Copyright © 2024 SAST Team"));
    m_descriptionLabel->setText(
        tr("A modern PDF reader built with Qt6 and ElaWidgetTools.\n"
           "Featuring a beautiful Fluent Design interface and powerful PDF "
           "viewing capabilities."));
    m_licenseBtn->setText(tr("License"));
    m_creditsBtn->setText(tr("Credits"));
    m_websiteBtn->setText(tr("Website"));
}
