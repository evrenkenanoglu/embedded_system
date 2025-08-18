from .Analysis_Standard_Base import Analysis_Standard_Base


class SimpleTestStandard(Analysis_Standard_Base):
    def __init__(self, script_dir):
        super().__init__(
            name="Simple Test",
            description="Basic pattern detection for testing",
            rule_file=f"{script_dir}/simple-test.yml",
            severity="INFO",
        )









