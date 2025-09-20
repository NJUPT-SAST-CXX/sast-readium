#!/usr/bin/env pwsh
<#
.SYNOPSIS
    测试文件重组织脚本
.DESCRIPTION
    统一所有测试文件的命名风格并重新组织目录结构
.PARAMETER DryRun
    只显示将要执行的操作，不实际修改文件
.PARAMETER BackupDir
    备份目录路径
.PARAMETER SkipBackup
    跳过备份步骤
.EXAMPLE
    .\reorganize_tests.ps1 -DryRun
    .\reorganize_tests.ps1 -BackupDir "D:\Backup\tests"
#>

param(
    [switch]$DryRun,
    [string]$BackupDir = "",
    [switch]$SkipBackup,
    [switch]$Force
)

# 设置路径
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$TestsDir = Join-Path $ProjectRoot "tests"
$ScriptsDir = $PSScriptRoot

# 颜色输出
function Write-Success { Write-Host $args -ForegroundColor Green }
function Write-Info { Write-Host $args -ForegroundColor Cyan }
function Write-Warning { Write-Host $args -ForegroundColor Yellow }
function Write-Error { Write-Host $args -ForegroundColor Red }

Write-Info "========================================="
Write-Info "测试文件重组织工具"
Write-Info "========================================="

# 检查测试目录
if (-not (Test-Path $TestsDir)) {
    Write-Error "错误: 测试目录不存在: $TestsDir"
    exit 1
}

# 步骤 1: 备份
if (-not $SkipBackup) {
    if (-not $BackupDir) {
        $BackupDir = Join-Path $ProjectRoot "tests_backup_$(Get-Date -Format 'yyyyMMdd_HHmmss')"
    }
    
    Write-Info "`n步骤 1: 备份测试目录"
    Write-Info "备份到: $BackupDir"
    
    if (-not $DryRun) {
        try {
            Copy-Item -Path $TestsDir -Destination $BackupDir -Recurse -Force
            Write-Success "✓ 备份完成"
        } catch {
            Write-Error "备份失败: $_"
            exit 1
        }
    } else {
        Write-Warning "DRY RUN: 将备份到 $BackupDir"
    }
} else {
    Write-Warning "跳过备份步骤"
}

# 步骤 2: 创建新目录结构
Write-Info "`n步骤 2: 创建新目录结构"

$NewDirectories = @(
    "Framework",
    "Unit",
    "Integration",
    "Performance",
    "Controller",
    "Command",
    "Factory",
    "Utilities"
)

foreach ($dir in $NewDirectories) {
    $dirPath = Join-Path $TestsDir $dir
    if (-not (Test-Path $dirPath)) {
        if (-not $DryRun) {
            New-Item -ItemType Directory -Path $dirPath -Force | Out-Null
            Write-Success "✓ 创建目录: $dir/"
        } else {
            Write-Info "将创建目录: $dir/"
        }
    }
}

# 步骤 3: 定义重命名映射
Write-Info "`n步骤 3: 准备文件重命名映射"

