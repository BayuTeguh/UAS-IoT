#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <PubSubClient.h>
#include <SimpleDHT.h>
#include <LiquidCrystal_I2C.h>

#define waterPump D5
#define buzzer D6

#define soilMoisture A0 //pin sensor LDR

const char *ssid = "Kontrakan TADIKAMESRA";    //silakan disesuaikan sendiri
const char *password = "sembarang"; //silakan disesuaikan sendiri

const char *mqtt_server = "ec2-54-157-36-1.compute-1.amazonaws.com";

WiFiClient espClient;
PubSubClient client(espClient);

SimpleDHT11 dht11(D7);

LiquidCrystal_I2C lcd(0x27, 16, 2);

long now = millis();
long lastMeasure = 0;
#define MSG_BUFFER_SIZE (50)
char msg[MSG_BUFFER_SIZE];

double soilVal = 0;

void waterPumpON()
{
  digitalWrite(waterPump, HIGH);
  delay(3000);
}

void waterPumpOFF()
{
  digitalWrite(waterPump, LOW);
}

void buzzerON()
{
  digitalWrite(buzzer, HIGH);
  delay(300);
}

void buzzerOFF()
{
  digitalWrite(buzzer, LOW);
}

void setup_wifi()
{
  delay(10);
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected - ESP IP address: ");
  Serial.println(WiFi.localIP());
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived on topic: ");
  Serial.print(topic);
  Serial.print(". Message: ");
  String messageTemp;

  for (int i = 0; i < length; i++)
  {
    Serial.print((char)payload[i]);
    messageTemp += (char)payload[i];
  }
  Serial.println();

  if (String(topic) == "room/pump")
  {
    Serial.print("Changing Water pump to ");
    if (messageTemp == "Hidup")
    {
      waterPumpON();
      buzzerON();
      Serial.print("On");
    }
    else if (messageTemp == "Mati")
    {
      waterPumpOFF();
      buzzerOFF();
      Serial.print("Off");
    }
  }
  Serial.println();
}

void reconnect()
{
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");

    // Attempt to connect
    if (client.connect("ESP8266Client", "jti", "1234"))
    {
      Serial.println("connected");
      client.subscribe("room/pump");
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);
  lcd.init(); // initialize the lcd
  lcd.backlight();
  lcd.home();
  pinMode(waterPump, OUTPUT);
  pinMode(buzzer, OUTPUT);
  waterPumpOFF();
  buzzerOFF();
  Serial.println("Mqtt Node-RED");
  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);
}

void loop()
{
  lcd.home();
  if (!client.connected())
  {
    reconnect();
  }
  if (!client.loop())
  {
    client.connect("ESP8266Client");
  }
  now = millis();
  if (now - lastMeasure > 3000)
  {
    lastMeasure = now;
    int err = SimpleDHTErrSuccess;
    
    soilVal = analogRead(soilMoisture);
    byte temperature = 0;
    byte humidity = 0;
    if ((err = dht11.read(&temperature, &humidity, NULL)) != SimpleDHTErrSuccess)
    {
      Serial.print("Pembacaan DHT11 gagal, err=");
      Serial.println(err);
      delay(1000);
      return;
    }
    static char temperatureTemp[7];
    static char humidityTemp[7];
    static char soilTemp[7];
    dtostrf(temperature, 4, 2, temperatureTemp);
    dtostrf(humidity, 4, 2, humidityTemp);
    dtostrf(soilVal, 4, 2, soilTemp);
    Serial.print("Temperature = ");
    Serial.println(temperatureTemp);
    //menampilkan suhu celcius pada lcd
    lcd.setCursor(0,0);
    lcd.print("T:");
    lcd.print(temperature);
    lcd.print((char)223);
    lcd.print("C");
    Serial.print("Humidity = ");
    Serial.println(humidityTemp);
    //menampilkan tingkat kelembapan udara pada lcd
    lcd.setCursor(8,0);
    lcd.print("H:");
    lcd.print(humidity);
    lcd.print("%");
    Serial.print("Soil Moisture = ");
    Serial.println(soilTemp);
    //menampilkan tingkat kelembapan tanah pada lcd
    lcd.setCursor(0,1);
    lcd.print("S:");
    lcd.print(soilVal);
    if (soilVal >= 600)
    {
      waterPumpON();
      buzzerON();
    }
    else
    {
      waterPumpOFF();
      buzzerOFF();
    }
    client.publish("room/temp", temperatureTemp);
    client.publish("room/humidity", humidityTemp);
    client.publish("room/soil", soilTemp);
  }
}
