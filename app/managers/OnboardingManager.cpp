#include "OnboardingManager.h"
#include "../ui/widgets/OnboardingWidget.h"
#include <QSettings>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <QTimer>
#include <QWidget>
#include <QDateTime>
#include <QDebug>

// Static member definitions
OnboardingManager* OnboardingManager::s_instance = nullptr;

const QString OnboardingManager::SETTINGS_GROUP = "Onboarding";
const QString OnboardingManager::SETTINGS_FIRST_TIME_KEY = "FirstTimeUser";
const QString OnboardingManager::SETTINGS_COMPLETED_KEY = "Completed";
const QString OnboardingManager::SETTINGS_COMPLETED_STEPS_KEY = "CompletedSteps";
const QString OnboardingManager::SETTINGS_SHOW_TIPS_KEY = "ShowTips";
const QString OnboardingManager::SETTINGS_SHOW_ON_STARTUP_KEY = "ShowOnStartup";
const QString OnboardingManager::SETTINGS_ANALYTICS_KEY = "Analytics";

OnboardingManager::OnboardingManager(QObject* parent)
    : QObject(parent)
    , m_isActive(false)
    , m_isFirstTimeUser(true)
    , m_currentStep(OnboardingStep::Welcome)
    , m_onboardingWidget(nullptr)
    , m_attachedWidget(nullptr)
    , m_showTips(true)
    , m_showOnStartup(true)
{
    m_settings = std::make_unique<QSettings>();
    initializeSteps();
    initializeTutorials();
    loadSettings();
    setupConnections();
}

OnboardingManager::~OnboardingManager()
{
    saveSettings();
}

OnboardingManager& OnboardingManager::instance()
{
    if (!s_instance) {
        s_instance = new OnboardingManager();
    }
    return *s_instance;
}

bool OnboardingManager::isFirstTimeUser() const
{
    return m_isFirstTimeUser;
}

bool OnboardingManager::isOnboardingCompleted() const
{
    return m_completedSteps.size() >= getTotalStepsCount() - 1; // Exclude Complete step
}

bool OnboardingManager::isOnboardingActive() const
{
    return m_isActive;
}

void OnboardingManager::startOnboarding()
{
    if (m_isActive) return;
    
    m_isActive = true;
    m_currentStep = OnboardingStep::Welcome;
    
    if (m_onboardingWidget) {
        m_onboardingWidget->showStep(m_currentStep);
    }
    
    trackStepStarted(m_currentStep);
    emit onboardingStarted();
    emit stepChanged(m_currentStep);
}

void OnboardingManager::stopOnboarding()
{
    if (!m_isActive) return;
    
    m_isActive = false;
    
    if (m_onboardingWidget) {
        m_onboardingWidget->hideStep();
    }
    
    saveSettings();
    emit onboardingStopped();
}

void OnboardingManager::resetOnboarding()
{
    m_completedSteps.clear();
    m_currentStep = OnboardingStep::Welcome;
    m_isFirstTimeUser = true;
    m_analyticsData = QJsonObject();
    
    saveSettings();
    
    if (m_isActive) {
        stopOnboarding();
        startOnboarding();
    }
}

void OnboardingManager::skipOnboarding()
{
    if (!m_isActive) return;
    
    // Mark all steps as completed
    for (int i = 0; i < static_cast<int>(OnboardingStep::Complete); ++i) {
        OnboardingStep step = static_cast<OnboardingStep>(i);
        if (!isStepCompleted(step)) {
            markStepCompleted(step);
            trackStepSkipped(step);
        }
    }
    
    m_isFirstTimeUser = false;
    stopOnboarding();
    emit onboardingSkipped();
    emit onboardingCompleted();
}

OnboardingStep OnboardingManager::currentStep() const
{
    return m_currentStep;
}

