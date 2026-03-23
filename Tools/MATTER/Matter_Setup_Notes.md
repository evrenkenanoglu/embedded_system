# Matter Setup For ESP32 

## .wslConfig

https://docs.espressif.com/projects/esp-matter/en/latest/esp32s3/using_chip_tool.html

[wsl2]
networkingMode = bridged
vmSwitch = Bridge
ipv6 = true

or 

[wsl2]
networkingMode=mirrored

WARNING: After this config, usbipd should get host-ip address to attach the device.

## Enable Matter Console to pass WiFi credentials to the device

After starting matter, call -> 
```
    esp_matter::console::init();
    esp_matter::console::wifi_register_commands();
```

### Pass WiFi credentials to the device

```
    matter esp wifi connect <SSID> <PASSWORD>
    matter esp wifi connect UniverseHome Lotr_2023
```


## Disable CHIPoBLE

idf.py menuconfig -> find -> CONFIG_ENABLE_CHIPOBLE and disable it. In this way, WiFi will be commissioning method.

## Start Commissioning

```
    chip-tool pairing already-discovered <node-id> <setup-pin-code> <ip-address> <port>
    chip-tool pairing already-discovered 1 20202021 192.168.31.71 5540
```

Auto commissioning with the following command, it will automatically discover the device and pair with it. sometimes it fails.
```
chip-tool pairing onnetwork 1 20202021
```

## Test the device

```
    chip-tool onoff on 1 1
    chip-tool onoff off 1 1
    chip-tool onoff on 1 2
    chip-tool onoff off 1 2
    chip-tool onoff toggle 1 3
```


## Discover the device

```
    sudo apt install avahi-utils
    avahi-browse -rt _matterc._udp
```

```
    chip-tool pairing discover -ipv4
```