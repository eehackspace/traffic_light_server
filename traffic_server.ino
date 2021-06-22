/*  Connects to the home WiFi network
 *  Asks some network parameters
 *  Starts WiFi server with fix IP and listens
 *  Receives and sends messages to the client
 *  Communicates: traffic_client.ino
 */
#include <SPI.h>
#include <ESP8266WiFi.h>

bool debugMode = false; // Debug will let you put in light values in the Serial Monitor and see the result in the output

int GREEN_RELAY = D2;
int AMBER_RELAY = D3;
int RED_RELAY = D4;
int AMBER_DWELL_TIME = 2900;
int POO_TIME_SECONDS = 60 * 4;
int AIR_OUT_AMBER_SECONDS = 60;

int currentLight = -1;
bool currentlyOccupied = false;
unsigned long startTime = 0;

char ssid[] = "Hackspace_2_4";            // SSID of your home WiFi
char pass[] = "xxxx_x_xxx";               // password of your home WiFi
WiFiServer server(80);                    
IPAddress ip(192, 168, 0, 80);            // IP address of the server
IPAddress gateway(192,168,0,1);           // gateway of your network
IPAddress subnet(255,255,255,0);          // subnet mask of your network

void setup() 
{
  Serial.begin(115200);                   // only for debug

  // Sets pins to output
  pinMode(GREEN_RELAY, OUTPUT);
  pinMode(AMBER_RELAY, OUTPUT);
  pinMode(RED_RELAY, OUTPUT);
  
  showLights(true, true, true);

  if (!debugMode)
  {
    WiFi.disconnect();                      // Because the wemos logs AP information to memory and that removes the last AP information.
  
    WiFi.config(ip, gateway, subnet);       // forces to use the fix IP
    WiFi.begin(ssid, pass);                 // connects to the WiFi router
    Serial.println("Connecting to wifi");
    
    while (WiFi.status() != WL_CONNECTED) {
      Serial.print(".");
      delay(500);
    }
      
    Serial.println("Connected");
    server.begin();                         // starts the server
  }
  
  showLights(false, false, true);
}

void printLights(bool red, bool amber, bool green)
{
  Serial.println("---");
  if (red)
    Serial.println("(x)");
  else
    Serial.println("( )");
  if (amber)
    Serial.println("(x)");
  else
    Serial.println("( )");
  if (green)
    Serial.println("(x)");
  else
    Serial.println("( )");
  Serial.println("---");
}

void showLights(bool red, bool amber, bool green)
{
  digitalWrite(RED_RELAY, red ? HIGH : LOW);
  digitalWrite(AMBER_RELAY, amber ? HIGH : LOW);
  digitalWrite(GREEN_RELAY, green ? HIGH : LOW);
  
  if (debugMode)
    printLights(red, amber, green);
}

void setOccupied(bool occupied)
{
  if (currentlyOccupied == occupied)
    return;

  if (occupied)
  {
    startTime = millis();
    showLights(false, true, false); // Amber
    delay(AMBER_DWELL_TIME);
    showLights(true, false, false); // Red
  }
  else
  {
    Serial.print("You spent ");
    unsigned long totalTimeSeconds = (millis() - startTime) / 1000;
    Serial.print(totalTimeSeconds);
    Serial.println(" seconds in the toilet");
    
    
    if (totalTimeSeconds > POO_TIME_SECONDS)
    {
      Serial.print("Long time taken, amber alert");
      for (int i = 0; i < AIR_OUT_AMBER_SECONDS; i++)
      {
        showLights(false, true, false);
        delay(500);
        showLights(false, false, false);
        delay(500);
      }
    }
    else
    {
      showLights(true, true, false); // Red/Amber
      delay(AMBER_DWELL_TIME);
    }
    showLights(false, false, true); // Green
  }
  
  currentlyOccupied = occupied;
}

void interpretLightValue(float lightValue)
{
  if(lightValue > 15 )
  {
    setOccupied(true);
    Serial.println("Toilet occupied");
  }
  else if(lightValue > 1)
  {
    setOccupied(false);
    Serial.println("Toilet unoccupied");
  }
  else
  {
    Serial.println("Erroneous reading, ignore!"); 
  }
}

void loop()
{
  if (debugMode)
    debugLoop();
  else
    wifiLoop();
}

void debugLoop()
{
    String request = Serial.readStringUntil('\r');
    float value = request.toFloat();     
    interpretLightValue(value);
}

void wifiLoop() {
  WiFiClient client = server.available();
  if (client) {
    if (client.connected()) 
    {
      Serial.println(".");
      String request = client.readStringUntil('\r');    // receives the message from the client
      float lightValue = request.toFloat();
      client.flush();
      interpretLightValue(lightValue);
    }
    client.stop();                // tarminates the connection with the client
  }  
}
