#!/usr/bin/env python3
"""
Test file renaming script for sast-readium project.
Converts CamelCase test files to snake_case naming convention.
"""

import os
import re
import shutil
from pathlib import Path


def camel_to_snake(name):
    """Convert CamelCase to snake_case."""
    # Handle special cases first
    if name == "smoke_test":
        return name  # Already correct

    # Insert underscore before uppercase letters (except first)
    s1 = re.sub("(.)([A-Z][a-z]+)", r"\1_\2", name)
    # Insert underscore before uppercase letters preceded by lowercase
    s2 = re.sub("([a-z0-9])([A-Z])", r"\1_\2", s1)
    return s2.lower()


def get_class_name_from_file(file_path):
    """Extract the test class name from a C++ test file."""
    try:
        with open(file_path, "r", encoding="utf-8") as f:
            content = f.read()
            # Look for class declaration
            match = re.search(r"class\s+(\w+)\s*:\s*public", content)
            if match:
                return match.group(1)
    except Exception as e:
        print(f"Warning: Could not read {file_path}: {e}")
    return None


# Define the test files to rename (our working tests)
TEST_FILES_TO_RENAME = [
    # Controller tests
    ("controller/ActionMapTest.cpp", "controller/action_map_test.cpp"),
    (
        "controller/ApplicationControllerTest.cpp",
        "controller/application_controller_test.cpp",
    ),
    (
        "controller/ConfigurationManagerTest.cpp",
        "controller/configuration_manager_test.cpp",
    ),
    (
        "controller/DocumentControllerTest.cpp",
        "controller/document_controller_test.cpp",
    ),
    ("controller/EventBusTest.cpp", "controller/event_bus_test.cpp"),
    ("controller/PageControllerTest.cpp", "controller/page_controller_test.cpp"),
    ("controller/ServiceLocatorTest.cpp", "controller/service_locator_test.cpp"),
    # Command tests
    ("command/CommandManagerTest.cpp", "command/command_manager_test.cpp"),
    ("command/DocumentCommandsTest.cpp", "command/document_commands_test.cpp"),
    (
        "command/InitializationCommandTest.cpp",
        "command/initialization_command_test.cpp",
    ),
    ("command/NavigationCommandsTest.cpp", "command/navigation_commands_test.cpp"),
    # Search tests
    ("search/SmartEvictionPolicyTest.cpp", "search/smart_eviction_policy_test.cpp"),
    # Logging tests
    ("logging/LoggingStubsTest.cpp", "logging/logging_stubs_test.cpp"),
    # Factory tests
    ("factory/ModelFactoryTest.cpp", "factory/model_factory_test.cpp"),
]

# Class name mappings
CLASS_NAME_MAPPINGS = {
    "ActionMapTest": "ActionMapTest",  # Keep same for now
    "ApplicationControllerTest": "ApplicationControllerTest",
    "ConfigurationManagerTest": "ConfigurationManagerTest",
    "DocumentControllerTest": "DocumentControllerTest",
    "EventBusTest": "EventBusTest",
    "PageControllerTest": "PageControllerTest",
    "ServiceLocatorTest": "ServiceLocatorTest",
    "CommandManagerTest": "CommandManagerTest",
    "DocumentCommandsTest": "DocumentCommandsTest",
    "InitializationCommandTest": "InitializationCommandTest",
    "NavigationCommandsTest": "NavigationCommandsTest",
    "SmartEvictionPolicyTest": "SmartEvictionPolicyTest",
    "LoggingStubsTest": "LoggingStubsTest",
    "ModelFactoryTest": "ModelFactoryTest",
}


def main():
    """Main function to perform the renaming."""
    tests_dir = Path(__file__).parent
    print(f"Working in directory: {tests_dir}")

    # Step 1: Rename the test files
    print("\n=== Step 1: Renaming test files ===")
    for old_path, new_path in TEST_FILES_TO_RENAME:
        old_full_path = tests_dir / old_path
        new_full_path = tests_dir / new_path

        if old_full_path.exists():
            print(f"Renaming: {old_path} -> {new_path}")
            # Create directory if it doesn't exist
            new_full_path.parent.mkdir(parents=True, exist_ok=True)
            shutil.move(str(old_full_path), str(new_full_path))
        else:
            print(f"Warning: {old_path} does not exist")

    print("\n=== Renaming complete ===")
    print("Next steps:")
    print("1. Update CMakeLists.txt files")
    print("2. Update class names in source files")
    print("3. Update include statements")
    print("4. Test the build system")


if __name__ == "__main__":
    main()
