#include "AnnotationSyncPlugin.h"
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPainter>
#include <QUuid>
#include <QXmlStreamWriter>
#include "controller/EventBus.h"
#include "plugin/PluginHookRegistry.h"

AnnotationSyncPlugin::AnnotationSyncPlugin(QObject* parent)
    : PluginBase(parent),
      m_annotationsCreated(0),
      m_annotationsExported(0),
      m_syncOperations(0),
      m_autoSync(false) {
    m_metadata.name = "Annotation Sync";
    m_metadata.version = "1.0.0";
    m_metadata.description =
        "Annotation import/export with cloud sync simulation";
    m_metadata.author = "SAST Readium Team";
    m_metadata.dependencies = QStringList();
    m_capabilities.provides = QStringList()
                              << "annotation.plugin" << "annotation.export"
                              << "annotation.import" << "annotation.sync";
}

AnnotationSyncPlugin::~AnnotationSyncPlugin() {}

bool AnnotationSyncPlugin::onInitialize() {
    m_logger.info("AnnotationSyncPlugin: Initializing...");
    m_autoSync = m_configuration.value("autoSync").toBool(false);
    m_cloudEndpoint = m_configuration.value("cloudEndpoint")
                          .toString("http://localhost:8080");
    registerHooks();
    m_logger.info("AnnotationSyncPlugin: Initialized successfully");
    return true;
}

void AnnotationSyncPlugin::onShutdown() {
    m_logger.info("AnnotationSyncPlugin: Shutting down...");
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
    m_logger.info(
        QString("AnnotationSyncPlugin: Created: %1, Exported: %2, Syncs: %3")
            .arg(m_annotationsCreated)
            .arg(m_annotationsExported)
            .arg(m_syncOperations));
}

void AnnotationSyncPlugin::handleMessage(const QString& from,
                                         const QVariant& message) {
    QVariantMap msgMap = message.toMap();
    QString action = msgMap.value("action").toString();

    if (action == "export") {
        QString docPath = msgMap.value("documentPath").toString();
        QString outPath = msgMap.value("outputPath").toString();
        QString format = msgMap.value("format").toString("json");
        bool success = exportAnnotations(docPath, outPath, format);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["success"] = success;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    } else if (action == "sync") {
        QString docPath = msgMap.value("documentPath").toString();
        bool upload = msgMap.value("upload").toBool(true);
        bool success = upload ? syncToCloud(docPath) : syncFromCloud(docPath);

        Event* resp = new Event("plugin.response");
        QVariantMap data;
        data["from"] = name();
        data["to"] = from;
        data["success"] = success;
        resp->setData(QVariant::fromValue(data));
        eventBus()->publish(resp);
    }
}

void AnnotationSyncPlugin::registerHooks() {
    auto& registry = PluginHookRegistry::instance();
    registry.registerCallback(
        StandardHooks::ANNOTATION_CREATED, name(),
        [this](const QVariantMap& ctx) { return onAnnotationCreated(ctx); });
    registry.registerCallback(
        StandardHooks::ANNOTATION_UPDATED, name(),
        [this](const QVariantMap& ctx) { return onAnnotationUpdated(ctx); });
    registry.registerCallback(
        StandardHooks::ANNOTATION_RENDER, name(),
        [this](const QVariantMap& ctx) { return onAnnotationRender(ctx); });
}

void AnnotationSyncPlugin::unregisterHooks() {
    PluginHookRegistry::instance().unregisterAllCallbacks(name());
}

QVariant AnnotationSyncPlugin::onAnnotationCreated(const QVariantMap& context) {
    QString docPath = context.value("documentPath").toString();
    m_annotationsCreated++;

    if (m_autoSync) {
        syncToCloud(docPath);
    }

    QVariantMap result;
    result["acknowledged"] = true;
    result["totalCreated"] = m_annotationsCreated;
    return result;
}

QVariant AnnotationSyncPlugin::onAnnotationUpdated(const QVariantMap& context) {
    QString docPath = context.value("documentPath").toString();

    if (m_autoSync) {
        syncToCloud(docPath);
    }

    QVariantMap result;
    result["acknowledged"] = true;
    return result;
}

QVariant AnnotationSyncPlugin::onAnnotationRender(const QVariantMap& context) {
    Q_UNUSED(context)
    QVariantMap result;
    result["customRendering"] = true;
    return result;
}

