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

// For the Depth sensor
#define DepEC_RX 2
#define DepEC_TX 3

//For the turbidity sensor
#define TURB_RX 10
#define TURB_TX 11



//Site specific config
#define SITEID "Rain_Garden_1"

//does not do anything atm // change values in transmit function
//#define APN "telstra.internet" //FOR TELSTRA
//#define APN "mdata.net.au" //FOR ALDI MOBILE

//default variable array (complilation purposes only)
bool defaultVars[8] = {1,1,1,1,1,1};

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
String DEPTH_EC[3]; // 1-depth reading, 2-Temp reading, 3-EC reading
String TRUBIDITY[2]; //1-turb with LED, 2-turb without LED
double pressVal, tempVal, ECVal, turbidityVal, turbiditywoledVal;
char Temp[10];
char Press[7];
char EC[6];
char turbidity[10];
char turbiditywoled[10];

//sensor variables
int scanInterval = 60;
uint8_t reps = 0;
double TempSum = 0, ECSum = 0, PressSum = 0, TurbiditySum = 0, TurbiditywoledSum = 0;//To get the average of the 6 mins readings 

int transmit;

//SIM7000 serial object
SoftwareSerial simCom = SoftwareSerial(BOSL_RX, BOSL_TX);
SoftwareSerial TurbScan = SoftwareSerial(TURB_RX, TURB_TX);
SoftwareSerial DepECScan = SoftwareSerial(DepEC_RX, DepEC_TX);

 
////clears char arrays////
void charBuffclr(bool clrVars[5] = defaultVars){
    if(clrVars[0]){
    memset(Temp, '\0', 5);
    }
    if(clrVars[1]){
    memset(Press, '\0', 7);
    }
    if(clrVars[2]){
    memset(turbidity, '\0', 10);
    }
    if(clrVars[3]){
    memset(turbiditywoled,'\0',10);
    }
    if(clrVars[4]){
    memset(EC,'\0',6);
    }

    
}

//convert float to string
void convertchart(){
  
  //water temperature
  dtostrf(tempVal, 2, 2, Temp);

  //water pressure
  dtostrf(pressVal, 2, 2, Press);

  //turbidityVal with LED
  dtostrf(turbidityVal, 2, 2, turbidity);

  //turbidityVal without LED
  dtostrf(turbiditywoledVal, 2, 2, turbiditywoled);

  //EC measurement
  dtostrf(ECVal, 2, 2, EC);
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
  turbread();
  DepECread();
  
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
   Serial.print("Temp: ");
   Serial.println(Temp);
   Serial.print("Depth: ");
   Serial.println(pressVal);
   Serial.print("EC: ");
   Serial.println(ECVal);
   Serial.print("Turbidity with LED: ");
   Serial.println(turbidityVal);
   Serial.print("Turbidity without LED: ");
   Serial.println(turbiditywoledVal);
}
    
void loop() {

  //set up a calibration mode
  if(digitalRead(6) == 1){
    turbread();
    Serial.print("Turbidity: ");
    Serial.println(turbidityVal);
    Serial.print("TurbiditywoLED: ");
    Serial.println(turbiditywoledVal);
  }else{

    Serial.print("reps: ");
    Serial.println(reps);
    Serial.print("ECSum: ");
    Serial.println(ECSum);
    Serial.print("TempSum: ");
    Serial.println(TempSum);
    Serial.print("PressSum: ");
    Serial.println(PressSum);
    Serial.print("TurbiditySum: ");
    Serial.println(TurbiditySum);
    Serial.print("TurbiditywoledSum: ");
    Serial.println(TurbiditywoledSum);
    Serial.println("Sleep");
  
    Sleepy(scanInterval-18);
  
    reps++; //counter

    charBuffclr();

    Serial.println("Scanning");
  
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, LOW);
    delay(10);
    digitalWrite(RESET, HIGH);
    pinMode(RESET, INPUT);
  
    turbread();
    TurbiditySum += turbidityVal;
    TurbiditywoledSum += turbiditywoledVal;
    
    DepECread();
    TempSum += tempVal;
    PressSum += pressVal;
    ECSum += ECVal;

    if(reps == 6){

      charBuffclr();
      simOn();
      netUnreg();
      CBCread();
      getaverage();
      convertchart();
      Transmit();
      
      reps = 0;
      TurbiditySum = 0;
      TurbiditywoledSum = 0;
      TempSum = 0;
      PressSum = 0;
      ECSum = 0;
      simOff();
    }
   
      
}
}

