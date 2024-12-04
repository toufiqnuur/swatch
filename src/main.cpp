#include <Arduino.h>
#include <ESP32Time.h>
#include <WiFi.h>
#include <Wire.h>
#include <SPI.h>
#include <QMC5883LCompass.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>

#define SSID "localhost"
#define PASS "nopassword"
#define DEFAULT_TIME_EPOCH 1609459200
#define GMT_OFFSET 25200
#define DAYLIGHT_OFFSET 0
#define NTP_SERVER "pool.ntp.org"
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
ESP32Time rtc(0);
QMC5883LCompass compass;
Adafruit_MPU6050 mpu;

float prevAccelMagnitude, accelMagnitudeDiff;
int step_count = 0;

void setup()
{
  Serial.begin(115200);

  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed"));
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  if (!mpu.begin())
  {
    Serial.println("Failed to find MPU6050 chip");
  }

  compass.setCalibrationOffsets(-679.00, -449.00, -1016.00);
  compass.setCalibrationScales(1.05, 1.07, 0.90);
  // mpu.setAccelerometerRange(MPU6050_RANGE_8_G);
  // mpu.setGyroRange(MPU6050_RANGE_500_DEG);
  // mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ);

  compass.init();

  WiFi.begin(SSID, PASS);
  delay(1000);
  if (WiFi.status() != WL_CONNECTED)
  {
    rtc.setTime(DEFAULT_TIME_EPOCH);
    Serial.print("Unable to sync time");
  }
  else
  {
    configTime(GMT_OFFSET, DAYLIGHT_OFFSET, NTP_SERVER);
    struct tm timeinfo;
    if (getLocalTime(&timeinfo))
    {
      rtc.setTimeStruct(timeinfo);
    }
  }
  delay(1000);
  WiFi.disconnect(true);
}

void loop()
{
  int16_t x1, y1, ax, ay, az;
  uint16_t width, height;
  int x, y;
  double angle;
  String bearing;

  sensors_event_t a,g, temp;
  mpu.getEvent(&a, &g, &temp);
  ax = a.acceleration.x;
  ay = a.acceleration.y;
  az = a.acceleration.z;
  long accelMagnitude = sqrt(ax * ax + ay * ay + az * az);

  compass.read();
  x = compass.getX();
  y = compass.getY();
  angle = atan2(y, x);
  float declinationAngle = 0.38;
  angle += declinationAngle;

  if (angle < 0)
    angle += 2 * PI;

  if (angle > 2 * PI)
    angle -= 2 * PI;

  float degrees = angle * 180 / PI;

  if ((degrees > 337.5) || (degrees < 22.5))
    bearing = "N";
  if ((degrees > 22.5) && (degrees < 67.5))
    bearing = "NE";
  if ((degrees > 67.5) && (degrees < 112.5))
    bearing = "E";
  if ((degrees > 112.5) && (degrees < 157.5))
    bearing = "SE";
  if ((degrees > 157.5) && (degrees < 202.5))
    bearing = "S";
  if ((degrees > 202.5) && (degrees < 247.5))
    bearing = "SW";
  if ((degrees > 247.5) && (degrees < 292.5))
    bearing = "W";
  if ((degrees > 292.5) && (degrees < 337.5))
    bearing = "NW";

  bearing = "< " + bearing;

  accelMagnitudeDiff = accelMagnitude - prevAccelMagnitude;

  if (accelMagnitude - prevAccelMagnitude > 2.0 && prevAccelMagnitude)
  {
    step_count++;
  }

  prevAccelMagnitude = accelMagnitude;

  Serial.println(accelMagnitudeDiff);

  struct tm timeinfo = rtc.getTimeStruct();

  char timeString[20];
  char dateString[20];

  strftime(timeString, sizeof(timeString), "%R", &timeinfo);
  strftime(dateString, sizeof(dateString), "%a, %d %b %y", &timeinfo);

  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S"); 

  display.clearDisplay();

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print("STEPS: ");
  display.println(step_count);

  display.setTextSize(1);
  display.getTextBounds(bearing, 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width), 0);
  display.println(bearing);

  display.setTextSize(3);
  display.getTextBounds(timeString, 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height) / 2);
  display.println(timeString);

  display.setTextSize(1);
  display.getTextBounds(dateString, 0, 0, &x1, &y1, &width, &height);
  display.setCursor((SCREEN_WIDTH - width) / 2, (SCREEN_HEIGHT - height));
  display.println(dateString);

  display.display();

  delay(1000);
}
