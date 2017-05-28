/* This example uses the AdafruitHTTPServer class to create a simple webserver 
    Temporary AP to host html page for wifi input*/


#define WLAN_SSID            "Lightbot"
#define WLAN_CHANNEL          1
#define PORT                  80            // The TCP port to use
#define MAX_CLIENTS           3

IPAddress apIP     (192, 168, 2, 1);
IPAddress apGateway(192, 168, 2, 1);
IPAddress apNetmask(255, 255, 255, 0);

int visit_count = 0;

void info_html_generator      (const char* url, const char* query, httppage_request_t* http_request);
void file_not_found_generator (const char* url, const char* query, httppage_request_t* http_request);
void login_generator          (const char* url, const char* query, httppage_request_t* http_request);

HTTPPage pages[] =
{
  HTTPPageRedirect("/", "/login.html"), // redirect root to hello page
  HTTPPage("/login.html", HTTP_MIME_TEXT_HTML, login_generator),
  HTTPPage("/connecting.html", HTTP_MIME_TEXT_HTML, saving_info),
  HTTPPage("/info.html" , HTTP_MIME_TEXT_HTML, info_html_generator),
  HTTPPage("/404.html" , HTTP_MIME_TEXT_HTML, file_not_found_generator),
};

uint8_t pagecount = sizeof(pages) / sizeof(HTTPPage);

// Declare HTTPServer with max number of pages
AdafruitHTTPServer httpserver(pagecount, WIFI_INTERFACE_SOFTAP);

void login_generator (const char* url, const char* query, httppage_request_t* http_request)
{
  (void) url;
  (void) query;
  (void) http_request;

  httpserver.print("<b>Wifi Login</b><br>");
  httpserver.print("<form method=GET action=\"/connecting.html\">SSID: <input type=text name=ssid><br>");
  httpserver.print("Pass: <input type=text name=pass><br><input type=submit></form>");
}

void saving_info (const char* url, const char* query, httppage_request_t* http_request)
{
  (void) url;
  String formData = query;
  (void) http_request;

  int ssidIndex = formData.indexOf("ssid=");
  int passIndex = formData.indexOf("&pass=");
  String ssid = formData.substring(ssidIndex + 5, passIndex);
  String pass = formData.substring(passIndex + 6);
  httpserver.print("<b>Attempting to connect to</b><br>SSID: ");
  httpserver.print(ssid);
  httpserver.print("<br>Pass: ");
  httpserver.print(pass);

  const char * s = ssid.c_str();
  const char * p = pass.c_str();
  if ( Feather.connect(s, p, ENC_TYPE_AUTO) )
  {
    Serial.print("Connected to ");
    Serial.println(Feather.SSID());
    httpserver.print("<br>Connected!");
    Feather.clearProfiles();
    delay(500);
    if (Feather.saveConnectedProfile())
      httpserver.print("<br>Wifi profile saved! Stopping server.");
    else
      httpserver.print("<br>Profile not saved.");
  }
  else
  {
    httpserver.printf("<br>Failed! %s (%d)", Feather.errstr(), Feather.errno());
  }
}

void info_html_generator (const char* url, const char* query, httppage_request_t* http_request)
{
  (void) url;
  (void) query;
  (void) http_request;

  httpserver.print("<b>Bootloader</b> : ");
  httpserver.print( Feather.bootloaderVersion() );
  httpserver.print("<br>");

  httpserver.print("<b>WICED SDK</b> : ");
  httpserver.print( Feather.sdkVersion() );
  httpserver.print("<br>");

  httpserver.print("<b>FeatherLib</b> : ");
  httpserver.print( Feather.firmwareVersion() );
  httpserver.print("<br>");

  httpserver.print("<b>Arduino API</b> : ");
  httpserver.print( Feather.arduinoVersion() );
  httpserver.print("<br>");
  httpserver.print("<br>");

  visit_count++;
  httpserver.print("<b>visit count</b> : ");
  httpserver.print(visit_count);
}

/**************************************************************************/
/*!
   @brief  HTTP 404 generator. The HTTP Server will automatically redirect
           to "/404.html" when it can't find the requested url in the
           list of registered pages

   The url and query string are already separated when this function
   is called.

   @param url           url of this page
   @param query         query string after '?' e.g "var=value"
   @param http_request  Details about this HTTP request
*/
/**************************************************************************/
void file_not_found_generator (const char* url, const char* query, httppage_request_t* http_request)
{
  (void) url;
  (void) query;
  (void) http_request;

  httpserver.print("<html><body>");
  httpserver.print("<h1>Error 404 File Not Found!</h1>");
  httpserver.print("<br>");

  httpserver.print("Available pages are:");
  httpserver.print("<br>");

  httpserver.print("<ul>");
  for (int i = 0; i < pagecount; i++)
  {
    httpserver.print("<li>");
    httpserver.print(pages[i].url);
    httpserver.print("</li>");
  }
  httpserver.print("</ul>");

  httpserver.print("</body></html>");
}

void runServer()
{
  lcd.write(0xFE); lcd.write(0x58);
  delay(10);
  lcd.write(0xFE); lcd.write(0x48);
  lcd.print("Wifi:   Lightbot");
  lcd.print("192.168.2.1");
  FeatherAP.err_actions(true, true);
  FeatherAP.begin(apIP, apGateway, apNetmask, WLAN_CHANNEL);

  Serial.println("Starting SoftAP\r\n");
  FeatherAP.start(WLAN_SSID);
  FeatherAP.printNetwork();

  // Tell the HTTP client to auto print error codes and halt on errors
  httpserver.err_actions(true, true);

  // Configure HTTP Server Pages
  Serial.println("Adding Pages to HTTP Server");
  httpserver.addPages(pages, pagecount);

  Serial.print("Starting HTTP Server ... ");
  httpserver.begin(PORT, MAX_CLIENTS);
  Serial.println(" running");
  while (!Feather.connected())
    delay(100);
}

void stopServer()
{
  httpserver.stop();
}