$RenameMap = @{
    # 框架文件
    "TestUtilities.h" = @{New="Framework/TestUtilities.h"; Category="Framework"}
    "TestUtilities.cpp" = @{New="Framework/TestUtilities.cpp"; Category="Framework"}
    "MockObject.h" = @{New="Framework/MockObject.h"; Category="Framework"}
    "MockObject.cpp" = @{New="Framework/MockObject.cpp"; Category="Framework"}
    
    # 单元测试
    "unit\test_search_engine_core.cpp" = @{New="Unit/Unit_SearchEngine_Test.cpp"; Category="Unit"}
    "unit\test_search_validation.cpp" = @{New="Unit/Unit_SearchValidation_Test.cpp"; Category="Unit"}
    "unit\test_search_components.cpp" = @{New="Unit/Unit_SearchComponents_Test.cpp"; Category="Unit"}
    "unit\test_search_advanced_features.cpp" = @{New="Unit/Unit_SearchAdvanced_Test.cpp"; Category="Unit"}
    "unit\test_search_edge_cases.cpp" = @{New="Unit/Unit_SearchEdgeCases_Test.cpp"; Category="Unit"}
    "unit\test_search_performance_caching.cpp" = @{New="Unit/Unit_SearchCache_Test.cpp"; Category="Unit"}
    "unit\test_error_handling.cpp" = @{New="Unit/Unit_ErrorHandling_Test.cpp"; Category="Unit"}
    "unit\test_error_recovery.cpp" = @{New="Unit/Unit_ErrorRecovery_Test.cpp"; Category="Unit"}
    "unit\test_pdf_optimizations.cpp" = @{New="Unit/Unit_PdfOptimizations_Test.cpp"; Category="Unit"}
    "unit\test_qgraphics_components.cpp" = @{New="Unit/Unit_QGraphicsComponents_Test.cpp"; Category="Unit"}
    "unit\test_thread_safety.cpp" = @{New="Unit/Unit_ThreadSafety_Test.cpp"; Category="Unit"}
    
    # 集成测试
    "integration\test_search_integration.cpp" = @{New="Integration/Integration_Search_Test.cpp"; Category="Integration"}
    "integration\test_rendering_mode_switch.cpp" = @{New="Integration/Integration_RenderingMode_Test.cpp"; Category="Integration"}
    "real_world\test_real_pdf_documents.cpp" = @{New="Integration/Integration_RealPdf_Test.cpp"; Category="Integration"}
    "smoke_test.cpp" = @{New="Integration/Integration_Smoke_Test.cpp"; Category="Integration"}
    "test_qgraphics_pdf.cpp" = @{New="Integration/Integration_QGraphicsPdf_Test.cpp"; Category="Integration"}
    
    # 性能测试
    "performance\test_rendering_performance.cpp" = @{New="Performance/Performance_Rendering_Test.cpp"; Category="Performance"}
    "performance\test_search_optimizations.cpp" = @{New="Performance/Performance_SearchOptimization_Test.cpp"; Category="Performance"}
    
    # 控制器测试
    "controller\ServiceLocatorTest.cpp" = @{New="Controller/Controller_ServiceLocator_Test.cpp"; Category="Controller"}
    "controller\StateManagerTest.cpp" = @{New="Controller/Controller_StateManager_Test.cpp"; Category="Controller"}
    
    # 命令测试
    "command\InitializationCommandTest.cpp" = @{New="Command/Command_Initialization_Test.cpp"; Category="Command"}
    
    # 工厂测试
    "factory\ModelFactoryTest.cpp" = @{New="Factory/Factory_Model_Test.cpp"; Category="Factory"}
}

# 步骤 4: 执行重命名
Write-Info "`n步骤 4: 重命名和移动文件"

$ProcessedFiles = 0
$SkippedFiles = 0

foreach ($oldPath in $RenameMap.Keys) {
    $oldFullPath = Join-Path $TestsDir $oldPath
    $newPath = $RenameMap[$oldPath].New
    $newFullPath = Join-Path $TestsDir $newPath
    
    if (Test-Path $oldFullPath) {
        if ($oldFullPath -ne $newFullPath) {
            Write-Info "移动: $oldPath -> $newPath"
            
            if (-not $DryRun) {
                try {
                    # 确保目标目录存在
                    $newDir = Split-Path -Parent $newFullPath
                    if (-not (Test-Path $newDir)) {
                        New-Item -ItemType Directory -Path $newDir -Force | Out-Null
                    }
                    
                    # 移动文件
                    Move-Item -Path $oldFullPath -Destination $newFullPath -Force
                    $ProcessedFiles++
                    Write-Success "  ✓ 完成"
                } catch {
                    Write-Error "  ✗ 失败: $_"
                }
            } else {
                $ProcessedFiles++
            }
        }
    } else {
        # 检查是否是 _new 文件
        if ($oldPath -like "*_new*") {
            $SkippedFiles++
        }
    }
}

# 步骤 5: 处理 _new 文件
Write-Info "`n步骤 5: 合并 _new 文件"

$NewFiles = @(
    "unit\test_search_engine_new.cpp",
    "integration\test_search_integration_new.cpp",
    "performance\test_rendering_performance_new.cpp",
    "controller\StateManagerTest_new.cpp",
    "command\InitializationCommandTest_new.cpp",
    "factory\ModelFactoryTest_new.cpp"
)

foreach ($newFile in $NewFiles) {
    $newFilePath = Join-Path $TestsDir $newFile
    if (Test-Path $newFilePath) {
        $baseFile = $newFile -replace "_new", ""
        Write-Warning "发现 _new 文件: $newFile"
        Write-Info "  需要手动合并到: $baseFile"
        
        if (-not $DryRun -and $Force) {
            # 如果强制模式，使用 _new 版本
            $targetName = $RenameMap[$baseFile].New
            if ($targetName) {
                $targetPath = Join-Path $TestsDir $targetName
                Write-Info "  使用 _new 版本覆盖"
                Move-Item -Path $newFilePath -Destination $targetPath -Force
            }
        }
    }
}

# 步骤 6: 移动工具脚本
Write-Info "`n步骤 6: 移动工具脚本"