void turbread(){
  char turbraw[10];
  char turbwoledraw[10];
  uint32_t tout = 0;
  uint8_t bytin = 0;
  // needs to turn the power on here via npn transistor

  memset(turbraw, '\0', 10);
  memset(turbwoledraw, '\0', 10);
  
  TurbScan.begin(9600);
  TurbScan.listen();


  //check if the sensor has correctly been reseted
  tout = millis();
  while(millis() - tout < 10000){
    if(TurbScan.available() != 0){
      bytin = TurbScan.read();
      if(bytin = 'R'){
        break;
      }
    }
  }
  
  TurbScan.print('V');

  TurbScan.setTimeout(1500);
  TurbScan.readBytes(turbraw, 10);

  TurbScan.print('T');
  TurbScan.setTimeout(1500);
  TurbScan.readBytes(turbwoledraw, 10);

  TurbScan.print('S');

  TurbScan.flush();
  TurbScan.end();
  simCom.listen();

  turbidityVal = atof(turbraw);
  turbiditywoledVal = atof(turbwoledraw);

  Serial.println(F("Turbidity:"));
  Serial.print(turbidityVal);
  Serial.print(" ");
  Serial.println(turbiditywoledVal);
  //TRUBIDITY[0] = turbraw;
  //TRUBIDITY[1] = turbwoledraw;
}


void DepECread(){
  uint32_t tout = 0;
  uint8_t bytin = 0;
  String input = "";
  char data[20];
  // Keep track of current position in array
  int counter = 0;
  // Keep track of the last comma so we know where to start the substring
  int lastIndex = 0;
  // needs to turn the power on here via npn transistor

  memset(data, '\0', 20);
  
  DepECScan.begin(9600);
  DepECScan.listen();
  
  tout = millis();

  while(millis() - tout < 10000){
    if(DepECScan.available() != 0){
      //Serial.println("Start");
      bytin = DepECScan.read();
      if(bytin = 'R'){
        break;
      }
    }
  }

  DepECScan.print('V');

  DepECScan.setTimeout(1500);
  DepECScan.readBytes(data, 20);
  input = data;

  //Serial.println(data);

  for(int i = 0; i<input.length(); i++){
    if(input.substring(i,i+1) == ","){
      // Grab the piece from the last index up to the current position and store it
      DEPTH_EC[counter] = input.substring(lastIndex, i);
      // Update the last position and add 1, so it starts from the next character
      lastIndex = i + 1;
      // Increase the position in the array that we store into
      counter++;
    }

    // If we're at the end of the string (no more commas to stop us)
    if(i == input.length() - 1){
      // Grab the last part of the string from the lastIndex to the end
      DEPTH_EC[counter] = input.substring(lastIndex, i);
    }
  }
  pressVal = DEPTH_EC[0].toFloat();
  tempVal = DEPTH_EC[1].toFloat();
  ECVal = DEPTH_EC[2].toFloat();
  
  input = "";
  counter = 0;
  lastIndex = 0;

  DepECScan.print('S');

  DepECScan.flush();
  DepECScan.end();

  Serial.println(F("Depth, Temp and EC: "));
  Serial.print(pressVal);
  Serial.print(" ");
  Serial.print(tempVal);
  Serial.print(" ");
  Serial.println(ECVal);

  simCom.listen();
}