void OnboardingManager::nextStep()
{
    if (!m_isActive) return;
    
    markStepCompleted(m_currentStep);
    
    int nextStepInt = static_cast<int>(m_currentStep) + 1;
    if (nextStepInt >= static_cast<int>(OnboardingStep::Complete)) {
        m_currentStep = OnboardingStep::Complete;
        m_isFirstTimeUser = false;
        stopOnboarding();
        emit onboardingCompleted();
    } else {
        m_currentStep = static_cast<OnboardingStep>(nextStepInt);
        
        if (m_onboardingWidget) {
            m_onboardingWidget->showStep(m_currentStep);
        }
        
        trackStepStarted(m_currentStep);
        emit stepChanged(m_currentStep);
    }
    
    updateProgress();
}

void OnboardingManager::previousStep()
{
    if (!m_isActive) return;
    
    int prevStepInt = static_cast<int>(m_currentStep) - 1;
    if (prevStepInt < 0) {
        return;
    }
    
    m_currentStep = static_cast<OnboardingStep>(prevStepInt);
    
    if (m_onboardingWidget) {
        m_onboardingWidget->showStep(m_currentStep);
    }
    
    emit stepChanged(m_currentStep);
}

void OnboardingManager::jumpToStep(OnboardingStep step)
{
    if (!m_isActive) return;
    
    m_currentStep = step;
    
    if (m_onboardingWidget) {
        m_onboardingWidget->showStep(m_currentStep);
    }
    
    trackStepStarted(m_currentStep);
    emit stepChanged(m_currentStep);
}

bool OnboardingManager::isStepCompleted(OnboardingStep step) const
{
    return m_completedSteps.contains(step);
}

void OnboardingManager::markStepCompleted(OnboardingStep step)
{
    if (!m_completedSteps.contains(step)) {
        m_completedSteps.append(step);
        trackStepCompleted(step);
        emit stepCompleted(step);
        updateProgress();
        saveSettings();
    }
}

void OnboardingManager::startTutorial(TutorialCategory category)
{
    QString tutorialId;
    
    switch (category) {
        case TutorialCategory::GettingStarted:
            tutorialId = "getting_started";
            break;
        case TutorialCategory::BasicFeatures:
            tutorialId = "basic_features";
            break;
        case TutorialCategory::AdvancedFeatures:
            tutorialId = "advanced_features";
            break;
        case TutorialCategory::ProductivityTips:
            tutorialId = "productivity_tips";
            break;
    }
    
    startSpecificTutorial(tutorialId);
}

void OnboardingManager::startSpecificTutorial(const QString& tutorialId)
{
    trackTutorialStarted(tutorialId);
    
    // Map tutorial ID to specific onboarding step
    if (tutorialId == "open_file") {
        jumpToStep(OnboardingStep::OpenFile);
    } else if (tutorialId == "navigation") {
        jumpToStep(OnboardingStep::Navigation);
    } else if (tutorialId == "search") {
        jumpToStep(OnboardingStep::Search);
    } else if (tutorialId == "bookmarks") {
        jumpToStep(OnboardingStep::Bookmarks);
    } else if (tutorialId == "annotations") {
        jumpToStep(OnboardingStep::Annotations);
    } else if (tutorialId == "view_modes") {
        jumpToStep(OnboardingStep::ViewModes);
    } else if (tutorialId == "keyboard_shortcuts") {
        jumpToStep(OnboardingStep::KeyboardShortcuts);
    }
    
    if (!m_isActive) {
        startOnboarding();
    }
    
    emit tutorialStarted(tutorialId);
}

QJsonArray OnboardingManager::getAvailableTutorials() const
{
    return m_availableTutorials;
}

QJsonObject OnboardingManager::getTutorialInfo(const QString& tutorialId) const
{
    for (const auto& tutorial : m_availableTutorials) {
        QJsonObject obj = tutorial.toObject();
        if (obj["id"].toString() == tutorialId) {
            return obj;
        }
    }
    return QJsonObject();
}

int OnboardingManager::getCompletedStepsCount() const
{
    return m_completedSteps.size();
}

int OnboardingManager::getTotalStepsCount() const
{
    return static_cast<int>(OnboardingStep::Complete);
}

