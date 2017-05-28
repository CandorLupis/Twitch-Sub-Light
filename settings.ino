#define TIMEZONE   0

String userList[] = {"userName"};
String displayGame = "";
long previousLightMills = 0;
long previousMessageMills = 0;
float brightness = 0.8;
int i, red, green, blue, prevRed, prevGreen, prevBlue, newRed, newGreen, newBlue,
    effectSpeed, count, effectModifier, endHour, startHour, intervalLight, intervalMessage;
bool rainbow, flash, tempColor, signOff, streamLive;

//test if user is capable of sending commands
bool approvedUser(String sentence) {
  int listSize = sizeof(userList) / sizeof(userList[0]);
  for (int x = 0; x < listSize; x++)
    if (sentence.startsWith(userList[x]))
      return true;
  return false;
}

bool isMod(String sentence) {
  if (sentence.startsWith("moderator/1")) {
    return true;
  }
  return false;
}

//using serial connection to lcd backplate
void lcdStartup() {
#define lcd Serial2
  lcd.begin(9600);

  // set the size of the display if it isn't 16x2 (you only have to do this once)
  lcd.write(0xFE); lcd.write(0xD1);
  lcd.write(16); lcd.write(2);
  delay(10);
  // Adafruit suggests putting delays after each command to make sure the data
  // is sent and the LCD is updated.

  // set the contrast, 200 is a good place to start, adjust as desired
  lcd.write(0xFE); lcd.write(0x50);
  lcd.write(200);
  delay(10);

  // set the brightness - we'll max it (255 is max brightness)
  lcd.write(0xFE); lcd.write(0x99);
  lcd.write(255);
  delay(10);

  // turn off cursors
  lcd.write(0xFE); lcd.write(0x4B);
  lcd.write(0xFE); lcd.write(0x54);

  // clear screen
  lcd.write(0xFE); lcd.write(0x58);
  delay(10);

  //autoscroll off
  lcd.write(0xFE); lcd.write(0x52);
  delay(10);

  // go 'home'
  lcd.write(0xFE); lcd.write(0x48);
  delay(10);
}

void startClock()
{
  setTime(Feather.getUtcTime());
  adjustTime(TIMEZONE * 3600);
}

int y = 0;
//make updates to clock and occasional stream status
void updateDisplay() {
  unsigned long currentMills = millis();
  if (streamLive) {
    lcd.write(0xFE); lcd.write(0x48);
    lcd.print("Now Streaming:  ");
    if (displayGame.length() > 16) {
      for (int x = 0; x < 16; x++) {
        if (x + y < displayGame.length())
          lcd.print(displayGame.charAt(x + y));
        else
          lcd.print(' ');
      }
      y++;
      if (y > displayGame.length())
        y = 0;
    } else {
      lcd.print(displayGame);
      for (int x = displayGame.length(); x < 16; x++)
        lcd.print(' ');
    }
  }
  if (previousMessageMills == 0)
    digitalClockDisplay();
  else if (currentMills - previousMessageMills > intervalMessage) {
    previousMessageMills = 0;
    streamLive = false;
  }
}

//pauses normal lcd updates
void tempMessage(int interval) {
  intervalMessage = interval;
  previousMessageMills = millis();
}

//if stream is live, display game
bool streamInfo(String command) {
  int gameIndex = command.indexOf("game");
  if (gameIndex > -1) {
    streamLive = true;
    tempMessage(10000);
    command.remove(0, gameIndex + 7);
    int endIndex = command.indexOf("\"");
    displayGame = command.substring(0, endIndex) + "  ";
    if (displayGame.length() <= 2)
      displayGame = "Game not set";
  }
  else
    streamLive = false;
}

