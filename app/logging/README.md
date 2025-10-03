# SAST Readium 日志模块

## 概述

本日志模块为 SAST Readium 项目提供了完整、高效、易用的日志功能。基于 spdlog 库构建，并与 Qt 框架深度集成。

## 目录结构

```
app/logging/
├── SimpleLogging.h        # 简化的日志接口（推荐使用）
├── SimpleLogging.cpp      # 简化接口实现
├── Logger.h              # 核心日志类
├── Logger.cpp            # 核心日志实现
├── LoggingManager.h      # 日志管理器
├── LoggingManager.cpp    # 日志管理器实现
├── LoggingConfig.h       # 配置管理类
├── LoggingConfig.cpp     # 配置管理实现
├── LoggingMacros.h       # 日志宏定义
├── LoggingMacros.cpp     # 日志宏实现
├── QtSpdlogBridge.h      # Qt-spdlog桥接
├── QtSpdlogBridge.cpp    # Qt-spdlog桥接实现
├── LoggingExample.md     # 使用示例文档
└── README.md            # 本文件
```

## 主要特性

### 1. 简洁的API设计

- **SimpleLogging.h** - 提供最简单的日志接口，一行代码即可使用
- 支持多种初始化方式：默认配置、简单配置、详细配置
- 清晰的函数命名，易于理解和使用

### 2. 功能完整

- **多级别日志**：Trace, Debug, Info, Warning, Error, Critical
- **多种输出目标**：控制台、文件、轮转文件、Qt Widget
- **格式化支持**：支持 printf 风格和 fmt 库格式化
- **分类日志**：支持按模块/类别记录日志
- **性能测量**：内置计时器，方便性能分析
- **条件日志**：支持条件判断和Debug构建专用日志

### 3. 高性能

- 基于 spdlog，业界领先的高性能日志库
- 支持异步日志，避免阻塞主线程
- 智能缓冲和批量写入
- 自动日志轮转，防止磁盘占用过多

### 4. Qt深度集成

- 完全兼容 Qt 的日志系统
- 支持 qDebug/qWarning 等宏的重定向
- 支持 QLoggingCategory
- 可输出到 QTextEdit 等 Qt 控件

### 5. 配置灵活

- 支持运行时配置修改
- 支持配置文件（JSON/INI）
- 支持环境变量配置
- 支持作用域临时配置

## 快速开始

### 最简单的使用

```cpp
#include "logging/SimpleLogging.h"

// 初始化
SastLogging::init();

// 记录日志
SastLogging::info("Application started");
SastLogging::debug("Debug info");
SastLogging::error("Error occurred");

// 关闭
SastLogging::shutdown();
```

### 推荐的初始化方式

```cpp
#include "logging/SimpleLogging.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // 配置日志
    SastLogging::Config config;
    config.level = SastLogging::Level::Debug;
    config.logFile = "app.log";
    config.console = true;
    config.async = true;

    if (!SastLogging::init(config)) {
        // 初始化失败，使用默认配置
        SastLogging::init();
    }

    // 应用逻辑...

    SastLogging::shutdown();
    return 0;
}
```

## 架构设计

### 分层架构

1. **接口层** (SimpleLogging.h)
   - 提供简洁的外部接口
   - 隐藏复杂实现细节
   - 便于外部模块调用

2. **管理层** (LoggingManager)
   - 统一管理日志配置
   - 协调各个组件
   - 处理运行时配置变更

3. **核心层** (Logger)
   - 实际的日志记录功能
   - 与 spdlog 交互
   - 处理格式化和输出

4. **桥接层** (QtSpdlogBridge)
   - Qt 日志系统集成
   - 消息重定向
   - 兼容性处理

5. **配置层** (LoggingConfig)
   - 配置管理
   - 配置持久化
   - 配置验证

### 设计原则

- **单一职责**：每个类只负责一个功能领域
- **依赖倒置**：高层模块不依赖底层实现细节
- **开闭原则**：易于扩展，不需修改现有代码
- **最小接口**：外部只需了解 SimpleLogging.h

## 使用建议

### 选择合适的接口

1. **新代码**：使用 SimpleLogging.h
2. **迁移代码**：使用 LoggingMacros.h 的兼容宏
3. **高级功能**：直接使用 Logger/LoggingManager

### 日志级别建议

- **Production**: Info 或 Warning
- **Development**: Debug
- **Testing**: Trace
- **Performance Analysis**: Debug + 性能计时器

### 性能优化

1. 生产环境启用异步日志
2. 合理设置日志级别
3. 避免在热路径记录过多日志
4. 使用条件日志减少开销

## 迁移指南

### 从 Qt 日志迁移

```cpp
// 旧代码
qDebug() << "Message" << value;

// 新代码（方式1：使用宏）
SLOG_DEBUG("Message: " + QString::number(value));

// 新代码（方式2：使用函数）
SastLogging::debug("Message: " + QString::number(value));
```

### 从旧日志系统迁移

1. 替换包含文件：

   ```cpp
   // 旧
   #include "utils/LoggingMacros.h"

   // 新
   #include "logging/SimpleLogging.h"
   ```

2. 更新初始化代码：

   ```cpp
   // 旧
   LoggingManager::instance().initialize(config);

   // 新
   SastLogging::init(config);
   ```

3. 更新日志调用：

   ```cpp
   // 旧
   LOG_INFO("Message");

   // 新
   SLOG_INFO("Message");
   ```

## 扩展开发

### 添加新的输出目标

1. 在 Logger.cpp 中添加新的 sink
2. 在 LoggingConfig 中添加配置项
3. 在 SimpleLogging 中添加接口（可选）

### 添加新的日志级别

1. 修改 Level 枚举
2. 更新转换函数
3. 添加对应的日志函数

### 自定义格式化

1. 修改 LoggingConfig 中的 pattern
2. 参考 spdlog 文档了解格式化选项

## 故障排除

### 常见问题

1. **日志文件未创建**
   - 检查文件路径权限
   - 确认目录存在
   - 查看 getLastError() 返回值

2. **日志未输出**
   - 检查日志级别设置
   - 确认已初始化
   - 检查输出目标配置

3. **性能问题**
   - 启用异步日志
   - 调整日志级别
   - 减少格式化操作

### 调试技巧

1. 使用 `SLOG_TRACE` 追踪执行流程
2. 使用 `SLOG_TIMER` 测量性能
3. 使用分类日志隔离不同模块
4. 使用条件日志减少噪音

## 维护说明

### 版本兼容性

- spdlog: >= 1.9.0
- Qt: >= 6.0
- C++: >= C++17

### 更新依赖

1. 更新 spdlog：注意 API 变化
2. 更新 Qt：测试兼容性
3. 更新 fmt：通常向后兼容

### 测试要点

1. 多线程环境测试
2. 大量日志压力测试
3. 日志轮转测试
4. 配置切换测试
5. Qt 集成测试

## 贡献指南

欢迎贡献代码和建议！请遵循以下原则：

1. 保持接口简洁
2. 确保向后兼容
3. 添加充分的文档
4. 编写单元测试
5. 遵循项目代码风格

## 许可证

本模块作为 SAST Readium 项目的一部分，遵循项目整体的许可证。

## 联系方式

如有问题或建议，请通过以下方式联系：

- 项目仓库：[SAST Readium](https://github.com/sast/readium)
- Issue 追踪：GitHub Issues
- 邮件列表：<dev@sast.org>

---

## 最后更新

2025-09-12
