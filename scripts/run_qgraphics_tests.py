#!/usr/bin/env python3
"""
Automated test script for QGraphics PDF support
Runs tests in both enabled and disabled configurations
"""

import os
import sys
import subprocess
import json
import time
from pathlib import Path
from typing import Dict, List, Optional, Tuple


class QGraphicsTestRunner:
    def __init__(self, project_root: str):
        self.project_root = Path(project_root)
        self.build_dir = self.project_root / "build"
        self.test_results = {}
        self.cmake_available = self._check_cmake()
        self.xmake_available = self._check_xmake()

    def _check_cmake(self) -> bool:
        """Check if CMake is available"""
        try:
            subprocess.run(["cmake", "--version"], capture_output=True, check=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False

    def _check_xmake(self) -> bool:
        """Check if xmake is available"""
        try:
            subprocess.run(["xmake", "--version"], capture_output=True, check=True)
            return True
        except (subprocess.CalledProcessError, FileNotFoundError):
            return False

    def clean_build_directory(self):
        """Clean the build directory"""
        if self.build_dir.exists():
            print(f"Cleaning build directory: {self.build_dir}")
            if os.name == "nt":  # Windows
                subprocess.run(["rmdir", "/s", "/q", str(self.build_dir)], shell=True)
            else:  # Unix-like
                subprocess.run(["rm", "-rf", str(self.build_dir)])

        self.build_dir.mkdir(exist_ok=True)

    def build_with_cmake(self, enable_qgraphics: bool) -> bool:
        """Build project using CMake"""
        if not self.cmake_available:
            print("CMake not available, skipping CMake build")
            return False

        print(f"Building with CMake (QGraphics: {'ON' if enable_qgraphics else 'OFF'})")

        # Configure
        configure_cmd = [
            "cmake",
            f"-DENABLE_QGRAPHICS_PDF_SUPPORT={'ON' if enable_qgraphics else 'OFF'}",
            "..",
        ]

        try:
            result = subprocess.run(
                configure_cmd,
                cwd=self.build_dir,
                capture_output=True,
                text=True,
                check=True,
            )
            print("CMake configuration successful")
        except subprocess.CalledProcessError as e:
            print(f"CMake configuration failed: {e}")
            print(f"stdout: {e.stdout}")
            print(f"stderr: {e.stderr}")
            return False

        # Build
        build_cmd = ["cmake", "--build", ".", "--config", "Release"]

        try:
            result = subprocess.run(
                build_cmd,
                cwd=self.build_dir,
                capture_output=True,
                text=True,
                check=True,
            )
            print("CMake build successful")
            return True
        except subprocess.CalledProcessError as e:
            print(f"CMake build failed: {e}")
            print(f"stdout: {e.stdout}")
            print(f"stderr: {e.stderr}")
            return False

    def build_with_xmake(self, enable_qgraphics: bool) -> bool:
        """Build project using xmake"""
        if not self.xmake_available:
            print("xmake not available, skipping xmake build")
            return False

        print(f"Building with xmake (QGraphics: {enable_qgraphics})")

        # Configure
        config_cmd = [
            "xmake",
            "config",
            f"--enable_qgraphics_pdf={str(enable_qgraphics).lower()}",
        ]

        try:
            result = subprocess.run(
                config_cmd,
                cwd=self.project_root,
                capture_output=True,
                text=True,
                check=True,
            )
            print("xmake configuration successful")
        except subprocess.CalledProcessError as e:
            print(f"xmake configuration failed: {e}")
            print(f"stdout: {e.stdout}")
            print(f"stderr: {e.stderr}")
            return False

        # Build
        build_cmd = ["xmake", "build"]

        try:
            result = subprocess.run(
                build_cmd,
                cwd=self.project_root,
                capture_output=True,
                text=True,
                check=True,
            )
            print("xmake build successful")
            return True
        except subprocess.CalledProcessError as e:
            print(f"xmake build failed: {e}")
            print(f"stdout: {e.stdout}")
            print(f"stderr: {e.stderr}")
            return False

    def run_test_executable(self, test_name: str, test_path: str) -> Dict:
        """Run a test executable and capture results"""
        if not Path(test_path).exists():
            return {
                "name": test_name,
                "status": "SKIP",
                "reason": "Executable not found",
                "output": "",
                "duration": 0,
            }

        print(f"Running test: {test_name}")
        start_time = time.time()

        try:
            result = subprocess.run(
                [test_path],
                capture_output=True,
                text=True,
                timeout=300,  # 5 minute timeout
            )

            duration = time.time() - start_time

            return {
                "name": test_name,
                "status": "PASS" if result.returncode == 0 else "FAIL",
                "returncode": result.returncode,
                "output": result.stdout,
                "error": result.stderr,
                "duration": duration,
            }

        except subprocess.TimeoutExpired:
            return {
                "name": test_name,
                "status": "TIMEOUT",
                "reason": "Test timed out after 5 minutes",
                "output": "",
                "duration": 300,
            }
        except Exception as e:
            return {
                "name": test_name,
                "status": "ERROR",
                "reason": str(e),
                "output": "",
                "duration": time.time() - start_time,
            }

    def run_tests_for_configuration(
        self, enable_qgraphics: bool, build_system: str
    ) -> Dict:
        """Run all tests for a specific configuration"""
        config_name = (
            f"{build_system}_qgraphics_{'enabled' if enable_qgraphics else 'disabled'}"
        )
        print(f"\n=== Running tests for configuration: {config_name} ===")

        # Clean and build
        self.clean_build_directory()

        if build_system == "cmake":
            build_success = self.build_with_cmake(enable_qgraphics)
        elif build_system == "xmake":
            build_success = self.build_with_xmake(enable_qgraphics)
        else:
            print(f"Unknown build system: {build_system}")
            return {"config": config_name, "build_success": False, "tests": []}

        if not build_success:
            return {"config": config_name, "build_success": False, "tests": []}

        # Define test executables to run
        test_executables = []

        if build_system == "cmake":
            test_base_dir = self.build_dir / "tests"
            test_executables = [
                ("unit_tests", test_base_dir / "unit" / "test_qgraphics_components"),
                (
                    "integration_tests",
                    test_base_dir / "integration" / "test_rendering_mode_switch",
                ),
                (
                    "performance_tests",
                    test_base_dir / "performance" / "test_rendering_performance",
                ),
                (
                    "real_world_tests",
                    test_base_dir / "real_world" / "test_real_pdf_documents",
                ),
            ]

        # Add .exe extension on Windows
        if os.name == "nt":
            test_executables = [
                (name, Path(str(path) + ".exe")) for name, path in test_executables
            ]

        # Run tests
        test_results = []
        for test_name, test_path in test_executables:
            result = self.run_test_executable(test_name, str(test_path))
            test_results.append(result)

            # Print immediate feedback
            status_symbol = (
                "✓"
                if result["status"] == "PASS"
                else "✗"
                if result["status"] == "FAIL"
                else "⚠"
            )
            print(
                f"  {status_symbol} {test_name}: {result['status']} ({result.get('duration', 0):.2f}s)"
            )

        return {
            "config": config_name,
            "build_success": True,
            "qgraphics_enabled": enable_qgraphics,
            "build_system": build_system,
            "tests": test_results,
        }

    def run_all_tests(self) -> Dict:
        """Run tests for all configurations"""
        print("Starting comprehensive QGraphics PDF support testing")
        print(f"Project root: {self.project_root}")
        print(f"CMake available: {self.cmake_available}")
        print(f"xmake available: {self.xmake_available}")

        all_results = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "project_root": str(self.project_root),
            "configurations": [],
        }

        # Test configurations
        configurations = []

        if self.cmake_available:
            configurations.extend(
                [
                    (False, "cmake"),  # QGraphics disabled
                    (True, "cmake"),  # QGraphics enabled
                ]
            )

        if self.xmake_available:
            configurations.extend(
                [
                    (False, "xmake"),  # QGraphics disabled
                    (True, "xmake"),  # QGraphics enabled
                ]
            )

        if not configurations:
            print("ERROR: No build systems available (CMake or xmake required)")
            return all_results

        # Run tests for each configuration
        for enable_qgraphics, build_system in configurations:
            try:
                config_results = self.run_tests_for_configuration(
                    enable_qgraphics, build_system
                )
                all_results["configurations"].append(config_results)
            except Exception as e:
                print(
                    f"Error running configuration {build_system} with QGraphics {enable_qgraphics}: {e}"
                )
                all_results["configurations"].append(
                    {
                        "config": f"{build_system}_qgraphics_{'enabled' if enable_qgraphics else 'disabled'}",
                        "build_success": False,
                        "error": str(e),
                        "tests": [],
                    }
                )

        return all_results

    def generate_report(self, results: Dict):
        """Generate a comprehensive test report"""
        print("\n" + "=" * 80)
        print("QGRAPHICS PDF SUPPORT TEST REPORT")
        print("=" * 80)

        total_configs = len(results["configurations"])
        successful_builds = sum(
            1
            for config in results["configurations"]
            if config.get("build_success", False)
        )

        print(f"Test run timestamp: {results['timestamp']}")
        print(f"Project root: {results['project_root']}")
        print(f"Configurations tested: {total_configs}")
        print(f"Successful builds: {successful_builds}")

        # Summary by configuration
        for config in results["configurations"]:
            print(f"\n--- {config['config']} ---")

            if not config.get("build_success", False):
                print("  Build: FAILED")
                if "error" in config:
                    print(f"  Error: {config['error']}")
                continue

            print("  Build: SUCCESS")

            tests = config.get("tests", [])
            if not tests:
                print("  Tests: No tests run")
                continue

            passed = sum(1 for test in tests if test["status"] == "PASS")
            failed = sum(1 for test in tests if test["status"] == "FAIL")
            skipped = sum(1 for test in tests if test["status"] == "SKIP")
            errors = sum(1 for test in tests if test["status"] == "ERROR")
            timeouts = sum(1 for test in tests if test["status"] == "TIMEOUT")

            print(
                f"  Tests: {len(tests)} total, {passed} passed, {failed} failed, {skipped} skipped, {errors} errors, {timeouts} timeouts"
            )

            # Show failed tests
            failed_tests = [
                test for test in tests if test["status"] in ["FAIL", "ERROR", "TIMEOUT"]
            ]
            if failed_tests:
                print("  Failed tests:")
                for test in failed_tests:
                    print(f"    - {test['name']}: {test['status']}")
                    if test.get("reason"):
                        print(f"      Reason: {test['reason']}")

        # Overall summary
        print(f"\n{'='*80}")
        print("OVERALL SUMMARY")
        print(f"{'='*80}")

        all_tests = []
        for config in results["configurations"]:
            all_tests.extend(config.get("tests", []))

        if all_tests:
            total_tests = len(all_tests)
            total_passed = sum(1 for test in all_tests if test["status"] == "PASS")
            total_failed = sum(1 for test in all_tests if test["status"] == "FAIL")

            print(f"Total tests run: {total_tests}")
            print(f"Total passed: {total_passed}")
            print(f"Total failed: {total_failed}")
            print(f"Success rate: {(total_passed/total_tests)*100:.1f}%")

        # Save detailed results to file
        report_file = self.project_root / "test_results.json"
        with open(report_file, "w") as f:
            json.dump(results, f, indent=2)

        print(f"\nDetailed results saved to: {report_file}")

        # Return overall success
        return successful_builds == total_configs and all(
            test["status"] == "PASS"
            for config in results["configurations"]
            for test in config.get("tests", [])
        )


def main():
    if len(sys.argv) > 1:
        project_root = sys.argv[1]
    else:
        project_root = os.getcwd()

    runner = QGraphicsTestRunner(project_root)
    results = runner.run_all_tests()
    success = runner.generate_report(results)

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
