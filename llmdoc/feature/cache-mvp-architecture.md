# Cache System MVP Architecture Refactoring

## 1. Purpose

The cache component has been refactored from a monolithic `CacheManager` into a Model-View-Presenter (MVP) architecture to improve separation of concerns, testability, and maintainability. This refactoring decouples data management, business logic, and presentation layers while maintaining backward compatibility with existing cache components. The new architecture enables independent testing and evolution of each layer without affecting others.

## 2. How it Works

### Architecture Overview

The cache MVP refactoring separates the cache system into three distinct layers:

```
┌────────────────────────────────────────────────────┐
│         VIEW LAYER (ICacheView interfaces)         │
│  - ICacheView (base interface)                     │
│  - ICacheStatsView (statistics observation)        │
│  - ICacheConfigView (configuration observation)    │
│  - ICacheMemoryView (memory pressure observation)  │
└────────────────────────────────────────────────────┘
                         ▲
                         │ notifies
                         │
┌────────────────────────────────────────────────────┐
│     PRESENTER LAYER (CachePresenter)               │
│  - Coordinates models and views                    │
│  - Implements business logic                       │
│  - Manages eviction policies                       │
│  - Handles memory management                       │
│  - Emits QObject signals for EventBus integration  │
└────────────────────────────────────────────────────┘
                         ▲
                         │ manages
                         │
┌────────────────────────────────────────────────────┐
│         MODEL LAYER (Four specialized models)      │
│  - CacheEntryModel (individual entry metadata)     │
│  - CacheDataModel (storage and retrieval ops)      │
│  - CacheConfigModel (configuration settings)       │
│  - CacheStatsModel (performance tracking)          │
└────────────────────────────────────────────────────┘
```

### Model Layer

**CacheEntryModel** represents a single cache entry with metadata:

- Key, data, and type identification
- Access tracking (timestamp, last accessed, access count)
- Priority and memory size information
- Eviction score calculation for intelligent eviction
- Age computation and expiration checks

**CacheDataModel** manages actual cache storage:

- O(1) hash-based lookups using `QHash<QString, CacheEntryModel>`
- Thread-safe operations via `QMutex` (pimpl pattern)
- Query operations: filter by type, retrieve LRU entries
- Maintenance: expire old entries, evict to target memory
- Supports both mutable and const access patterns

**CacheConfigModel** centralizes configuration:

- Global limits: total memory, cleanup interval
- Memory pressure thresholds (warning, critical, eviction triggers)
- Per-cache-type limits and eviction strategies
- Feature flags: LRU, memory pressure eviction, adaptation, preloading, system monitoring, predictive eviction, compression, emergency eviction
- System memory monitoring settings
- Thread-safe via `QMutex`

**CacheStatsModel** tracks performance metrics:

- Hit/miss counters and ratios per cache type
- Memory usage aggregation
- Eviction counts and freed bytes
- Access pattern tracking (up to 1000 recent accesses)
- Entry count tracking
- Thread-safe with `QMutex` protection

### Presenter Layer

**CachePresenter** (QObject-derived) orchestrates the cache system:

- Owns and manages the three model instances
- Registers/unregisters multiple view types
- Implements cache operations (insert, get, remove, clear)
- Enforces memory limits and handles pressure situations
- Implements eviction policies: LRU, expiration-based, adaptive
- Emits QObject signals for EventBus integration:
  - `cacheHit(CacheType, QString)`
  - `cacheMiss(CacheType, QString)`
  - `cacheEvictionOccurred(CacheType, qint64)`
  - `memoryPressureWarning(double)`
  - `memoryPressureCritical(double)`
- Notifies registered views when state changes occur

### View Layer

View interfaces define contracts for cache consumers. Four view types provided:

**ICacheView** - Base observation interface:

- `onCacheUpdated(CacheType, key)` - Cache entry changed
- `onCacheCleared(CacheType)` - Entire cache type cleared
- `onCacheEvicted(CacheType, key, reason)` - Entry removed

**ICacheStatsView** - Statistics observation:

- `onStatsUpdated(CacheType, CacheStats)` - Per-type stats changed
- `onGlobalStatsUpdated(totalMemory, hitRatio)` - Global stats changed

**ICacheConfigView** - Configuration observation:

- `onConfigChanged(CacheType)` - Per-type config changed
- `onGlobalConfigChanged()` - Global config changed

**ICacheMemoryView** - Memory pressure observation:

- `onMemoryLimitExceeded(usage, limit)` - Limit exceeded
- `onMemoryPressureDetected(ratio)` - Pressure threshold crossed
- `onSystemMemoryPressureDetected(ratio)` - System memory pressure

### CacheManager Integration

The existing `CacheManager` class continues to serve as the public API singleton. Recent enhancements include:

- **Performance fix**: Atomic flag `timerCallbackActive` prevents overlapping timer callbacks that caused event loop starvation
- **Elapsed time tracking**: All periodic operations (`handleMemoryPressure`, `performPeriodicCleanup`, `onMemoryPressureTimer`, `updateCacheStatistics`, `handleSystemMemoryPressure`) now log execution time
- **Callback serialization**: Compare-and-swap atomic operations skip redundant callbacks during high-pressure periods
- **Debug instrumentation**: Comprehensive logging via `SLOG_DEBUG_F` for performance analysis

## 3. Relevant Code Modules

- `app/cache/model/CacheEntryModel.h` - Individual cache entry representation
- `app/cache/model/CacheDataModel.h` - Storage and retrieval operations
- `app/cache/model/CacheConfigModel.h` - Configuration management
- `app/cache/model/CacheStatsModel.h` - Performance metrics
- `app/cache/presenter/CachePresenter.h` - Business logic orchestration
- `app/cache/view/ICacheView.h` - View interfaces and contracts
- `app/cache/CacheManager.h` - Public API singleton (existing)
- `app/cache/CacheManager.cpp` - Public API implementation with performance fixes
- `app/cache/CacheTypes.h` - Shared type definitions

## 4. Attention

- **Thread Safety**: All models use `QMutex` for thread-safe operations. CachePresenter is QObject-based and should be accessed from the main thread when using signals/slots.
- **Pimpl Pattern**: CacheDataModel and CachePresenter use pimpl to hide implementation details and reduce compilation dependencies.
- **Performance**: Atomic flag prevents timer callback overlap; use elapsed time logging to identify bottlenecks.
- **Backward Compatibility**: CacheManager remains the public API; MVP layers are internal implementation details.
- **EventBus Integration**: CachePresenter signals can be connected to EventBus for decoupled event handling across the application.
- **Memory Pressure**: System memory monitoring is optional and configurable; emergency eviction triggers on critical pressure.
