#include "HighlightModel.h"
#include <QFile>
#include <QJsonDocument>
#include <algorithm>

// TextHighlight implementation
QJsonObject TextHighlight::toJson() const {
    QJsonObject obj;
    obj["id"] = id;
    obj["pageNumber"] = pageNumber;
    obj["text"] = text;
    obj["color"] = color.name(QColor::HexArgb);
    obj["opacity"] = opacity;
    obj["note"] = note;
    obj["author"] = author;
    obj["createdTime"] = createdTime.toString(Qt::ISODate);
    obj["modifiedTime"] = modifiedTime.toString(Qt::ISODate);
    obj["isVisible"] = isVisible;
    obj["colorPreset"] = static_cast<int>(colorPreset);
    obj["startCharIndex"] = startCharIndex;
    obj["endCharIndex"] = endCharIndex;
    obj["startPoint"] =
        QJsonObject{{"x", startPoint.x()}, {"y", startPoint.y()}};
    obj["endPoint"] = QJsonObject{{"x", endPoint.x()}, {"y", endPoint.y()}};

    QJsonArray rectsArray;
    for (const auto& rect : rects) {
        QJsonObject rectObj;
        rectObj["x"] = rect.x();
        rectObj["y"] = rect.y();
        rectObj["width"] = rect.width();
        rectObj["height"] = rect.height();
        rectsArray.append(rectObj);
    }
    obj["rects"] = rectsArray;

    return obj;
}

TextHighlight TextHighlight::fromJson(const QJsonObject& json) {
    TextHighlight highlight;
    highlight.id = json["id"].toString();
    highlight.pageNumber = json["pageNumber"].toInt();
    highlight.text = json["text"].toString();
    highlight.color = QColor(json["color"].toString());
    highlight.opacity = json["opacity"].toDouble();
    highlight.note = json["note"].toString();
    highlight.author = json["author"].toString();
    highlight.createdTime =
        QDateTime::fromString(json["createdTime"].toString(), Qt::ISODate);
    highlight.modifiedTime =
        QDateTime::fromString(json["modifiedTime"].toString(), Qt::ISODate);
    highlight.isVisible = json["isVisible"].toBool();
    highlight.colorPreset =
        static_cast<HighlightColor>(json["colorPreset"].toInt());
    highlight.startCharIndex = json["startCharIndex"].toInt();
    highlight.endCharIndex = json["endCharIndex"].toInt();

    QJsonObject startPt = json["startPoint"].toObject();
    highlight.startPoint =
        QPointF(startPt["x"].toDouble(), startPt["y"].toDouble());
    QJsonObject endPt = json["endPoint"].toObject();
    highlight.endPoint = QPointF(endPt["x"].toDouble(), endPt["y"].toDouble());

    QJsonArray rectsArray = json["rects"].toArray();
    for (const auto& rectValue : rectsArray) {
        QJsonObject rectObj = rectValue.toObject();
        QRectF rect(rectObj["x"].toDouble(), rectObj["y"].toDouble(),
                    rectObj["width"].toDouble(), rectObj["height"].toDouble());
        highlight.rects.append(rect);
    }

    return highlight;
}

bool TextHighlight::containsPoint(const QPointF& point) const {
    for (const auto& rect : rects) {
        if (rect.contains(point)) {
            return true;
        }
    }
    return false;
}

QRectF TextHighlight::boundingRect() const {
    if (rects.isEmpty())
        return QRectF();

    QRectF bounds = rects.first();
    for (int i = 1; i < rects.size(); ++i) {
        bounds = bounds.united(rects[i]);
    }
    return bounds;
}

QString TextHighlight::getColorName() const {
    return HighlightColorManager::getColorName(colorPreset);
}

QColor TextHighlight::getColorFromPreset(HighlightColor preset) {
    return HighlightColorManager::getDefaultColor(preset);
}

HighlightColor TextHighlight::getPresetFromColor(const QColor& color) {
    // Simple color matching
    if (color == Qt::yellow)
        return HighlightColor::Yellow;
    if (color == Qt::green)
        return HighlightColor::Green;
    if (color == Qt::blue)
        return HighlightColor::Blue;
    if (color == QColor(255, 192, 203))
        return HighlightColor::Pink;
    if (color == QColor(255, 165, 0))
        return HighlightColor::Orange;
    if (color == QColor(128, 0, 128))
        return HighlightColor::Purple;
    if (color == Qt::red)
        return HighlightColor::Red;
    return HighlightColor::Custom;
}

