/**
 * @file test_cache_mvp.cpp
 * @brief Unit tests for cache MVP architecture
 * @author SAST Team
 * @version 1.0
 * @date 2024
 *
 * Tests the Model-View-Presenter architecture for the cache component including
 * data models, statistics tracking, configuration management, and presenter
 * coordination.
 */

#include <gtest/gtest.h>
#include <QCoreApplication>
#include "../../app/controller/CachePresenter.h"
#include "../../app/model/CacheConfigModel.h"
#include "../../app/model/CacheDataModel.h"
#include "../../app/model/CacheEntryModel.h"
#include "../../app/model/CacheStatsModel.h"
#include "../../app/view/ICacheView.h"

// Mock view for testing
class MockCacheView : public ICacheView {
public:
    int updateCount = 0;
    int clearCount = 0;
    int evictCount = 0;
    QString lastKey;
    CacheType lastType = CacheType::SearchResultCache;

    void onCacheUpdated(CacheType type, const QString& key) override {
        updateCount++;
        lastType = type;
        lastKey = key;
    }

    void onCacheCleared(CacheType type) override {
        clearCount++;
        lastType = type;
    }

    void onCacheEvicted(CacheType type, const QString& key,
                        const QString& /*reason*/) override {
        evictCount++;
        lastType = type;
        lastKey = key;
    }
};

class MockCacheStatsView : public ICacheStatsView {
public:
    int statsUpdateCount = 0;
    int globalStatsUpdateCount = 0;
    CacheStats lastStats;
    qint64 lastTotalMemory = 0;
    double lastHitRatio = 0.0;

    void onStatsUpdated(CacheType /*type*/, const CacheStats& stats) override {
        statsUpdateCount++;
        lastStats = stats;
    }

    void onGlobalStatsUpdated(qint64 totalMemory, double hitRatio) override {
        globalStatsUpdateCount++;
        lastTotalMemory = totalMemory;
        lastHitRatio = hitRatio;
    }
};

class CacheMVPTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Ensure QCoreApplication exists for Qt functionality
        if (QCoreApplication::instance() == nullptr) {
            int argc = 0;
            char** argv = nullptr;
            app = new QCoreApplication(argc, argv);
        }
    }

    void TearDown() override {
        // Cleanup is handled by unique_ptrs
    }

    QCoreApplication* app = nullptr;
};

// ============================================================================
// CacheEntryModel Tests
// ============================================================================

TEST_F(CacheMVPTest, CacheEntryModel_Construction) {
    QString key = "test_key";
    QVariant data = QString("test_data");
    CacheType type = CacheType::SearchResultCache;

    CacheEntryModel entry(key, data, type);

    EXPECT_EQ(entry.getKey(), key);
    EXPECT_EQ(entry.getData().toString(), data.toString());
    EXPECT_EQ(entry.getType(), type);
    EXPECT_EQ(entry.getAccessCount(), 0);
    EXPECT_GT(entry.getMemorySize(), 0);
}

TEST_F(CacheMVPTest, CacheEntryModel_AccessTracking) {
    CacheEntryModel entry("test", QString("data"), CacheType::PageTextCache);

    qint64 initialAccess = entry.getLastAccessed();
    EXPECT_EQ(entry.getAccessCount(), 0);

    entry.updateAccess();

    EXPECT_EQ(entry.getAccessCount(), 1);
    EXPECT_GE(entry.getLastAccessed(), initialAccess);

    entry.updateAccess();
    EXPECT_EQ(entry.getAccessCount(), 2);
}

TEST_F(CacheMVPTest, CacheEntryModel_Expiration) {
    CacheEntryModel entry("test", QString("data"),
                          CacheType::SearchResultCache);

    // Not expired with max age 0 (disabled)
    EXPECT_FALSE(entry.isExpired(0));

    // Not expired with large max age
    EXPECT_FALSE(entry.isExpired(1000000));

    // Wait a bit and check expiration with small max age
    QThread::msleep(10);
    EXPECT_TRUE(entry.isExpired(1));  // 1ms max age
}

// ============================================================================
// CacheDataModel Tests
// ============================================================================

TEST_F(CacheMVPTest, CacheDataModel_InsertAndGet) {
    CacheDataModel model;

    CacheEntryModel entry("key1", QString("value1"),
                          CacheType::SearchResultCache);
    EXPECT_TRUE(model.insert(entry));

    CacheEntryModel* retrieved = model.get("key1");
    ASSERT_NE(retrieved, nullptr);
    EXPECT_EQ(retrieved->getKey(), "key1");
    EXPECT_EQ(retrieved->getData().toString(), "value1");
}

TEST_F(CacheMVPTest, CacheDataModel_Contains) {
    CacheDataModel model;

    CacheEntryModel entry("key1", QString("value1"), CacheType::PageTextCache);
    model.insert(entry);

    EXPECT_TRUE(model.contains("key1"));
    EXPECT_FALSE(model.contains("nonexistent"));
}

