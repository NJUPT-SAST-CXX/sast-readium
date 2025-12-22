#include "ReadingProgressPlugin.h"
#include <QAction>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QProgressBar>
#include <QStandardPaths>
#include <QVBoxLayout>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

ReadingProgressPlugin::ReadingProgressPlugin(QObject* parent)
    : PluginBase(parent),
      m_sessionActive(false),
      m_averageReadingSpeed(0.5),
      m_totalReadingTime(0),
      m_totalPagesRead(0),
      m_sessionTimeoutMinutes(30) {
    m_metadata.name = "Reading Progress";
    m_metadata.version = "1.0.0";
    m_metadata.description = "Track reading progress, sessions, and statistics";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "progress.tracking" << "progress.statistics"
                              << "progress.history" << "ui.dock";
}

ReadingProgressPlugin::~ReadingProgressPlugin() { qDeleteAll(m_menuActions); }

bool ReadingProgressPlugin::onInitialize() {
    m_logger.info("ReadingProgressPlugin: Initializing...");

    m_storageFile =
        QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
        "/reading_progress.json";
    m_sessionTimeoutMinutes =
        m_configuration.value("sessionTimeoutMinutes").toInt(30);

    loadProgress();

    // Create actions
    QAction* showProgressAction = new QAction("Reading Progress", this);
    connect(showProgressAction, &QAction::triggered, this,
            &ReadingProgressPlugin::onShowProgress);
    m_menuActions.append(showProgressAction);

    QAction* showHistoryAction = new QAction("Reading History", this);
    connect(showHistoryAction, &QAction::triggered, this,
            &ReadingProgressPlugin::onShowHistory);
    m_menuActions.append(showHistoryAction);

    registerHooks();
    setupEventSubscriptions();

    m_logger.info(QString("ReadingProgressPlugin: Tracking %1 documents")
                      .arg(m_progress.size()));
    return true;
}

void ReadingProgressPlugin::onShutdown() {
    m_logger.info("ReadingProgressPlugin: Shutting down...");
    endSession();
    saveProgress();
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    eventBus()->unsubscribeAll(this);
    m_logger.info(
        QString("ReadingProgressPlugin: Total pages read: %1, Time: %2 min")
            .arg(m_totalPagesRead)
            .arg(m_totalReadingTime / 60));
}

void ReadingProgressPlugin::handleMessage(const QString& from,
                                          const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "get_progress") {
        QString docPath = msgMap.value("documentPath").toString();
        DocumentProgress prog = getProgress(docPath);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["totalPages"] = prog.totalPages;
        data["lastReadPage"] = prog.lastReadPage;
        data["percentComplete"] = prog.percentComplete;
        data["totalReadingTime"] = prog.totalReadingTime;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "get_recent") {
        int limit = msgMap.value("limit").toInt(10);
        QList<DocumentProgress> recent = getRecentDocuments(limit);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;

        QJsonArray arr;
        for (const auto& doc : recent) {
            QJsonObject obj;
            obj["documentPath"] = doc.documentPath;
            obj["documentTitle"] = doc.documentTitle;
            obj["percentComplete"] = doc.percentComplete;
            obj["lastAccessed"] = doc.lastAccessed.toString(Qt::ISODate);
            arr.append(obj);
        }
        data["documents"] = arr.toVariantList();
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);

    } else if (action == "reset_progress") {
        QString docPath = msgMap.value("documentPath").toString();
        resetProgress(docPath);

    } else if (action == "get_statistics") {
        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["readingSpeed"] = getReadingSpeed();
        data["totalReadingTime"] = m_totalReadingTime;
        data["totalPagesRead"] = m_totalPagesRead;
        data["documentsTracked"] = m_progress.size();
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

// ============================================================================
// IUIExtension
// ============================================================================

QList<QAction*> ReadingProgressPlugin::menuActions() const {
    return m_menuActions;
}

QString ReadingProgressPlugin::statusBarMessage() const {
    if (m_currentSession.documentPath.isEmpty()) {
        return QString();
    }

    DocumentProgress prog = getProgress(m_currentSession.documentPath);
    return QString("Progress: %1% | Page %2/%3")
        .arg(prog.percentComplete, 0, 'f', 1)
        .arg(prog.lastReadPage)
        .arg(prog.totalPages);
}

QWidget* ReadingProgressPlugin::createDockWidget() {
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);

    QLabel* titleLabel = new QLabel("<b>Reading Progress</b>");
    layout->addWidget(titleLabel);

    QLabel* documentLabel = new QLabel("No document open");
    documentLabel->setObjectName("documentLabel");
    layout->addWidget(documentLabel);

    QProgressBar* progressBar = new QProgressBar();
    progressBar->setObjectName("progressBar");
    progressBar->setMinimum(0);
    progressBar->setMaximum(100);
    progressBar->setValue(0);
    layout->addWidget(progressBar);

    QLabel* statsLabel = new QLabel("Pages: 0 | Time: 0 min");
    statsLabel->setObjectName("statsLabel");
    layout->addWidget(statsLabel);

    QLabel* speedLabel = new QLabel("Reading speed: -- pages/min");
    speedLabel->setObjectName("speedLabel");
    layout->addWidget(speedLabel);

    layout->addStretch();
    return widget;
}

