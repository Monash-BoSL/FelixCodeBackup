//this code is made to upload the data of the I2C depth sensor from XIAN TECHNOLOGY
#include <avr/power.h>
#include <SoftwareSerial.h>
#include <string.h>
#include <LowPower.h>

#define SIMCOM_7000 // SIM7000A/C/E/G
#define BAUDRATE 9600 // MUST be below 19200 (for stability) but 9600 is more stable

#define CHARBUFF 196 //SIM7000 serial response buffer //longer than 255 will cause issues
//#define Maxinterval 345000//milli seconds

// For SIM7000 BoSL board
#define PWRKEY 4
#define DTR 5 // Connect with solder jumper
#define BOSL_RX 9 // Microcontroller RX
#define BOSL_TX 8 // Microcontroller TX

//Define pins for both sensors

#define RESET 7// Reset for both sensors


//For the turbidity sensor 1 2 and 3
#define TURB_RX1 2
#define TURB_TX1 3

#define TURB_RX2 10
#define TURB_TX2 11

#define TURB_RX3 12
#define TURB_TX3 13



//Site specific config
#define SITEID "3Turb_3"

//does not do anything atm // change values in transmit function
//#define APN "telstra.internet" //FOR TELSTRA
//#define APN "mdata.net.au" //FOR ALDI MOBILE

//default variable array (complilation purposes only)
bool defaultVars[7] = {1, 1, 1, 1, 1, 1, 1};

//millis timer variable
extern volatile unsigned long timer0_millis;

//gobal variables
char response[CHARBUFF]; //sim7000 serial response buffer
uint32_t lstTransmit = 0; //timestamp of last transmit (milli seconds)
String dataStr; //Transmit URL

//reponse stings
char CBC[5];
const int numberOfPieces = 5;
String pieces[numberOfPieces];
String TRUBIDITY1[2]; //1-turb with LED, 2-turb without LED
String TRUBIDITY2[2]; //1-turb with LED, 2-turb without LED
String TRUBIDITY3[2]; //1-turb with LED, 2-turb without LED

double  turbidityVal1, turbiditywoledVal1,turbidityVal2, turbiditywoledVal2,turbidityVal3, turbiditywoledVal3;

char turbidity1[10];
char turbiditywoled1[10];

char turbidity2[10];
char turbiditywoled2[10];

char turbidity3[10];
char turbiditywoled3[10];

//sensor variables
int scanInterval = 60;
uint8_t reps = 0;
double  TurbiditySum1 = 0, TurbiditywoledSum1 = 0,TurbiditySum2 = 0, TurbiditywoledSum2 = 0,TurbiditySum3 = 0, TurbiditywoledSum3 = 0;//To get the average of the 6 mins readings

int transmit;

//SIM7000 serial object
SoftwareSerial simCom = SoftwareSerial(BOSL_RX, BOSL_TX);
SoftwareSerial TurbScan1 = SoftwareSerial(TURB_RX1, TURB_TX1);
SoftwareSerial TurbScan2 = SoftwareSerial(TURB_RX2, TURB_TX2);
SoftwareSerial TurbScan3 = SoftwareSerial(TURB_RX3, TURB_TX3);


////clears char arrays////
void charBuffclr(bool clrVars[7] = defaultVars) {
 
  if (clrVars[0]) {
    memset(turbidity1, '\0', 10);
  }
  if (clrVars[1]) {
    memset(turbiditywoled1, '\0', 10);
  }
  if (clrVars[2]) {
    memset(turbidity2, '\0', 10);
  }
  if (clrVars[3]) {
    memset(turbiditywoled2, '\0', 10);
  }
  if (clrVars[4]) {
    memset(turbidity3, '\0', 10);
  }
  if (clrVars[5]) {
    memset(CBC, '\0', 5);
  }
    if (clrVars[6]) {
    memset(turbiditywoled3, '\0', 10);
  }



}

//convert float to string
void convertchart() {



  //turbidityVal with LED
  dtostrf(turbidityVal1, 2, 2, turbidity1);

  //turbidityVal without LED
  dtostrf(turbiditywoledVal1, 2, 2, turbiditywoled1);

  //turbidityVal with LED
  dtostrf(turbidityVal2, 2, 2, turbidity2);

  //turbidityVal without LED
  dtostrf(turbiditywoledVal2, 2, 2, turbiditywoled2);

  //turbidityVal with LED
  dtostrf(turbidityVal3, 2, 2, turbidity3);

  //turbidityVal without LED
  dtostrf(turbiditywoledVal3, 2, 2, turbiditywoled3);

}

