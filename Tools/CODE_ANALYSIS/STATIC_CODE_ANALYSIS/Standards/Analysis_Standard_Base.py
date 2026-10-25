from pathlib import Path


class Analysis_Standard_Base:
    """Base class for all coding standards"""

    def __init__(self, name, description, rule_file, severity="INFO"):
        self.name = name
        self.description = description
        self.rule_file = rule_file
        self.severity = severity
        self.is_active = False
        self.findings = []

    def is_available(self):
        """Check if rule file exists"""
        return Path(self.rule_file).exists()

    def get_config(self):
        """Get configuration for this standard"""
        return {
            "rule_file": self.rule_file,
            "severity": self.severity,
            "name": self.name,
        }

    def print_status(self):
        """Print status of this standard"""
        status = "✅" if self.is_available() else "❌"
        active = "🔵" if self.is_active else "⚪"
        print(f"   {active} {status} {self.name}: {self.description}")
