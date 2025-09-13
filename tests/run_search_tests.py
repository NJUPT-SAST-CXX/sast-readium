#!/usr/bin/env python3
"""
Comprehensive Test Runner for Enhanced PDF Search Engine
Runs all search-related tests and generates detailed reports
"""

import os
import sys
import subprocess
import time
import json
from pathlib import Path
from typing import Dict, List, Tuple, Optional

class SearchTestRunner:
    def __init__(self, build_dir: str = "build/Debug-MSYS2"):
        self.build_dir = Path(build_dir)
        self.test_dir = self.build_dir / "tests"
        self.results = {}
        self.start_time = time.time()
        
    def run_test(self, test_name: str, test_category: str) -> Tuple[bool, str, float]:
        """Run a single test and return (success, output, duration)"""
        test_path = self.test_dir / test_category / f"{test_name}.exe"
        
        if not test_path.exists():
            return False, f"Test executable not found: {test_path}", 0.0
        
        print(f"Running {test_name}...")
        start_time = time.time()
        
        try:
            # Set environment for headless testing
            env = os.environ.copy()
            env["QT_QPA_PLATFORM"] = "offscreen"
            
            result = subprocess.run(
                [str(test_path)],
                capture_output=True,
                text=True,
                timeout=300,  # 5 minute timeout
                env=env
            )
            
            duration = time.time() - start_time
            success = result.returncode == 0
            output = result.stdout + result.stderr
            
            return success, output, duration
            
        except subprocess.TimeoutExpired:
            duration = time.time() - start_time
            return False, "Test timed out after 5 minutes", duration
        except Exception as e:
            duration = time.time() - start_time
            return False, f"Test execution failed: {str(e)}", duration
    
    def run_all_search_tests(self) -> Dict:
        """Run all search-related tests"""
        test_suites = {
            "Core Search Functionality": [
                ("test_search_engine_core", "unit"),
            ],
            "Advanced Search Features": [
                ("test_search_advanced_features", "unit"),
            ],
            "Performance and Caching": [
                ("test_search_performance_caching", "unit"),
                ("test_search_optimizations", "performance"),
            ],
            "Edge Cases and Error Handling": [
                ("test_search_edge_cases", "unit"),
            ],
            "Integration Tests": [
                ("test_search_integration", "integration"),
            ]
        }
        
        all_results = {}
        total_tests = 0
        passed_tests = 0
        
        for suite_name, tests in test_suites.items():
            print(f"\n{'='*60}")
            print(f"Running {suite_name}")
            print(f"{'='*60}")
            
            suite_results = {}
            suite_passed = 0
            suite_total = 0
            
            for test_name, category in tests:
                success, output, duration = self.run_test(test_name, category)
                
                suite_results[test_name] = {
                    "success": success,
                    "output": output,
                    "duration": duration,
                    "category": category
                }
                
                suite_total += 1
                total_tests += 1
                
                if success:
                    suite_passed += 1
                    passed_tests += 1
                    print(f"  ✓ {test_name} ({duration:.2f}s)")
                else:
                    print(f"  ✗ {test_name} ({duration:.2f}s)")
                    print(f"    Error: {output[:200]}...")
            
            all_results[suite_name] = {
                "tests": suite_results,
                "passed": suite_passed,
                "total": suite_total,
                "success_rate": (suite_passed / suite_total * 100) if suite_total > 0 else 0
            }
            
            print(f"\nSuite Summary: {suite_passed}/{suite_total} tests passed ({suite_passed/suite_total*100:.1f}%)")
        
        # Overall summary
        total_duration = time.time() - self.start_time
        overall_success_rate = (passed_tests / total_tests * 100) if total_tests > 0 else 0
        
        summary = {
            "total_tests": total_tests,
            "passed_tests": passed_tests,
            "failed_tests": total_tests - passed_tests,
            "success_rate": overall_success_rate,
            "total_duration": total_duration,
            "suites": all_results
        }
        
        return summary
    
    def generate_report(self, results: Dict, output_file: str = "search_test_report.json"):
        """Generate detailed test report"""
        report_path = Path(output_file)
        
        with open(report_path, 'w') as f:
            json.dump(results, f, indent=2)
        
        print(f"\nDetailed report saved to: {report_path}")
        
        # Generate human-readable summary
        self.print_summary(results)
    
    def print_summary(self, results: Dict):
        """Print human-readable test summary"""
        print(f"\n{'='*80}")
        print("ENHANCED PDF SEARCH ENGINE TEST SUMMARY")
        print(f"{'='*80}")
        
        print(f"Total Tests: {results['total_tests']}")
        print(f"Passed: {results['passed_tests']}")
        print(f"Failed: {results['failed_tests']}")
        print(f"Success Rate: {results['success_rate']:.1f}%")
        print(f"Total Duration: {results['total_duration']:.2f} seconds")
        
        print(f"\n{'Test Suite Results:':<30} {'Passed':<8} {'Total':<8} {'Rate':<8}")
        print("-" * 60)
        
        for suite_name, suite_data in results['suites'].items():
            passed = suite_data['passed']
            total = suite_data['total']
            rate = suite_data['success_rate']
            print(f"{suite_name:<30} {passed:<8} {total:<8} {rate:<7.1f}%")
        
        # Show failed tests
        failed_tests = []
        for suite_name, suite_data in results['suites'].items():
            for test_name, test_data in suite_data['tests'].items():
                if not test_data['success']:
                    failed_tests.append((suite_name, test_name, test_data['output'][:100]))
        
        if failed_tests:
            print(f"\n{'Failed Tests:'}")
            print("-" * 60)
            for suite, test, error in failed_tests:
                print(f"  {suite} -> {test}")
                print(f"    Error: {error}...")
        
        print(f"\n{'='*80}")
    
    def run_performance_benchmarks(self):
        """Run performance-specific benchmarks"""
        print(f"\n{'='*60}")
        print("Running Performance Benchmarks")
        print(f"{'='*60}")
        
        benchmark_tests = [
            ("test_search_performance_caching", "unit"),
            ("test_search_optimizations", "performance")
        ]
        
        for test_name, category in benchmark_tests:
            success, output, duration = self.run_test(test_name, category)
            
            if success:
                print(f"✓ {test_name} completed in {duration:.2f}s")
                # Extract performance metrics from output if available
                self.extract_performance_metrics(output)
            else:
                print(f"✗ {test_name} failed: {output[:100]}...")
    
    def extract_performance_metrics(self, output: str):
        """Extract performance metrics from test output"""
        lines = output.split('\n')
        for line in lines:
            if 'benchmark' in line.lower() or 'performance' in line.lower():
                if any(keyword in line for keyword in ['ms', 'seconds', 'time', 'speed']):
                    print(f"    Metric: {line.strip()}")
    
    def check_prerequisites(self) -> bool:
        """Check if all prerequisites are met"""
        if not self.build_dir.exists():
            print(f"Error: Build directory not found: {self.build_dir}")
            print("Please build the project first using CMake")
            return False
        
        if not self.test_dir.exists():
            print(f"Error: Test directory not found: {self.test_dir}")
            print("Please build the tests first")
            return False
        
        return True