float OnboardingManager::getProgressPercentage() const
{
    int total = getTotalStepsCount() - 1; // Exclude Complete step
    if (total == 0) return 100.0f;
    return (static_cast<float>(getCompletedStepsCount()) / total) * 100.0f;
}

QList<OnboardingStep> OnboardingManager::getCompletedSteps() const
{
    return m_completedSteps;
}

QList<OnboardingStep> OnboardingManager::getRemainingSteps() const
{
    QList<OnboardingStep> remaining;
    for (int i = 0; i < static_cast<int>(OnboardingStep::Complete); ++i) {
        OnboardingStep step = static_cast<OnboardingStep>(i);
        if (!isStepCompleted(step)) {
            remaining.append(step);
        }
    }
    return remaining;
}

void OnboardingManager::setOnboardingWidget(OnboardingWidget* widget)
{
    m_onboardingWidget = widget;
    
    if (m_onboardingWidget) {
        connect(m_onboardingWidget, &OnboardingWidget::nextClicked,
                this, &OnboardingManager::nextStep);
        connect(m_onboardingWidget, &OnboardingWidget::previousClicked,
                this, &OnboardingManager::previousStep);
        connect(m_onboardingWidget, &OnboardingWidget::skipClicked,
                this, &OnboardingManager::skipOnboarding);
        connect(m_onboardingWidget, &OnboardingWidget::closeClicked,
                this, &OnboardingManager::stopOnboarding);
    }
}

OnboardingWidget* OnboardingManager::onboardingWidget() const
{
    return m_onboardingWidget;
}

void OnboardingManager::attachToWidget(QWidget* widget)
{
    m_attachedWidget = widget;
    
    if (m_onboardingWidget && m_attachedWidget) {
        m_onboardingWidget->setParent(m_attachedWidget);
        m_onboardingWidget->resize(m_attachedWidget->size());
        m_onboardingWidget->raise();
    }
}

void OnboardingManager::detachFromWidget()
{
    if (m_onboardingWidget) {
        m_onboardingWidget->setParent(nullptr);
    }
    m_attachedWidget = nullptr;
}

void OnboardingManager::loadSettings()
{
    m_settings->beginGroup(SETTINGS_GROUP);
    
    m_isFirstTimeUser = m_settings->value(SETTINGS_FIRST_TIME_KEY, true).toBool();
    m_showTips = m_settings->value(SETTINGS_SHOW_TIPS_KEY, true).toBool();
    m_showOnStartup = m_settings->value(SETTINGS_SHOW_ON_STARTUP_KEY, true).toBool();
    
    // Load completed steps
    QStringList completedStepStrings = m_settings->value(SETTINGS_COMPLETED_STEPS_KEY).toStringList();
    m_completedSteps.clear();
    for (const QString& stepStr : completedStepStrings) {
        m_completedSteps.append(stringToStep(stepStr));
    }
    
    // Load analytics data
    QString analyticsJson = m_settings->value(SETTINGS_ANALYTICS_KEY).toString();
    if (!analyticsJson.isEmpty()) {
        QJsonDocument doc = QJsonDocument::fromJson(analyticsJson.toUtf8());
        m_analyticsData = doc.object();
    }
    
    m_settings->endGroup();
}

void OnboardingManager::saveSettings()
{
    m_settings->beginGroup(SETTINGS_GROUP);
    
    m_settings->setValue(SETTINGS_FIRST_TIME_KEY, m_isFirstTimeUser);
    m_settings->setValue(SETTINGS_COMPLETED_KEY, isOnboardingCompleted());
    m_settings->setValue(SETTINGS_SHOW_TIPS_KEY, m_showTips);
    m_settings->setValue(SETTINGS_SHOW_ON_STARTUP_KEY, m_showOnStartup);
    
    // Save completed steps
    QStringList completedStepStrings;
    for (OnboardingStep step : m_completedSteps) {
        completedStepStrings.append(stepToString(step));
    }
    m_settings->setValue(SETTINGS_COMPLETED_STEPS_KEY, completedStepStrings);
    
    // Save analytics data
    QJsonDocument doc(m_analyticsData);
    m_settings->setValue(SETTINGS_ANALYTICS_KEY, QString::fromUtf8(doc.toJson(QJsonDocument::Compact)));
    
    m_settings->endGroup();
    m_settings->sync();
}

