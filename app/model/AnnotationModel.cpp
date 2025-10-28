#include "AnnotationModel.h"
#include <poppler-annotation.h>
#include <QColor>
#include <QDateTime>
#include <QDebug>
#include <QHash>
#include <QJsonDocument>
#include <QPointF>
#include <QRandomGenerator>
#include <QtCore>
#include <algorithm>

// PDFAnnotation serialization implementation
QJsonObject PDFAnnotation::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["type"] = static_cast<int>(type);
    obj["pageNumber"] = pageNumber;
    obj["content"] = content;
    obj["author"] = author;
    obj["createdTime"] = createdTime.toString(Qt::ISODate);
    obj["modifiedTime"] = modifiedTime.toString(Qt::ISODate);
    obj["color"] = color.name();
    obj["opacity"] = opacity;
    obj["isVisible"] = isVisible;
    obj["lineWidth"] = lineWidth;
    obj["fontFamily"] = fontFamily;
    obj["fontSize"] = fontSize;

    // Bounding rect
    QJsonObject rectObj;
    rectObj["x"] = boundingRect.x();
    rectObj["y"] = boundingRect.y();
    rectObj["width"] = boundingRect.width();
    rectObj["height"] = boundingRect.height();
    obj["boundingRect"] = rectObj;

    // Points for line/arrow annotations
    if (type == AnnotationType::Line || type == AnnotationType::Arrow) {
        QJsonObject startObj;
        startObj["x"] = startPoint.x();
        startObj["y"] = startPoint.y();
        obj["startPoint"] = startObj;

        QJsonObject endObj;
        endObj["x"] = endPoint.x();
        endObj["y"] = endPoint.y();
        obj["endPoint"] = endObj;
    }

    // Ink path for freehand drawing
    if (type == AnnotationType::Ink && !inkPath.isEmpty()) {
        QJsonArray pathArray;
        for (const QPointF& point : inkPath) {
            QJsonObject pointObj;
            pointObj["x"] = point.x();
            pointObj["y"] = point.y();
            pathArray.append(pointObj);
        }
        obj["inkPath"] = pathArray;
    }

    return obj;
}

PDFAnnotation PDFAnnotation::fromJson(const QJsonObject& json) {
    PDFAnnotation annotation;
    annotation.id = json["id"].toString();
    annotation.type = static_cast<AnnotationType>(json["type"].toInt());
    annotation.pageNumber = json["pageNumber"].toInt();
    annotation.content = json["content"].toString();
    annotation.author = json["author"].toString();
    annotation.createdTime =
        QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    annotation.modifiedTime =
        QDateTime::fromString(json["modifiedTime"].toString(), Qt::ISODate);
    annotation.color = QColor(json["color"].toString());
    annotation.opacity = json["opacity"].toDouble();
    annotation.isVisible = json["isVisible"].toBool();
    annotation.lineWidth = json["lineWidth"].toDouble();
    annotation.fontFamily = json["fontFamily"].toString();
    annotation.fontSize = json["fontSize"].toInt();

    // Bounding rect
    if (json.contains("boundingRect")) {
        QJsonObject rectObj = json["boundingRect"].toObject();
        annotation.boundingRect =
            QRectF(rectObj["x"].toDouble(), rectObj["y"].toDouble(),
                   rectObj["width"].toDouble(), rectObj["height"].toDouble());
    }

    // Points for line/arrow annotations
    if (json.contains("startPoint")) {
        QJsonObject startObj = json["startPoint"].toObject();
        annotation.startPoint =
            QPointF(startObj["x"].toDouble(), startObj["y"].toDouble());
    }
    if (json.contains("endPoint")) {
        QJsonObject endObj = json["endPoint"].toObject();
        annotation.endPoint =
            QPointF(endObj["x"].toDouble(), endObj["y"].toDouble());
    }

    // Ink path
    if (json.contains("inkPath")) {
        QJsonArray pathArray = json["inkPath"].toArray();
        for (const QJsonValue& value : pathArray) {
            QJsonObject pointObj = value.toObject();
            annotation.inkPath.append(
                QPointF(pointObj["x"].toDouble(), pointObj["y"].toDouble()));
        }
    }

    return annotation;
}

