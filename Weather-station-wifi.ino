#include <Adafruit_BMP280.h>
#include <Adafruit_BME680.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <RTClib.h>
 //#include <VirtualWire.h>

#define batt_in 34
#define rx_pin 2

/*-------------------- Sensori ------------------*/
RTC_DS3231 rtc;
Adafruit_BME680 bme;
Adafruit_BMP280 bmp;

/*------------------ Variabili globali -------------------*/

static const char daysOfTheWeek[7][12] = {
    "Domenica",
    "Lunedì",
    "Martedì",
    "Mercoledì",
    "Giovedì",
    "Venerdì",
    "Sabato"
};
DateTime currentTime;
float voltage;
int vPercent;
float voltage_ext = 0;
int vPercent_ext = 0;
float temp;
float temp_ext = 0;
float humidity;
float humidity_ext = 0;
float pressure;
float pressure_ext = 0;
float airIndex = 0;

void setup() {

    Serial.begin(115200);
    Wire.begin(); //I2C start
    adcAttachPin(batt_in);

    /*------------------- Inizializzazione sensori ---------------------*/
    if (!rtc.begin()) Serial.println("Couldn't find RTC");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    if (!bmp.begin(0x76)) Serial.println("Couldn't find BMP");

    pinMode(rx_pin, INPUT);
    //vw_set_rx_pin(rx_pin);
    //vw_setup(3000);
    /*------------------------------------------------------------------*/
}

void readBattery() {
    const byte nReadings = 64;
    float voltage_reading = 0;

    for (byte x = 0; x < nReadings; x++)
        voltage_reading += analogRead(batt_in);

    /** 5.1 maximum USB voltage
     * r1 (to device) 1Mohm --- r2 (to gnd) 1Mohm
     * In case of not equal resistors the read value 
     * is scaled down by a factor of  r1/(r1+r2)
     * so multiply per 1/attenuation (now 2)
     * Sperimental offset in reading of 0.505V
     */
    voltage = ((voltage_reading / 64) * 3.3 / 4095 * 2) + 0.505;

    /** Map looks to not work   vPercent = map(voltage, 3.1 , 4.20, 0, 100);
     * Minimum voltage 3.1V (as 0%)
     * Maximum voltage 4.2V (100%)
     * Percent value can rise up to 100% while charging
     * That's the desired behaviour to detect the "charging rate"
     */
    if (voltage >= 3.1)					  //4.2-3.1
         vPercent = (voltage - 3.1) * 100 / 1.1;
    else vPercent = 0;
}


void getTime() {
    currentTime = rtc.now();

    Serial.print((String) currentTime.day() + "/" + (String) currentTime.month() + "/" + (String) currentTime.year() +
        "  " + (String) daysOfTheWeek[currentTime.dayOfTheWeek()] + "  " + (String) currentTime.hour() +
        ":" + (String) currentTime.minute() + ":" + (String) currentTime.second());
    //Misura circa 1°C in eccesso
    Serial.println("  RTC-Temp: " + (String)(rtc.getTemperature() - 1));
}


void ambientMeasurement() {
    temp = bmp.readTemperature();
    pressure = bmp.readPressure() / 100;
}


/*void getExternalRF(){
  uint8_t buf[VW_MAX_MESSAGE_LEN];
  uint8_t buflen = VW_MAX_MESSAGE_LEN;
  if (vw_get_message(buf, &buflen)) // Non-blocking
  {
    buf[buflen]='\0'; // Metto il terminatore di stringa
          if (strncmp((char *)buf,"LED_ON",buflen)==0)
              digitalWrite(LED_pin, HIGH); // Accendo il LED
          else if (strncmp((char *)buf,"LED_OFF",buflen)==0)
              digitalWrite(LED_pin,LOW); // Spengo il LED
      }
}*/


void printToSerial() {
    /*--------------------------------------------------------------------------------*/
    Serial.print("Voltage: " + (String) voltage + "V");
    Serial.println("\t" + (String) vPercent + "%");
    /*--------------------------------------------------------------------------------*/
    Serial.println("Temperature: " + (String) temp + " °C");
    Serial.println("Pressure: " + (String) pressure + " hPa");
    Serial.println();
    Serial.println("----------------------------------------");
    /*--------------------------------------------------------------------------------*/
}


void loop() {

    readBattery();
    getTime();
    ambientMeasurement();
    //getExternalRF();
	Serial.println("VALORE: "+(String) analogRead(batt_in));
    printToSerial();
    delay(2500);
}