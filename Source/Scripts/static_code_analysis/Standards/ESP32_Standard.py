import Analysis_Standard_Base


class ESP32_Standard(Analysis_Standard_Base):
    def __init__(self, rules_dir):
        super().__init__(
            name="ESP32 Platform",
            description="ESP32 hardware and FreeRTOS patterns",
            rule_file=f"{rules_dir}/esp32-specific.yml",
            severity="INFO",
        )
