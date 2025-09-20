#!/usr/bin/env python3
"""
测试文件重组织脚本
统一所有测试文件的命名风格，并重新组织目录结构
"""

import os
import shutil
import re
from pathlib import Path
from typing import Dict, List, Tuple

class TestReorganizer:
    """
    统一测试命名规范：
    - 单元测试: Unit_ComponentName_Test.cpp
    - 集成测试: Integration_FeatureName_Test.cpp
    - 性能测试: Performance_AspectName_Test.cpp
    - 控制器测试: Controller_ComponentName_Test.cpp
    - 命令测试: Command_CommandName_Test.cpp
    - 工厂测试: Factory_FactoryName_Test.cpp
    """
    
    def __init__(self, tests_dir: Path, dry_run: bool = True):
        self.tests_dir = tests_dir
        self.dry_run = dry_run
        self.rename_map = {}
        self.move_map = {}
        
    def analyze_current_structure(self) -> Dict[str, List[str]]:
        """分析当前的测试文件结构"""
        structure = {
            'unit': [],
            'integration': [],
            'performance': [],
            'controller': [],
            'command': [],
            'factory': [],
            'misc': []
        }
        
        for root, dirs, files in os.walk(self.tests_dir):
            for file in files:
                if file.endswith('.cpp') or file.endswith('.h'):
                    if file in ['TestUtilities.cpp', 'TestUtilities.h', 
                               'MockObject.cpp', 'MockObject.h']:
                        continue  # Skip framework files
                        
                    rel_path = Path(root).relative_to(self.tests_dir)
                    full_path = Path(root) / file
                    
                    # Categorize based on path and name
                    if 'unit' in str(rel_path) or 'test_' in file:
                        if 'search' in file.lower():
                            structure['unit'].append(str(full_path))
                        elif 'error' in file.lower():
                            structure['unit'].append(str(full_path))
                        elif 'qgraphics' in file.lower():
                            structure['unit'].append(str(full_path))
                        else:
                            structure['unit'].append(str(full_path))
                    elif 'integration' in str(rel_path):
                        structure['integration'].append(str(full_path))
                    elif 'performance' in str(rel_path):
                        structure['performance'].append(str(full_path))
                    elif 'controller' in str(rel_path):
                        structure['controller'].append(str(full_path))
                    elif 'command' in str(rel_path):
                        structure['command'].append(str(full_path))
                    elif 'factory' in str(rel_path):
                        structure['factory'].append(str(full_path))
                    else:
                        structure['misc'].append(str(full_path))
                        
        return structure
    
    def generate_new_name(self, old_path: str, category: str) -> str:
        """生成新的标准化文件名"""
        old_name = Path(old_path).stem
        extension = Path(old_path).suffix
        
        # 清理名称
        clean_name = old_name.replace('test_', '').replace('Test', '')
        clean_name = clean_name.replace('_new', '').replace('_test', '')
        
        # 转换为 PascalCase
        parts = clean_name.split('_')
        pascal_case = ''.join(word.capitalize() for word in parts)
        
        # 根据类别生成新名称
        category_map = {
            'unit': 'Unit',
            'integration': 'Integration',
            'performance': 'Performance',
            'controller': 'Controller',
            'command': 'Command',
            'factory': 'Factory'
        }
        
        prefix = category_map.get(category, 'Test')
        
        # 特殊处理某些名称
        if 'search' in old_name.lower() and 'integration' in old_name.lower():
            pascal_case = 'SearchIntegration'
        elif 'search' in old_name.lower() and 'engine' in old_name.lower():
            pascal_case = 'SearchEngine'
        elif 'search' in old_name.lower() and 'validation' in old_name.lower():
            pascal_case = 'SearchValidation'
        elif 'rendering' in old_name.lower() and 'performance' in old_name.lower():
            pascal_case = 'RenderingPerformance'
        elif 'state' in old_name.lower() and 'manager' in old_name.lower():
            pascal_case = 'StateManager'
        elif 'service' in old_name.lower() and 'locator' in old_name.lower():
            pascal_case = 'ServiceLocator'
        elif 'model' in old_name.lower() and 'factory' in old_name.lower():
            pascal_case = 'ModelFactory'
            
        new_name = f"{prefix}_{pascal_case}_Test{extension}"
        return new_name
    
    def generate_reorganization_plan(self) -> Tuple[Dict, Dict]:
        """生成重组织计划"""
        structure = self.analyze_current_structure()
        
        # 新的目录结构
        new_dirs = {
            'Unit': 'Unit',
            'Integration': 'Integration', 
            'Performance': 'Performance',
            'Controller': 'Controller',
            'Command': 'Command',
            'Factory': 'Factory',
            'Framework': 'Framework',
            'Utilities': 'Utilities'
        }
        
        rename_map = {}
        move_map = {}
        
        for category, files in structure.items():
            if category == 'misc':
                continue
                
            for file_path in files:
                if '_new' in file_path and Path(file_path.replace('_new', '')).exists():
                    # Skip duplicate _new files
                    continue
                    
                old_path = Path(file_path)
                new_name = self.generate_new_name(file_path, category)
                
                # 确定新目录
                target_dir = new_dirs.get(category.capitalize(), category.capitalize())
                new_path = self.tests_dir / target_dir / new_name
                
                if old_path.name != new_name or old_path.parent != new_path.parent:
                    rename_map[str(old_path)] = str(new_path)
                    
        return rename_map, move_map
    
    def update_includes(self, file_path: Path, rename_map: Dict[str, str]):
        """更新文件中的 include 语句"""
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
            original_content = content
            
            # 更新 include 语句
            for old_path, new_path in rename_map.items():
                old_name = Path(old_path).name
                new_name = Path(new_path).name
                
                # 更新各种形式的 include
                patterns = [
                    (f'#include "{old_name}"', f'#include "{new_name}"'),
                    (f'#include <{old_name}>', f'#include <{new_name}>'),
                    (f'#include "../{old_name}"', f'#include "../{new_name}"'),
                    (f'#include "../../{old_name}"', f'#include "../../{new_name}"'),
                ]
                
                for old_pattern, new_pattern in patterns:
                    content = content.replace(old_pattern, new_pattern)
                    
            if content != original_content:
                if not self.dry_run:
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                return True
        except Exception as e:
            print(f"Error updating includes in {file_path}: {e}")
            return False
        
        return False
    
    def update_cmake_files(self, rename_map: Dict[str, str]):
        """更新 CMakeLists.txt 文件"""
        cmake_files = list(self.tests_dir.rglob('CMakeLists*.txt'))
        
        for cmake_file in cmake_files:
            try:
                with open(cmake_file, 'r', encoding='utf-8') as f:
                    content = f.read()
                    
                original_content = content
                
                # 更新文件引用
                for old_path, new_path in rename_map.items():
                    old_name = Path(old_path).name
                    new_name = Path(new_path).name
                    
                    # 更新各种 CMake 引用
                    content = content.replace(old_name, new_name)
                    
                    # 更新相对路径
                    old_rel = Path(old_path).relative_to(self.tests_dir)
                    new_rel = Path(new_path).relative_to(self.tests_dir)
                    content = content.replace(str(old_rel).replace('\\', '/'), 
                                            str(new_rel).replace('\\', '/'))
                    
                if content != original_content:
                    if not self.dry_run:
                        with open(cmake_file, 'w', encoding='utf-8') as f:
                            f.write(content)
                    print(f"Updated CMake file: {cmake_file}")
                    
            except Exception as e:
                print(f"Error updating CMake file {cmake_file}: {e}")
    
    def execute_reorganization(self):
        """执行重组织"""
        rename_map, _ = self.generate_reorganization_plan()
        
        if not rename_map:
            print("No files need reorganization")
            return
            
        print(f"\n{'DRY RUN - ' if self.dry_run else ''}Reorganization Plan:")
        print("=" * 80)
        
        # 创建新目录结构
        new_dirs = ['Unit', 'Integration', 'Performance', 
                   'Controller', 'Command', 'Factory', 
                   'Framework', 'Utilities']
        
        for dir_name in new_dirs:
            dir_path = self.tests_dir / dir_name
            if not dir_path.exists():
                if not self.dry_run:
                    dir_path.mkdir(parents=True, exist_ok=True)
                print(f"Create directory: {dir_name}/")
        
        # 执行重命名和移动
        for old_path, new_path in rename_map.items():
            old = Path(old_path)
            new = Path(new_path)
            
            if not old.exists():
                continue
                
            print(f"\nRename/Move:")
            print(f"  From: {old.relative_to(self.tests_dir)}")
            print(f"  To:   {new.relative_to(self.tests_dir)}")
            
            if not self.dry_run:
                # 创建目标目录
                new.parent.mkdir(parents=True, exist_ok=True)
                
                # 备份原文件
                backup_path = old.with_suffix(old.suffix + '.bak')
                shutil.copy2(old, backup_path)
                
                # 移动/重命名文件
                shutil.move(str(old), str(new))
        
        # 更新所有文件中的 include 语句
        print("\n" + "=" * 80)
        print("Updating include statements...")
        
        for file_path in self.tests_dir.rglob('*.cpp'):
            if self.update_includes(file_path, rename_map):
                print(f"  Updated: {file_path.relative_to(self.tests_dir)}")
                
        for file_path in self.tests_dir.rglob('*.h'):
            if self.update_includes(file_path, rename_map):
                print(f"  Updated: {file_path.relative_to(self.tests_dir)}")
        
        # 更新 CMake 文件
        print("\n" + "=" * 80)
        print("Updating CMake files...")
        self.update_cmake_files(rename_map)
        
        # 清理空目录
        if not self.dry_run:
            for root, dirs, files in os.walk(self.tests_dir, topdown=False):
                for dir_name in dirs:
                    dir_path = Path(root) / dir_name
                    if not any(dir_path.iterdir()):
                        dir_path.rmdir()
                        print(f"Removed empty directory: {dir_path.relative_to(self.tests_dir)}")
        
        print("\n" + "=" * 80)
        if self.dry_run:
            print("DRY RUN COMPLETE - No files were modified")
            print("Run without --dry-run to apply changes")
        else:
            print("Reorganization complete!")
            print("Backup files created with .bak extension")


def main():
    import argparse
    
    parser = argparse.ArgumentParser(description='Reorganize test files with unified naming')
    parser.add_argument('--tests-dir', type=Path, 
                       default=Path('D:/Project/sast-readium/tests'),
                       help='Path to tests directory')
    parser.add_argument('--dry-run', action='store_true',
                       help='Show what would be changed without modifying files')
    
    args = parser.parse_args()
    
    if not args.tests_dir.exists():
        print(f"Error: Tests directory not found: {args.tests_dir}")
        return 1
        
    reorganizer = TestReorganizer(args.tests_dir, args.dry_run)
    reorganizer.execute_reorganization()
    
    return 0


if __name__ == '__main__':
    exit(main())