def main():
    """Main entry point"""
    import argparse
    
    parser = argparse.ArgumentParser(description="Run Enhanced PDF Search Engine Tests")
    parser.add_argument("--build-dir", default="build/Debug-MSYS2", 
                       help="Build directory path")
    parser.add_argument("--output", default="search_test_report.json",
                       help="Output report file")
    parser.add_argument("--benchmarks-only", action="store_true",
                       help="Run only performance benchmarks")
    parser.add_argument("--verbose", action="store_true",
                       help="Verbose output")
    
    args = parser.parse_args()
    
    runner = SearchTestRunner(args.build_dir)
    
    if not runner.check_prerequisites():
        sys.exit(1)
    
    try:
        if args.benchmarks_only:
            runner.run_performance_benchmarks()
        else:
            print("Starting Enhanced PDF Search Engine Test Suite...")
            print(f"Build directory: {runner.build_dir}")
            print(f"Test directory: {runner.test_dir}")
            
            results = runner.run_all_search_tests()
            runner.generate_report(results, args.output)
            
            # Run benchmarks if all tests passed
            if results['success_rate'] > 90:
                runner.run_performance_benchmarks()
            
            # Exit with error code if tests failed
            if results['failed_tests'] > 0:
                sys.exit(1)
                
    except KeyboardInterrupt:
        print("\nTest execution interrupted by user")
        sys.exit(1)
    except Exception as e:
        print(f"Error running tests: {e}")
        sys.exit(1)

if __name__ == "__main__":
    main()