// HighlightModel implementation
HighlightModel::HighlightModel(QObject* parent) : QAbstractListModel(parent) {}

int HighlightModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid())
        return 0;
    return m_highlights.size();
}

QVariant HighlightModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || index.row() >= m_highlights.size()) {
        return QVariant();
    }

    const TextHighlight& highlight = m_highlights[index.row()];

    switch (role) {
        case IdRole:
            return highlight.id;
        case PageNumberRole:
            return highlight.pageNumber;
        case RectsRole: {
            QVariantList rectList;
            for (const auto& rect : highlight.rects) {
                rectList.append(QVariant::fromValue(rect));
            }
            return rectList;
        }
        case TextRole:
            return highlight.text;
        case ColorRole:
            return highlight.color;
        case OpacityRole:
            return highlight.opacity;
        case NoteRole:
            return highlight.note;
        case AuthorRole:
            return highlight.author;
        case CreatedTimeRole:
            return highlight.createdTime;
        case ModifiedTimeRole:
            return highlight.modifiedTime;
        case VisibilityRole:
            return highlight.isVisible;
        case ColorPresetRole:
            return static_cast<int>(highlight.colorPreset);
        case Qt::DisplayRole:
            return QString("Page %1: %2")
                .arg(highlight.pageNumber + 1)
                .arg(highlight.text.left(50));
        default:
            return QVariant();
    }
}

bool HighlightModel::setData(const QModelIndex& index, const QVariant& value,
                             int role) {
    if (!index.isValid() || index.row() >= m_highlights.size()) {
        return false;
    }

    TextHighlight& highlight = m_highlights[index.row()];
    bool changed = false;

    switch (role) {
        case TextRole:
            highlight.text = value.toString();
            changed = true;
            break;
        case ColorRole:
            highlight.color = value.value<QColor>();
            changed = true;
            break;
        case OpacityRole:
            highlight.opacity = value.toDouble();
            changed = true;
            break;
        case NoteRole:
            highlight.note = value.toString();
            changed = true;
            break;
        case VisibilityRole:
            highlight.isVisible = value.toBool();
            changed = true;
            break;
        default:
            return false;
    }

    if (changed) {
        highlight.modifiedTime = QDateTime::currentDateTime();
        emit dataChanged(index, index, {role});
        emit highlightUpdated(highlight);
        return true;
    }

    return false;
}

Qt::ItemFlags HighlightModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> HighlightModel::roleNames() const {
    QHash<int, QByteArray> roles;
    roles[IdRole] = "id";
    roles[PageNumberRole] = "pageNumber";
    roles[RectsRole] = "rects";
    roles[TextRole] = "text";
    roles[ColorRole] = "color";
    roles[OpacityRole] = "opacity";
    roles[NoteRole] = "note";
    roles[AuthorRole] = "author";
    roles[CreatedTimeRole] = "createdTime";
    roles[ModifiedTimeRole] = "modifiedTime";
    roles[VisibilityRole] = "isVisible";
    roles[ColorPresetRole] = "colorPreset";
    return roles;
}

bool HighlightModel::addHighlight(const TextHighlight& highlight) {
    if (highlight.isEmpty()) {
        return false;
    }

    beginInsertRows(QModelIndex(), m_highlights.size(), m_highlights.size());
    m_highlights.append(highlight);
    endInsertRows();

    emit highlightAdded(highlight);
    return true;
}

bool HighlightModel::removeHighlight(const QString& highlightId) {
    int index = findHighlightIndex(highlightId);
    if (index == -1) {
        return false;
    }

    beginRemoveRows(QModelIndex(), index, index);
    m_highlights.removeAt(index);
    endRemoveRows();

    emit highlightRemoved(highlightId);
    return true;
}

bool HighlightModel::updateHighlight(const QString& highlightId,
                                     const TextHighlight& updatedHighlight) {
    int index = findHighlightIndex(highlightId);
    if (index == -1) {
        return false;
    }

    m_highlights[index] = updatedHighlight;
    m_highlights[index].modifiedTime = QDateTime::currentDateTime();

    QModelIndex modelIndex = this->index(index);
    emit dataChanged(modelIndex, modelIndex);
    emit highlightUpdated(updatedHighlight);
    return true;
}

TextHighlight HighlightModel::getHighlight(const QString& highlightId) const {
    int index = findHighlightIndex(highlightId);
    if (index != -1) {
        return m_highlights[index];
    }
    return TextHighlight();
}

