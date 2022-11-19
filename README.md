# OASAelectric by Bluedrive team

OASAelectric IOT box, is the hardware - sofware LoRaWAN telemetry solution for OASA micromobility proposal for Open Data Hackathon 2022. 
This box sends GPS position of vehicle, the main battery status and can alert OASA services in case of mechanical failure. Telemetry data also incude vehicle ID, and client Ath.ENA NFC card ID.
 
- Arduino board based
- GPS UBLOX M8 module (  GPS - Glonass - Gallileo capable  )
- Sensor of 13V battery
- GPIO on / off alarm 
- LoRaWAN module EBYTE E220-900T22D 868 MHz
- NFC card reader
- OLED display for bus and metro timetables and announcements ( to be implemented )


## Harware 

Connecting the LoRa module

Arduino Pins -------- LoRa Pins


Pin 2		——>		RX

Pin 3		——>		TX

Pin 4		——>		M0  

Pin 5		——>		M1  

Pin 6		——>		AX   

5V		——>		VCC

GND		——>		GND


Connecting the GPS module


Arduino Pins -------- GPS Pins


Pin 7		——>		TX

Pin 8		——>		RX

5V		——>		VCC

GND		——>		GND


## Author
    Chris Papathanasiou developer@drmac.gr
    
    
## License


This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details


## Acknowledgments

    This code is initialy written for OpenData Hackathon https://crowdhackathon.com/open-data/ Bluedrive Team entry

## To Do

    - Add NFC card reader
    - Add ability to receive data from OASA API and display bus and metro timetable live on TFT display of EV

## Links