bool PDFAnnotation::containsPoint(const QPointF& point) const {
    return boundingRect.contains(point);
}

QString PDFAnnotation::getTypeString() const {
    switch (type) {
        case AnnotationType::Highlight:
            return "Highlight";
        case AnnotationType::Note:
            return "Note";
        case AnnotationType::FreeText:
            return "FreeText";
        case AnnotationType::Underline:
            return "Underline";
        case AnnotationType::StrikeOut:
            return "StrikeOut";
        case AnnotationType::Squiggly:
            return "Squiggly";
        case AnnotationType::Rectangle:
            return "Rectangle";
        case AnnotationType::Circle:
            return "Circle";
        case AnnotationType::Line:
            return "Line";
        case AnnotationType::Arrow:
            return "Arrow";
        case AnnotationType::Ink:
            return "Ink";
        default:
            return "Unknown";
    }
}

AnnotationType PDFAnnotation::typeFromString(const QString& typeStr) {
    if (typeStr == "Highlight") {
        return AnnotationType::Highlight;
    }
    if (typeStr == "Note") {
        return AnnotationType::Note;
    }
    if (typeStr == "FreeText") {
        return AnnotationType::FreeText;
    }
    if (typeStr == "Underline") {
        return AnnotationType::Underline;
    }
    if (typeStr == "StrikeOut") {
        return AnnotationType::StrikeOut;
    }
    if (typeStr == "Squiggly") {
        return AnnotationType::Squiggly;
    }
    if (typeStr == "Rectangle") {
        return AnnotationType::Rectangle;
    }
    if (typeStr == "Circle") {
        return AnnotationType::Circle;
    }
    if (typeStr == "Line") {
        return AnnotationType::Line;
    }
    if (typeStr == "Arrow") {
        return AnnotationType::Arrow;
    }
    if (typeStr == "Ink") {
        return AnnotationType::Ink;
    }
    return AnnotationType::Highlight;  // Default
}

AnnotationModel::AnnotationModel(QObject* parent)
    : QAbstractListModel(parent), m_document(nullptr) {}

int AnnotationModel::rowCount(const QModelIndex& parent) const {
    Q_UNUSED(parent)
    return m_annotations.size();
}

QVariant AnnotationModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_annotations.size()) {
        return QVariant();
    }

    const PDFAnnotation& annotation = m_annotations.at(index.row());

    switch (role) {
        case Qt::DisplayRole:
            return QString("%1 - Page %2")
                .arg(annotation.getTypeString())
                .arg(annotation.pageNumber + 1);
        case Qt::ToolTipRole:
            return QString(
                       "Type: %1\nPage: %2\nAuthor: %3\nCreated: %4\nContent: "
                       "%5")
                .arg(annotation.getTypeString())
                .arg(annotation.pageNumber + 1)
                .arg(annotation.author)
                .arg(annotation.createdTime.toString())
                .arg(annotation.content);
        case IdRole:
            return annotation.id;
        case TypeRole:
            return static_cast<int>(annotation.type);
        case PageNumberRole:
            return annotation.pageNumber;
        case BoundingRectRole:
            return annotation.boundingRect;
        case ContentRole:
            return annotation.content;
        case AuthorRole:
            return annotation.author;
        case CreatedTimeRole:
            return annotation.createdTime;
        case ModifiedTimeRole:
            return annotation.modifiedTime;
        case ColorRole:
            return annotation.color;
        case OpacityRole:
            return annotation.opacity;
        case VisibilityRole:
            return annotation.isVisible;
        default:
            return QVariant();
    }
}

