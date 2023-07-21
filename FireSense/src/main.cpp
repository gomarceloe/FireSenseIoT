#include <Arduino.h>
#include <StarterKitNB.h>
#include <SparkFun_SHTC3.h>
#include <Wire.h>
#include "ADC121C021.h"
#include "SparkFun_MLX90632_Arduino_Library.h"

#define PIN_VBAT WB_A0

#define VBAT_MV_PER_LSB (0.73242188F) // 3.0V ADC range and 12 - bit ADC resolution = 3000mV / 4096
#define VBAT_DIVIDER_COMP (1.73)      // Compensation factor for the VBAT divider, depend on the board (por default 1.73); 1905
#define REAL_VBAT_MV_PER_LSB (VBAT_DIVIDER_COMP * VBAT_MV_PER_LSB)

//Sensor de gas
#define EN_PIN WB_IO6	 //Logic high enables the device. Logic low disables the device
#define ALERT_PIN WB_IO5 //a high indicates that the respective limit has been violated.
#define MQ2_ADDRESS 0x51 //the device i2c address

#define RatioMQ2CleanAir (1.0) //RS / R0 = 1.0 ppm
#define MQ2_RL (10.0)	
ADC121C021 MQ2;

//Sensor de temperatura
SHTC3 mySHTC3;  // Variable que guarda los datos del sensor
StarterKitNB sk;  // Variable para la funcionalidad del Starter Kit

//Sensor Infrarrojo
#define MLX90632_ADDRESS 0x3A
MLX90632 RAK_TempSensor;

// NB
String apn = "m2m.entel.cl";
String user = "entelpcs";
String pw = "entelpcs";

String band = "B28 LTE";

// Thingsboard
String clientId = "grupo1";
String userName = "11111";
String password = "11111";
String msg = "";
String resp = "";
String Operador = "";
String Banda = "";
String Red = "";



void errorDecoder(SHTC3_Status_TypeDef message)             // Función que facilita la detección de errores
{
  switch(message)
  {
    case SHTC3_Status_Nominal : Serial.print("Nominal"); break;
    case SHTC3_Status_Error : Serial.print("Error"); break;
    case SHTC3_Status_CRC_Fail : Serial.print("CRC Fail"); break;
    default : Serial.print("Unknown return code"); break;
  }
}


void setup()
{
  Serial.println("setup");
  sk.Setup();
  delay(500);
  sk.UserAPN(apn,user,pw);
  delay(500);
  Wire.begin();
  sk.Connect(apn, band);
  Serial.print("Beginning sensor. Result = ");            
  errorDecoder(mySHTC3.begin());                         // Detectar errores
  Serial.println("\n\n");

//Sensor de gas

	pinMode(ALERT_PIN, INPUT);
	pinMode(EN_PIN, OUTPUT);
	digitalWrite(EN_PIN, HIGH); //power on RAK12004
	delay(500);
	time_t timeout = millis();
	Serial.begin(115200);
	while (!Serial)
	{
	if ((millis() - timeout) < 5000)
	{
	delay(100);
	}
	else
	{
	break;
	}
	}

	//********ADC121C021 ADC convert init ********************************
	while (!(MQ2.begin(MQ2_ADDRESS, Wire)))
	{
	Serial.println("please check device!!!");
	delay(200);
	}
	Serial.println("RAK12004 test Example");

	//*******config ADC121C021 *******************************************
	MQ2.configCycleTime(CYCLE_TIME_32); //set ADC121C021 Conversion Cycle Time
	MQ2.configAlertHold(Disable);		//set ADC121C021 alerts  self-clear
	MQ2.configAlertFlag(Disable);		//Disables ADC121C021 alert status bit [D15] in the Conversion Result register.
	MQ2.configAlertPin(Enable);			//Enables the ALERT output pin of ADC121C021
	MQ2.configPolarity(High);			//Sets ADC121C021 ALERT pin to active high
	MQ2.setAlertLowThreshold(1);		//set ADC121C021 Alert low Limit the register default value is 0
	MQ2.setAlertHighThreshold(2500);	//set ADC121C021 Alert high Limit when adc value over 3000 alert PIN output level to high

	//**************init MQ2********************************************
	MQ2.setRL(MQ2_RL);
	MQ2.setA(-0.890);			//A -> Slope, -0.685
	MQ2.setB(1.125);			//B -> Intersect with X - Axis  1.019
						//Set math model to calculate the PPM concentration and the value of constants
	MQ2.setRegressionMethod(0); //PPM =  pow(10, (log10(ratio)-B)/A)


	float calcR0 = 0;
	for (int i = 1; i <= 100; i++)
	{
		calcR0 += MQ2.calibrateR0(RatioMQ2CleanAir);
	}
	MQ2.setR0(calcR0 / 10);
	if (isinf(calcR0))
	{
		Serial.println("Warning: Conection issue founded, R0 is infite (Open circuit detected) please check your wiring and supply");
		while (1)
			;
	}
	if (calcR0 == 0)
	{
		Serial.println("Warning: Conection issue founded, R0 is zero (Analog pin with short circuit to ground) please check your wiring and supply");
		while (1)
			;
	}
	Serial.printf("R0 Value is:%3.2f\r\n", MQ2.getR0());


  //Sensor infrarrojo CONSULTAR CON MARCELO SI ES QUE ES NECESARIO REPETIR ESTE CÓDIGO
  TwoWire &wirePort = Wire;
  MLX90632::status returnError;
    Serial.begin(115200);
    while (!Serial)
    {
      if ((millis() - timeout) < 5000)
      {
        delay(100);
      }
      else
      {
        break;
      }
    }  

  Wire.begin(); //I2C init

  if (RAK_TempSensor.begin(MLX90632_ADDRESS, wirePort, returnError) == true) //MLX90632 init 
  {
     Serial.println("MLX90632 Init Succeed");
  }
  else
  {
     Serial.println("MLX90632 Init Failed");
  }



	}

