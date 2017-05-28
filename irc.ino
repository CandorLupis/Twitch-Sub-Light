#define SERVER      "irc.chat.twitch.tv"
#define PORT        6667
#define NICK        "name"
#define PASS        "password"

String currentChannel;
int tempColors[3];
int pingCount = 0;
long previousIRCMills = 0;
AdafruitTCP client;

void connectIRC(String twitchChannel) {
  //show IRC channel name on lcd
  lcd.write(0xFE); lcd.write(0x58);
  int centerTab;
  if (twitchChannel.length() <= 16)
    centerTab = (16 - twitchChannel.length()) / 2;
  else
    centerTab = 0;
  lcd.write(0xFE); lcd.write(0x47);
  lcd.write(centerTab + 1); lcd.write(2);
  lcd.print(twitchChannel);
  delay(10);
  lcd.write(0xFE); lcd.write(0x48);
  delay(10);
  lcd.print(" Connecting IRC ");
  
  twitchChannel = '#' + twitchChannel;
  //leave current IRC
  if (client.connected()) {
    Serial.println("Leaving IRC");
    client.disconnect();
  }
  //setup TCP connection to twitch channel IRC
  if (client.connect(SERVER, PORT)) {

    Serial.println("Connected!");
    delay(5000);
    Serial.println("Logging in..");

    client.print("PASS "); client.println(PASS);
    client.print("NICK "); client.println(NICK);
    delay(500);
    client.print("JOIN "); client.println(twitchChannel);
    client.println("CAP REQ :twitch.tv/commands");
    client.println("CAP REQ :twitch.tv/tags");
    pingCount = 0;
    currentChannel = twitchChannel;
  } else {
    Serial.println("Connection failed.");
    previousIRCMills = millis();
  }
}
int messageIndex;

void readFromServer() {
  String sentence, badges, info, response;
  long currentMills = millis();

  //send PING request if 6 minutes with no message
  if (currentMills - previousIRCMills > 360000) {
    client.println("PING");
    pingCount++;
    previousIRCMills = currentMills;
    Serial.printf("PING %d\n", pingCount);
  }
  while (client.available()) {
    char c = client.read();
    sentence = "";
    //pull out characters from the client serial and assemble a string
    while (client.available() && c != '\n') {
      sentence = sentence + c;
      c = client.read();
    }

    //respond to twitch PING message to stay connected
    if (sentence.startsWith("PING")) {
      response = sentence;
      response.setCharAt(1, 'O');
      Serial.println(sentence);
      client.println(response);
      previousIRCMills = currentMills;
      return;
    } else if (sentence.startsWith("PONG")) {
      previousIRCMills = currentMills;
      pingCount = 0;
    }
    //chop up message into useable bits
    if (sentence.startsWith("@")) {
      messageIndex = sentence.indexOf(" :") + 2;
      badges = sentence.substring(7, messageIndex);
      sentence.remove(0, messageIndex);
    }
    messageIndex = sentence.indexOf(" :") + 2;
    if (messageIndex > 2) {
      info = sentence.substring(0, messageIndex);
      sentence.remove(0, messageIndex);
    } else {
      info = sentence;
      sentence = "";
    }
Serial.println(badges);
    Serial.println(info);
    Serial.println(sentence);

    //automated messages
    if (info.startsWith("tmi.twitch.tv")) {
      if (info.indexOf(" NOTICE") > info.indexOf(" USERNOTICE"))
        commandResponse(sentence, 2);
      else if (info.indexOf(" NOTICE") < info.indexOf(" USERNOTICE"))
        commandResponse(badges, 1);

      Serial.println(badges);
      Serial.println(info);
      Serial.println(sentence);
      return;
    }

    //moderator can send alert
    if (isMod(badges)) {
      response = sentence.substring(messageIndex);
      if (response.startsWith("+sos")) {
        setEffect("flash");
        String user = info.substring(0, info.indexOf("!"));
        lcd.write(0xFE); lcd.write(0x58);
        delay(10);
        lcd.write(0xFE); lcd.write(0x48);
        delay(10);
        lcd.println(user);
        lcd.print("says HALP!");
      }
      return;
    }

    //user commands
    if (info.indexOf("WHISPER") > -1 && approvedUser(info)) {
      editSetting(sentence);
    }
    //reset time out counter
    previousIRCMills = millis();
  }
  if (pingCount == 2) {
    connectIRC(currentChannel);
    previousIRCMills = millis();
  } else if (pingCount > 2) {
    Feather.disconnect();
    while (!connectAP())
    {
      delay(500);
    }
    connectIRC(currentChannel);
  }
}

void commandResponse(String command, int i) {
  String response = "";
  String subName;
  int spaceIndex, endIndex, centerTab;
  setEffect("flash");
  lcd.write(0xFE); lcd.write(0x58);
  delay(10);
  switch (i) {
    case 1: {//user subs
        int index = command.indexOf("color=") + 6;
        command.remove(0, index);
        if (command.startsWith("#")) {
          command.remove(0, 1);
          setEffect("tempColor" + command.substring(0, 6) +  " 4000");
        }
        index = command.indexOf("system-msg=") + 11;
        command.remove(0, index);
        index = command.indexOf(";");
        command = command.substring(0, index);
        command.replace("\\s", " ");
        spaceIndex = command.indexOf(' ') + 1;
        subName = command.substring(0, spaceIndex);
        endIndex = command.lastIndexOf('!');
        command = command.substring(endIndex - 18, endIndex - 9);
        subName.trim();
        if (subName.length() <= 16)
          centerTab = (16 - subName.length()) / 2;
        else
          centerTab = 0;
        lcd.write(0xFE); lcd.write(0x47);
        lcd.write(centerTab + 1); lcd.write(1);
        lcd.print(subName);
        lcd.write(0xFE); lcd.write(0x47);
        lcd.write(1); lcd.write(2);
        if (command.indexOf("months") > -1) {
          lcd.print("   ");
          lcd.print(command);
          lcd.print("!");
        } else {
          lcd.print("Just Subscribed!");
        }
        break;
      }
    case 2: {//hosting notifications
        lcd.write(0xFE); lcd.write(0x58);
        lcd.write(0xFE); lcd.write(0x48);
        int hostIndex = command.indexOf("hosting") + 8;
        if (hostIndex > 7) {
          lcd.println("Now Hosting:");
          String hostName = command.substring(hostIndex, command.lastIndexOf("."));
          lcd.print(hostName);
        } else
          lcd.print("Exiting host    mode.");
        break;
      }
  }
  tempMessage(5000);
}

void checkServerDisconnection() {
  if (!client.connected() ) {
    client.stop();
    Serial.println("DISCONNECTED.");
  }
}