QList<AnnotationType> AnnotationSyncPlugin::supportedTypes() const {
    return QList<AnnotationType>()
           << AnnotationType::Highlight << AnnotationType::Underline
           << AnnotationType::Strikethrough << AnnotationType::Note
           << AnnotationType::FreeText;
}

bool AnnotationSyncPlugin::createAnnotation(const AnnotationData& data,
                                            const QString& documentPath) {
    AnnotationData newData = data;
    if (newData.id.isEmpty()) {
        newData.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    }
    newData.createdAt = QDateTime::currentDateTime();
    newData.modifiedAt = newData.createdAt;

    m_annotations[documentPath].append(newData);
    m_annotationsCreated++;

    m_logger.info(
        QString("AnnotationSyncPlugin: Created annotation %1").arg(newData.id));
    return true;
}

bool AnnotationSyncPlugin::updateAnnotation(const QString& annotationId,
                                            const AnnotationData& data,
                                            const QString& documentPath) {
    auto& annotations = m_annotations[documentPath];
    for (int i = 0; i < annotations.size(); ++i) {
        if (annotations[i].id == annotationId) {
            annotations[i] = data;
            annotations[i].modifiedAt = QDateTime::currentDateTime();
            return true;
        }
    }
    return false;
}

bool AnnotationSyncPlugin::deleteAnnotation(const QString& annotationId,
                                            const QString& documentPath) {
    auto& annotations = m_annotations[documentPath];
    for (int i = 0; i < annotations.size(); ++i) {
        if (annotations[i].id == annotationId) {
            annotations.removeAt(i);
            return true;
        }
    }
    return false;
}

QList<AnnotationData> AnnotationSyncPlugin::getAnnotationsForPage(
    int pageNumber, const QString& documentPath) const {
    QList<AnnotationData> result;
    if (!m_annotations.contains(documentPath)) {
        return result;
    }

    for (const auto& ann : m_annotations[documentPath]) {
        if (ann.pageNumber == pageNumber) {
            result.append(ann);
        }
    }
    return result;
}

bool AnnotationSyncPlugin::exportAnnotations(const QString& documentPath,
                                             const QString& outputPath,
                                             const QString& format) {
    if (!m_annotations.contains(documentPath)) {
        return false;
    }

    const auto& annotations = m_annotations[documentPath];
    bool success = false;

    if (format == "json") {
        success = exportToJson(annotations, outputPath);
    } else if (format == "xfdf") {
        success = exportToXfdf(annotations, outputPath);
    }

    if (success) {
        m_annotationsExported += annotations.size();
    }
    return success;
}

int AnnotationSyncPlugin::importAnnotations(const QString& inputPath,
                                            const QString& documentPath,
                                            const QString& format) {
    QList<AnnotationData> imported;

    if (format == "json") {
        imported = importFromJson(inputPath);
    }

    for (auto& ann : imported) {
        ann.modifiedAt = QDateTime::currentDateTime();
        m_annotations[documentPath].append(ann);
    }

    return imported.size();
}

void AnnotationSyncPlugin::renderAnnotation(QPainter* painter,
                                            const AnnotationData& annotation,
                                            const QRect& pageRect,
                                            double zoom) {
    if (!painter)
        return;

    painter->save();

    QRect scaledRect(static_cast<int>(annotation.boundingRect.x() * zoom),
                     static_cast<int>(annotation.boundingRect.y() * zoom),
                     static_cast<int>(annotation.boundingRect.width() * zoom),
                     static_cast<int>(annotation.boundingRect.height() * zoom));

    QColor color = annotation.color;
    color.setAlpha(80);

    switch (annotation.type) {
        case AnnotationType::Highlight:
            painter->fillRect(scaledRect, color);
            break;
        case AnnotationType::Underline:
            painter->setPen(QPen(annotation.color, 2));
            painter->drawLine(scaledRect.bottomLeft(),
                              scaledRect.bottomRight());
            break;
        case AnnotationType::Strikethrough:
            painter->setPen(QPen(annotation.color, 2));
            painter->drawLine(scaledRect.left(), scaledRect.center().y(),
                              scaledRect.right(), scaledRect.center().y());
            break;
        case AnnotationType::Note:
            painter->fillRect(scaledRect, color);
            painter->setPen(annotation.color);
            painter->drawRect(scaledRect);
            break;
        default:
            break;
    }

    painter->restore();
}

