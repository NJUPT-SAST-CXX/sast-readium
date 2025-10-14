#include "CacheTestHelpers.h"

// MockCacheComponent implementation
// This file provides the implementation for the MockCacheComponent class
// to ensure proper vtable generation and Q_OBJECT macro processing

MockCacheComponent::MockCacheComponent(QObject* parent)
    : QObject(parent),
      m_memoryUsage(0),
      m_maxMemoryLimit(1024 * 1024),
      m_entryCount(0),
      m_hitCount(0),
      m_missCount(0),
      m_enabled(true) {}

MockCacheComponent::~MockCacheComponent() {}

qint64 MockCacheComponent::getMemoryUsage() const { return m_memoryUsage; }

qint64 MockCacheComponent::getMaxMemoryLimit() const {
    return m_maxMemoryLimit;
}

void MockCacheComponent::setMaxMemoryLimit(qint64 limit) {
    m_maxMemoryLimit = limit;
}

void MockCacheComponent::clear() {
    m_memoryUsage = 0;
    m_entryCount = 0;
}

int MockCacheComponent::getEntryCount() const { return m_entryCount; }

void MockCacheComponent::evictLRU(qint64 bytesToFree) {
    m_memoryUsage = qMax(qint64(0), m_memoryUsage - bytesToFree);
    m_entryCount = qMax(0, m_entryCount - 1);
}

qint64 MockCacheComponent::getHitCount() const { return m_hitCount; }

qint64 MockCacheComponent::getMissCount() const { return m_missCount; }

void MockCacheComponent::resetStatistics() {
    m_hitCount = 0;
    m_missCount = 0;
}

void MockCacheComponent::setEnabled(bool enabled) { m_enabled = enabled; }

bool MockCacheComponent::isEnabled() const { return m_enabled; }

void MockCacheComponent::setMemoryUsage(qint64 usage) { m_memoryUsage = usage; }

void MockCacheComponent::setEntryCount(int count) { m_entryCount = count; }

void MockCacheComponent::incrementHits() { m_hitCount++; }

void MockCacheComponent::incrementMisses() { m_missCount++; }