void setup() {

  //TWBR = 158;
  //TWSR |= bit (TWPS1);

  //clear buffers
  charBuffclr();

  //ensure sim is in the off state
  simOff();

  //begin serial
  Serial.begin(BAUDRATE);
  //Serial.println(F("START TESTING"));

  pinMode(RESET, OUTPUT);
  digitalWrite(RESET, LOW);
  delay(10);
  digitalWrite(RESET, HIGH);
  pinMode(RESET, INPUT);
  
  turbread1();

  turbread2();
  
  turbread3();


  convertchart();

  Serial.println("Initialising SIM 7000");
  simCom.begin(BAUDRATE);

  //initialise sim (on arduino startup only)
  simOn();
  simInit();

  CBCread();
  //getaverage();
  Transmit();

  simOff();



  Serial.println("Setup Down!");
  
  Serial.print("Turbidity 1 with LED: ");
  Serial.println(turbidityVal1);
  Serial.print("Turbidity 1 without LED: ");
  Serial.println(turbiditywoledVal1);

  Serial.print("Turbidity 2 with LED: ");
  Serial.println(turbidityVal2);
  Serial.print("Turbidity 2 without LED: ");
  Serial.println(turbiditywoledVal2);

  Serial.print("Turbidity 3 with LED: ");
  Serial.println(turbidityVal3);
  Serial.print("Turbidity 3 without LED: ");
  Serial.println(turbiditywoledVal3);
}

void loop() {

  //set up a calibration mode
  if (digitalRead(6) == 1) {
    turbread1();
    Serial.print("Turbidity 1: ");
    Serial.println(turbidityVal1);
    Serial.print("TurbiditywoLED 1: ");
    Serial.println(turbiditywoledVal1);

    turbread2();
    Serial.print("Turbidity 2: ");
    Serial.println(turbidityVal2);
    Serial.print("TurbiditywoLED 2: ");
    Serial.println(turbiditywoledVal2);

    turbread3();
    Serial.print("Turbidity 3: ");
    Serial.println(turbidityVal3);
    Serial.print("TurbiditywoLED 3: ");
    Serial.println(turbiditywoledVal3);
  } else {

    Serial.print("reps: ");
    Serial.println(reps);
    Serial.print("TurbiditySum 1: ");
    Serial.println(TurbiditySum1);
    Serial.print("TurbiditywoledSum 1: ");
    Serial.println(TurbiditywoledSum1);

    Serial.print("TurbiditySum 2: ");
    Serial.println(TurbiditySum2);
    Serial.print("TurbiditywoledSum 2: ");
    Serial.println(TurbiditywoledSum2);

    Serial.print("TurbiditySum 3: ");
    Serial.println(TurbiditySum3);
    Serial.print("TurbiditywoledSum 3: ");
    Serial.println(TurbiditywoledSum3);
    Serial.println("Sleep");

    Sleepy(scanInterval - 18);

    reps++; //counter

    charBuffclr();

    Serial.println("Scanning");

    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, LOW);
    delay(10);
    digitalWrite(RESET, HIGH);
    pinMode(RESET, INPUT);

    turbread1();
    TurbiditySum1 += turbidityVal1;
    TurbiditywoledSum1 += turbiditywoledVal1;

    turbread2();
    TurbiditySum2 += turbidityVal2;
    TurbiditywoledSum2 += turbiditywoledVal2;
    
    turbread3();
    TurbiditySum3 += turbidityVal3;
    TurbiditywoledSum3 += turbiditywoledVal3;


    if (reps == 6) {

      charBuffclr();
      simOn();
      netUnreg();
      CBCread();
      getaverage();
      convertchart();
      Transmit();

      reps = 0;
      TurbiditySum1 = 0;
      TurbiditywoledSum1 = 0;

      TurbiditySum2 = 0;
      TurbiditywoledSum2 = 0;

      TurbiditySum3 = 0;
      TurbiditywoledSum3 = 0;
      
      simOff();
    }


  }
}