QList<TextHighlight> HighlightModel::getHighlightsForPage(
    int pageNumber) const {
    QList<TextHighlight> pageHighlights;
    for (const auto& highlight : m_highlights) {
        if (highlight.pageNumber == pageNumber) {
            pageHighlights.append(highlight);
        }
    }
    return pageHighlights;
}

bool HighlightModel::removeHighlightsForPage(int pageNumber) {
    bool removed = false;
    for (int i = m_highlights.size() - 1; i >= 0; --i) {
        if (m_highlights[i].pageNumber == pageNumber) {
            beginRemoveRows(QModelIndex(), i, i);
            QString id = m_highlights[i].id;
            m_highlights.removeAt(i);
            endRemoveRows();
            emit highlightRemoved(id);
            removed = true;
        }
    }
    return removed;
}

int HighlightModel::getHighlightCountForPage(int pageNumber) const {
    return getHighlightsForPage(pageNumber).size();
}

QList<TextHighlight> HighlightModel::searchHighlights(
    const QString& query) const {
    QList<TextHighlight> results;
    QString lowerQuery = query.toLower();

    for (const auto& highlight : m_highlights) {
        if (highlight.text.toLower().contains(lowerQuery) ||
            highlight.note.toLower().contains(lowerQuery)) {
            results.append(highlight);
        }
    }
    return results;
}

QList<TextHighlight> HighlightModel::getHighlightsByColor(
    HighlightColor color) const {
    QList<TextHighlight> results;
    for (const auto& highlight : m_highlights) {
        if (highlight.colorPreset == color) {
            results.append(highlight);
        }
    }
    return results;
}

QList<TextHighlight> HighlightModel::getHighlightsByAuthor(
    const QString& author) const {
    QList<TextHighlight> results;
    for (const auto& highlight : m_highlights) {
        if (highlight.author == author) {
            results.append(highlight);
        }
    }
    return results;
}

QList<TextHighlight> HighlightModel::getRecentHighlights(int count) const {
    QList<TextHighlight> sorted = m_highlights;
    std::sort(sorted.begin(), sorted.end(),
              [](const TextHighlight& a, const TextHighlight& b) {
                  return a.createdTime > b.createdTime;
              });
    return sorted.mid(0, count);
}

QList<TextHighlight> HighlightModel::getHighlightsWithNotes() const {
    QList<TextHighlight> results;
    for (const auto& highlight : m_highlights) {
        if (!highlight.note.isEmpty()) {
            results.append(highlight);
        }
    }
    return results;
}

bool HighlightModel::editHighlightNote(const QString& highlightId,
                                       const QString& newNote) {
    int index = findHighlightIndex(highlightId);
    if (index == -1)
        return false;

    return setData(this->index(index), newNote, NoteRole);
}

bool HighlightModel::changeHighlightColor(const QString& highlightId,
                                          const QColor& newColor) {
    int index = findHighlightIndex(highlightId);
    if (index == -1)
        return false;

    return setData(this->index(index), newColor, ColorRole);
}

bool HighlightModel::changeHighlightOpacity(const QString& highlightId,
                                            double opacity) {
    int index = findHighlightIndex(highlightId);
    if (index == -1)
        return false;

    return setData(this->index(index), opacity, OpacityRole);
}

bool HighlightModel::toggleHighlightVisibility(const QString& highlightId) {
    int index = findHighlightIndex(highlightId);
    if (index == -1)
        return false;

    bool newVisibility = !m_highlights[index].isVisible;
    bool result = setData(this->index(index), newVisibility, VisibilityRole);
    if (result) {
        emit highlightVisibilityChanged(highlightId, newVisibility);
    }
    return result;
}

bool HighlightModel::mergeHighlights(const QStringList& highlightIds) {
    // TODO: Implement merge logic
    return false;
}

bool HighlightModel::splitHighlight(const QString& highlightId,
                                    int splitCharIndex) {
    // TODO: Implement split logic
    return false;
}

bool HighlightModel::removeAllHighlights() {
    if (m_highlights.isEmpty())
        return false;

    beginResetModel();
    m_highlights.clear();
    endResetModel();

    emit highlightsCleared();
    return true;
}

bool HighlightModel::saveToFile(const QString& filePath) const {
    QJsonObject root = toJson();
    QJsonDocument doc(root);

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        return false;
    }

    file.write(doc.toJson(QJsonDocument::Indented));
    return true;
}

bool HighlightModel::loadFromFile(const QString& filePath) {
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    QByteArray data = file.readAll();
    QJsonDocument doc = QJsonDocument::fromJson(data);

    if (doc.isNull() || !doc.isObject()) {
        return false;
    }

    return fromJson(doc.object());
}