$ScriptFiles = @(
    @{Source="$ScriptsDir\run_tests.ps1"; Dest="$TestsDir\Utilities\run_tests.ps1"},
    @{Source="$ScriptsDir\migrate_tests.py"; Dest="$TestsDir\Utilities\migrate_tests.py"},
    @{Source="$ScriptsDir\reorganize_tests.py"; Dest="$TestsDir\Utilities\reorganize_tests.py"}
)

foreach ($script in $ScriptFiles) {
    if (Test-Path $script.Source) {
        if (-not $DryRun) {
            Copy-Item -Path $script.Source -Destination $script.Dest -Force
            Write-Success "✓ 复制: $(Split-Path -Leaf $script.Source) -> Utilities/"
        } else {
            Write-Info "将复制: $(Split-Path -Leaf $script.Source) -> Utilities/"
        }
    }
}

# 步骤 7: 清理空目录
Write-Info "`n步骤 7: 清理空目录"

if (-not $DryRun) {
    $EmptyDirs = Get-ChildItem -Path $TestsDir -Recurse -Directory | 
                 Where-Object { -not (Get-ChildItem -Path $_.FullName -Recurse -File) }
    
    foreach ($dir in $EmptyDirs) {
        if ($dir.Name -notin $NewDirectories) {
            Remove-Item -Path $dir.FullName -Force
            Write-Success "✓ 删除空目录: $($dir.Name)"
        }
    }
} else {
    Write-Info "将删除所有空目录"
}

# 步骤 8: 更新 CMakeLists.txt
Write-Info "`n步骤 8: 更新 CMakeLists.txt"

$CmakeFile = Join-Path $TestsDir "CMakeLists.txt"
if (Test-Path $CmakeFile) {
    if (-not $DryRun) {
        # 备份 CMakeLists.txt
        Copy-Item -Path $CmakeFile -Destination "$CmakeFile.bak" -Force
        
        # 读取文件内容
        $content = Get-Content -Path $CmakeFile -Raw
        
        # 替换文件名
        foreach ($oldPath in $RenameMap.Keys) {
            $oldName = Split-Path -Leaf $oldPath
            $newPath = $RenameMap[$oldPath].New
            $newName = Split-Path -Leaf $newPath
            
            $content = $content -replace [regex]::Escape($oldName), $newName
            
            # 更新路径
            $oldRelPath = $oldPath -replace '\\', '/'
            $newRelPath = $newPath -replace '\\', '/'
            $content = $content -replace [regex]::Escape($oldRelPath), $newRelPath
        }
        
        # 写回文件
        Set-Content -Path $CmakeFile -Value $content -Encoding UTF8
        Write-Success "✓ 更新 CMakeLists.txt"
    } else {
        Write-Info "将更新 CMakeLists.txt"
    }
}

# 汇总
Write-Info "`n========================================="
Write-Info "重组织完成汇总"
Write-Info "========================================="

if ($DryRun) {
    Write-Warning "DRY RUN 模式 - 没有实际修改文件"
    Write-Info "将处理文件数: $ProcessedFiles"
    Write-Info "将跳过文件数: $SkippedFiles"
    Write-Info ""
    Write-Info "要执行实际重组织，请运行:"
    Write-Info "  .\reorganize_tests.ps1"
} else {
    Write-Success "✓ 重组织完成!"
    Write-Info "处理文件数: $ProcessedFiles"
    Write-Info "跳过文件数: $SkippedFiles"
    if (-not $SkipBackup) {
        Write-Info "备份位置: $BackupDir"
    }
    Write-Info ""
    Write-Warning "下一步操作:"
    Write-Info "1. 检查并手动合并 _new 文件"
    Write-Info "2. 重新构建项目: cmake -B build && cmake --build build"
    Write-Info "3. 运行测试验证: .\tests\Utilities\run_tests.ps1 -TestType All"
}

# 生成报告
$ReportFile = Join-Path $TestsDir "reorganization_report.txt"
$Report = @"
测试重组织报告
生成时间: $(Get-Date -Format 'yyyy-MM-dd HH:mm:ss')
=====================================

处理文件数: $ProcessedFiles
跳过文件数: $SkippedFiles
备份位置: $(if ($SkipBackup) {"无"} else {$BackupDir})

新目录结构:
- Framework/    测试框架核心文件
- Unit/        单元测试
- Integration/ 集成测试
- Performance/ 性能测试
- Controller/  控制器测试
- Command/     命令测试
- Factory/     工厂测试
- Utilities/   工具脚本

需要手动处理的文件:
"@

foreach ($newFile in $NewFiles) {
    if (Test-Path (Join-Path $TestsDir $newFile)) {
        $Report += "`n- $newFile"
    }
}

if (-not $DryRun) {
    $Report | Out-File -FilePath $ReportFile -Encoding UTF8
    Write-Info "`n报告已保存到: $ReportFile"
}
