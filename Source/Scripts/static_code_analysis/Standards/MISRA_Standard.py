from .Analysis_Standard_Base import Analysis_Standard_Base

class Misra_Standard(Analysis_Standard_Base):
    def __init__(self, rules_dir):
        super().__init__(
            name="MISRA C++ 2008",
            description="Safety and reliability standards",
            rule_file=f"{rules_dir}/misra-cpp.yml",
            severity="WARNING",
        )
