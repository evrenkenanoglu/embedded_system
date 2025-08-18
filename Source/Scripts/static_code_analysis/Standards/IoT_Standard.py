import Analysis_Standard_Base


class IoT_Standard(Analysis_Standard_Base):
    def __init__(self, rules_dir):
        super().__init__(
            name="IoT Security",
            description="IoT and embedded security practices",
            rule_file=f"{rules_dir}/iot-security.yml",
            severity="WARNING",
        )