void turbread1() {
  char turbraw1[10];
  char turbwoledraw1[10];
  uint32_t tout = 0;
  uint8_t bytin = 0;
  // needs to turn the power on here via npn transistor

  memset(turbraw1, '\0', 10);
  memset(turbwoledraw1, '\0', 10);

  TurbScan1.begin(9600);
  TurbScan1.listen();


  //check if the sensor has correctly been reseted
  tout = millis();
  while (millis() - tout < 10000) {
    if (TurbScan1.available() != 0) {
      bytin = TurbScan1.read();
      if (bytin = 'R') {
        break;
      }
    }
  }

  TurbScan1.print('V');

  TurbScan1.setTimeout(1500);
  TurbScan1.readBytes(turbraw1, 10);

  TurbScan1.print('T');
  TurbScan1.setTimeout(1500);
  TurbScan1.readBytes(turbwoledraw1, 10);

  TurbScan1.print('S');

  TurbScan1.flush();
  TurbScan1.end();

  turbidityVal1 = atof(turbraw1);
  turbiditywoledVal1 = atof(turbwoledraw1);

  Serial.println(F("Turbidity 1:"));
  Serial.print(turbidityVal1);
  Serial.print(" ");
  Serial.println(turbiditywoledVal1);
  //TRUBIDITY[0] = turbraw;
  //TRUBIDITY[1] = turbwoledraw;

}

void turbread2() {
  char turbraw2[10];
  char turbwoledraw2[10];
  uint32_t tout = 0;
  uint8_t bytin = 0;
  // needs to turn the power on here via npn transistor

  memset(turbraw2, '\0', 10);
  memset(turbwoledraw2, '\0', 10);

  TurbScan2.begin(9600);
  TurbScan2.listen();


  //check if the sensor has correctly been reseted
  tout = millis();
  while (millis() - tout < 10000) {
    if (TurbScan2.available() != 0) {
      bytin = TurbScan2.read();
      if (bytin = 'R') {
        break;
      }
    }
  }

  TurbScan2.print('V');

  TurbScan2.setTimeout(1500);
  TurbScan2.readBytes(turbraw2, 10);

  TurbScan2.print('T');
  TurbScan2.setTimeout(1500);
  TurbScan2.readBytes(turbwoledraw2, 10);

  TurbScan2.print('S');

  TurbScan2.flush();
  TurbScan2.end();

  turbidityVal2 = atof(turbraw2);
  turbiditywoledVal2 = atof(turbwoledraw2);

  Serial.println(F("Turbidity 2:"));
  Serial.print(turbidityVal2);
  Serial.print(" ");
  Serial.println(turbiditywoledVal2);
  //TRUBIDITY[0] = turbraw;
  //TRUBIDITY[1] = turbwoledraw;

}

void turbread3() {
  char turbraw3[10];
  char turbwoledraw3[10];
  uint32_t tout = 0;
  uint8_t bytin = 0;
  // needs to turn the power on here via npn transistor

  memset(turbraw3, '\0', 10);
  memset(turbwoledraw3, '\0', 10);

  TurbScan3.begin(9600);
  TurbScan3.listen();


  //check if the sensor has correctly been reseted
  tout = millis();
  while (millis() - tout < 10000) {
    if (TurbScan3.available() != 0) {
      bytin = TurbScan3.read();
      if (bytin = 'R') {
        break;
      }
    }
  }

  TurbScan3.print('V');

  TurbScan3.setTimeout(1500);
  TurbScan3.readBytes(turbraw3, 10);

  TurbScan3.print('T');
  TurbScan3.setTimeout(1500);
  TurbScan3.readBytes(turbwoledraw3, 10);

  TurbScan3.print('S');

  TurbScan3.flush();
  TurbScan3.end();

  turbidityVal3 = atof(turbraw3);
  turbiditywoledVal3 = atof(turbwoledraw3);

  Serial.println(F("Turbidity 3:"));
  Serial.print(turbidityVal3);
  Serial.print(" ");
  Serial.println(turbiditywoledVal3);
  //TRUBIDITY[0] = turbraw;
  //TRUBIDITY[1] = turbwoledraw;

  simCom.listen();

}

