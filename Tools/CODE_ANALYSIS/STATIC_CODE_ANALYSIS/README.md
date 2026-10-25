# Embedded Systems IoT Static Code Analysis Framework

A comprehensive static code analysis framework for ESP32 embedded systems using Semgrep.

## Quick Start

```bash
# Install dependencies
pip install -r requirements.txt

# Analyze a file
python main.py --file src/main.cpp

# Analyze with specific standards
python main.py --standards misra cert iot

# List available standards
python main.py --list

# Analyze with sourcefiles, you should update sourcefiles.py
python main.py
```

## Available Standards

| Standard  | Description             | Status         |
| --------- | ----------------------- | -------------- |
| **MISRA** | C++ 2008 Compliance     | ✅ Active      |
| **CERT**  | Secure Coding           | ✅ Active      |
| **IoT**   | Security Best Practices | 🔧 Development |
| **ESP32** | Platform-specific       | 🔧 Development |

## Architecture

```
static_code_analysis/
├── main.py                 # Entry point
├── Analyzer/analyzer.py    # Core engine
├── Standards/              # Standard implementations
├── rules/                  # YAML rule files
└── reports/                # Generated reports
```

## Adding Custom Rules

1. **Create rule file** in `rules/`:

```yaml
rules:
  - id: my-rule
    pattern: dangerous_function($ARG)
    message: "Use safe_function() instead"
    languages: [c, cpp]
    severity: ERROR
```

2. **Create standard class** in `Standards/`:

```python
from .Analysis_Standard_Base import Analysis_Standard_Base

class MyStandard(Analysis_Standard_Base):
    def __init__(self, rules_dir):
        super().__init__(
            name="My Standard",
            rule_file=str(rules_dir / "my-rules.yml")
        )
```

## Report Types

- **ERROR** 🔴 - Critical issues (fix before release)
- **WARNING** 🟡 - Code quality issues (should fix)
- **INFO** 🔵 - Style suggestions (consider fixing)

## CI/CD Integration

```yaml
# .github/workflows/analysis.yml
- name: Run Static Analysis
  run: |
    cd embedded_system/Source/Scripts/static_code_analysis
    python main.py --standards misra cert
```

## References

- [Semgrep Documentation](https://semgrep.dev/docs/)
- [MISRA C++ Guidelines](https://www.misra.org.uk/)
- [CERT C++ Secure Coding](https://wiki.sei.cmu.edu/confluence/x/Wnw-BQ)

---

**Start analyzing:** `python main.py --list` 🚀
