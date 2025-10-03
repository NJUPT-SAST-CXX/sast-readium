#!/usr/bin/env python3
"""
Test runner script to identify failing tests
"""
import os
import subprocess
import sys


def run_test(test_exe, test_function=None):
    """Run a test and return success status and output"""
    try:
        if test_function:
            cmd = [test_exe, test_function, "-txt"]
        else:
            cmd = [test_exe, "-txt"]

        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        return result.returncode == 0, result.stdout, result.stderr
    except subprocess.TimeoutExpired:
        return False, "", "TIMEOUT"
    except Exception as e:
        return False, "", str(e)


def main():
    test_dir = "build/Debug-MSYS2"
    tests = [
        "action_map_test.exe",
        "configuration_manager_test.exe",
        "event_bus_test.exe",
        "service_locator_test.exe",
        "state_manager_test.exe",
    ]

    print("=== Running tests individually ===")

    for test in tests:
        test_path = os.path.join(test_dir, test)
        if not os.path.exists(test_path):
            print(f"❌ {test}: NOT FOUND")
            continue

        print(f"\n🧪 Testing {test}...")

        # First try to list functions
        success, stdout, stderr = run_test(test_path, "-functions")
        if success:
            functions = stdout.strip().split("\n")
            print(f"   Found {len(functions)} test functions")

            # Try running a simple test first
            if functions:
                first_func = functions[0].strip()
                success, _, _ = run_test(test_path, first_func)
                if success:
                    print(f"   ✅ {first_func}: PASSED")
                else:
                    print(f"   ❌ {first_func}: FAILED")
        else:
            print(f"   ❌ Failed to list functions: {stderr}")

        # Try running the full test suite
        success, _, stderr = run_test(test_path)
        if success:
            print(f"   ✅ {test}: ALL TESTS PASSED")
        else:
            print(f"   ❌ {test}: FAILED ({stderr})")


if __name__ == "__main__":
    main()