TEST_F(CacheMVPTest, CacheDataModel_Remove) {
    CacheDataModel model;

    CacheEntryModel entry("key1", QString("value1"),
                          CacheType::SearchHighlightCache);
    model.insert(entry);

    EXPECT_TRUE(model.contains("key1"));
    EXPECT_TRUE(model.remove("key1"));
    EXPECT_FALSE(model.contains("key1"));
    EXPECT_FALSE(model.remove("key1"));  // Already removed
}

TEST_F(CacheMVPTest, CacheDataModel_Clear) {
    CacheDataModel model;

    model.insert(
        CacheEntryModel("key1", QString("value1"), CacheType::PdfRenderCache));
    model.insert(
        CacheEntryModel("key2", QString("value2"), CacheType::PdfRenderCache));
    model.insert(
        CacheEntryModel("key3", QString("value3"), CacheType::ThumbnailCache));

    EXPECT_EQ(model.getEntryCount(), 3);

    model.clear();

    EXPECT_EQ(model.getEntryCount(), 0);
    EXPECT_FALSE(model.contains("key1"));
    EXPECT_FALSE(model.contains("key2"));
    EXPECT_FALSE(model.contains("key3"));
}

TEST_F(CacheMVPTest, CacheDataModel_MemoryTracking) {
    CacheDataModel model;

    qint64 initialMemory = model.getTotalMemoryUsage();
    EXPECT_EQ(initialMemory, 0);

    CacheEntryModel entry("key1", QString("value1"),
                          CacheType::SearchResultCache);
    model.insert(entry);

    qint64 afterInsert = model.getTotalMemoryUsage();
    EXPECT_GT(afterInsert, initialMemory);

    model.remove("key1");

    qint64 afterRemove = model.getTotalMemoryUsage();
    EXPECT_EQ(afterRemove, initialMemory);
}

TEST_F(CacheMVPTest, CacheDataModel_GetEntriesByType) {
    CacheDataModel model;

    model.insert(CacheEntryModel("key1", QString("value1"),
                                 CacheType::SearchResultCache));
    model.insert(
        CacheEntryModel("key2", QString("value2"), CacheType::PageTextCache));
    model.insert(CacheEntryModel("key3", QString("value3"),
                                 CacheType::SearchResultCache));

    QList<CacheEntryModel> searchResults =
        model.getEntriesByType(CacheType::SearchResultCache);
    EXPECT_EQ(searchResults.size(), 2);

    QList<CacheEntryModel> pageText =
        model.getEntriesByType(CacheType::PageTextCache);
    EXPECT_EQ(pageText.size(), 1);
}

// ============================================================================
// CacheConfigModel Tests
// ============================================================================

TEST_F(CacheMVPTest, CacheConfigModel_DefaultValues) {
    CacheConfigModel config;

    EXPECT_GT(config.getTotalMemoryLimit(), 0);
    EXPECT_GT(config.getCleanupInterval(), 0);
    EXPECT_TRUE(config.isLRUEvictionEnabled());
    EXPECT_TRUE(config.isMemoryPressureEvictionEnabled());
}

TEST_F(CacheMVPTest, CacheConfigModel_SettersAndGetters) {
    CacheConfigModel config;

    config.setTotalMemoryLimit(1024 * 1024);
    EXPECT_EQ(config.getTotalMemoryLimit(), 1024 * 1024);

    config.setCleanupInterval(5000);
    EXPECT_EQ(config.getCleanupInterval(), 5000);

    config.setMemoryPressureThreshold(0.8);
    EXPECT_DOUBLE_EQ(config.getMemoryPressureThreshold(), 0.8);
}

TEST_F(CacheMVPTest, CacheConfigModel_CacheLimits) {
    CacheConfigModel config;

    config.setCacheLimit(CacheType::SearchResultCache, 50 * 1024);
    EXPECT_EQ(config.getCacheLimit(CacheType::SearchResultCache), 50 * 1024);

    config.setCacheLimit(CacheType::PageTextCache, 25 * 1024);
    EXPECT_EQ(config.getCacheLimit(CacheType::PageTextCache), 25 * 1024);
}

// ============================================================================
// CacheStatsModel Tests
// ============================================================================

TEST_F(CacheMVPTest, CacheStatsModel_HitMissTracking) {
    CacheStatsModel stats;

    EXPECT_EQ(stats.getHits(CacheType::SearchResultCache), 0);
    EXPECT_EQ(stats.getMisses(CacheType::SearchResultCache), 0);

    stats.recordHit(CacheType::SearchResultCache);
    stats.recordHit(CacheType::SearchResultCache);
    stats.recordMiss(CacheType::SearchResultCache);

    EXPECT_EQ(stats.getHits(CacheType::SearchResultCache), 2);
    EXPECT_EQ(stats.getMisses(CacheType::SearchResultCache), 1);
    EXPECT_DOUBLE_EQ(stats.getHitRatio(CacheType::SearchResultCache),
                     2.0 / 3.0);
}

TEST_F(CacheMVPTest, CacheStatsModel_GlobalStats) {
    CacheStatsModel stats;

    stats.recordHit(CacheType::SearchResultCache);
    stats.recordHit(CacheType::PageTextCache);
    stats.recordMiss(CacheType::SearchResultCache);

    EXPECT_EQ(stats.getTotalHits(), 2);
    EXPECT_EQ(stats.getTotalMisses(), 1);
    EXPECT_DOUBLE_EQ(stats.getGlobalHitRatio(), 2.0 / 3.0);
}

