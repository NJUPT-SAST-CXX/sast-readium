#!/usr/bin/env python3
"""
Test Migration Script
Migrates existing test files to use the new test framework and modular architecture.
"""

import os
import re
import argparse
from pathlib import Path
from typing import List, Dict, Tuple

class TestMigrator:
    """Handles migration of test files to new framework."""
    
    def __init__(self, dry_run: bool = False):
        self.dry_run = dry_run
        self.files_processed = 0
        self.files_modified = 0
        
    def migrate_file(self, filepath: Path) -> bool:
        """Migrate a single test file."""
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            
            # Apply migrations
            content = self._migrate_includes(content)
            content = self._migrate_base_class(content)
            content = self._migrate_setup_teardown(content)
            content = self._migrate_assertions(content)
            content = self._migrate_service_usage(content)
            content = self._migrate_wait_statements(content)
            
            if content != original_content:
                if not self.dry_run:
                    # Backup original
                    backup_path = filepath.with_suffix('.cpp.bak')
                    with open(backup_path, 'w', encoding='utf-8') as f:
                        f.write(original_content)
                    
                    # Write migrated content
                    with open(filepath, 'w', encoding='utf-8') as f:
                        f.write(content)
                
                print(f"✓ Migrated: {filepath}")
                self.files_modified += 1
                return True
            else:
                print(f"  No changes needed: {filepath}")
                return False
                
        except Exception as e:
            print(f"✗ Error migrating {filepath}: {e}")
            return False
        finally:
            self.files_processed += 1
    
    def _migrate_includes(self, content: str) -> str:
        """Update include statements."""
        # Add TestUtilities include if not present
        if '../TestUtilities.h' not in content and 'TestUtilities.h' not in content:
            # Find first include and add after it
            first_include = re.search(r'#include\s+[<"].*?[>"]', content)
            if first_include:
                insert_pos = first_include.end()
                content = content[:insert_pos] + '\n#include "../TestUtilities.h"' + content[insert_pos:]
        
        # Add new architecture includes
        includes_to_add = [
            '../../app/controller/ServiceLocator.h',
            '../../app/controller/StateManager.h',
            '../../app/controller/EventBus.h'
        ]
        
        for include in includes_to_add:
            if include not in content:
                # Check if we need this include based on usage
                filename = os.path.basename(include).replace('.h', '')
                if filename in content:
                    content = re.sub(
                        r'(#include\s+[<"].*?[>"]\n)',
                        r'\1#include "' + include + '"\n',
                        content,
                        count=1
                    )
        
        return content
    
    def _migrate_base_class(self, content: str) -> str:
        """Change test class inheritance to TestBase."""
        # Find test class declarations
        class_pattern = r'class\s+(\w+)\s*:\s*public\s+QObject\s*{'
        replacement = r'class \1 : public TestBase {'
        
        content = re.sub(class_pattern, replacement, content)
        
        return content
    
    def _migrate_setup_teardown(self, content: str) -> str:
        """Update setup/teardown methods."""
        # Add override to virtual methods
        methods = ['initTestCase', 'cleanupTestCase', 'init', 'cleanup']
        
        for method in methods:
            # Pattern to find method declaration
            pattern = rf'void\s+{method}\s*\(\s*\)'
            # Check if override is already present
            if re.search(pattern + r'\s+override', content):
                continue
            # Add override
            content = re.sub(
                pattern + r'(\s*[{;])',
                f'void {method}() override\\1',
                content
            )
        
        return content
    
    def _migrate_assertions(self, content: str) -> str:
        """Update assertion macros."""
        # Map old assertions to new ones if needed
        replacements = {
            r'QVERIFY2\((.*?),\s*(.*?)\)': r'QVERIFY_MSG(\1, \2)',
            r'assertTrue\((.*?)\)': r'QVERIFY(\1)',
            r'assertFalse\((.*?)\)': r'QVERIFY(!\1)',
            r'assertEquals\((.*?),\s*(.*?)\)': r'QCOMPARE(\1, \2)',
        }
        
        for pattern, replacement in replacements.items():
            content = re.sub(pattern, replacement, content)
        
        # Add QVERIFY_TIMEOUT where appropriate
        # Look for patterns like: while (!condition) { wait(); }
        timeout_pattern = r'while\s*\(\s*!(.*?)\s*\)\s*{\s*(?:.*?wait.*?)\s*}'
        content = re.sub(
            timeout_pattern,
            r'QVERIFY_TIMEOUT(\1, 5000)',
            content
        )
        
        return content
    
    def _migrate_service_usage(self, content: str) -> str:
        """Update service usage patterns."""
        # Replace singleton patterns with ServiceLocator
        patterns = [
            (r'(\w+)::instance\(\)->', r'ServiceLocator::instance().getService<\1>()->'),
            (r'(\w+)::getInstance\(\)->', r'ServiceLocator::instance().getService<\1>()->'),
        ]
        
        for pattern, replacement in patterns:
            # Only replace if ServiceLocator is included
            if 'ServiceLocator' in content:
                content = re.sub(pattern, replacement, content)
        
        return content
    
    def _migrate_wait_statements(self, content: str) -> str:
        """Update wait/delay statements."""
        replacements = {
            r'QTest::qWait\((\d+)\)': r'waitMs(\1)',
            r'QThread::msleep\((\d+)\)': r'waitMs(\1)',
            r'QThread::sleep\((\d+)\)': r'waitMs(\1 * 1000)',
            r'QCoreApplication::processEvents\(\)': r'processEvents()',
        }
        
        for pattern, replacement in replacements:
            content = re.sub(pattern, replacement, content)
        
        return content
    
    def migrate_directory(self, directory: Path) -> Dict[str, int]:
        """Migrate all test files in a directory."""
        test_files = list(directory.rglob('test*.cpp'))
        test_files.extend(list(directory.rglob('*Test.cpp')))
        
        print(f"\nMigrating tests in {directory}")
        print(f"Found {len(test_files)} test files")
        
        for filepath in test_files:
            # Skip already migrated files
            if '_new.cpp' in str(filepath):
                continue
            self.migrate_file(filepath)
        
        return {
            'processed': self.files_processed,
            'modified': self.files_modified
        }