bool AnnotationModel::setData(const QModelIndex& index, const QVariant& value,
                              int role) {
    if (!index.isValid() || index.row() >= m_annotations.size()) {
        return false;
    }

    PDFAnnotation& annotation = m_annotations[index.row()];
    bool changed = false;

    switch (role) {
        case ContentRole:
            if (annotation.content != value.toString()) {
                annotation.content = value.toString();
                annotation.modifiedTime = QDateTime::currentDateTime();
                changed = true;
            }
            break;
        case ColorRole:
            if (annotation.color != value.value<QColor>()) {
                annotation.color = value.value<QColor>();
                annotation.modifiedTime = QDateTime::currentDateTime();
                changed = true;
            }
            break;
        case OpacityRole: {
            double newOpacity = value.toDouble();
            // Clamp opacity to valid range [0.0, 1.0]
            newOpacity = qBound(0.0, newOpacity, 1.0);
            if (annotation.opacity != newOpacity) {
                annotation.opacity = newOpacity;
                annotation.modifiedTime = QDateTime::currentDateTime();
                changed = true;
            }
            break;
        }
        case VisibilityRole:
            if (annotation.isVisible != value.toBool()) {
                annotation.isVisible = value.toBool();
                annotation.modifiedTime = QDateTime::currentDateTime();
                changed = true;
            }
            break;
        default:
            return false;
    }

    if (changed) {
        emit dataChanged(index, index, {role});
        emit annotationUpdated(annotation);
        return true;
    }

    return false;
}

Qt::ItemFlags AnnotationModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> AnnotationModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[IdRole] = "id";
    roles[TypeRole] = "type";
    roles[PageNumberRole] = "pageNumber";
    roles[BoundingRectRole] = "boundingRect";
    roles[ContentRole] = "content";
    roles[AuthorRole] = "author";
    roles[CreatedTimeRole] = "createdTime";
    roles[ModifiedTimeRole] = "modifiedTime";
    roles[ColorRole] = "color";
    roles[OpacityRole] = "opacity";
    roles[VisibilityRole] = "isVisible";
    return roles;
}

bool AnnotationModel::addAnnotation(const PDFAnnotation& annotation) {
    // Validate annotation
    if (annotation.id.isEmpty()) {
        qWarning() << "Cannot add annotation with empty ID";
        return false;
    }

    if (annotation.pageNumber < 0) {
        qWarning() << "Cannot add annotation with invalid page number:"
                   << annotation.pageNumber;
        return false;
    }

    // Check for duplicate ID
    if (findAnnotationIndex(annotation.id) >= 0) {
        qWarning() << "Annotation with ID" << annotation.id << "already exists";
        return false;
    }

    beginInsertRows(QModelIndex(), m_annotations.size(), m_annotations.size());
    m_annotations.append(annotation);
    endInsertRows();

    sortAnnotations();
    emit annotationAdded(annotation);

    return true;
}

