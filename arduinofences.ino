//#include <WiShield.h>
#include <WiServer.h>
#include <IRremote.h>

int RECV_PIN = 5;
int BUTTON_PIN = 6;

IRrecv irrecv(RECV_PIN);
IRsend irsend;

decode_results results;

#define WIRELESS_MODE_INFRA	1
#define WIRELESS_MODE_ADHOC	2

#define ledPin1 4

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

//---------------------------------------------------------------------------

void setup()
{
    irrecv.enableIRIn(); // Start the receiver
    pinMode(BUTTON_PIN, INPUT);
    Serial.begin(9600);
    
    // Initialize WiServer and have it use the sendMyPage function to serve pages
    pinMode(ledPin1, OUTPUT);
    pinMode(RECV_PIN, INPUT);
    pinMode(BUTTON_PIN, INPUT);
  
    //WiFi.init();
    WiServer.init(sendPage);
    WiServer.enableVerboseMode(true);
    states[0] = false;
    Serial.println("Started webserver ...");
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

// This is our page serving function that generates web pages
boolean sendPage(char* URL) {
  
  changeStates();
    
  //check whether we need to change the led state
  if (URL[1] == '?' && URL[2] == 'L' && URL[3] == 'E' && URL[4] == 'D') { //url has a leading /
    ledChange = (int)(URL[5] - 48); //get the led to change.
    
    for (stateCounter = 0 ; stateCounter < 1; stateCounter++) {
      if (ledChange == stateCounter) {
        states[stateCounter] = !states[stateCounter];
        Serial.print("Sending RFID .... ");
        sendCode();
      }
    }
    
    //after having change state, return the user to the index page.
    WiServer.print("<HTML><HEAD><meta http-equiv='REFRESH' content='0;url=/'></HEAD></HTML>");
    return true;
  }
 
  
  if (strcmp(URL, "/") == false) //why is this not true?
   {
      WiServer.print("<html><head><title>Led switch</title></head>");
    
      WiServer.print("<body><center>Please select the led state:<center>\n<center>");
      for (stateCounter = 0; stateCounter < 1; stateCounter++) //for each led
      {
        numAsCharBuff[0] = (char)(stateCounter + 49); //as this is displayed use 1 - 3 rather than 0 - 2
        numAsCharBuff[1] = '\0'; //strcat expects a string (array of chars) rather than a single character.
                                 //This string is a character plus string terminator.
        
        tmpStrCat[0] = '\0'; //initialise string
        strcat(tmpStrCat, "<a href=?LED"); //start the string
        tmpStrCat[12] = (char)(stateCounter + 48); //add the led number
        tmpStrCat[13] = '\0'; //terminate the string properly for later.
    
        strcat(tmpStrCat, ">Led ");
        strcat(tmpStrCat, numAsCharBuff);
        strcat(tmpStrCat, ": ");
        
        boolToString(states[stateCounter], stateBuff);
        strcat(tmpStrCat, stateBuff);
        strcat(tmpStrCat, "</a> "); //we now have something in the range of <a href=?LED0>Led 0: Off</a>
    
        WiServer.print(tmpStrCat);
      }

        WiServer.print("</html> ");
        return true;
   }
   else {
     return false;
   }
}

// Storage for the recorded code
int codeType = -1; // The type of code
unsigned long codeValue; // The code value if not raw
int codeLen; // The length of the code
int toggle = 0; // The RC5/6 toggle state

int lastButtonState;

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

void loop()
{
  int buttonState = digitalRead(BUTTON_PIN);

  if (buttonState == HIGH) {
    Serial.println("Feed me");
    if (irrecv.decode(&results)) {
      storeCode(&results);
      irrecv.resume(); // resume receiver
      irrecv.enableIRIn(); // Restart the receiver
    }
  }
  else {
    //sendCode();
  }
  lastButtonState = buttonState;
   
  WiServer.server_task();
}