void OnboardingManager::resetSettings()
{
    m_settings->beginGroup(SETTINGS_GROUP);
    m_settings->remove("");
    m_settings->endGroup();
    
    m_isFirstTimeUser = true;
    m_completedSteps.clear();
    m_showTips = true;
    m_showOnStartup = true;
    m_analyticsData = QJsonObject();
}

bool OnboardingManager::shouldShowTips() const
{
    return m_showTips;
}

void OnboardingManager::setShowTips(bool show)
{
    if (m_showTips != show) {
        m_showTips = show;
        emit showTipsChanged(show);
        saveSettings();
    }
}

bool OnboardingManager::shouldShowOnStartup() const
{
    return m_showOnStartup;
}

void OnboardingManager::setShowOnStartup(bool show)
{
    if (m_showOnStartup != show) {
        m_showOnStartup = show;
        emit showOnStartupChanged(show);
        saveSettings();
    }
}

void OnboardingManager::trackStepStarted(OnboardingStep step)
{
    QString stepStr = stepToString(step);
    QJsonObject stepData = m_analyticsData[stepStr].toObject();
    stepData["started_count"] = stepData["started_count"].toInt() + 1;
    stepData["last_started"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_analyticsData[stepStr] = stepData;
}

void OnboardingManager::trackStepCompleted(OnboardingStep step)
{
    QString stepStr = stepToString(step);
    QJsonObject stepData = m_analyticsData[stepStr].toObject();
    stepData["completed_count"] = stepData["completed_count"].toInt() + 1;
    stepData["last_completed"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_analyticsData[stepStr] = stepData;
}

void OnboardingManager::trackStepSkipped(OnboardingStep step)
{
    QString stepStr = stepToString(step);
    QJsonObject stepData = m_analyticsData[stepStr].toObject();
    stepData["skipped_count"] = stepData["skipped_count"].toInt() + 1;
    stepData["last_skipped"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    m_analyticsData[stepStr] = stepData;
}

void OnboardingManager::trackTutorialStarted(const QString& tutorialId)
{
    QJsonObject tutorialData = m_analyticsData["tutorials"].toObject();
    QJsonObject specific = tutorialData[tutorialId].toObject();
    specific["started_count"] = specific["started_count"].toInt() + 1;
    specific["last_started"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    tutorialData[tutorialId] = specific;
    m_analyticsData["tutorials"] = tutorialData;
}

void OnboardingManager::trackTutorialCompleted(const QString& tutorialId)
{
    QJsonObject tutorialData = m_analyticsData["tutorials"].toObject();
    QJsonObject specific = tutorialData[tutorialId].toObject();
    specific["completed_count"] = specific["completed_count"].toInt() + 1;
    specific["last_completed"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    tutorialData[tutorialId] = specific;
    m_analyticsData["tutorials"] = tutorialData;
    
    emit tutorialCompleted(tutorialId);
}

void OnboardingManager::onApplicationStarted()
{
    if (m_isFirstTimeUser && m_showOnStartup) {
        // Start onboarding after a short delay to let the UI settle
        QTimer::singleShot(500, this, [this]() {
            startOnboarding();
        });
    }
}

void OnboardingManager::onDocumentOpened()
{
    if (m_isActive && m_currentStep == OnboardingStep::OpenFile) {
        markStepCompleted(m_currentStep);
        nextStep();
    }
}

void OnboardingManager::onFeatureUsed(const QString& featureName)
{
    // Track feature usage and potentially advance onboarding
    if (!m_isActive) return;
    
    if (featureName == "search" && m_currentStep == OnboardingStep::Search) {
        markStepCompleted(m_currentStep);
    } else if (featureName == "bookmark" && m_currentStep == OnboardingStep::Bookmarks) {
        markStepCompleted(m_currentStep);
    } else if (featureName == "annotation" && m_currentStep == OnboardingStep::Annotations) {
        markStepCompleted(m_currentStep);
    }
}

void OnboardingManager::onStepTimeout()
{
    // Handle timeout for automatic progression
    if (m_isActive) {
        nextStep();
    }
}

void OnboardingManager::updateProgress()
{
    float percentage = getProgressPercentage();
    emit progressUpdated(percentage);
}

void OnboardingManager::initializeSteps()
{
    // Steps are defined in the enum
}

void OnboardingManager::initializeTutorials()
{
    m_availableTutorials = QJsonArray{
        QJsonObject{
            {"id", "open_file"},
            {"title", "Opening Files"},
            {"description", "Learn how to open PDF files and folders"},
            {"category", "getting_started"},
            {"duration", "2 min"},
            {"difficulty", "Beginner"}
        },
        QJsonObject{
            {"id", "navigation"},
            {"title", "Document Navigation"},
            {"description", "Navigate through pages and sections efficiently"},
            {"category", "basic_features"},
            {"duration", "3 min"},
            {"difficulty", "Beginner"}
        },
        QJsonObject{
            {"id", "search"},
            {"title", "Search Features"},
            {"description", "Master the powerful search capabilities"},
            {"category", "basic_features"},
            {"duration", "5 min"},
            {"difficulty", "Intermediate"}
        },
        QJsonObject{
            {"id", "bookmarks"},
            {"title", "Managing Bookmarks"},
            {"description", "Organize your reading with bookmarks"},
            {"category", "basic_features"},
            {"duration", "3 min"},
            {"difficulty", "Beginner"}
        },
        QJsonObject{
            {"id", "annotations"},
            {"title", "Annotations & Notes"},
            {"description", "Add highlights and notes to documents"},
            {"category", "advanced_features"},
            {"duration", "4 min"},
            {"difficulty", "Intermediate"}
        },
        QJsonObject{
            {"id", "view_modes"},
            {"title", "View Modes"},
            {"description", "Customize your reading experience"},
            {"category", "basic_features"},
            {"duration", "2 min"},
            {"difficulty", "Beginner"}
        },
        QJsonObject{
            {"id", "keyboard_shortcuts"},
            {"title", "Keyboard Shortcuts"},
            {"description", "Work faster with keyboard shortcuts"},
            {"category", "productivity_tips"},
            {"duration", "5 min"},
            {"difficulty", "Advanced"}
        }
    };
}

void OnboardingManager::setupConnections()
{
    // Setup internal connections if needed
}

QString OnboardingManager::stepToString(OnboardingStep step) const
{
    switch (step) {
        case OnboardingStep::Welcome: return "Welcome";
        case OnboardingStep::OpenFile: return "OpenFile";
        case OnboardingStep::Navigation: return "Navigation";
        case OnboardingStep::Search: return "Search";
        case OnboardingStep::Bookmarks: return "Bookmarks";
        case OnboardingStep::Annotations: return "Annotations";
        case OnboardingStep::ViewModes: return "ViewModes";
        case OnboardingStep::Settings: return "Settings";
        case OnboardingStep::KeyboardShortcuts: return "KeyboardShortcuts";
        case OnboardingStep::Complete: return "Complete";
        default: return "Unknown";
    }
}

OnboardingStep OnboardingManager::stringToStep(const QString& str) const
{
    if (str == "Welcome") return OnboardingStep::Welcome;
    if (str == "OpenFile") return OnboardingStep::OpenFile;
    if (str == "Navigation") return OnboardingStep::Navigation;
    if (str == "Search") return OnboardingStep::Search;
    if (str == "Bookmarks") return OnboardingStep::Bookmarks;
    if (str == "Annotations") return OnboardingStep::Annotations;
    if (str == "ViewModes") return OnboardingStep::ViewModes;
    if (str == "Settings") return OnboardingStep::Settings;
    if (str == "KeyboardShortcuts") return OnboardingStep::KeyboardShortcuts;
    if (str == "Complete") return OnboardingStep::Complete;
    return OnboardingStep::Welcome;
}