//////////// Correción de error en la temperatura ////////////////////;
// Durante los primeros 2400 segundos (40 minutos) el sensor de temperatura mide T°Ambiente + T° placa;
// Este error alcanza su máxima y permanece constante al minuto 40, con 7.5°C de error;
// En base a exp realizados, se calculó que la ecuación del error (T° placa) es de 1,0078*ln(x)-0.8294;
// X representa múltiplos de 10 sumados con 1, es decir, X = DelayLoopsg()*j + 1;
// Donde DelayLoopsg()= DelayLoopmili()/1000 se mide en segundos;
// La ecuación X solo será válida hasta la iteración j = 26, luego de eso se mantendrá en un valor constante igual a 7.5 °C;
int j = 0; // variable que indica el número de iteración del loop();
int iteraciones = 15; // Variable para guardar cantidad de iteraciones a trabajar en el FOR (por defecto 15);
int DelayFormili = 10000; // Delay dentro del For;
float DelayLoopmili = DelayFormili*iteraciones; // Delay total que hay entro del loop;
// DelayLoopmili = Delay_For_total + DelayEspera; (por default DelayFor = 10000*iteraciones y DelayEspera=0);
float DelayLoopsg = DelayLoopmili/1000; // se divide por 1000 para cambiar a segundos;
  // Variables
float sensorPPM;          // Variable de niveles de gas
float batery0;            // Variable Auxiliar de bateria
float batery;             // Variable del nivel de bateria
float initialHumedity;    // Variable para almacenar la temperatura inicial en cada ciclo
float temperature_placa;  // Variable temperatura de la placa
float object_temp = 0;    // Temperatura del objeto que detecta el sensor infrarrojo