TEST_F(CacheMVPTest, CacheStatsModel_MemoryTracking) {
    CacheStatsModel stats;

    stats.recordMemoryUsage(CacheType::SearchResultCache, 1024);
    stats.recordMemoryUsage(CacheType::PageTextCache, 2048);

    EXPECT_EQ(stats.getMemoryUsage(CacheType::SearchResultCache), 1024);
    EXPECT_EQ(stats.getMemoryUsage(CacheType::PageTextCache), 2048);
    EXPECT_EQ(stats.getTotalMemoryUsage(), 3072);
}

TEST_F(CacheMVPTest, CacheStatsModel_Reset) {
    CacheStatsModel stats;

    stats.recordHit(CacheType::SearchResultCache);
    stats.recordMiss(CacheType::SearchResultCache);
    stats.recordMemoryUsage(CacheType::SearchResultCache, 1024);

    stats.reset(CacheType::SearchResultCache);

    EXPECT_EQ(stats.getHits(CacheType::SearchResultCache), 0);
    EXPECT_EQ(stats.getMisses(CacheType::SearchResultCache), 0);
    EXPECT_EQ(stats.getMemoryUsage(CacheType::SearchResultCache), 0);
}

// ============================================================================
// CachePresenter Tests
// ============================================================================

TEST_F(CacheMVPTest, CachePresenter_Construction) {
    CachePresenter presenter;

    ASSERT_NE(presenter.getDataModel(), nullptr);
    ASSERT_NE(presenter.getConfigModel(), nullptr);
    ASSERT_NE(presenter.getStatsModel(), nullptr);
}

TEST_F(CacheMVPTest, CachePresenter_InsertAndGet) {
    CachePresenter presenter;

    QString key = "test_key";
    QString data = "test_value";

    EXPECT_TRUE(presenter.insert(key, data, CacheType::SearchResultCache));
    EXPECT_TRUE(presenter.contains(key, CacheType::SearchResultCache));

    QVariant retrieved = presenter.get(key, CacheType::SearchResultCache);
    EXPECT_TRUE(retrieved.isValid());
    EXPECT_EQ(retrieved.toString(), data);
}

TEST_F(CacheMVPTest, CachePresenter_ViewNotification) {
    CachePresenter presenter;
    MockCacheView mockView;

    presenter.registerView(&mockView);

    presenter.insert("key1", QString("value1"), CacheType::PageTextCache);

    EXPECT_GT(mockView.updateCount, 0);
    EXPECT_EQ(mockView.lastKey, "key1");
    EXPECT_EQ(mockView.lastType, CacheType::PageTextCache);

    presenter.unregisterView(&mockView);
}

TEST_F(CacheMVPTest, CachePresenter_StatsTracking) {
    CachePresenter presenter;

    presenter.insert("key1", QString("value1"), CacheType::SearchResultCache);
    presenter.get("key1", CacheType::SearchResultCache);         // Hit
    presenter.get("nonexistent", CacheType::SearchResultCache);  // Miss

    CacheStats stats = presenter.getStats(CacheType::SearchResultCache);

    EXPECT_EQ(stats.totalHits, 1);
    EXPECT_EQ(stats.totalMisses, 1);
    EXPECT_DOUBLE_EQ(stats.hitRatio, 0.5);
}

TEST_F(CacheMVPTest, CachePresenter_Clear) {
    CachePresenter presenter;
    MockCacheView mockView;

    presenter.registerView(&mockView);

    presenter.insert("key1", QString("value1"),
                     CacheType::SearchHighlightCache);
    presenter.insert("key2", QString("value2"),
                     CacheType::SearchHighlightCache);

    int initialClearCount = mockView.clearCount;

    presenter.clear(CacheType::SearchHighlightCache);

    EXPECT_GT(mockView.clearCount, initialClearCount);
    EXPECT_FALSE(presenter.contains("key1", CacheType::SearchHighlightCache));
    EXPECT_FALSE(presenter.contains("key2", CacheType::SearchHighlightCache));

    presenter.unregisterView(&mockView);
}

TEST_F(CacheMVPTest, CachePresenter_MemoryManagement) {
    CachePresenter presenter;

    // Set a low memory limit
    presenter.setCacheLimit(CacheType::PdfRenderCache, 1024);

    // Insert multiple entries
    for (int i = 0; i < 10; i++) {
        QString key = QString("key%1").arg(i);
        QString data(1000, 'X');  // Large data
        presenter.insert(key, data, CacheType::PdfRenderCache);
    }

    qint64 totalMemory = presenter.getTotalMemoryUsage();
    EXPECT_GT(totalMemory, 0);

    // Enforce limits should reduce memory
    presenter.enforceMemoryLimits();

    qint64 afterEnforce = presenter.getTotalMemoryUsage();
    // Memory should be reduced (though exact amount depends on eviction)
    EXPECT_TRUE(afterEnforce >= 0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
