# IoT-Based Commercial Freezer Monitoring System

<div style="text-align: center;">
    <img src="Images/Iot_Based_Commercial_Freezer_Monitoring_System-Prototype.jpg" alt="Freezer Monitoring System" style="width: 70%;">
</div>

IoT Based commercial freezer monitoring system is designed to address the challenges faced by dairy item 
suppliers, laboratories, and pharmacies in ensuring the quality of stored products in freezers provided 
to sellers. This system provides real-time monitoring of freezer conditions, including 
temperature level, humidity, and pressure levels, to ensure that products are stored at optimal 
conditions.

## Features
- This system contains two main units
    1. Main Unit - Installed outside the freezer
    2. Sub Unit - Installed inside the freezer
- Use BME280 sensor to measure temperature, humidity, and pressure.
- Use ESP-NOW to transmit data between two units
- Use Sim800L module, which enables cellular communication to send data to the database when Wi-Fi is not available

## PCB Design
The PCB is designed as a two-layer board using **Altium Designer**.
<br><br>

| **PCB - Main Unit** | **PCB - Sub Unit** |
|:--:|:--:|
| <img src="Images/PCB-Internal-1.jpg" style="width: 90%;"> | <img src="Images/PCB-outer-1.jpg" style="width: 80%;"> |

<br><br>

| **Soldered PCB - Main Unit** | **Soldered PCB - Sub Unit** |
|:--:|:--:|
| <img src="Images/PCB-Internal-2.jpg" style="width: 90%;"> | <img src="Images/PCB-outer-2.jpg" style="width: 80%;"> |

Schematics and PCB Design files can be found <a href="PCB Files">here</a>.

## Enclosure Design
Enclosure is designed using **SOLIDWORKS**.

<br><br>

| **Enclosure - Main Unit** | **Enclosure - Sub Unit** |
|:--:|:--:|
| <img src="Images/Enc-Inner.jpg" style="width: 90%;"> | <img src="Images/Enc-Outer.jpg" style="width: 100%;"> |

Solidworks Design files can be found <a href="Enclosure">here</a>.

## Coding
To develop the firmware we used **ESP-IDF**.

Codes for the firmware development can be found <a href="Codes">here</a>.