void getaverage(){

    ECVal = (double) ECSum / (double) reps;
    tempVal = (double) TempSum / (double) reps;
    pressVal = (double) PressSum / (double) reps;
    turbidityVal = (double) TurbiditySum / (double)reps;
    turbiditywoledVal = (double) TurbiditywoledSum / (double)reps;

    ECSum = TempSum = PressSum = TurbiditySum = TurbiditywoledSum =0;
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
void storeCBCresponse(){
    
    bool end = 0;
    uint8_t x = 0;
    uint8_t j = 0;
    
    bool clrVars[8] = {0,0,0,0,0,1,0,0};
    
    charBuffclr(clrVars);
    
    //loop through reponce to extract data
    for (uint8_t i=0; i < CHARBUFF; i++){

        //string splitting cases
        switch(response[i]){
        case ':':
            x = 9;
            j = 0;
            i += 2;
            break;

        case ',':
            x++;
            j=0;
            break;

        case '\0':
            end = 1;
            break;
        case '\r':
            x++;
            j=0;
            break;
        }
        //write to char arrays
        if (response[i] != ','){
            switch(x){
                case 11:
                    CBC[j] = response[i];
                break;             
            }
            //increment char array counter
            j++;
        }
        //break loop when end flag is high
        if (end){
            i = CHARBUFF; 
        }
    }
}


//like delay but lower power and more dodgy//
void xDelay(uint32_t tmz){
  uint32_t tmzslc = tmz/64;
  //64 chosen as ballance between power savings and time spent in full clock mode
  clock_prescale_set(clock_div_64);
    delay(tmzslc);
  clock_prescale_set(clock_div_1);
  
  cli();
  timer0_millis += 63*tmzslc; 
  sei();
  
  delay(tmz-64*tmzslc);
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
void netUnreg(){
    sendATcmd(F("AT+CFUN=0"), "OK", 1000);
}

////register to network////
void netReg(){
     sendATcmd(F("AT+CFUN=1"), "OK", 1000);//enable the modem
    
     xDelay(2000);
     sendATcmd(F("AT+CGDCONT=1,\"IP\",\"hologram\""), "OK", 2000);//set IPv4 and apn
     if(sendATcmd(F("AT+CREG?"), "+CREG: 0,5", 2000) == 0){//check that modem is registered as roaming
         sendATcmd(F("AT+COPS=1,2,50501"), "OK",50000); //register to telstra network
         sendATcmd(F("AT+CREG?"), "+CREG: 0,5", 2000); //check that modem is registered as roaming
     }
}

////sends at command, checks for reply////
bool sendATcmd(String ATcommand, char* expctAns, unsigned int timeout){
    uint32_t timeStart;
    bool answer;
    uint8_t a=0;
    
    do{a++;
    
    Serial.println(ATcommand);
    
    answer=0;
    
    timeStart = 0;


    delay(100);

    while( simCom.available() > 0) {
        simCom.read();    // Clean the input buffer
    }
    
    simCom.println(ATcommand);    // Send the AT command 


    uint8_t i = 0;
    timeStart = millis();
    memset(response, '\0', CHARBUFF);    // Initialize the string

    // this loop waits for the answer

    do{
        if(simCom.available() != 0){    
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
    while((answer == 0) && ((millis() - timeStart) < timeout)); 

    if (expctAns == "0"){
                answer = 1;
            }
    Serial.println(response);
    
    }while(answer == 0 && a < 5);
    
     a = 0;
     return answer;
}



////initialises sim on arduino startup////
void simInit(){
   
      sendATcmd(F("AT+IPR=9600"),"OK",1000);
      
      sendATcmd(F("ATE0"),"OK",1000);
      
      sendATcmd(F("AT&W0"),"OK",1000);
  
}


void CBCread(){
    //get GNSS data
    if (sendATcmd(F("AT+CBC"), "OK",1000)){
        
        storeCBCresponse();
        
    }
}



////SLEEPS FOR SET TIME////
void Sleepy(uint16_t ScanInterval){ //Sleep Time in seconds
    
    simCom.flush(); // must run before going to sleep
  
    Serial.flush(); // ensures that all messages have sent through serial before arduino sleeps
/*
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF); //8 seconds dosen't work on the 8mhz
    //advance millis timer as it is paused in sleep
    noInterrupts();
    timer0_millis += 4000;
    interrupts(); 
    */
    
    while(ScanInterval >= 1){
        LowPower.powerDown(SLEEP_1S, ADC_OFF, BOD_OFF); //8 seconds dosen't work on the 8mhz
        //advance millis timer as it is paused in sleep
        noInterrupts();
        timer0_millis += 1000;
        interrupts();
        
        ScanInterval -= 1;
    }
}

////TRANSMITS LAST GPS CORDINATES TO WEB////
void Transmit(){
    
    dataStr = "AT+HTTPPARA=\"URL\",\"www.bosl.com.au/IoT/UoM_CoM_Biofilters/scripts/WriteMe.php?SiteName=";
    
    dataStr += SITEID;
    dataStr += ".csv";
    dataStr += "&T=";
    dataStr += Temp; //temperature of the low cost sensor
    dataStr += "&D=";
    dataStr += Press; //pressure of the low cost sensor --> need to convert to water level based on temperature data
    dataStr += "&EC=";
    dataStr += EC; //EC data --> need to check the sensor's code to fix
    dataStr += "&U=";
    dataStr += turbidity; //Turbidity with LED
    dataStr += "&V=";
    dataStr += turbiditywoled; //Turbidity without LED
    dataStr += "&W=";
    dataStr += CBC;
    dataStr += "\"";

    
    
    netReg();

    ///***check logic
   //set CSTT - if it is already set, then no need to do again...
        sendATcmd(F("AT+CSTT?"), "OK",1000);   
        if (strstr(response, "hologram") != NULL){
            //this means the cstt has been set, so no need to set again!
            Serial.println("CSTT already set to APN ...no need to set again");
       } else {
            sendATcmd(F("AT+CSTT=\"hologram\""), "OK",1000);
        }
    
    
    //close open bearer
    sendATcmd(F("AT+SAPBR=2,1"), "OK",1000);
    if (strstr(response, "1,1") == NULL){
        if (strstr(response, "1,3") == NULL){
        sendATcmd(F("AT+SAPBR=0,1"), "OK",1000);
        }
        sendATcmd(F("AT+SAPBR=3,1,\"APN\",\"hologram\""), "OK",1000); //set bearer apn
        sendATcmd(F("AT+SAPBR=1,1"), "OK",1000);
    }

    
    sendATcmd(F("AT+HTTPINIT"), "OK",1000);
    sendATcmd(F("AT+HTTPPARA=\"CID\",1"), "OK",1000);
   
    sendATcmd(dataStr, "OK",1000);
   
   sendATcmd(F("AT+HTTPACTION=0"), "200",2000);
    sendATcmd(F("AT+HTTPTERM"), "OK",1000);
  //close the bearer connection
    sendATcmd(F("AT+SAPBR=0,1"), "OK",1000);
    
    netUnreg();

    lstTransmit = millis();
}
