# ESP32 IoT Static Code Analysis

Comprehensive static code analysis setup for ESP32 IoT embedded systems with MISRA C++, CERT, and IoT security standards compliance.

## Features

✅ **MISRA C++ 2008** compliance checking
✅ **CERT C++ Secure Coding** standards
✅ **IoT Security** best practices (OWASP IoT Top 10)
✅ **ESP32-specific** patterns and optimizations
✅ **Security vulnerabilities** detection
✅ **Code quality** metrics

## Quick Start

### 1. Install Dependencies

```bash
cd embedded_system/Source/Scripts/static_code_analysis
pip install -r requirements.txt
```

### 2. Run Analysis

```bash
# Windows
run_analysis.bat

# Or directly with Python
python analyze.py
```

### 3. View Results

Reports are generated in the `reports/` directory:

- `analysis-report.txt` - Human-readable detailed report
- `comprehensive-analysis.json` - Machine-readable JSON format

## Coding Standards Checked

### MISRA C++ 2008 Rules

- **2-10-6**: Safe string functions (sprintf → snprintf)
- **5-0-3**: Explicit C++ casts instead of C-style casts
- **6-4-2**: Switch statements must have default clause
- **8-4-4**: Single function definition rule
- **15-1-2**: Exception safety and memory management
- **18-4-1**: Proper malloc/free pairing

### CERT C++ Secure Coding

- **STR31-C**: Bounded string operations
- **MEM30-C**: Memory leak prevention
- **MEM31-C**: Double-free prevention
- **INT32-C**: Integer overflow protection
- **FIO30-C**: No hardcoded credentials
- **ENV33-C**: Safe system calls
- **ERR33-C**: Return value checking

### IoT Security Standards

- **Credential Management**: No hardcoded WiFi/API keys
- **Encryption**: Strong cryptographic algorithms only
- **Communication**: HTTPS/TLS enforcement
- **Data Validation**: Sensor input validation
- **Debug Security**: No sensitive data in logs

### ESP32-Specific Checks

- **FreeRTOS**: Task stack sizes, ISR safety
- **Memory**: Heap allocation checking, DMA alignment
- **GPIO**: Interrupt handler IRAM placement
- **Power**: Deep sleep configuration
- **Peripherals**: ADC calibration, NVS handle management

## Integration with Your Code

### Error Handling Macros

Your `error_macros.h` has been updated for compliance:

```cpp
// MISRA compliant - uses const and safe casts
#define RETURN_ON_ERROR(expr, ...)
#define ON_ERROR_WITH_LOG(expr, message)
```

### Suppressing False Positives

Add comments to suppress specific rules:

```cpp
// semgrep: ignore
strcpy(dest, src);  // Legacy code, will be fixed in v2.0
```

## Customization

### Adding Custom Rules

Create new `.yml` files in the `rules/` directory:

```yaml
rules:
  - id: my-custom-rule
    pattern: dangerous_function($ARG)
    message: "Use safe_function() instead"
    languages: [c, cpp]
    severity: ERROR
```

### Configuring Severity Levels

Edit the main analysis script to filter by severity:

```python
"--severity=ERROR"  # Only show errors
"--severity=WARNING"  # Show warnings and errors  
"--severity=INFO"   # Show all findings
```

## CI/CD Integration

### GitHub Actions

```yaml
- name: Static Code Analysis
  run: |
    cd embedded_system/Source/Scripts/static_code_analysis
    pip install -r requirements.txt
    python analyze.py
    # Fail build if critical errors found
```

### Pre-commit Hook

```bash
#!/bin/bash
cd embedded_system/Source/Scripts/static_code_analysis
python analyze.py
if [ $? -ne 0 ]; then
    echo "Static analysis failed - commit rejected"
    exit 1
fi
```

## Report Interpretation

### Severity Levels

- 🔴 **ERROR**: Must fix before release (security, safety)
- 🟡 **WARNING**: Should fix (quality, best practices)
- 🔵 **INFO**: Consider fixing (style, optimization)

### Priority Order

1. **Security vulnerabilities** (hardcoded credentials, buffer overflows)
2. **MISRA violations** (safety-critical for embedded systems)
3. **Memory issues** (leaks, double-free, bounds checking)
4. **ESP32-specific** (performance, reliability)
5. **Code quality** (style, maintainability)

## Troubleshooting

### Common Issues

**"Semgrep not found"**

```bash
pip install semgrep
```

**"Rules file not found"**
Ensure you're running from the correct directory:

```bash
cd embedded_system/Source/Scripts/static_code_analysis
```

**"Too many false positives"**
Adjust severity level or add suppressions:

```bash
python analyze.py --severity=ERROR
```

### Performance Tips

- Run analysis on changed files only for faster feedback
- Use CI/CD for full project analysis
- Focus on ERROR and WARNING levels during development

## Standards References

- [MISRA C++ 2008](https://www.misra.org.uk/)
- [CERT C++ Secure Coding](https://wiki.sei.cmu.edu/confluence/pages/viewpage.action?pageId=88046682)
- [OWASP IoT Top 10](https://owasp.org/www-project-internet-of-things/)
- [ESP32 Programming Guide](https://docs.espressif.com/projects/esp-idf/en/latest/)

---

**Next Steps**: Review the generated report and start fixing issues by priority level!
