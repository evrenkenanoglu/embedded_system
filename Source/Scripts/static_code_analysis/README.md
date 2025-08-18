
# ESP32 IoT Static Code Analysis Framework

A comprehensive, extensible static code analysis framework designed for ESP32 IoT embedded systems. This framework provides automated code quality checking, security vulnerability detection, and compliance verification for multiple coding standards.

## 🎯 Overview

This framework leverages **Semgrep** as the core analysis engine and provides a modular architecture for implementing various coding standards and security checks. It's designed to be easily extensible and customizable for different project requirements.

### Key Features

✅ **Multi-Standard Support** - MISRA C++, CERT, IoT Security, ESP32-specific
✅ **Modular Architecture** - Easy to add new standards and rules
✅ **Multiple Output Formats** - Console, JSON, HTML reports
✅ **Extensible Rule System** - Custom YAML rule definitions
✅ **CI/CD Integration** - Ready for automated workflows
✅ **Configurable Severity** - Filter by ERROR, WARNING, INFO levels

## 🏗️ Architecture

### Framework Components

```
static_code_analysis/
├── analyze.py              # Main entry point
├── Analyzer/               # Core analysis engine
│   └── analyzer.py         # Semgrep wrapper and orchestration
├── Standards/              # Coding standards implementations
│   ├── Analysis_Standard_Base.py    # Base class for all standards
│   ├── Simple_Test_Standard.py      # Example/test standard
│   ├── CERT_Standard.py             # CERT C++ secure coding
│   ├── MISRA_Standard.py            # MISRA C++ compliance
│   ├── IoT_Standard.py              # IoT security best practices
│   └── ESP32_Standard.py            # ESP32-specific patterns
├── rules/                  # YAML rule definitions
│   ├── simple-test.yml     # Basic pattern testing
│   ├── cert-cpp.yml        # CERT security rules
│   ├── misra-cpp.yml       # MISRA compliance rules
│   ├── iot-security.yml    # IoT security patterns
│   └── esp32-specific.yml  # ESP32 hardware/software patterns
├── reports/                # Generated analysis reports
└── requirements.txt        # Python dependencies
```

### Design Principles

1. **Modularity**: Each coding standard is implemented as a separate class
2. **Extensibility**: Easy to add new standards without modifying core code
3. **Configurability**: Rule files are external YAML configurations
4. **Separation of Concerns**: Analysis engine separate from standards logic

## 🚀 Quick Start

### Prerequisites

- Python 3.7+
- Semgrep (installed via pip)

### Installation

```bash
# Clone or navigate to the project directory
cd embedded_system/Source/Scripts/static_code_analysis

# Install dependencies
pip install -r requirements.txt
```

### Basic Usage

```bash
# Analyze a single file
python analyze.py --file src/main.cpp

# Analyze with specific standards
python analyze.py --standards misra cert --file src/main.cpp

# Analyze all project files (requires sourcefiles.py configuration)
python analyze.py

# List available standards
python analyze.py --list

# Windows batch script (comprehensive analysis)
run_analysis.bat
```

## 📋 Available Standards

### Built-in Standards

| Standard         | Description                        | Rules File             | Status         |
| ---------------- | ---------------------------------- | ---------------------- | -------------- |
| **Simple** | Basic pattern testing and examples | `simple-test.yml`    | ✅ Active      |
| **CERT**   | CERT C++ Secure Coding Standards   | `cert-cpp.yml`       | 🔧 Development |
| **MISRA**  | MISRA C++ 2008 Compliance          | `misra-cpp.yml`      | 🔧 Development |
| **IoT**    | IoT Security Best Practices        | `iot-security.yml`   | 🔧 Development |
| **ESP32**  | ESP32-specific Patterns            | `esp32-specific.yml` | 🔧 Development |

### Standard Categories

#### 🛡️ Security Standards

- **CERT C++**: Memory safety, input validation, secure APIs
- **IoT Security**: Credential management, encryption, secure communication

#### 📐 Compliance Standards

- **MISRA C++**: Safety-critical embedded systems compliance
- **Industry**: Automotive, aerospace, medical device standards

#### ⚡ Platform-Specific

- **ESP32**: FreeRTOS patterns, GPIO handling, power management
- **Embedded**: Memory constraints, real-time considerations

## 🔧 Configuration

### Adding New Standards

1. **Create Standard Class** in `Standards/` directory:

```python
from .Analysis_Standard_Base import Analysis_Standard_Base

class MyCustomStandard(Analysis_Standard_Base):
    def __init__(self, rules_dir):
        super().__init__(
            name="My Custom Standard",
            description="Custom coding standard",
            rule_file=str(rules_dir / "my-custom.yml"),
            severity="WARNING"
        )
```

2. **Create Rule File** in `rules/` directory:

```yaml
# my-custom.yml
rules:
  - id: my-custom-rule
    pattern: dangerous_function($ARG)
    message: "Use safe_function() instead of dangerous_function()"
    languages: [c, cpp]
    severity: ERROR
```

3. **Register Standard** in `analyze.py`:

