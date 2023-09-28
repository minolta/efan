#include <Arduino.h>
#include <unity.h>
#include "Configfile.h"
#include <LITTLEFS.h>
#include <HTTPClient.h>
#include <Update.h>
#include <OneWire.h>
#include <DallasTemperature.h>
Configfile testconfig("/testconfig");
void setUp(void)
{
    // set stuff up here
}

void tearDown(void)
{
    // clean stuff up here
}
void addCfg()
{
    testconfig.addConfig("0-40", "25");
    testconfig.addConfig("41-80", "80");
    testconfig.addConfig("81", "100");
}
void testgetvalue()
{
    Configfile c("/a");
    c.openFile();
    c.addConfig("0-40", "25");
    c.addConfig("41-80", "80");
    c.addConfig("81", "100");
    int r1 = c.getIntConfig("0-40");
    int r2 = c.getIntConfig("41-80");
    int r3 = c.getIntConfig("81");
    float t = 20.5;
    int p = 0;
    if (t <= r1)
    {
        Serial.println(r1);
        p = r1;
    }
    else if (t <= r2)
    {
        Serial.println(r2);
        p = r2;
    }
    else
    {
        Serial.println(r3);
        p = r3;
    }
    TEST_ASSERT_EQUAL_INT(25, p);
}

void testDS18b20()
{
    const int oneWireBus = 32;
    // Setup a oneWire instance to communicate with any OneWire devices
    OneWire oneWire(oneWireBus);
    // Pass our oneWire reference to Dallas Temperature sensor
    DallasTemperature sensors(&oneWire);
    sensors.begin();
    sensors.requestTemperatures();
    int dc = sensors.getDeviceCount();
    float temperatureC = sensors.getTempCByIndex(0);
    float temperatureF = sensors.getTempFByIndex(0);
    Serial.printf(" Found %d %.2f %.2f\n",dc, temperatureC, temperatureF);
}

void setup()
{
    pinMode(2, OUTPUT);
    Serial.begin(115200);
    delay(2000);
    UNITY_BEGIN();
    // RUN_TEST(testgetvalue);
    RUN_TEST(testDS18b20);
    UNITY_END();
}

void loop()
{
}