#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BMP280.h>
#include <Adafruit_HDC1000.h>
#include <RTClib.h>

#include <TFT_eSPI.h>
#include <User_Setup_Select.h>
#include <User_Setup.h>

#include <WiFi.h>
#include <HTTPClient.h>


static const byte batt_in = 34;

/*-------------------- Sensori ------------------*/
RTC_DS3231 rtc;
Adafruit_BMP280 bmp;
Adafruit_HDC1000 hdc;
/*------------------ Variabili globali -------------------*/

const char * ssid = "ESP-sensor";
const char * password = "esp8266sensor";
static
const int TIME_TO_NEXT_READ = 10000 + 100; //TIME_TO_SLEEP in "ext_sensor.ino"

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
float voltage_ext = NULL;
int vPercent_ext = NULL;
float temp;
float temp_ext = NULL;
float humidity;
float humidity_ext = NULL;
float pressure;
float pressure_ext = NULL;
float airTVOC = NULL;
String airQuality = "NULL";
/*-------------------------------------------------------*/

void setup() {
    Serial.begin(115200);
    Wire.begin(); //I2C start

    adcAttachPin(batt_in);

    /*--------------------- Sensor initializing ----------------------*/
    if (!rtc.begin()) Serial.println("Couldn't find RTC");
    rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));

    if (!bmp.begin(0x76))   Serial.println("Couldn't find BMP");
    /** Weather/Climate-monitor  Calibration */
    bmp.setSampling(Adafruit_BMP280::MODE_FORCED, 
                    Adafruit_BMP280::SAMPLING_X1, // temperature 
                    Adafruit_BMP280::SAMPLING_X1, // pressure
                    Adafruit_BMP280::FILTER_OFF ); 
    
    if(! hdc.begin())     Serial.println("Couldn't find HDC");

    /*------------------------------------------------------------------*/
}


int battPercentage(float v) {
    /** Map seems to not work   vPercent = map(voltage, 3.1 , 4.20, 0, 100);
     * Minimum voltage 3.1V (as 0%)
     * Maximum voltage 4.2V (100%)
     */
    if (v >= 3.1) //4.2-3.1
        return (v - 3.1) * 100 / 1.1;
    else return 0;
}


void readBattery() {

    const byte nReadings = 64;
    float voltage_reading = 0;

    /** Multiple readings to get a stabilised value */
    for (byte x = 0; x < nReadings; x++)
        voltage_reading += analogRead(batt_in);

    /** 5.1 maximum voltage to measure (USB)
     * r1 (to device) 1Mohm --- r2 (to gnd) 1Mohm
     * In case of not equal resistors the read value 
     * is scaled down by a factor of  r2/(r1+r2)
     * so multiply per 1/attenuation (now 2)
     * Sperimental offset in reading of 0.52V
     */
    voltage = (voltage_reading / nReadings) * 3.3 / 4095 * 2 ) + 0.52;
    vPercent = battPercentage(voltage);
}


void getTime() {
    currentTime = rtc.now();

    Serial.print((String) currentTime.day() + "/" + (String) currentTime.month() + "/" + (String) currentTime.year() +
        "  " + (String) daysOfTheWeek[currentTime.dayOfTheWeek()] + "  " + (String) currentTime.hour() +
        ":" + (String) currentTime.minute() + ":" + (String) currentTime.second());
}


void ambientMeasurement() {
    bmp.MODE_FORCED;
    temp = bmp.readTemperature() - 1.389; //BMP self heating;
    pressure = bmp.readPressure() / 100;    //hPa
    humidity = hdc.readHumidity();
    bmp.MODE_SLEEP;
}


String httpGETRequest(const char * serverName) {
    HTTPClient http;
    http.begin(serverName);

    if (http.GET() > 0) //contains the ResponseCode
        return http.getString();    //contains the Payload
    else {
        Serial.println("Bad request; code: " + http.GET());
        return "";
    }
    http.end(); // Free resources
}


void getExtSensor() {
    /*--------------------- Initialization -----------------*/
    WiFi.begin(ssid, password);
    Serial.print("Connecting: ");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.print("\n External sensor WIFI connected: ");
    Serial.println(WiFi.localIP());
    /*------------------------------------------------------*/

    static const String gateway = WiFi.gatewayIP().toString();
    Serial.println("Gateway: " + gateway);

    voltage_ext = httpGETRequest(("http://" + gateway + "/volt").c_str()).toFloat();
    vPercent_ext = battPercentage(voltage_ext);
    temp_ext = httpGETRequest(("http://" + gateway + "/temp").c_str()).toFloat();
    humidity_ext = httpGETRequest(("http://" + gateway + "/hum").c_str()).toFloat();
    pressure_ext = httpGETRequest(("http://" + gateway + "/press").c_str()).toFloat();
    airTVOC = httpGETRequest(("http://" + gateway + "/air").c_str()).toFloat();

    /**Doesn't repeat the connection attempt until the ext_sensor
     * will be ready to answer the requests. 
     * It goes in deepSleep mode after a cycle of requests (to implement)
     * This is a syncronization constant => TIME_TO_SLEEP in the "ext_sensor.ino"
     **/
}


void printToSerial() {
    /*--------------------------------------------------------------------------------*/
    Serial.print("Voltage: ");
    Serial.print(voltage, 3);
    Serial.print("V");
    Serial.println("\t" + (String) vPercent + "%");
    /*--------------------------------------------------------------------------------*/
    Serial.println("Temperature BMP: " + (String) temp + " °C");
    Serial.println("Temperature HDC: " + (String) hdc.readTemperature() + " °C");
    Serial.println("Temperature RTC: " + (String) (rtc.getTemperature() - 1) + " °C");  //About 1°C in excess
    Serial.println("Pressure: " + (String) pressure + " hPa");
    Serial.println("Humidity: " + (String) humidity + " %RH");
    Serial.println();
    /*--------------------------------------------------------------------------------*/
    Serial.println("EXT_Voltage: " + (String) voltage_ext + " V\t" + (String) vPercent_ext + "%");
    Serial.println("EXT_Temperature: " + (String) temp_ext + " °C");
    Serial.println("EXT_Pressure: " + (String) pressure_ext + " hPa");
    Serial.println("EXT_Humidity: " + (String) humidity_ext + " %RH");
    Serial.println("Air Quality: " + (String) airTVOC + " %RH");
    Serial.println("\n-----------------------------------------");
}


void loop() {

    readBattery();
    getTime();
    ambientMeasurement();
    //getExtSensor();

    printToSerial();
    delay(4000);
}