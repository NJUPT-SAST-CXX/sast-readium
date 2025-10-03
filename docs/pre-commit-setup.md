# Pre-commit Setup Guide

This document explains how to use the pre-commit hooks configured for this project to ensure code quality and consistency.

## Overview

The project uses [pre-commit](https://pre-commit.com/) to run automated checks before each commit. These hooks ensure:

- Consistent code formatting with clang-format
- CMake file quality with cmake-format and cmake-lint
- C++ static analysis with clang-tidy
- Python code formatting with black and isort
- Shell script formatting with shfmt
- Markdown documentation quality with markdownlint
- General file hygiene (no trailing whitespace, proper line endings, etc.)

## Installation

### Prerequisites

Ensure you have the following tools installed:

- **Python 3.8+** (for pre-commit and some hooks)
- **clang-format 17+** (C++ formatting)
- **clang-tidy 17+** (C++ static analysis)
- **cmake-format** (CMake formatting) - optional, will be installed by pre-commit
- **Git** (version control)

### Quick Setup

1. **Install pre-commit:**

   ```bash
   pip install pre-commit
   ```

2. **Install the hooks:**

   ```bash
   pre-commit install
   ```

3. **Install additional hook types (optional):**

   ```bash
   pre-commit install --hook-type commit-msg
   pre-commit install --hook-type pre-push
   ```

That's it! The hooks are now installed and will run automatically before each commit.

## Usage

### Automatic Usage

Once installed, pre-commit hooks run automatically when you run `git commit`. If any hook finds issues, it will:

1. **Fix issues automatically** (when possible, e.g., formatting, trailing whitespace)
2. **Report errors** that need manual fixes
3. **Prevent the commit** until issues are resolved

### Manual Usage

You can also run hooks manually on specific files or the entire codebase:

```bash
# Run on all files
pre-commit run --all-files

# Run on specific files
pre-commit run clang-format --files src/main.cpp

# Run only specific hooks
pre-commit run trailing-whitespace cmake-format
```

### Skipping Hooks

If you need to bypass hooks (not recommended):

```bash
# Skip all hooks for one commit
git commit --no-verify

# Skip specific hooks
SKIP=clang-tidy git commit
```

## Hook Configuration

### Enabled Hooks

The following hooks are configured in `.pre-commit-config.yaml`:

#### General File Hygiene

- **trailing-whitespace**: Removes trailing whitespace
- **end-of-file-fixer**: Ensures files end with newline
- **check-added-large-files**: Prevents large files (>5MB for PDF handling)
- **check-merge-conflict**: Detects merge conflict markers
- **check-yaml/check-json/check-xml/check-toml**: Validates configuration files
- **no-commit-to-branch**: Prevents commits to main/master
- **check-executables-have-shebangs**: Ensures executable scripts have proper shebangs
- **check-case-conflict**: Detects case conflicts for cross-platform compatibility

#### Code Quality

- **clang-format**: Formats C/C++ code according to project style
- **cmake-format**: Formats CMake files for consistency
- **cmake-lint**: Validates CMake file quality
- **clang-tidy**: Performs C++ static analysis
- **black**: Formats Python code
- **isort**: Sorts Python imports
- **shfmt**: Formats shell scripts
- **markdownlint**: Validates and fixes Markdown files

#### Exclusions

Some patterns are excluded from certain hooks:

- Build directories (`build/`, `vcpkg_installed/`)
- Auto-generated files (`*_autogen/`, `*.moc`, `moc_*.cpp`, `ui_*.h`, `qrc_*.cpp`)
- Test files (for clang-tidy to avoid long analysis times)
- PDF files (for file size checks)
- Log files

### Hook Versions

The configuration uses specific versions of each hook to ensure consistent behavior across all development environments. Update with:

```bash
pre-commit autoupdate
```

## Code Style Configuration

### C++ Code Style

The project uses clang-format with configuration in `.clang-format`. Key settings:

- **Based on**: Google style with customizations
- **Indentation**: 4 spaces, no tabs
- **Line length**: 80 characters
- **Pointer alignment**: Left (`int* ptr`)
- **Brace style**: Attach (`if (condition) {`)

### CMake Style

CMake files are formatted with:

- **Line width**: 100 characters
- **Tab size**: 4 spaces
- Consistent argument formatting

### Python Style

Python code follows:

- **Black formatter** with 88-character line length
- **isort** for import sorting with black profile

## Troubleshooting

### Common Issues

#### Hook Not Found

```
`check-shebangs` is not present in repository
```

This indicates an outdated hook name. Update the configuration or run:
`pre-commit autoupdate`

#### Clang-format Not Found

Ensure clang-format is installed and in your PATH:

```bash
# On Windows with MSYS2
pacman -S mingw-w64-x86_64-clang

# On macOS
brew install clang-format

# On Linux
apt-get install clang-format  # Ubuntu/Debian
dnf install clang-format     # Fedora
```

#### Clang-tidy Not Found

Install clang-tidy (usually comes with clang-format):

```bash
# Same installation commands as clang-format
```

#### CMake Format Issues

If cmake-format has issues, the hook will automatically install it in a virtual environment.

#### Permissions Issues on Windows

If you get permission errors, try:

```bash
# Run as administrator or adjust Git hooks directory permissions
git config --global core.hooksPath ~/.git/hooks
```

### Performance Optimization

For large codebases, you can:

1. **Exclude files** from certain hooks in the configuration
2. **Run hooks on specific files** instead of all files
3. **Use parallel execution** (enabled by default)
4. **Cache dependencies** (automatic with pre-commit)

### IDE Integration

#### VS Code

Install the [Pre-commit](https://marketplace.visualstudio.com/items?itemName=timonwong.shellcheck) extension for better IDE integration.

#### JetBrains IDEs

Use the [pre-commit integration](https://www.jetbrains.com/help/idea/pre-commit.html) in IDE settings.

## CI/CD Integration

The configuration includes CI-specific settings in `.pre-commit-config.yaml`:

```yaml
ci:
  autofix_commit_msg: |
    [pre-commit.ci] auto fixes from pre-commit hooks
  autofix_prs: true
  autoupdate_schedule: weekly
  skip: [clang-tidy]  # Skip slow hooks in CI
```

## Customization

### Adding New Hooks

1. Add the hook to `.pre-commit-config.yaml`
2. Run `pre-commit install` to activate
3. Test with `pre-commit run --all-files`

### Modifying Existing Hooks

Edit the hook configuration in `.pre-commit-config.yaml` and reinstall:

```bash
pre-commit uninstall
pre-commit install
```

### Project-Specific Adjustments

For project-specific needs:

- Modify file exclusions in the regex patterns
- Adjust hook arguments for your coding standards
- Update CI skip lists for performance

## Best Practices

1. **Commit frequently** with smaller changes for faster hook execution
2. **Review hook output** to understand and fix issues
3. **Keep hooks updated** with `pre-commit autoupdate`
4. **Document custom hooks** for team knowledge sharing
5. **Use IDE integration** for immediate feedback
6. **Test hooks in CI** to ensure they work across environments

## Support

For issues with:

- **pre-commit itself**: Check the [pre-commit documentation](https://pre-commit.com/)
- **Specific hooks**: Refer to each hook's documentation
- **Project configuration**: Contact the project maintainers or create an issue

## Advanced Usage

### Custom Hooks

You can create custom hooks for project-specific needs:

```yaml
repos:
  - repo: local
    hooks:
      - id: custom-check
        name: Custom Check
        entry: python scripts/custom_check.py
        language: system
        files: \.(cpp|h)$
```

### Conditional Hooks

Run hooks based on conditions:

```bash
# Only run clang-tidy on changed C++ files
git diff --name-only HEAD~1 | grep -E '\.(cpp|h)$' | xargs pre-commit run clang-tidy --files
```

### Performance Monitoring

Monitor hook execution time:

```bash
pre-commit run --all-files --verbose
```

This setup ensures consistent code quality across the entire project while maintaining developer productivity.
