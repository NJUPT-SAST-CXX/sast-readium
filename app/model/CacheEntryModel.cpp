#include "CacheEntryModel.h"
#include <QPixmap>

CacheEntryModel::CacheEntryModel(QString key, QVariant data, CacheType type)
    : m_key(std::move(key)),
      m_data(std::move(data)),
      m_type(type),
      m_timestamp(QDateTime::currentMSecsSinceEpoch()),
      m_lastAccessed(QDateTime::currentMSecsSinceEpoch()),
      m_accessCount(0),
      m_memorySize(0),
      m_priority(1) {
    m_memorySize = calculateDataSize();
}

bool CacheEntryModel::isExpired(qint64 maxAge) const {
    if (maxAge <= 0) {
        return false;
    }
    return getAge() > maxAge;
}

void CacheEntryModel::setData(const QVariant& data) {
    m_data = data;
    m_memorySize = calculateDataSize();
}

void CacheEntryModel::setPriority(int priority) { m_priority = priority; }

void CacheEntryModel::updateAccess() {
    m_lastAccessed = QDateTime::currentMSecsSinceEpoch();
    m_accessCount++;
}

void CacheEntryModel::resetAccessCount() { m_accessCount = 0; }

qint64 CacheEntryModel::getAge() const {
    return QDateTime::currentMSecsSinceEpoch() - m_timestamp;
}

double CacheEntryModel::calculateEvictionScore(double priorityWeight) const {
    // Lower score = higher eviction priority
    // Factors: age (older = higher score), access frequency (less = higher
    // score), priority (lower = higher score)
    double ageScore =
        static_cast<double>(getAge()) / 1000.0;  // Convert to seconds
    double accessScore = 1.0 / (static_cast<double>(m_accessCount) + 1.0);
    double priorityScore =
        1.0 / (static_cast<double>(m_priority) * priorityWeight);

    return ageScore + accessScore + priorityScore;
}

qint64 CacheEntryModel::calculateDataSize() const {
    qint64 baseSize = sizeof(CacheEntryModel);

    // Add key size
    baseSize += m_key.size() * sizeof(QChar);

    // Add data size based on type
    if (m_data.canConvert<QPixmap>()) {
        QPixmap pixmap = m_data.value<QPixmap>();
        baseSize += pixmap.width() * pixmap.height() * 4;  // 32-bit ARGB
    } else if (m_data.canConvert<QString>()) {
        baseSize += m_data.toString().size() * sizeof(QChar);
    } else if (m_data.canConvert<QByteArray>()) {
        baseSize += m_data.toByteArray().size();
    } else {
        // Conservative estimate for other types
        baseSize += 1024;
    }

    return baseSize;
}
