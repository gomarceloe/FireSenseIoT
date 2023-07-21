# FireSenseIoT
Elaborado por: 
Maximiliano Morales maximiliano.amh@gmail.com, 
Tamara González tamara.isidora.g@gmail.com, 
Marcelo García gomarceloe@gmail.com.

Este es un proyecto que utiliza una placa modular WisBlock RAK, sensores y otros dispositivos para enviar datos a través de las redes NB-IoT.
Los dispositivos utilizados son:
- RAK19001 WisBlock Dual IO Base Board
- RAK5860 WisBlock NB-IoT Interface Module
- RAK11200 WisBlock WiFi Module, ESP32.
- Sensor de temperatura infrarroja RAK12003 WisBlock
- Sensor de temperatura y humedad RAK1901 WisBlock
- Sesor de partículas (Gas) RAK12004 WisBlock MQ2

Una demostración del funcionamiento del proyecto: https://youtu.be/YDZYobhPOIg

El código está explicado a grandes rasgos en los comentarios del mismo, y se encuentra en src/main.cpp. Es necesario instalar PlatformIO en VSCode.
Se incluye un proceso de iteraciones para reducir los errores de conexión, y mejorar el envío de datos.
Se incluye una rectificación de la medición de la temperatura, teniendo en cuenta el calentamiento de la placa debido a su funcionamiento.
