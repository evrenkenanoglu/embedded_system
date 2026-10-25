from .Analysis_Standard_Base import Analysis_Standard_Base


class Cert_Standard(Analysis_Standard_Base):
    def __init__(self, rules_dir):
        super().__init__(
            name="CERT C++ Secure Coding",
            description="Security coding standards",
            rule_file=f"{rules_dir}/cert-cpp.yml",
            severity="ERROR",
        )
