/* LICENSE
 * Copyright (C) 2014 openQCM
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see http://www.gnu.org/licenses/gpl-3.0.txt
 *
 * INTRO
 * openQCM is the unique opensource quartz crystal microbalance http://openqcm.com/
 * openQCM Java software project is available on github repository 
 * https://github.com/marcomauro/openQCM
 * 
 * Measure QCM frequency using FreqCount library developed by Paul Stoffregen 
 * https://github.com/PaulStoffregen/FreqCount
 *
 * NOTE       - designed for 6 and 10 Mhz At-cut quartz crystal
 *            - 3.3 VDC supply voltage quartz crystal oscillator
 *            - Configure EXTERNAL reference voltage used for analog input
 *            - Thermistor temperature sensor
 *
 * author     Marco Mauro / Luciano Zu
 * version    2.0
 * date       september 2016 
 *
 */

#include <EEPROM.h>

/*
Unique ID is suggested by connected PC. Then it is stored and used for ever.
Unique ID is stored in EEPROM at a given address. A magic number (2 bytes) "LZ"
is used to understand if it is already stored or not.
Actually Unique ID is a UUID in the string format (36 bytes instead of just 16, yes it could be better).
https://en.wikipedia.org/wiki/Universally_unique_identifier
So the final length to store the unique id is 38 bytes.
*/
#define UNIQUE_ID_EEPROM_ADDRESS 0
#define UNIQUE_ID_LENGTH 38
#define UNIQUE_ID_MAGIC_NUMBER_HIGH 76
#define UNIQUE_ID_MAGIC_NUMBER_LOW 90

String inputString = "";         // a string to hold incoming data (Ardulink)
boolean stringComplete = false;  // whether the string is complete (Ardulink)
boolean initialized = false;  // whether the string is complete (Ardulink)

// print data to serial port 
void dataPrint(unsigned long Count, int Temperature){
  Serial.print("alp://cevnt/");
  Serial.print("RAWMONITOR");
  Serial.print(Count);
  Serial.print("_");
  Serial.print(Temperature);
  //Serial.write(255); // End of Message
  Serial.print('\n');
  Serial.flush();
}

// read commands sent with Ardulink
void readInputCommands(){
  while (Serial.available() && !stringComplete) {
     // get the new byte:
    char inChar = (char)Serial.read();
    // add it to the inputString:
    inputString += inChar;
    // if the incoming character is a newline, set a flag
    // so the main loop can do something about it:
    if (inChar == '\n') {
      stringComplete = true;
    }
  }

  if (stringComplete) {
    
    if(inputString.startsWith("alp://")) { // OK is a message I know (Ardulink)
    
      boolean msgRecognized = true;
      String uniqueID = "";
      
      if(inputString.substring(6,10) == "cust") { // Custom Message
        int separatorPosition = inputString.indexOf('/', 11 );
        int messageIdPosition = inputString.indexOf('?', 11 );
        String customCommand = inputString.substring(11,separatorPosition);
        String value = inputString.substring(separatorPosition + 1, messageIdPosition); // suggested uniqueID
        if(customCommand == "getUniqueID") {
        	 uniqueID = getUniqueID(value);
        } else {
          msgRecognized = false; // this sketch doesn't know other messages in this case command is ko (not ok)
        }
      } else {
        msgRecognized = false; // this sketch doesn't know other messages in this case command is ko (not ok)
      }
      
      // Prepare reply message if caller supply a message id (this is general code you can reuse)
      int idPosition = inputString.indexOf("?id=");
      if(idPosition != -1) {
        String id = inputString.substring(idPosition + 4);
        id.replace("\n", "");
        // print the reply
        Serial.print("alp://rply/");
        if(msgRecognized) { // this sketch doesn't know other messages in this case command is ko (not ok)
          Serial.print("ok?id=");
        } else {
          Serial.print("ko?id=");
        }
        Serial.print(id);
        if(uniqueID.length() > 0) {
        	Serial.print("&UniqueID=");
        	Serial.print(uniqueID);
          initialized = true;
        }
        Serial.print('\n'); // End of Message
        Serial.flush();
      }
    }
    
    // clear the string:
    inputString = "";
    stringComplete = false;
  }

}

String getUniqueID(String suggested) {

	char buffer[UNIQUE_ID_LENGTH + 1];
	String retvalue = suggested;
	
	EEPROM.get( UNIQUE_ID_EEPROM_ADDRESS, buffer );
	if(buffer[0] == UNIQUE_ID_MAGIC_NUMBER_HIGH && buffer[1] == UNIQUE_ID_MAGIC_NUMBER_LOW) {
		retvalue = String(&buffer[2]);
	} else {
		buffer[0] = UNIQUE_ID_MAGIC_NUMBER_HIGH;
		buffer[1] = UNIQUE_ID_MAGIC_NUMBER_LOW;
		suggested.toCharArray(&buffer[2], UNIQUE_ID_LENGTH - 1);
		EEPROM.put( UNIQUE_ID_EEPROM_ADDRESS, buffer );
	}

	return retvalue;
}


// QCM frequency by counting the number of pulses in a fixed time 
unsigned long frequency = 0;
// thermistor temperature
int temperature = 0;

void setup(){
  Serial.begin(115200);
  while(!Serial); // Wait until Serial not connected (because difference between Leonardo and Micro with UNO and others)

  while(!initialized) {
    readInputCommands();
  }
}

void loop(){
  frequency = 20;       // measure QCM frequency
  temperature = 10;     // measure temperature 
  dataPrint(frequency, temperature);  // print data
  delay(1000);
}

