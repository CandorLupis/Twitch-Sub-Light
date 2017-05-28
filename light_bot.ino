/*
   Twitch subscriber notification light
   Powered by Adafruit WICED Feather Arduino
   More detailed info and guides can be found on the
   adafruit site. Heavy conversion required for use
   with standard ESP8266 based arduino Wifi modules.
*/

#include <adafruit_feather.h>
#include <adafruit_constants.h>
#include <adafruit_http.h>
#include <adafruit_featherap.h>
#include <adafruit_http_server.h>
#include "certificates.h"
#include <TimeLib.h>

#define SERVER                  "api.twitch.tv"
#define PAGE                    "/kraken/streams/********"
#define HTTPS_PORT              443
#define CHANNEL                 "channel"
#define USER_AGENT_HEADER       "curl/7.45.0"
#define lcd Serial2

int ledPin = PA15;
long previousMills1 = 0;
long previousMills2 = 0;

// Use the HTTP class
AdafruitHTTP http;

void receive_callback(void)
{
  String sentence = "";
  // If there are incoming bytes available from the server, read then print them:
  while ( http.available() ) {
    char c = http.read();
    while (http.available() && c != '\n') {
      sentence = sentence + c;
      c = http.read();
    }
  }
  int index = sentence.indexOf("{");
  sentence = sentence.substring(index);
  Serial.println(">>" + sentence);
  //check if stream is online
  if (sentence.indexOf("stream") > -1) {
    streamInfo(sentence);
  }
}

void disconnect_callback(void)
{
  Serial.println();
  Serial.println("---------------------");
  Serial.println("DISCONNECTED CALLBACK");
  Serial.println("---------------------");
  Serial.println();
}

void setup()
{
  Serial.begin(115200);
  lcdStartup();
  char* ssid = NULL;
  //check for saved wifi profile launch AP if none found
  for (int x = 0; x < 5 && ssid == NULL; x++) {
    ssid = Feather.profileSSID(x);
    Serial.println(ssid);
  }
  if (ssid == NULL)
    runServer();

  for (int x = 0; x < 3 && !connectAP(); x++)
  {
    delay(500); // Small delay between each attempt
  }
  if (!connectAP())
    runServer();
  stopServer();

  //RGB pins
  pinMode(PA1, OUTPUT);
  pinMode(PB4, OUTPUT);
  pinMode(PC5, OUTPUT);

  // Add custom RootCA since target server is not covered by default list
  Feather.addRootCA(rootca_certs, ROOTCA_CERTS_LEN);

  // Tell the HTTP client to auto print error codes and not halt on errors
  http.err_actions(true, false);

  // Set up callbacks
  http.setReceivedCallback(receive_callback);
  http.setDisconnectCallback(disconnect_callback);

  // Start a secure connection
  Serial.printf("Connecting to %s port %d ... ", SERVER, HTTPS_PORT );
  http.connectSSL(SERVER, HTTPS_PORT);
  Serial.println("OK");

  // Make a HTTP request
  http.addHeader("User-Agent", USER_AGENT_HEADER);
  http.addHeader("Accept", "application/vnd.twitchtv.v5+json");
  http.addHeader("Client-ID", "id-number here");
  http.addHeader("Connection", "keep-alive");

  connectIRC(CHANNEL);

  startClock();
  lcd.write(0xFE); lcd.write(0x58);

  requestStatus();
  editSetting("color magenta");
  setNighttime("22,8");
  lightEffects();
}

void loop()
{
  unsigned long currentMills = millis();
  readFromServer();
  lightEffects();
  if (currentMills - previousMills1 > 200) {
    updateDisplay();
    previousMills1 = currentMills;
  }
  //check stream status every 10 minutes
  if (currentMills - previousMills2 > 600000) {
    requestStatus();
    previousMills2 = currentMills;
  }
  delay(10);
}

//reconnect to twitch API
void requestStatus() {
  if ( !http.connected() ) {
    Serial.printf("Connecting to %s port %d ... ", SERVER, HTTPS_PORT );
    http.connectSSL(SERVER, HTTPS_PORT); // Will halt if an error occurs
    Serial.println("OK");

    // Make a HTTP request
    http.addHeader("User-Agent", USER_AGENT_HEADER);
    http.addHeader("Accept", "application/vnd.twitchtv.v5+json");
    http.addHeader("Client-ID", "id-number here");
    http.addHeader("Connection", "keep-alive");
  }
  if (http.connected()) {
    Serial.printf("Requesting '%s' ... ", PAGE);
    http.get(SERVER, PAGE);
    Serial.println("OK");
  }
}

bool connectAP(void)
{
  lcd.write(0xFE); lcd.write(0x58);
  delay(10);
  // Attempt to connect to an AP
  lcd.write(0xFE); lcd.write(0x48);
  lcd.println("Connecting to:");
  lcd.print(Feather.profileSSID(0));
  delay(500);
  lcd.write(0xFE); lcd.write(0x58);
  delay(10);
  lcd.write(0xFE); lcd.write(0x48);
  //default .connect() tries saved wifi profiles
  if ( Feather.connect() )
    lcd.print("Connected!");
  else
  {
    lcd.printf("Failed! %s (%d)", Feather.errstr(), Feather.errno());
    delay(500);
  }
  return Feather.connected();
}