void digitalClockDisplay() {
  // digital clock display of the time
  lcd.write(0xFE); lcd.write(0x48);
  if ((hour() < 10) || (12 < hour() && hour() < 22))
    lcd.print(" ");
  if (hour() > 12)
    lcd.print(hour() - 12);
  else
    lcd.print(hour());
  lcd.print(":");
  printDigits(minute());
  lcd.print(" ");
  printDigits(month());
  lcd.print("/");
  printDigits(day());
  lcd.print("/");
  lcd.print(year());
  //Stream A Day number
  int streamNumber = now() / 86400 - 16042;
  lcd.print("   SAD# ");
  if (streamNumber > 0)
    lcd.print(streamNumber);
  lcd.print("    ");
}

//auto darkening during nighttime hours
void setNighttime(String command) {
  int commaIndex = command.lastIndexOf(',');
  if (commaIndex > -1) {
    endHour = command.substring(commaIndex + 1).toInt();
    command.remove(commaIndex);
    startHour = command.toInt();
  }
}

bool isNighttime() {
  int currentHour = hour();
  if (currentHour >= startHour || currentHour < endHour)
    return true;
  return false;
}

void printDigits(int digits) {
  // utility function for digital clock display: prints preceding colon and leading 0
  if (digits < 10)
    lcd.print('0');
  lcd.print(digits);
}

byte currentColors[3];
//accepts hex RGB code or common color names
void setColor(String colorCode) {
  if (isAlpha(colorCode.charAt(0))) {
    colorCode.replace("red", "FF0000");
    colorCode.replace("orange", "FF7F00");
    colorCode.replace("yellow", "FFFF00");
    colorCode.replace("green", "00FF00");
    colorCode.replace("cyan", "00FFFF");
    colorCode.replace("blue", "0000FF");
    colorCode.replace("purple", "FF00FF");
    colorCode.replace("magenta", "FF007F");
    colorCode.replace("white", "FFFFFF");
  }

  for (int x = 0; x < 3; x++) {
    char c[2] = {colorCode.charAt(0), colorCode.charAt(1)};
    //convert c[] to long with base 16
    long hex = strtol(c, NULL, 16);
    Serial.println(hex);
    currentColors[x] = hex;
    colorCode.remove(0, 2);
  }
  red = currentColors[0];
  green = currentColors[1];
  blue = currentColors[2];

  lcd.write(0xFE); lcd.write(0xD0);
  lcd.write(red);
  lcd.write(green);
  lcd.write(blue);
  delay(10);
  analogWrite(PA1, red);
  analogWrite(PB4, green);
  analogWrite(PC7, blue);

  rainbow = false;
}