bool AnnotationModel::removeAnnotation(const QString& annotationId) {
    int index = findAnnotationIndex(annotationId);
    if (index < 0) {
        return false;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_annotations.removeAt(index);
    endRemoveRows();

    emit annotationRemoved(annotationId);
    return true;
}

bool AnnotationModel::updateAnnotation(const QString& annotationId,
                                       const PDFAnnotation& updatedAnnotation) {
    if (annotationId.isEmpty()) {
        qWarning() << "Cannot update annotation with empty ID";
        return false;
    }

    int index = findAnnotationIndex(annotationId);
    if (index < 0) {
        qWarning() << "Annotation with ID" << annotationId << "not found";
        return false;
    }

    // Validate updated annotation
    if (updatedAnnotation.pageNumber < 0) {
        qWarning() << "Cannot update annotation with invalid page number:"
                   << updatedAnnotation.pageNumber;
        return false;
    }

    // Preserve the original ID and ensure it matches
    PDFAnnotation annotation = updatedAnnotation;
    annotation.id = annotationId;
    annotation.modifiedTime = QDateTime::currentDateTime();

    m_annotations[index] = annotation;

    QModelIndex modelIndex = this->index(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit annotationUpdated(annotation);

    return true;
}

PDFAnnotation AnnotationModel::getAnnotation(
    const QString& annotationId) const {
    int index = findAnnotationIndex(annotationId);
    if (index >= 0) {
        return m_annotations.at(index);
    }
    return PDFAnnotation();
}

QList<PDFAnnotation> AnnotationModel::getAllAnnotations() const {
    return m_annotations;
}

QList<PDFAnnotation> AnnotationModel::getAnnotationsForPage(
    int pageNumber) const {
    QList<PDFAnnotation> result;
    for (const PDFAnnotation& annotation : m_annotations) {
        if (annotation.pageNumber == pageNumber) {
            result.append(annotation);
        }
    }
    return result;
}

bool AnnotationModel::removeAnnotationsForPage(int pageNumber) {
    bool removed = false;
    for (int i = m_annotations.size() - 1; i >= 0; --i) {
        if (m_annotations.at(i).pageNumber == pageNumber) {
            beginRemoveRows(QModelIndex(), i, i);
            QString removedId = m_annotations.at(i).id;
            m_annotations.removeAt(i);
            endRemoveRows();
            emit annotationRemoved(removedId);
            removed = true;
        }
    }
    return removed;
}

int AnnotationModel::getAnnotationCountForPage(int pageNumber) const {
    int count = 0;
    for (const PDFAnnotation& annotation : m_annotations) {
        if (annotation.pageNumber == pageNumber) {
            count++;
        }
    }
    return count;
}

void AnnotationModel::setDocument(Poppler::Document* document) {
    m_document = document;
    clearAnnotations();
    if (document) {
        loadAnnotationsFromDocument();
    }
}

void AnnotationModel::clearAnnotations() {
    beginResetModel();
    m_annotations.clear();
    endResetModel();
    emit annotationsCleared();
}

int AnnotationModel::findAnnotationIndex(const QString& annotationId) const {
    for (int i = 0; i < m_annotations.size(); ++i) {
        if (m_annotations.at(i).id == annotationId) {
            return i;
        }
    }
    return -1;
}

void AnnotationModel::sortAnnotations() {
    std::sort(m_annotations.begin(), m_annotations.end(),
              [](const PDFAnnotation& a, const PDFAnnotation& b) {
                  if (a.pageNumber != b.pageNumber) {
                      return a.pageNumber < b.pageNumber;
                  }
                  return a.createdTime > b.createdTime;
              });
}

QString AnnotationModel::generateUniqueId() const {
    return QString("ann_%1_%2")
        .arg(QDateTime::currentMSecsSinceEpoch())
        .arg(QRandomGenerator::global()->bounded(10000));
}

bool AnnotationModel::loadAnnotationsFromDocument() {
    if (!m_document) {
        return false;
    }

    beginResetModel();
    m_annotations.clear();

    int loadedCount = 0;
    for (int pageNum = 0; pageNum < m_document->numPages(); ++pageNum) {
        std::unique_ptr<Poppler::Page> page(m_document->page(pageNum));
        if (!page) {
            continue;
        }

        std::vector<std::unique_ptr<Poppler::Annotation>> popplerAnnotations =
            page->annotations();
        for (auto& popplerAnnot : popplerAnnotations) {
            try {
                PDFAnnotation annotation = PDFAnnotation::fromPopplerAnnotation(
                    popplerAnnot.get(), pageNum);
                if (!annotation.id.isEmpty()) {
                    m_annotations.append(annotation);
                    loadedCount++;
                }
            } catch (const std::exception& e) {
                qWarning() << "Failed to load annotation from page" << pageNum
                           << ":" << e.what();
            }
        }
    }

    sortAnnotations();
    endResetModel();

    emit annotationsLoaded(loadedCount);
    qDebug() << "Loaded" << loadedCount << "annotations from document";

    return true;
}

bool AnnotationModel::saveAnnotationsToDocument() {
    if (!m_document) {
        return false;
    }

    int savedCount = 0;

    // Group annotations by page for efficient processing
    QMap<int, QList<PDFAnnotation>> annotationsByPage;
    for (const PDFAnnotation& annotation : m_annotations) {
        annotationsByPage[annotation.pageNumber].append(annotation);
    }

    // Process each page
    for (auto it = annotationsByPage.begin(); it != annotationsByPage.end();
         ++it) {
        int pageNum = it.key();
        const QList<PDFAnnotation>& pageAnnotations = it.value();

        std::unique_ptr<Poppler::Page> page(m_document->page(pageNum));
        if (!page) {
            continue;
        }

        // Add new annotations to the page
        for (const PDFAnnotation& annotation : pageAnnotations) {
            try {
                Poppler::Annotation* popplerAnnot =
                    annotation.toPopplerAnnotation();
                if (popplerAnnot) {
                    page->addAnnotation(popplerAnnot);
                    savedCount++;
                }
            } catch (const std::exception& e) {
                qWarning() << "Failed to save annotation to page" << pageNum
                           << ":" << e.what();
            }
        }
    }

    emit annotationsSaved(savedCount);
    qDebug() << "Saved" << savedCount << "annotations to document";

    return savedCount > 0;
}

QList<PDFAnnotation> AnnotationModel::searchAnnotations(
    const QString& query) const {
    QList<PDFAnnotation> result;

    if (query.isEmpty()) {
        return result;
    }

    QString lowerQuery = query.trimmed().toLower();
    if (lowerQuery.isEmpty()) {
        return result;
    }

    for (const PDFAnnotation& annotation : m_annotations) {
        if (annotation.content.toLower().contains(lowerQuery) ||
            annotation.author.toLower().contains(lowerQuery) ||
            annotation.getTypeString().toLower().contains(lowerQuery) ||
            annotation.id.toLower().contains(lowerQuery)) {
            result.append(annotation);
        }
    }

    return result;
}

QList<PDFAnnotation> AnnotationModel::getAnnotationsByType(
    AnnotationType type) const {
    QList<PDFAnnotation> result;
    for (const PDFAnnotation& annotation : m_annotations) {
        if (annotation.type == type) {
            result.append(annotation);
        }
    }
    return result;
}

QList<PDFAnnotation> AnnotationModel::getAnnotationsByAuthor(
    const QString& author) const {
    QList<PDFAnnotation> result;
    for (const PDFAnnotation& annotation : m_annotations) {
        if (annotation.author == author) {
            result.append(annotation);
        }
    }
    return result;
}

QList<PDFAnnotation> AnnotationModel::getRecentAnnotations(int count) const {
    if (count <= 0) {
        return QList<PDFAnnotation>();
    }

    QList<PDFAnnotation> sorted = m_annotations;
    std::sort(sorted.begin(), sorted.end(),
              [](const PDFAnnotation& a, const PDFAnnotation& b) {
                  return a.modifiedTime > b.modifiedTime;
              });

    if (sorted.size() > count) {
        return sorted.mid(0, count);
    }

    return sorted;
}

QMap<AnnotationType, int> AnnotationModel::getAnnotationCountByType() const {
    QMap<AnnotationType, int> counts;

    for (const PDFAnnotation& annotation : m_annotations) {
        counts[annotation.type]++;
    }

    return counts;
}

QStringList AnnotationModel::getAuthors() const {
    QStringList authors;
    for (const PDFAnnotation& annotation : m_annotations) {
        if (!annotation.author.isEmpty() &&
            !authors.contains(annotation.author)) {
            authors.append(annotation.author);
        }
    }
    authors.sort();
    return authors;
}

// Poppler integration methods
Poppler::Annotation* PDFAnnotation::toPopplerAnnotation() const {
    if (pageNumber < 0) {
        return nullptr;
    }

    Poppler::Annotation* annotation = nullptr;

    try {
        // Create annotation based on type
        switch (type) {
            case AnnotationType::Highlight: {
                auto* highlightAnnot = new Poppler::HighlightAnnotation();
                highlightAnnot->setHighlightType(
                    Poppler::HighlightAnnotation::Highlight);
                annotation = highlightAnnot;
                break;
            }

            case AnnotationType::Note: {
                auto* textAnnot = new Poppler::TextAnnotation(
                    Poppler::TextAnnotation::InPlace);
                annotation = textAnnot;
                break;
            }

            case AnnotationType::FreeText: {
                auto* textAnnot = new Poppler::TextAnnotation(
                    Poppler::TextAnnotation::InPlace);
                annotation = textAnnot;
                break;
            }

            case AnnotationType::Underline: {
                auto* highlightAnnot = new Poppler::HighlightAnnotation();
                highlightAnnot->setHighlightType(
                    Poppler::HighlightAnnotation::Underline);
                annotation = highlightAnnot;
                break;
            }

            case AnnotationType::StrikeOut: {
                auto* highlightAnnot = new Poppler::HighlightAnnotation();
                highlightAnnot->setHighlightType(
                    Poppler::HighlightAnnotation::StrikeOut);
                annotation = highlightAnnot;
                break;
            }

            case AnnotationType::Squiggly: {
                auto* highlightAnnot = new Poppler::HighlightAnnotation();
                highlightAnnot->setHighlightType(
                    Poppler::HighlightAnnotation::Squiggly);
                annotation = highlightAnnot;
                break;
            }

            case AnnotationType::Rectangle: {
                auto* geomAnnot = new Poppler::GeomAnnotation();
                geomAnnot->setGeomType(
                    Poppler::GeomAnnotation::InscribedSquare);
                annotation = geomAnnot;
                break;
            }

            case AnnotationType::Circle: {
                auto* geomAnnot = new Poppler::GeomAnnotation();
                geomAnnot->setGeomType(
                    Poppler::GeomAnnotation::InscribedCircle);
                annotation = geomAnnot;
                break;
            }

            case AnnotationType::Line: {
                auto* lineAnnot = new Poppler::LineAnnotation(
                    Poppler::LineAnnotation::StraightLine);
                lineAnnot->setLinePoints({startPoint, endPoint});
                annotation = lineAnnot;
                break;
            }

            case AnnotationType::Arrow: {
                auto* lineAnnot = new Poppler::LineAnnotation(
                    Poppler::LineAnnotation::StraightLine);
                lineAnnot->setLinePoints({startPoint, endPoint});
                lineAnnot->setLineEndStyle(
                    Poppler::LineAnnotation::ClosedArrow);
                annotation = lineAnnot;
                break;
            }

            case AnnotationType::Ink: {
                auto* inkAnnot = new Poppler::InkAnnotation();
                QList<QList<QPointF>> paths;
                if (!inkPath.isEmpty()) {
                    paths.append(inkPath);
                    inkAnnot->setInkPaths(paths);
                }
                annotation = inkAnnot;
                break;
            }

            default:
                qWarning() << "Unsupported annotation type for conversion:"
                           << static_cast<int>(type);
                return nullptr;
        }

        if (annotation) {
            // Set common properties
            annotation->setBoundary(boundingRect);
            annotation->setContents(content);
            annotation->setAuthor(author);
            annotation->setCreationDate(createdTime);
            annotation->setModificationDate(modifiedTime);

            // Set style properties
            Poppler::Annotation::Style style;
            style.setColor(color);
            style.setOpacity(opacity);
            style.setWidth(lineWidth);
            annotation->setStyle(style);

            // Set flags
            Poppler::Annotation::Flags flags = Poppler::Annotation::Flag(0);
            if (!isVisible) {
                flags |= Poppler::Annotation::Hidden;
            }
            annotation->setFlags(flags);

            // Set unique name
            if (!id.isEmpty()) {
                annotation->setUniqueName(id);
            }
        }

    } catch (const std::exception& e) {
        qWarning() << "Failed to create Poppler annotation:" << e.what();
        if (annotation) {
            delete annotation;
            annotation = nullptr;
        }
    }

    return annotation;
}

PDFAnnotation PDFAnnotation::fromPopplerAnnotation(
    Poppler::Annotation* annotation, int pageNum) {
    PDFAnnotation result;

    if (!annotation) {
        return result;
    }

    try {
        // Basic properties
        result.pageNumber = pageNum;
        result.boundingRect = annotation->boundary();
        result.content = annotation->contents();
        result.author = annotation->author();
        result.createdTime = annotation->creationDate();
        result.modifiedTime = annotation->modificationDate();

        // Extract style properties
        Poppler::Annotation::Style style = annotation->style();
        result.color = style.color();
        result.opacity = style.opacity();
        result.lineWidth = style.width();

        // Extract flags
        Poppler::Annotation::Flags flags = annotation->flags();
        result.isVisible = !(flags & Poppler::Annotation::Hidden);

        // Get unique name if available
        QString uniqueName = annotation->uniqueName();
        if (!uniqueName.isEmpty()) {
            result.id = uniqueName;
        }

        // Convert Poppler annotation type to our enum and extract type-specific
        // data
        switch (annotation->subType()) {
            case Poppler::Annotation::AHighlight: {
                auto* highlightAnnot =
                    static_cast<Poppler::HighlightAnnotation*>(annotation);
                switch (highlightAnnot->highlightType()) {
                    case Poppler::HighlightAnnotation::Highlight:
                        result.type = AnnotationType::Highlight;
                        break;
                    case Poppler::HighlightAnnotation::Underline:
                        result.type = AnnotationType::Underline;
                        break;
                    case Poppler::HighlightAnnotation::StrikeOut:
                        result.type = AnnotationType::StrikeOut;
                        break;
                    case Poppler::HighlightAnnotation::Squiggly:
                        result.type = AnnotationType::Squiggly;
                        break;
                    default:
                        result.type = AnnotationType::Highlight;
                        break;
                }
                break;
            }

            case Poppler::Annotation::AText: {
                auto* textAnnot =
                    static_cast<Poppler::TextAnnotation*>(annotation);
                switch (textAnnot->textType()) {
                    case Poppler::TextAnnotation::InPlace:
                        result.type = AnnotationType::FreeText;
                        break;
                    case Poppler::TextAnnotation::Linked:
                    default:
                        result.type = AnnotationType::Note;
                        break;
                }

                // Extract font information if available
                result.fontFamily = "Arial";  // Default, as Poppler doesn't
                                              // expose font info directly
                result.fontSize = 12;
                break;
            }

            case Poppler::Annotation::ALine: {
                auto* lineAnnot =
                    static_cast<Poppler::LineAnnotation*>(annotation);
                QList<QPointF> linePoints = lineAnnot->linePoints();
                if (linePoints.size() >= 2) {
                    result.startPoint = linePoints.first();
                    result.endPoint = linePoints.last();
                }

                // Check if it's an arrow
                if (lineAnnot->lineStartStyle() !=
                        Poppler::LineAnnotation::None ||
                    lineAnnot->lineEndStyle() !=
                        Poppler::LineAnnotation::None) {
                    result.type = AnnotationType::Arrow;
                } else {
                    result.type = AnnotationType::Line;
                }
                break;
            }

            case Poppler::Annotation::AInk: {
                auto* inkAnnot =
                    static_cast<Poppler::InkAnnotation*>(annotation);
                QList<QList<QPointF>> inkPaths = inkAnnot->inkPaths();
                result.type = AnnotationType::Ink;

                // Combine all paths into a single path for simplicity
                result.inkPath.clear();
                for (const QList<QPointF>& path : inkPaths) {
                    result.inkPath.append(path);
                }
                break;
            }

            case Poppler::Annotation::AGeom: {
                auto* geomAnnot =
                    static_cast<Poppler::GeomAnnotation*>(annotation);
                switch (geomAnnot->geomType()) {
                    case Poppler::GeomAnnotation::InscribedSquare:
                        result.type = AnnotationType::Rectangle;
                        break;
                    case Poppler::GeomAnnotation::InscribedCircle:
                        result.type = AnnotationType::Circle;
                        break;
                    default:
                        result.type = AnnotationType::Rectangle;
                        break;
                }
                break;
            }

            case Poppler::Annotation::AStamp:
                result.type =
                    AnnotationType::Note;  // Map stamp to note for simplicity
                break;

            case Poppler::Annotation::ACaret:
                result.type = AnnotationType::Note;  // Map caret to note
                break;

            default:
                result.type = AnnotationType::Highlight;
                qWarning() << "Unknown annotation type:"
                           << static_cast<int>(annotation->subType());
                break;
        }

        // Generate unique ID if not already set
        if (result.id.isEmpty()) {
            result.id = QString("imported_%1_%2_%3")
                            .arg(pageNum)
                            .arg(QDateTime::currentMSecsSinceEpoch())
                            .arg(qHash(result.content + result.author));
        }

    } catch (const std::exception& e) {
        qWarning() << "Failed to convert Poppler annotation:" << e.what();
        // Return a default annotation in case of error
        result.type = AnnotationType::Highlight;
        result.pageNumber = pageNum;
        result.id = QString("error_%1_%2")
                        .arg(pageNum)
                        .arg(QDateTime::currentMSecsSinceEpoch());
    }

    return result;
}

bool AnnotationModel::editAnnotationContent(const QString& annotationId,
                                            const QString& newContent) {
    int index = findAnnotationIndex(annotationId);
    if (index == -1) {
        return false;
    }

    m_annotations[index].content = newContent;
    m_annotations[index].modifiedTime = QDateTime::currentDateTime();

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit annotationUpdated(m_annotations[index]);

    return true;
}

bool AnnotationModel::moveAnnotation(const QString& annotationId,
                                     const QPointF& newPosition) {
    int index = findAnnotationIndex(annotationId);
    if (index == -1) {
        return false;
    }

    QRectF currentBoundary = m_annotations[index].boundingRect;
    QSizeF size = currentBoundary.size();
    m_annotations[index].boundingRect = QRectF(newPosition, size);
    m_annotations[index].modifiedTime = QDateTime::currentDateTime();

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit annotationUpdated(m_annotations[index]);

    return true;
}

bool AnnotationModel::resizeAnnotation(const QString& annotationId,
                                       const QRectF& newBoundary) {
    int index = findAnnotationIndex(annotationId);
    if (index == -1) {
        return false;
    }

    m_annotations[index].boundingRect = newBoundary;
    m_annotations[index].modifiedTime = QDateTime::currentDateTime();

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit annotationUpdated(m_annotations[index]);

    return true;
}

bool AnnotationModel::changeAnnotationColor(const QString& annotationId,
                                            const QColor& newColor) {
    int index = findAnnotationIndex(annotationId);
    if (index == -1) {
        return false;
    }

    m_annotations[index].color = newColor;
    m_annotations[index].modifiedTime = QDateTime::currentDateTime();

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit annotationUpdated(m_annotations[index]);

    return true;
}

bool AnnotationModel::changeAnnotationOpacity(const QString& annotationId,
                                              qreal opacity) {
    int index = findAnnotationIndex(annotationId);
    if (index == -1) {
        return false;
    }

    m_annotations[index].opacity = qBound(0.0, opacity, 1.0);
    m_annotations[index].modifiedTime = QDateTime::currentDateTime();

    QModelIndex modelIndex = createIndex(index, 0);
    emit dataChanged(modelIndex, modelIndex);
    emit annotationUpdated(m_annotations[index]);

    return true;
}

bool AnnotationModel::addStickyNote(int pageNumber, const QPointF& position,
                                    const QString& content,
                                    const QColor& color) {
    PDFAnnotation stickyNote;
    stickyNote.type = AnnotationType::Note;
    stickyNote.pageNumber = pageNumber;
    stickyNote.boundingRect =
        QRectF(position, QSizeF(24, 24));  // Standard sticky note size
    stickyNote.content = content;
    stickyNote.color = color;
    stickyNote.author = "User";  // Should get from settings
    stickyNote.createdTime = QDateTime::currentDateTime();
    stickyNote.modifiedTime = stickyNote.createdTime;
    stickyNote.isVisible = true;
    stickyNote.opacity = 1.0;

    return addAnnotation(stickyNote);
}

QList<PDFAnnotation> AnnotationModel::getStickyNotesForPage(
    int pageNumber) const {
    QList<PDFAnnotation> result;
    for (const PDFAnnotation& annotation : m_annotations) {
        if (annotation.pageNumber == pageNumber &&
            annotation.type == AnnotationType::Note) {
            result.append(annotation);
        }
    }
    return result;
}
