#include <WiServer.h>
#include <IRremote.h>
#include <dht11.h>
#include <aJSON.h>

int RECV_PIN = 5;
int ANALOG_BUTTON_PIN = 5;
int ANALOG_LIGHTSENSOR_PIN = 1;
int ledPin1 = 4;

IRrecv irrecv(RECV_PIN);
IRsend irsend;
dht11 DHT11;

decode_results results;

#define CR 13
#define LF 10

#define WIRELESS_MODE_INFRA	1
#define WIRELESS_MODE_ADHOC	2

// Wireless configuration parameters ----------------------------------------
unsigned char local_ip[] = {10,0,0,50};	// IP address of WiShield
unsigned char gateway_ip[] = {10,0,0,1};	// router or gateway IP address
unsigned char subnet_mask[] = {255,255,255,0};	// subnet mask for the local network
const prog_char ssid[] PROGMEM = {"arduino"};		// max 32 bytes

unsigned char security_type = 3;	// 0 - open; 1 - WEP; 2 - WPA; 3 - WPA2

// WPA/WPA2 passphrase
const prog_char security_passphrase[] PROGMEM = {"sujrndshibuya"};	// max 64 characters

// WEP 128-bit keys
// sample HEX keys
prog_uchar wep_keys[] PROGMEM = {	0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x07, 0x08, 0x04, 0x08, 0x00, 0x07, 0x00,	// Key 0
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00,	// Key 1
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00,	// Key 2
					0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,	0x00	// Key 3
				};

// setup the wireless mode
// infrastructure - connect to AP
// adhoc - connect to another WiFi device
unsigned char wireless_mode = WIRELESS_MODE_INFRA;

unsigned char ssid_len;
unsigned char security_passphrase_len;

boolean states[1]; //holds led states
char stateCounter; //used as a temporary variable
char tmpStrCat[64]; //used in processing the web page
char stateBuff[4]; //used in text processing around boolToString()
char numAsCharBuff[2];
char ledChange;

// Weather
float prevTemp = 0;
float prevHumidity = 0;

//---------------------------------------------------------------------------

// Server IP Address
uint8 ip[] = {10,0,0,5};

// Request to server
String string_request = String("/?");
char request[99];
//char reply[299];

// A request that sends the latest data
GETrequest getWeather(ip, 4567, "10.0.0.5", request);

// Functions that print data from the server
void printData(char* data, int len) {
	// Address after the last byte of data
	char* end = data + len;
	// Start of current line
	char* start = data;
        bool inData = false;

	// Scan through the bytes in the packet looking for a Line Feed character
	while (data < end) {
		if (*data == LF) {
			// Determine the length of the line excluding the Line Feed
			int lineLength = data - start;

			if (*(data - 1) == CR) {
				lineLength--;
			}

			*(start + lineLength) = 0;
			// Process the line
                        //Serial.println(String("_") + String(lineLength) + String("_") + start + String("_"));
                        
                        // Data starts after an empty line
                        if (lineLength == 0 && inData == false) {
                        	inData = true;
                        }
                        
			// Set up for the start of the next line
			start = ++data;
		} else {
			// Go to the next byte
			data++;
		}
	}

        if (inData) {
                //aJsonObject* jsonObject = aJson.parse(start);
                // Use as name->valuestring
                Serial.println(start);
                // Delete memory:
                // aJson.deleteItem(root);
        }
}

void setup()
{
    irrecv.enableIRIn(); // Start the receiver
    DHT11.attach(6);
    Serial.begin(9600);
    
    // Initialize WiServer and have it use the sendMyPage function to serve pages
    pinMode(ledPin1, OUTPUT);
    pinMode(RECV_PIN, INPUT);
    pinMode(ANALOG_BUTTON_PIN, INPUT);
    pinMode(ANALOG_LIGHTSENSOR_PIN, INPUT);
  
    //WiFi.init();
    WiServer.init(NULL);
    WiServer.enableVerboseMode(false);
    states[0] = false;
    // Have the processData function called when data is returned by the server
    getWeather.setReturnFunc(printData);
}

void changeStates() {
        for (stateCounter = 0 ; stateCounter < 1; stateCounter++) {
            boolToString(states[stateCounter], stateBuff);
            digitalWrite(ledPin1, states[stateCounter]);
        } 
}

void boolToString (boolean test, char returnBuffer[4]) {
  returnBuffer[0] = '\0';
  if (test) {
    strcat(returnBuffer, "On");
  }
  else {
    strcat(returnBuffer, "Off");
  }
}

// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state
unsigned long sentLockMillis = 0;
unsigned long tempLockMillis = 0;

// Stores the code for later playback
// Most of this code is just logging
void storeCode(decode_results *results) {
  codeType = results->decode_type;
  int count = results->rawlen;
  
  if (codeType == UNKNOWN) {
    //Serial.println("Received unknown code, ignoring");
  }
  else {
    if (codeType == NEC) {
      Serial.print("Received NEC: ");
      if (results->value == REPEAT) {
        // Don't record a NEC repeat value as that's useless.
        Serial.println("repeat; ignoring.");
        return;
      }
    } 
    else if (codeType == SONY) {
      Serial.print("Received SONY: ");
    } 
    else if (codeType == RC5) {
      Serial.print("Received RC5: ");
    } 
    else if (codeType == RC6) {
      Serial.print("Received RC6: ");
    } 
    else {
      Serial.print("Unexpected codeType ");
      Serial.print(codeType, DEC);
      Serial.println("");
    }
    Serial.println(results->value, HEX);
    codeValue = results->value;
    codeLen = results->bits;
  }
  
}

void sendCode() {
  if (codeType == NEC) {
    Serial.print("Sent NEC ");
    Serial.println(codeValue, HEX);
    irsend.sendNEC(codeValue, codeLen);
  } 
  else if (codeType == SONY) {
    Serial.print("Sent Sony ");
    Serial.println(codeValue, HEX);
    irsend.sendSony(codeValue, codeLen);
  } 
  else if (codeType == RC5 || codeType == RC6) {
    // Flip the toggle bit for a new button press
    toggle = 1 - toggle;
    
    // Put the toggle bit into the code to send
    codeValue = codeValue & ~(1 << (codeLen - 1));
    codeValue = codeValue | (toggle << (codeLen - 1));
    if (codeType == RC5) {
      Serial.print("Sent RC5 ");
      Serial.println(codeValue, HEX);
      irsend.sendRC5(codeValue, codeLen);
    } 
    else {
      Serial.print("Sent RC6 ");
      Serial.println(codeValue, HEX);
      irsend.sendRC6(codeValue, codeLen);
    }
  }
  else if (codeType == UNKNOWN) {
    //Serial.println("Ignoring unknown code");
  }
}

void sendCodeOnce() {
  unsigned long time = millis();

  if ( (time - sentLockMillis) > 10000) { //10 seconds
    Serial.println("Sending once");
    sendCode();
    sentLockMillis = time;
  }
}

void readTempHumidity() {
    unsigned long time = millis();
    if ( (time - tempLockMillis) > 10000) { //10 seconds

      int chk = DHT11.read();

      switch (chk)
      {
        case 0: break;
        case -1: Serial.println("Checksum error"); break;
        case -2: Serial.println("Time out error"); break;
        default: Serial.println("Unknown error"); break;
      }
      
      prevTemp = (float)DHT11.temperature;
      prevHumidity = (float)DHT11.humidity;
      
      // Reset string
      string_request = String("/?");
      
      char temp[10];
      if (prevTemp > 10) {
        dtostrf(prevTemp, 4, 2, temp);
      }
      else {
        dtostrf(prevTemp, 4, 3, temp);
      }
      
      char humid[10];
      if (prevHumidity > 10) {
        dtostrf(prevHumidity, 4, 2, humid);
      }
      else {
        dtostrf(prevHumidity, 4, 3, humid);
      }
      
      //string_request = string_request + String("tmp=") + String(temp);
      //string_request = string_request + String("&hum=") + String(humid);
      string_request.toCharArray(request,100);

      tempLockMillis = time;
    }
}

// Time (in millis) when the data should be retrieved 
long updateTime = 0;

void loop()
{
  int analogButtonState = analogRead(ANALOG_BUTTON_PIN);
  int lightValue = analogRead(ANALOG_LIGHTSENSOR_PIN);
  
  if (lightValue < 10) {
    sendCodeOnce();
  }

  readTempHumidity();
  
  // Check if it's time to get an update
  if (millis() >= updateTime) {
    getWeather.submit();    
    // Get another update one hour from now
    updateTime += 1000 * 1;
  }
  
  // We use analog in as we don't have any other digital
  // inputs, needed for sensors
  if (analogButtonState >= 1000) {
    if (irrecv.decode(&results)) {
      storeCode(&results);
      irrecv.resume(); // resume receiver
      irrecv.enableIRIn(); // Restart the receiver
    }
  }
  else {
    //sendCode();
  }
   
  WiServer.server_task();
}