class CMakeUpdater:
    """Updates CMake files for new test structure."""
    
    def __init__(self, dry_run: bool = False):
        self.dry_run = dry_run
    
    def update_cmake(self, cmake_path: Path) -> bool:
        """Update CMakeLists.txt for tests."""
        try:
            with open(cmake_path, 'r', encoding='utf-8') as f:
                content = f.read()
            
            original_content = content
            
            # Add test utilities to build
            if 'TestUtilities' not in content:
                content = self._add_test_utilities(content)
            
            # Update test executable definitions
            content = self._update_test_executables(content)
            
            # Add new dependencies
            content = self._add_dependencies(content)
            
            if content != original_content:
                if not self.dry_run:
                    # Backup original
                    backup_path = cmake_path.with_suffix('.txt.bak')
                    with open(backup_path, 'w', encoding='utf-8') as f:
                        f.write(original_content)
                    
                    # Write updated content
                    with open(cmake_path, 'w', encoding='utf-8') as f:
                        f.write(content)
                
                print(f"✓ Updated CMake: {cmake_path}")
                return True
            else:
                print(f"  No CMake changes needed")
                return False
                
        except Exception as e:
            print(f"✗ Error updating CMake {cmake_path}: {e}")
            return False
    
    def _add_test_utilities(self, content: str) -> str:
        """Add TestUtilities to CMake."""
        utilities_block = """
# Test utilities and framework
add_library(TestUtilities STATIC
    TestUtilities.h
    TestUtilities.cpp
    MockObject.h
    MockObject.cpp
)

target_link_libraries(TestUtilities
    Qt5::Test
    Qt5::Core
    Qt5::Widgets
)

target_include_directories(TestUtilities PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
"""
        
        # Find a good place to insert (after project declaration or first add_executable)
        insert_marker = 'enable_testing()'
        if insert_marker in content:
            insert_pos = content.find(insert_marker) + len(insert_marker)
            content = content[:insert_pos] + utilities_block + content[insert_pos:]
        
        return content
    
    def _update_test_executables(self, content: str) -> str:
        """Update test executable definitions to link with TestUtilities."""
        # Pattern to find add_executable for tests
        pattern = r'(add_executable\((\w*[Tt]est\w*)\s+[^)]+\))'
        
        def add_test_utilities_link(match):
            executable_block = match.group(0)
            test_name = match.group(2)
            
            # Check if TestUtilities is already linked
            link_pattern = rf'target_link_libraries\({test_name}[^)]*\)'
            link_match = re.search(link_pattern, content[match.end():match.end()+500])
            
            if link_match:
                link_statement = link_match.group(0)
                if 'TestUtilities' not in link_statement:
                    # Add TestUtilities to existing link libraries
                    new_link = link_statement.replace(
                        f'target_link_libraries({test_name}',
                        f'target_link_libraries({test_name}\n    TestUtilities'
                    )
                    return executable_block + content[match.end():match.end()+link_match.start()] + new_link
            
            return match.group(0)
        
        content = re.sub(pattern, add_test_utilities_link, content)
        
        return content
    
    def _add_dependencies(self, content: str) -> str:
        """Add new architecture dependencies."""
        # Ensure controller and factory libraries are linked
        dependencies = [
            'ControllerLib',
            'FactoryLib',
            'ServiceLocator',
            'StateManager',
            'EventBus'
        ]
        
        for dep in dependencies:
            if dep not in content:
                # Add to main test link libraries if needed
                content = re.sub(
                    r'(target_link_libraries\([^)]*Test[^)]*)',
                    rf'\1\n    {dep}',
                    content,
                    count=1
                )
        
        return content