QJsonObject HighlightModel::toJson() const {
    QJsonObject root;
    root["version"] = "1.0";
    root["documentPath"] = m_documentPath;
    root["totalHighlights"] = m_highlights.size();

    QJsonArray highlightsArray;
    for (const auto& highlight : m_highlights) {
        highlightsArray.append(highlight.toJson());
    }
    root["highlights"] = highlightsArray;

    return root;
}

bool HighlightModel::fromJson(const QJsonObject& json) {
    beginResetModel();
    m_highlights.clear();

    m_documentPath = json["documentPath"].toString();
    QJsonArray highlightsArray = json["highlights"].toArray();

    for (const auto& value : highlightsArray) {
        TextHighlight highlight = TextHighlight::fromJson(value.toObject());
        m_highlights.append(highlight);
    }

    endResetModel();
    emit highlightsLoaded(m_highlights.size());
    return true;
}

QMap<HighlightColor, int> HighlightModel::getHighlightCountByColor() const {
    QMap<HighlightColor, int> counts;
    for (const auto& highlight : m_highlights) {
        counts[highlight.colorPreset]++;
    }
    return counts;
}

QStringList HighlightModel::getAuthors() const {
    QSet<QString> authors;
    for (const auto& highlight : m_highlights) {
        if (!highlight.author.isEmpty()) {
            authors.insert(highlight.author);
        }
    }
    return authors.values();
}

int HighlightModel::getTotalPages() const {
    QSet<int> pages;
    for (const auto& highlight : m_highlights) {
        pages.insert(highlight.pageNumber);
    }
    return pages.size();
}

double HighlightModel::getAverageHighlightsPerPage() const {
    int totalPages = getTotalPages();
    if (totalPages == 0)
        return 0.0;
    return static_cast<double>(m_highlights.size()) / totalPages;
}

TextHighlight HighlightModel::findHighlightAtPoint(int pageNumber,
                                                   const QPointF& point) const {
    for (const auto& highlight : m_highlights) {
        if (highlight.pageNumber == pageNumber &&
            highlight.containsPoint(point)) {
            return highlight;
        }
    }
    return TextHighlight();
}

QList<TextHighlight> HighlightModel::findHighlightsInRect(
    int pageNumber, const QRectF& rect) const {
    QList<TextHighlight> results;
    for (const auto& highlight : m_highlights) {
        if (highlight.pageNumber == pageNumber) {
            for (const auto& hRect : highlight.rects) {
                if (rect.intersects(hRect)) {
                    results.append(highlight);
                    break;
                }
            }
        }
    }
    return results;
}

QString HighlightModel::exportToMarkdown() const {
    QString markdown;
    markdown += QString("# Highlights for %1\n\n").arg(m_documentPath);

    QMap<int, QList<TextHighlight>> byPage;
    for (const auto& highlight : m_highlights) {
        byPage[highlight.pageNumber].append(highlight);
    }

    for (auto it = byPage.constBegin(); it != byPage.constEnd(); ++it) {
        markdown += QString("## Page %1\n\n").arg(it.key() + 1);
        for (const auto& highlight : it.value()) {
            markdown += QString("- **%1** (%2)\n")
                            .arg(highlight.text)
                            .arg(highlight.getColorName());
            if (!highlight.note.isEmpty()) {
                markdown += QString("  > %1\n").arg(highlight.note);
            }
            markdown += "\n";
        }
    }

    return markdown;
}

QString HighlightModel::exportToPlainText() const {
    QString text;
    text += QString("Highlights for: %1\n").arg(m_documentPath);
    text += QString("Total: %1 highlights\n\n").arg(m_highlights.size());

    for (const auto& highlight : m_highlights) {
        text += QString("[Page %1] %2\n")
                    .arg(highlight.pageNumber + 1)
                    .arg(highlight.text);
        if (!highlight.note.isEmpty()) {
            text += QString("Note: %1\n").arg(highlight.note);
        }
        text += "\n";
    }

    return text;
}

QJsonArray HighlightModel::exportToJson() const {
    QJsonArray array;
    for (const auto& highlight : m_highlights) {
        array.append(highlight.toJson());
    }
    return array;
}

int HighlightModel::findHighlightIndex(const QString& highlightId) const {
    for (int i = 0; i < m_highlights.size(); ++i) {
        if (m_highlights[i].id == highlightId) {
            return i;
        }
    }
    return -1;
}