```python
from Standards.MyCustomStandard import MyCustomStandard

def create_available_standards():
    standards = []
    standards.append(MyCustomStandard(rules_dir))
    return standards
```

### Customizing Rules

Edit YAML files in the `rules/` directory to:

- Add new patterns to detect
- Modify severity levels
- Update messages and descriptions
- Add language-specific rules

### Output Configuration

The framework supports multiple output formats:

- **Console**: Real-time colored output with progress
- **JSON**: Machine-readable results in `reports/results_timestamp.json`
- **Text**: Detailed human-readable reports

## 📊 Report Interpretation

### Severity Levels

| Level             | Icon | Description                               | Action Required    |
| ----------------- | ---- | ----------------------------------------- | ------------------ |
| **ERROR**   | 🔴   | Critical issues, security vulnerabilities | Fix before release |
| **WARNING** | 🟡   | Code quality, best practices              | Should fix         |
| **INFO**    | 🔵   | Style, optimization suggestions           | Consider fixing    |

### Common Issue Categories

1. **Security**: Buffer overflows, hardcoded credentials, weak crypto
2. **Memory Safety**: Leaks, double-free, bounds checking
3. **Compliance**: MISRA violations, coding standard deviations
4. **Quality**: Dead code, complexity, maintainability
5. **Performance**: Inefficient patterns, resource usage

## 🔄 CI/CD Integration

### GitHub Actions Example

```yaml
name: Static Code Analysis
on: [push, pull_request]

jobs:
  code-analysis:
    runs-on: ubuntu-latest
    steps:
    - uses: actions/checkout@v3
    - name: Setup Python
      uses: actions/setup-python@v4
      with:
        python-version: '3.9'
    - name: Install dependencies
      run: |
        cd embedded_system/Source/Scripts/static_code_analysis
        pip install -r requirements.txt
    - name: Run analysis
      run: |
        cd embedded_system/Source/Scripts/static_code_analysis
        python analyze.py --standards cert misra iot
```

### Pre-commit Hook

```bash
#!/bin/bash
cd embedded_system/Source/Scripts/static_code_analysis
python analyze.py --standards cert misra
if [ $? -ne 0 ]; then
    echo "❌ Static analysis failed - commit rejected"
    exit 1
fi
```

## 🛠️ Extending the Framework

### Custom Analysis Logic

Implement pre/post analysis hooks in your standard:

```python
class CustomStandard(Analysis_Standard_Base):
    def pre_analysis(self, target_file):
        """Called before Semgrep analysis"""
        print(f"Preparing analysis for {target_file}")
  
    def post_analysis(self, target_file, results, reports_dir):
        """Called after Semgrep analysis"""
        # Custom result processing
        self.generate_custom_report(results, reports_dir)
```

### Advanced Rule Patterns

Use Semgrep's advanced pattern matching:

```yaml
rules:
  - id: complex-pattern
    patterns:
      - pattern: |
          if ($COND) {
            ...
            free($PTR);
            ...
            free($PTR);
          }
    message: "Potential double-free vulnerability"
    severity: ERROR
```

## 📚 Best Practices

### Development Workflow

1. **Start with Simple Standard** - Test framework setup
2. **Add Relevant Standards** - CERT for security, MISRA for safety
3. **Customize Rules** - Adjust for project-specific needs
4. **Integrate Early** - Add to development workflow
5. **Iterate and Improve** - Refine rules based on results

### Performance Tips

- **Incremental Analysis**: Run on changed files only during development
- **Severity Filtering**: Focus on ERROR and WARNING levels initially
- **Rule Optimization**: Profile and optimize slow rules
- **Parallel Processing**: Use multiple standards simultaneously

## 🔍 Troubleshooting

### Common Issues

**Module Import Errors**

```bash
# Ensure you're in the correct directory
cd embedded_system/Source/Scripts/static_code_analysis
python analyze.py
```

**Semgrep Not Found**

```bash
pip install semgrep
semgrep --version
```

**Rule File Not Found**

- Check file paths in standard constructors
- Verify YAML syntax in rule files
- Ensure rules directory contains expected files

**No Results Generated**

- Verify target files exist and have appropriate extensions
- Check rule patterns match your code syntax
- Enable debug output for more information

## 📖 References

- [Semgrep Documentation](https://semgrep.dev/docs/)
- [MISRA C++ Guidelines](https://www.misra.org.uk/)
- [CERT C++ Secure Coding](https://wiki.sei.cmu.edu/confluence/x/Wnw-BQ)
- [OWASP IoT Security](https://owasp.org/www-project-internet-of-things/)
- [ESP32 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)

---

## 🤝 Contributing

To add new standards or improve existing ones:

1. Fork the repository
2. Create a feature branch
3. Implement your standard following the existing patterns
4. Add comprehensive rule definitions
5. Test with sample code
6. Submit a pull request

## 📄 License

This static code analysis framework is part of the ESP32 IoT project and follows the same licensing terms.

---

**Ready to start?** Run `python analyze.py --list` to see available standards and begin your code analysis journey! 🚀