bool AnnotationSyncPlugin::exportToJson(
    const QList<AnnotationData>& annotations, const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QJsonArray arr;
    for (const auto& ann : annotations) {
        QJsonObject obj;
        obj["id"] = ann.id;
        obj["type"] = static_cast<int>(ann.type);
        obj["pageNumber"] = ann.pageNumber;
        obj["content"] = ann.content;
        obj["color"] = ann.color.name();
        obj["author"] = ann.author;
        obj["createdAt"] = ann.createdAt.toString(Qt::ISODate);
        obj["modifiedAt"] = ann.modifiedAt.toString(Qt::ISODate);

        QJsonObject rect;
        rect["x"] = ann.boundingRect.x();
        rect["y"] = ann.boundingRect.y();
        rect["width"] = ann.boundingRect.width();
        rect["height"] = ann.boundingRect.height();
        obj["boundingRect"] = rect;

        arr.append(obj);
    }

    QJsonObject root;
    root["version"] = "1.0";
    root["exportedBy"] = name();
    root["exportedAt"] = QDateTime::currentDateTime().toString(Qt::ISODate);
    root["annotations"] = arr;

    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

bool AnnotationSyncPlugin::exportToXfdf(
    const QList<AnnotationData>& annotations, const QString& path) {
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    QXmlStreamWriter xml(&file);
    xml.setAutoFormatting(true);
    xml.writeStartDocument();
    xml.writeStartElement("xfdf");
    xml.writeAttribute("xmlns", "http://ns.adobe.com/xfdf/");
    xml.writeStartElement("annots");

    for (const auto& ann : annotations) {
        QString elemName;
        switch (ann.type) {
            case AnnotationType::Highlight:
                elemName = "highlight";
                break;
            case AnnotationType::Underline:
                elemName = "underline";
                break;
            case AnnotationType::Strikethrough:
                elemName = "strikeout";
                break;
            case AnnotationType::Note:
                elemName = "text";
                break;
            default:
                elemName = "text";
                break;
        }

        xml.writeStartElement(elemName);
        xml.writeAttribute("page", QString::number(ann.pageNumber));
        xml.writeAttribute("color", ann.color.name());
        xml.writeAttribute("title", ann.author);
        xml.writeAttribute("date", ann.modifiedAt.toString(Qt::ISODate));

        if (!ann.content.isEmpty()) {
            xml.writeTextElement("contents", ann.content);
        }

        xml.writeEndElement();
    }

    xml.writeEndElement();  // annots
    xml.writeEndElement();  // xfdf
    xml.writeEndDocument();
    file.close();
    return true;
}

QList<AnnotationData> AnnotationSyncPlugin::importFromJson(
    const QString& path) {
    QList<AnnotationData> result;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return result;
    }

    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonArray arr = doc.object()["annotations"].toArray();
    for (const auto& val : arr) {
        QJsonObject obj = val.toObject();
        AnnotationData ann;
        ann.id = obj["id"].toString();
        ann.type = static_cast<AnnotationType>(obj["type"].toInt());
        ann.pageNumber = obj["pageNumber"].toInt();
        ann.content = obj["content"].toString();
        ann.color = QColor(obj["color"].toString());
        ann.author = obj["author"].toString();
        ann.createdAt =
            QDateTime::fromString(obj["createdAt"].toString(), Qt::ISODate);
        ann.modifiedAt =
            QDateTime::fromString(obj["modifiedAt"].toString(), Qt::ISODate);

        QJsonObject rect = obj["boundingRect"].toObject();
        ann.boundingRect = QRect(rect["x"].toInt(), rect["y"].toInt(),
                                 rect["width"].toInt(), rect["height"].toInt());

        result.append(ann);
    }

    return result;
}

bool AnnotationSyncPlugin::syncToCloud(const QString& documentPath) {
    m_logger.info(QString("AnnotationSyncPlugin: Simulating upload to %1")
                      .arg(m_cloudEndpoint));
    m_syncOperations++;
    // Simulated sync - in real implementation, HTTP POST to cloud endpoint
    return true;
}

bool AnnotationSyncPlugin::syncFromCloud(const QString& documentPath) {
    m_logger.info(QString("AnnotationSyncPlugin: Simulating download from %1")
                      .arg(m_cloudEndpoint));
    m_syncOperations++;
    // Simulated sync - in real implementation, HTTP GET from cloud endpoint
    return true;
}
