# WSL SETUP


## USB Passthrough for ESP32

To enable USB passthrough for ESP32 devices in WSL (Windows Subsystem for Linux), follow these steps:

On Windows:
- usbipd list
- usbipd bind --busid <busid>
- usbipd attach --wsl --busid <busid>

usbipd list && usbipd bind --busid 11-3 && usbipd attach --wsl --busid 11-3 && usbipd list


On WSL:
sudo usermod -aG dialout $USER
ls -l /dev/ttyUSB*
sudo chmod 666 /dev/ttyUSB*