void lightEffects() {
  unsigned long currentMills = millis();
  newRed = red;
  newGreen = green;
  newBlue = blue;
  //gradient through colors
  if (rainbow) {
    if (green <= 0 && blue == 255 && red < 255) {
      newRed = red;
      newGreen = green = 0x00;
      newBlue = 0xFF;
      red += effectSpeed;
    } else if (green == 0 && red == 255 && blue > 0) {
      newRed = 0xFF;
      newGreen = green = 0x00;
      newBlue = blue;
      blue -= effectSpeed;
    } else if (blue <= 0 && red == 255 && green < 255) {
      newRed = 0xFF;
      newGreen = green;
      newBlue = blue = 0x00;
      green += effectSpeed;
    } else if (blue == 0 && green == 255 && red > 0) {
      newRed = red;
      newGreen = 0xFF;
      newBlue = blue = 0x00;
      red -= effectSpeed;
    } else if (red <= 0 && green == 255 && blue < 255) {
      newRed = red = 0x00;
      newGreen = 0xFF;
      newBlue = blue;
      blue += effectSpeed;
    } else if (red == 0 && blue == 255 && green > 0) {
      newRed = red = 0x00;
      newGreen = green;
      newBlue = 0xFF;
      green -= effectSpeed;
    }
    //fix overflow
    if (red > 0xFF)
      red = 0xFF;
    if (green > 0xFF)
      green = 0xFF;
    if (blue > 0xFF)
      blue = 0xFF;
  }
  //flash light off and on
  if (flash) {
    if (currentMills - previousLightMills > 300) {
      previousLightMills = currentMills;
      if (i % 2 > 0)
        signOff = true;
      else
        signOff = false;
      i++;
    }
    if (i > count * 2) {
      flash = false;
      signOff = false;
    }
  }
  
  //save current color and display new for set time
  if (tempColor) {
    newRed = tempColors[0];
    newGreen = tempColors[1];
    newBlue = tempColors[2];
    if (currentMills - previousLightMills > intervalLight) {
      previousLightMills = currentMills;
      tempColor = false;
    }
  }
  
  //override all other settings to keep lights off
  if (signOff || isNighttime()) {
    newRed = 0x00;
    newGreen = 0x00;
    newBlue = 0x00;
  }
  
  //gamma correction
  float gammaRed = 255.00 * pow((newRed / 255.00), 2.8);
  float gammaGreen = 255.00 * pow((newGreen / 255.00), 2.8);
  float gammaBlue = 255.00 * pow((newBlue / 255.00), 2.8);

  //brightness adjustments
  newRed = gammaRed * brightness;
  newGreen = gammaGreen * brightness;
  newBlue = gammaBlue * brightness;

  lcd.write(0xFE); lcd.write(0xD0);
  lcd.write(newRed);
  lcd.write(newGreen);
  lcd.write(newBlue);
  delay(10);
  analogWrite(PA1, newRed);
  analogWrite(PB4, newGreen);
  analogWrite(PC7, newBlue);
}

//activate effect flags and durations
void setEffect(String command) {
  int spaceIndex = command.indexOf(' ') + 1;
  if (command.substring(spaceIndex).toInt() > 0)
    effectModifier = command.substring(spaceIndex).toInt();
  else
    effectModifier = 3;
  if (command.startsWith("clear")) {
    rainbow = false;
    flash = false;
    tempColor = false;
    red = prevRed;
    green = prevGreen;
    blue = prevBlue;
    analogWrite(PA1, red);
    analogWrite(PB4, green);
    analogWrite(PC7, blue);
  } else if (command.startsWith("flash")) {
    flash = true;
    count = effectModifier; i = 0;
    previousLightMills = millis();
  } else {
    prevRed = red;
    prevGreen = green;
    prevBlue = blue;
    spaceIndex = command.indexOf(' ') + 1;
    if (command.startsWith("rainbow")) {
      rainbow = true;
      effectSpeed = effectModifier;
      red = 0x00; green = 0x00; blue = 0xFF;
    } else if (command.startsWith("tempColor")) {
      tempColor = true;
      command.remove(0, 9);
      spaceIndex = command.indexOf(' ') + 1;
      intervalLight = command.substring(spaceIndex).toInt();
      for (int x = 0; x < 3; x++) {
        char c[2] = {command.charAt(0), command.charAt(1)};
        long hex = strtol(c, NULL, 16);
        Serial.println(hex);
        tempColors[x] = hex;
        command.remove(0, 2);
      }
      previousLightMills = millis();
    }
  }
}

//user commands
void editSetting(String command) {
  int spaceIndex = command.indexOf(' ') + 1;
  if (command.startsWith("color"))
    setColor(command.substring(spaceIndex));
  else if (command.startsWith("effect"))
    setEffect(command.substring(spaceIndex));
  else if (command.startsWith("blackout"))
    setNighttime(command.substring(spaceIndex));
  else if (command.startsWith("brightness"))
    brightness = (float)command.substring(spaceIndex).toInt() / 10;
  else if (command.startsWith("clear wifi"))
    Feather.clearProfiles();
  else if (command.startsWith("channel")) {
    String twitchChannel = command.substring(8);
    if (twitchChannel.length() < 1)
      twitchChannel = CHANNEL;
    connectIRC(twitchChannel);
  }
}

