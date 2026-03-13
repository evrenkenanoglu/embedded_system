import pytest
import pytest_embedded
import pytest_embedded_idf
import pytest_embedded_serial
import pytest_embedded_serial_esp

# To run this test manually -> 
# pytest -s --log-cli-level=INFO tests/hil/test_serial.py --port=COM3 --target=esp32 --embedded-services esp,serial

def test_port_open_close(dut: pytest_embedded.Dut):
    dut.expect("WiFi Credentials retrieved from memory:", timeout=10)
    dut.close()