void HighlightModel::sortHighlights() {
    std::sort(m_highlights.begin(), m_highlights.end(),
              [](const TextHighlight& a, const TextHighlight& b) {
                  if (a.pageNumber != b.pageNumber) {
                      return a.pageNumber < b.pageNumber;
                  }
                  return a.startCharIndex < b.startCharIndex;
              });
}

QString HighlightModel::generateUniqueId() const {
    return QUuid::createUuid().toString(QUuid::WithoutBraces);
}

// HighlightColorManager implementation
QColor HighlightColorManager::getDefaultColor(HighlightColor preset) {
    switch (preset) {
        case HighlightColor::Yellow:
            return QColor(255, 255, 0, 102);
        case HighlightColor::Green:
            return QColor(0, 255, 0, 102);
        case HighlightColor::Blue:
            return QColor(0, 191, 255, 102);
        case HighlightColor::Pink:
            return QColor(255, 192, 203, 102);
        case HighlightColor::Orange:
            return QColor(255, 165, 0, 102);
        case HighlightColor::Purple:
            return QColor(147, 112, 219, 102);
        case HighlightColor::Red:
            return QColor(255, 0, 0, 102);
        case HighlightColor::Custom:
            return QColor(255, 255, 0, 102);
    }
    return QColor(255, 255, 0, 102);
}

QString HighlightColorManager::getColorName(HighlightColor preset) {
    switch (preset) {
        case HighlightColor::Yellow:
            return "Yellow";
        case HighlightColor::Green:
            return "Green";
        case HighlightColor::Blue:
            return "Blue";
        case HighlightColor::Pink:
            return "Pink";
        case HighlightColor::Orange:
            return "Orange";
        case HighlightColor::Purple:
            return "Purple";
        case HighlightColor::Red:
            return "Red";
        case HighlightColor::Custom:
            return "Custom";
    }
    return "Yellow";
}

QList<HighlightColor> HighlightColorManager::getAllPresets() {
    return {HighlightColor::Yellow, HighlightColor::Green,
            HighlightColor::Blue,   HighlightColor::Pink,
            HighlightColor::Orange, HighlightColor::Purple,
            HighlightColor::Red};
}

HighlightColor HighlightColorManager::getPresetFromName(const QString& name) {
    if (name == "Yellow")
        return HighlightColor::Yellow;
    if (name == "Green")
        return HighlightColor::Green;
    if (name == "Blue")
        return HighlightColor::Blue;
    if (name == "Pink")
        return HighlightColor::Pink;
    if (name == "Orange")
        return HighlightColor::Orange;
    if (name == "Purple")
        return HighlightColor::Purple;
    if (name == "Red")
        return HighlightColor::Red;
    return HighlightColor::Custom;
}

// HighlightStatistics implementation
QJsonObject HighlightStatistics::toJson() const {
    QJsonObject obj;
    obj["totalHighlights"] = totalHighlights;
    obj["totalPages"] = totalPages;
    obj["highlightsWithNotes"] = highlightsWithNotes;
    obj["averageHighlightLength"] = averageHighlightLength;
    obj["averageHighlightsPerPage"] = averageHighlightsPerPage;
    obj["oldestHighlight"] = oldestHighlight.toString(Qt::ISODate);
    obj["newestHighlight"] = newestHighlight.toString(Qt::ISODate);
    return obj;
}

HighlightStatistics HighlightStatistics::fromHighlights(
    const QList<TextHighlight>& highlights) {
    HighlightStatistics stats;
    stats.totalHighlights = highlights.size();

    if (highlights.isEmpty()) {
        return stats;
    }

    QSet<int> pages;
    int totalLength = 0;

    for (const auto& highlight : highlights) {
        pages.insert(highlight.pageNumber);
        totalLength += highlight.text.length();
        if (!highlight.note.isEmpty()) {
            stats.highlightsWithNotes++;
        }

        stats.colorDistribution[highlight.colorPreset]++;
        stats.pageDistribution[highlight.pageNumber]++;
        if (!highlight.author.isEmpty()) {
            stats.authorDistribution[highlight.author]++;
        }

        if (stats.oldestHighlight.isNull() ||
            highlight.createdTime < stats.oldestHighlight) {
            stats.oldestHighlight = highlight.createdTime;
        }
        if (stats.newestHighlight.isNull() ||
            highlight.createdTime > stats.newestHighlight) {
            stats.newestHighlight = highlight.createdTime;
        }
    }

    stats.totalPages = pages.size();
    stats.averageHighlightLength =
        static_cast<double>(totalLength) / highlights.size();
    stats.averageHighlightsPerPage =
        static_cast<double>(highlights.size()) / stats.totalPages;

    return stats;
}