def main():
    """Main entry point."""
    parser = argparse.ArgumentParser(description='Migrate tests to new framework')
    parser.add_argument(
        'path',
        type=Path,
        help='Path to tests directory or specific test file'
    )
    parser.add_argument(
        '--dry-run',
        action='store_true',
        help='Show what would be changed without modifying files'
    )
    parser.add_argument(
        '--update-cmake',
        action='store_true',
        help='Also update CMakeLists.txt files'
    )
    
    args = parser.parse_args()
    
    if args.dry_run:
        print("DRY RUN MODE - No files will be modified\n")
    
    migrator = TestMigrator(dry_run=args.dry_run)
    
    if args.path.is_file():
        # Migrate single file
        migrator.migrate_file(args.path)
    else:
        # Migrate directory
        stats = migrator.migrate_directory(args.path)
        
        print(f"\nMigration Summary:")
        print(f"  Files processed: {stats['processed']}")
        print(f"  Files modified: {stats['modified']}")
    
    if args.update_cmake:
        cmake_updater = CMakeUpdater(dry_run=args.dry_run)
        
        # Find CMakeLists.txt files
        if args.path.is_dir():
            cmake_files = list(args.path.rglob('CMakeLists.txt'))
            for cmake_file in cmake_files:
                cmake_updater.update_cmake(cmake_file)
        else:
            # Look for CMakeLists.txt in parent directory
            cmake_file = args.path.parent / 'CMakeLists.txt'
            if cmake_file.exists():
                cmake_updater.update_cmake(cmake_file)
    
    if args.dry_run:
        print("\nDRY RUN COMPLETE - No files were modified")
    else:
        print("\nMigration complete!")
        print("Backup files created with .bak extension")


if __name__ == '__main__':
    main()