void loop()
{
  if (!sk.ConnectionStatus()) // Si no hay conexion a NB
  {
   sk.Reconnect(apn, band);  // Se intenta reconectar
   delay(1000);
  }
  Serial.print("loop");
  sk.ConnectBroker(clientId, userName, password);  // Se conecta a ThingsBoard
  Serial.print("Iteración_Loop_n°:" + String(j)); 
  delay(2000);

  resp = sk.bg77_at((char *)"AT+COPS?", 500, false);
  Operador = resp.substring(resp.indexOf("\""), resp.indexOf("\"",resp.indexOf("\"")+1)+1);     // ENTEL PCS
  delay(1000);

  resp = sk.bg77_at((char *)"AT+QNWINFO", 500, false);
  Red = resp.substring(resp.indexOf("\""), resp.indexOf("\"",resp.indexOf("\"")+1)+1);          // NB o eMTC
  Banda = resp.substring(resp.lastIndexOf("\"",resp.lastIndexOf("\"")-1), resp.lastIndexOf("\"")+1); 
  delay(1000);
	Serial.println("1");
  SHTC3_Status_TypeDef result = mySHTC3.update();             // Se actualiza el sensor de temperatura
	uint8_t PINstatus = digitalRead(ALERT_PIN);
	if (PINstatus)
	{
		Serial.println("value over AlertHighThreshold !!!");
	}

for (int i = 0; i < iteraciones; i++) {
  if (mySHTC3.lastStatus == SHTC3_Status_Nominal) {
    sensorPPM = MQ2.readSensor() / 1000; // Se divide por 100 para llevarlo a valores comunes
    batery0 = (analogRead(PIN_VBAT) * REAL_VBAT_MV_PER_LSB);
    batery = batery0 / 37;
    mySHTC3.update();
    object_temp = (RAK_TempSensor.getObjectTempF()-32)/1.8; // Obtiene la temperatura del objeto que está mirando

    //****************************************Temperatura_placa_variando*******************************************//;
    if (j<=26){
    temperature_placa = 1.0078*log(DelayLoopsg*j+1)-0.8294 ; // Función de correción para la temperatura;
    if (i == 0) {
      initialHumedity = mySHTC3.toPercent(); // Almacenar el valor de humedad inicial;
    }

    // Resta entre la temperatura actual y la temperatura inicial
    float temperature_actual = mySHTC3.toDegC()- temperature_placa; // temperatura_actual corregida
    float humedad_actual = mySHTC3.toPercent();
    float dfhumedity = humedad_actual - initialHumedity;
    msg = "{\"humidity_g1\":" + String(mySHTC3.toPercent()) +
          ",\"temperature_g1\":" + String(temperature_actual) +
          ",\"battery_g1\":" + String(batery) +
          ",\"ppm_g1\":" + String(sensorPPM) +
          ",\"DFHume_g1\":" + String(dfhumedity) + 
          ",\"object_temp_g1\":" + String(object_temp) + "}";

    Serial.println(msg);
    Serial.println("prueba");
    sk.SendMessage(msg);
    }
    
    //****************************************Temperatura_placa_cte*******************************************//;
    else {
    Serial.println("################## Se ha terminado de corregir el error de Temperatura ##################");
    temperature_placa = 7.5;
    if (i == 0) {
      initialHumedity = mySHTC3.toPercent(); // Almacenar el valor de humedad inicial
    }

    // Resta entre la temperatura actual y la temperatura inicial
    float temperature_actual = mySHTC3.toDegC()- temperature_placa; // temperatura_actual corregida
    float humedad_actual = mySHTC3.toPercent();
    float dfhumedity = humedad_actual - initialHumedity;
    msg = "{\"humidity_g1\":" + String(mySHTC3.toPercent()) +
          ",\"temperature_g1\":" + String(temperature_actual) +
          ",\"battery_g1\":" + String(batery) +
          ",\"ppm_g1\":" + String(sensorPPM) +
          ",\"DFHume_g1\":" + String(dfhumedity) + 
          ",\"object_temp_g1\":" + String(object_temp) + "}";

    Serial.println(msg);
    sk.SendMessage(msg);
    }
    }
    else {
    batery0 = (analogRead(PIN_VBAT) * REAL_VBAT_MV_PER_LSB);
    batery = batery0 / 37;
    msg = "{\"humidity_g1\":\"ERROR\", \"temperature_g1\":\"ERROR\",\"battery_g1\":" +
          String(batery) + ",\"object_temp_g1\":\"ERROR\"" +
          ",\"Operador\":" + Operador +
          ",\"Banda de frecuencia\":" + Banda +
          ",\"Red IoT\":" + Red + "}";

    Serial.println(msg);
    sk.SendMessage(msg);
      }
    
    Serial.println("Temperatura teórica de la placa:" + String(temperature_placa));
    if (i < iteraciones-1) {
      Serial.println("Siguiente iteración n°" + String(i+1));
    }
    delay(1000); // Aquí se podría poner un delay de 10sg antes de empezar la siguiente iteración del FOR!!!
    }
  j++; //aumenta el valor de j:= calcula cuantas iteraciones del loop se han realizado
  Serial.println("######################### Reiniciando código... #########################");
//	delay(5000);                                        // Tiempo entre envio de datos y el reinicio del código (5 sg) (si considera sk.Disconnect, se trata del tiempo entre envio de datos y desconect)
//  sk.DisconnectBroker();
//  delay(1200000);                                         // Tiempo de espera para cada entrega de datos en milisegundos, por defecto, dejar en 10000 (10 segundos)
}