void getaverage() {

  turbidityVal1 = (double) TurbiditySum1 / (double)reps;
  turbiditywoledVal1 = (double) TurbiditywoledSum1 / (double)reps;

  turbidityVal2 = (double) TurbiditySum2 / (double)reps;
  turbiditywoledVal2 = (double) TurbiditywoledSum2 / (double)reps;

  turbidityVal3 = (double) TurbiditySum3 / (double)reps;
  turbiditywoledVal3 = (double) TurbiditywoledSum3 / (double)reps;
  
   TurbiditySum1 = TurbiditywoledSum1 = TurbiditySum2 = TurbiditywoledSum2 = TurbiditySum3 = TurbiditywoledSum3 = 0;
}

/*bool shouldTransmit(){
  //checks to see if it has been longer than max transmit interval
    if (Maxinterval < millis() - lstTransmit){
        return 1;
    }

    //if all checks for transmit are not nessesary, return false
    return 0;
  }
*/

////reads battery voltage from responce char array////
void storeCBCresponse() {

  bool end = 0;
  uint8_t x = 0;
  uint8_t j = 0;

  bool clrVars[7] = {0, 0, 0, 0, 0, 1, 0};

  charBuffclr(clrVars);

  //loop through reponce to extract data
  for (uint8_t i = 0; i < CHARBUFF; i++) {

    //string splitting cases
    switch (response[i]) {
      case ':':
        x = 9;
        j = 0;
        i += 2;
        break;

      case ',':
        x++;
        j = 0;
        break;

      case '\0':
        end = 1;
        break;
      case '\r':
        x++;
        j = 0;
        break;
    }
    //write to char arrays
    if (response[i] != ',') {
      switch (x) {
        case 11:
          CBC[j] = response[i];
          break;
      }
      //increment char array counter
      j++;
    }
    //break loop when end flag is high
    if (end) {
      i = CHARBUFF;
    }
  }
}


//like delay but lower power and more dodgy//
void xDelay(uint32_t tmz) {
  uint32_t tmzslc = tmz / 64;
  //64 chosen as ballance between power savings and time spent in full clock mode
  clock_prescale_set(clock_div_64);
  delay(tmzslc);
  clock_prescale_set(clock_div_1);

  cli();
  timer0_millis += 63 * tmzslc;
  sei();

  delay(tmz - 64 * tmzslc);
}

////powers on SIM7000////
void simOn() {
  //do check for if sim is on
  pinMode(PWRKEY, OUTPUT);
  pinMode(BOSL_TX, OUTPUT);
  digitalWrite(BOSL_TX, HIGH);
  pinMode(BOSL_RX, INPUT_PULLUP);


  digitalWrite(PWRKEY, LOW);
  // See spec sheets for your particular module
  xDelay(1000); // For SIM7000

  digitalWrite(PWRKEY, HIGH);
  xDelay(4000);
}

////powers off SIM7000////
void simOff() {
  //  TX / RX pins off to save power

  digitalWrite(BOSL_TX, LOW);
  digitalWrite(BOSL_RX, LOW);

  digitalWrite(PWRKEY, LOW);

  // See spec sheets for your particular module
  xDelay(1200); // For SIM7000

  digitalWrite(PWRKEY, HIGH);
  xDelay(2000);
}

////power down cellular functionality////
void netUnreg() {
  sendATcmd(F("AT+CFUN=0"), "OK", 1000);
}

////register to network////
void netReg() {
  sendATcmd(F("AT+CFUN=1"), "OK", 1000);//enable the modem

  xDelay(2000);
  sendATcmd(F("AT+CGDCONT=1,\"IP\",\"hologram\""), "OK", 2000);//set IPv4 and apn
  if (sendATcmd(F("AT+CREG?"), "+CREG: 0,5", 2000) == 0) { //check that modem is registered as roaming
    sendATcmd(F("AT+COPS=1,2,50501"), "OK", 50000); //register to telstra network
    sendATcmd(F("AT+CREG?"), "+CREG: 0,5", 2000); //check that modem is registered as roaming
  }
}