// ============================================================================
// Progress API
// ============================================================================

DocumentProgress ReadingProgressPlugin::getProgress(
    const QString& documentPath) const {
    return m_progress.value(documentPath);
}

QList<DocumentProgress> ReadingProgressPlugin::getRecentDocuments(
    int limit) const {
    QList<DocumentProgress> all = m_progress.values();

    std::sort(all.begin(), all.end(),
              [](const DocumentProgress& a, const DocumentProgress& b) {
                  return a.lastAccessed > b.lastAccessed;
              });

    return all.mid(0, limit);
}

void ReadingProgressPlugin::markPageRead(const QString& documentPath,
                                         int pageNumber) {
    if (!m_progress.contains(documentPath)) {
        DocumentProgress prog;
        prog.documentPath = documentPath;
        prog.firstAccessed = QDateTime::currentDateTime();
        m_progress[documentPath] = prog;
    }

    DocumentProgress& prog = m_progress[documentPath];
    prog.lastReadPage = pageNumber;
    prog.lastAccessed = QDateTime::currentDateTime();

    if (prog.totalPages > 0) {
        prog.percentComplete =
            (static_cast<double>(pageNumber) / prog.totalPages) * 100.0;
    }

    m_totalPagesRead++;

    Event* event = new Event("progress.updated");
    QVariantMap data;
    data["documentPath"] = documentPath;
    data["pageNumber"] = pageNumber;
    data["percentComplete"] = prog.percentComplete;
    event->setData(QVariant::fromValue(data));
    eventBus()->publish(event);
}

void ReadingProgressPlugin::resetProgress(const QString& documentPath) {
    if (m_progress.contains(documentPath)) {
        m_progress[documentPath].lastReadPage = 1;
        m_progress[documentPath].percentComplete = 0.0;
        m_progress[documentPath].totalReadingTime = 0;
        m_progress[documentPath].sessions.clear();
    }
}

double ReadingProgressPlugin::getReadingSpeed() const {
    if (m_totalReadingTime > 0 && m_totalPagesRead > 0) {
        return static_cast<double>(m_totalPagesRead) /
               (m_totalReadingTime / 60.0);
    }
    return m_averageReadingSpeed;
}

int ReadingProgressPlugin::estimateTimeToComplete(
    const QString& documentPath) const {
    DocumentProgress prog = getProgress(documentPath);
    if (prog.totalPages == 0)
        return 0;

    int pagesRemaining = prog.totalPages - prog.lastReadPage;
    double speed = getReadingSpeed();
    if (speed <= 0)
        return 0;

    return static_cast<int>(pagesRemaining / speed);  // minutes
}

// ============================================================================
// Session Management
// ============================================================================

void ReadingProgressPlugin::startSession(const QString& documentPath,
                                         int startPage) {
    endSession();  // End any existing session

    m_currentSession.documentPath = documentPath;
    m_currentSession.startTime = QDateTime::currentDateTime();
    m_currentSession.startPage = startPage;
    m_currentSession.pagesRead = 0;
    m_sessionActive = true;

    m_logger.debug(QString("ReadingProgressPlugin: Session started for '%1'")
                       .arg(documentPath));
}

void ReadingProgressPlugin::endSession() {
    if (!m_sessionActive)
        return;

    m_currentSession.endTime = QDateTime::currentDateTime();
    m_currentSession.durationSeconds =
        m_currentSession.startTime.secsTo(m_currentSession.endTime);

    if (m_currentSession.durationSeconds >
        10) {  // Only count sessions > 10 seconds
        if (m_progress.contains(m_currentSession.documentPath)) {
            m_progress[m_currentSession.documentPath].sessions.append(
                m_currentSession);
            m_progress[m_currentSession.documentPath].totalReadingTime +=
                m_currentSession.durationSeconds;
        }
        m_totalReadingTime += m_currentSession.durationSeconds;
    }

    m_sessionActive = false;
    m_currentSession = ReadingSession();
}

void ReadingProgressPlugin::updateStatistics() {
    if (m_totalReadingTime > 0 && m_totalPagesRead > 0) {
        m_averageReadingSpeed =
            static_cast<double>(m_totalPagesRead) / (m_totalReadingTime / 60.0);
    }
}

// ============================================================================
// Persistence
// ============================================================================

void ReadingProgressPlugin::loadProgress() {
    QFile file(m_storageFile);
    if (!file.open(QIODevice::ReadOnly))
        return;

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject root = doc.object();
    m_totalReadingTime = root["totalReadingTime"].toInteger();
    m_totalPagesRead = root["totalPagesRead"].toInt();
    m_averageReadingSpeed = root["averageReadingSpeed"].toDouble(0.5);

    QJsonArray arr = root["documents"].toArray();
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        DocumentProgress prog;
        prog.documentPath = obj["documentPath"].toString();
        prog.documentTitle = obj["documentTitle"].toString();
        prog.totalPages = obj["totalPages"].toInt();
        prog.lastReadPage = obj["lastReadPage"].toInt();
        prog.percentComplete = obj["percentComplete"].toDouble();
        prog.totalReadingTime = obj["totalReadingTime"].toInteger();
        prog.lastAccessed =
            QDateTime::fromString(obj["lastAccessed"].toString(), Qt::ISODate);
        prog.firstAccessed =
            QDateTime::fromString(obj["firstAccessed"].toString(), Qt::ISODate);

        if (!prog.documentPath.isEmpty()) {
            m_progress[prog.documentPath] = prog;
        }
    }
}

void ReadingProgressPlugin::saveProgress() {
    QJsonArray arr;
    for (const auto& prog : m_progress) {
        QJsonObject obj;
        obj["documentPath"] = prog.documentPath;
        obj["documentTitle"] = prog.documentTitle;
        obj["totalPages"] = prog.totalPages;
        obj["lastReadPage"] = prog.lastReadPage;
        obj["percentComplete"] = prog.percentComplete;
        obj["totalReadingTime"] = prog.totalReadingTime;
        obj["lastAccessed"] = prog.lastAccessed.toString(Qt::ISODate);
        obj["firstAccessed"] = prog.firstAccessed.toString(Qt::ISODate);
        arr.append(obj);
    }

    QJsonObject root;
    root["documents"] = arr;
    root["totalReadingTime"] = m_totalReadingTime;
    root["totalPagesRead"] = m_totalPagesRead;
    root["averageReadingSpeed"] = m_averageReadingSpeed;
    root["savedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);

    QFile file(m_storageFile);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(QJsonDocument(root).toJson());
        file.close();
    }
}

// ============================================================================
// Slots
// ============================================================================

void ReadingProgressPlugin::onShowProgress() {
    eventBus()->publish(new Event("ui.showProgressPanel"));
}

void ReadingProgressPlugin::onShowHistory() {
    eventBus()->publish(new Event("ui.showReadingHistory"));
}

// ============================================================================
// Hooks & Events
// ============================================================================

void ReadingProgressPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        "page.viewed", name(),
        [this](const QVariantMap& ctx) { return onPageViewed(ctx); });
}

void ReadingProgressPlugin::setupEventSubscriptions() {
    eventBus()->subscribe("document.opened", this, [this](Event* event) {
        QVariantMap data = event->data().toMap();
        QString docPath = data.value("filePath").toString();
        int totalPages = data.value("pageCount").toInt(1);

        if (!m_progress.contains(docPath)) {
            DocumentProgress prog;
            prog.documentPath = docPath;
            prog.totalPages = totalPages;
            prog.firstAccessed = QDateTime::currentDateTime();
            m_progress[docPath] = prog;
        } else {
            m_progress[docPath].totalPages = totalPages;
        }

        startSession(docPath, m_progress[docPath].lastReadPage);
    });

    eventBus()->subscribe("document.closed", this, [this](Event*) {
        endSession();
        saveProgress();
    });

    eventBus()->subscribe("page.changed", this, [this](Event* event) {
        int page = event->data().toInt();
        if (m_sessionActive) {
            markPageRead(m_currentSession.documentPath, page);
            m_currentSession.endPage = page;
            m_currentSession.pagesRead++;
        }
    });
}

QVariant ReadingProgressPlugin::onPageViewed(const QVariantMap& context) {
    int page = context.value("pageNumber").toInt();
    QString docPath = context.value("documentPath").toString();

    if (!docPath.isEmpty()) {
        markPageRead(docPath, page);
    }

    QVariantMap result;
    result["tracked"] = true;
    return result;
}