////sends at command, checks for reply////
bool sendATcmd(String ATcommand, char* expctAns, unsigned int timeout) {
  uint32_t timeStart;
  bool answer;
  uint8_t a = 0;

  do {
    a++;

    Serial.println(ATcommand);

    answer = 0;
    

    timeStart = 0;


    delay(100);

    while ( simCom.available() > 0) {
      simCom.read();    // Clean the input buffer
    }

    simCom.println(ATcommand);    // Send the AT command


    uint8_t i = 0;
    timeStart = millis();
    memset(response, '\0', CHARBUFF);    // Initialize the string

    // this loop waits for the answer

    do {
      if (simCom.available() != 0) {
        response[i] = simCom.read();
        i++;
        // check if the desired answer is in the response of the module
        if (strstr(response, expctAns) != NULL)
        {
          answer = 1;
        }
      }



      // Waits for the asnwer with time out
    }
    while ((answer == 0) && ((millis() - timeStart) < timeout));

    if (expctAns == "0") {
      answer = 1;
    }
    Serial.println(response);

  } while (answer == 0 && a < 5);

  a = 0;
  return answer;
}



////initialises sim on arduino startup////
void simInit() {

  sendATcmd(F("AT+IPR=9600"), "OK", 1000);

  sendATcmd(F("ATE0"), "OK", 1000);

  sendATcmd(F("AT&W0"), "OK", 1000);

}


void CBCread() {
  //get GNSS data
  if (sendATcmd(F("AT+CBC"), "OK", 1000)) {

    storeCBCresponse();

  }
}



////SLEEPS FOR SET TIME////
void Sleepy(uint16_t ScanInterval) { //Sleep Time in seconds

  simCom.flush(); // must run before going to sleep

  Serial.flush(); // ensures that all messages have sent through serial before arduino sleeps
  /*
      LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); //8 seconds dosen't work on the 8mhz
      //advance millis timer as it is paused in sleep
      noInterrupts();
      timer0_millis += 4000;
      interrupts();
  */

  while (ScanInterval >= 1) {
    LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); //8 seconds dosen't work on the 8mhz
    //advance millis timer as it is paused in sleep
    noInterrupts();
    timer0_millis += 1000;
    interrupts();

    ScanInterval -= 1;
  }
}

////TRANSMITS LAST GPS CORDINATES TO WEB////
void Transmit() {

    dataStr = "AT+HTTPPARA=\"URL\",\"http://www.bosl.com.au/IoT/UoM_CoM_Biofilters/scripts/WriteMe.php?SiteName=";

  dataStr += SITEID;
  dataStr += ".csv";
  dataStr += "&T=";
  dataStr += turbidity1; //Turbidity with LED
  dataStr += "&EC=";
  dataStr += turbiditywoled1; //Turbidity without LED
  dataStr += "&D=";
  dataStr += turbidity2; //Turbidity with LED
  dataStr += "&B=";
  dataStr += turbiditywoled2; //Turbidity without LED
  dataStr += "&U=";
  dataStr += turbidity3; //Turbidity with LED
  dataStr += "&V=";
  dataStr += turbiditywoled3; //Turbidity without LED
  dataStr += "&W=";
  dataStr += CBC; // battery level
  dataStr += "\"";



  netReg();

  ///***check logic
  //set CSTT - if it is already set, then no need to do again...
  sendATcmd(F("AT+CSTT?"), "OK", 1000);
  if (strstr(response, "hologram") != NULL) {
    //this means the cstt has been set, so no need to set again!
    Serial.println("CSTT already set to APN ...no need to set again");
  } else {
    sendATcmd(F("AT+CSTT=\"hologram\""), "OK", 1000);
  }


  //close open bearer
  sendATcmd(F("AT+SAPBR=2,1"), "OK", 1000);
  if (strstr(response, "1,1") == NULL) {
    if (strstr(response, "1,3") == NULL) {
      sendATcmd(F("AT+SAPBR=0,1"), "OK", 1000);
    }
    sendATcmd(F("AT+SAPBR=3,1,\"APN\",\"hologram\""), "OK", 1000); //set bearer apn
    sendATcmd(F("AT+SAPBR=1,1"), "OK", 1000);
  }


  sendATcmd(F("AT+HTTPINIT"), "OK", 1000);
  sendATcmd(F("AT+HTTPPARA=\"CID\",1"), "OK", 1000);

  sendATcmd(dataStr, "OK", 1000);
  Serial.printf("1");
  Serial.print(dataStr);

  sendATcmd(F("AT+HTTPACTION=0"), "200", 2000);
  sendATcmd(F("AT+HTTPTERM"), "OK", 1000);
  //close the bearer connection
  sendATcmd(F("AT+SAPBR=0,1"), "OK", 1000);

  netUnreg();

  lstTransmit = millis();
}
