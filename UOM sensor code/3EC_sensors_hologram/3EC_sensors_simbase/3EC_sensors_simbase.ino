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

// For the Depth sensor 1
#define DepEC_RX1 2
#define DepEC_TX1 3

//For the Depth sensor 2
#define DepEC_RX2 10
#define DepEC_TX2 11

//For the Depth sensor 3
#define DepEC_RX3 12
#define DepEC_TX3 13

//Site specific config
#define SITEID "Rain_Test"

//does not do anything atm // change values in transmit function
//#define APN "telstra.internet" //FOR TELSTRA
//#define APN "mdata.net.au" //FOR ALDI MOBILE

//default variable array (complilation purposes only)
bool defaultVars[10] = {1,1,1,1,1,1,1,1,1,1};

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
String DEPTH_EC1[3]; // 1-depth reading, 2-Temp reading, 3-EC reading
String DEPTH_EC2[3];// 1-depth reading, 2-Temp reading, 3-EC reading
String DEPTH_EC3[3];// 1-depth reading, 2-Temp reading, 3-EC reading
double pressVal1, tempVal1, ECVal1,pressVal2, tempVal2, ECVal2,pressVal3, tempVal3, ECVal3;
char Temp1[10];
char Press1[7];
char EC1[6];
char Temp2[10];
char Press2[7];
char EC2[6];
char Temp3[10];
char Press3[7];
char EC3[6];

//sensor variables
int scanInterval = 60;
uint8_t reps = 0;
double TempSum1 = 0, ECSum1 = 0, PressSum1 = 0,TempSum2 = 0, ECSum2 = 0, PressSum2 = 0,TempSum3 = 0, ECSum3 = 0, PressSum3 = 0 ;//To get the average of the 6 mins readings 

int transmit;

//SIM7000 serial object
SoftwareSerial simCom = SoftwareSerial(BOSL_RX, BOSL_TX);
SoftwareSerial DepECScan1 = SoftwareSerial(DepEC_RX1, DepEC_TX1);
SoftwareSerial DepECScan2 = SoftwareSerial(DepEC_RX2, DepEC_TX2);
SoftwareSerial DepECScan3 = SoftwareSerial(DepEC_RX3, DepEC_TX3);

 
////clears char arrays////
void charBuffclr(bool clrVars[10] = defaultVars){
    if(clrVars[0]){
    memset(Temp1, '\0', 10);
    }
    if(clrVars[1]){
    memset(Press1, '\0', 7);
    }
    if(clrVars[2]){
    memset(EC1,'\0',6);
    }
    if(clrVars[3]){
    memset(Temp2, '\0', 10);
    }
    if(clrVars[4]){
    memset(Press2, '\0', 7);
    }
    if(clrVars[5]){
    memset(CBC,'\0',5);
    }
    if(clrVars[6]){
    memset(Temp3, '\0', 10);
    }
    if(clrVars[7]){
    memset(Press3, '\0', 7);
    }
    if(clrVars[8]){
    memset(EC3,'\0',6);
    }
    if(clrVars[9]){
    memset(EC2,'\0',6);
    }
}

//convert float to string
void convertchart(){
  
  //water temperature 1
  dtostrf(tempVal1, 2, 2, Temp1);

  //water temperature 2
  dtostrf(tempVal2, 2, 2, Temp2);


  //water temperature 3
  dtostrf(tempVal3, 2, 2, Temp3);
  
  //water pressure 1
  dtostrf(pressVal1, 2, 2, Press1);
  
 //water pressure 2
  dtostrf(pressVal2, 2, 2, Press2);

 //water pressure 3
  dtostrf(pressVal3, 2, 2, Press3);
  
  //EC measurement 1
  dtostrf(ECVal1, 2, 2, EC1);

    //EC measurement 2
  dtostrf(ECVal2, 2, 2, EC2);

    //EC measurement 3
  dtostrf(ECVal3, 2, 2, EC3);
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
  DepECread1();
  DepECread2();
  DepECread3();

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
   Serial.print("Temp 1: ");
   Serial.println(Temp1);
   Serial.print("Depth 1: ");
   Serial.println(pressVal1);
   Serial.print("EC 1: ");
   Serial.println(ECVal1);
   
   Serial.print("Temp 2: ");
   Serial.println(Temp2);
   Serial.print("Depth 2: ");
   Serial.println(pressVal2);
   Serial.print("EC 2: ");
   Serial.println(ECVal2);
   
   Serial.print("Temp 3: ");
   Serial.println(Temp3);
   Serial.print("Depth 3: ");
   Serial.println(pressVal3);
   Serial.print("EC 3: ");
   Serial.println(ECVal3);

}
    
void loop() {

  //set up a calibration mode


    Serial.print("reps: ");
    Serial.println(reps);
    Serial.print("ECSum 1: ");
    Serial.println(ECSum1);
    Serial.print("TempSum 1: ");
    Serial.println(TempSum1);
    Serial.print("PressSum 1: ");
    Serial.println(PressSum1);
    
    Serial.print("ECSum 2: ");
    Serial.println(ECSum2);
    Serial.print("TempSum 2: ");
    Serial.println(TempSum2);
    Serial.print("PressSum 2: ");
    Serial.println(PressSum2);

    Serial.print("ECSum 3: ");
    Serial.println(ECSum3);
    Serial.print("TempSum 3: ");
    Serial.println(TempSum3);
    Serial.print("PressSum 3: ");
    Serial.println(PressSum3);
    
    Serial.println("Sleep");

  if(digitalRead(6) == 1){
    Sleepy(60);
  }
  else{
    Sleepy(scanInterval-18);
  }
    reps++; //counter

    charBuffclr();

    Serial.println("Scanning");
  
    pinMode(RESET, OUTPUT);
    digitalWrite(RESET, LOW);
    delay(10);
    digitalWrite(RESET, HIGH);
    pinMode(RESET, INPUT);
    
    DepECread1();
    TempSum1 += tempVal1;
    PressSum1 += pressVal1;
    ECSum1 += ECVal1;
    
    DepECread2();
    TempSum2 += tempVal2;
    PressSum2 += pressVal2;
    ECSum2 += ECVal2;

    DepECread3();
    TempSum3 += tempVal3;
    PressSum3 += pressVal3;
    ECSum3 += ECVal3;
    
    if(reps == 6){

      charBuffclr();
      simOn();
      netUnreg();
      CBCread();
      getaverage();
      convertchart();
      Transmit();
      
      reps = 0;

      TempSum1 = 0;
      PressSum1 = 0;
      ECSum1 = 0;
      
      TempSum2 = 0;
      PressSum2 = 0;
      ECSum2 = 0;
      
      TempSum3 = 0;
      PressSum3 = 0;
      ECSum3 = 0;
      
      simOff();
    }
   
      

}

void DepECread1(){
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
  
  DepECScan1.begin(9600);
  DepECScan1.listen();
  
  tout = millis();

  while(millis() - tout < 10000){
    if(DepECScan1.available() != 0){
      //Serial.println("Start");
      bytin = DepECScan1.read();
      if(bytin = 'R'){
        break;
      }
    }
  }

  DepECScan1.print('V');

  DepECScan1.setTimeout(1500);
  DepECScan1.readBytes(data, 20);
  input = data;

  //Serial.println(data);

  for(int i = 0; i<input.length(); i++){
    if(input.substring(i,i+1) == ","){
      // Grab the piece from the last index up to the current position and store it
      DEPTH_EC1[counter] = input.substring(lastIndex, i);
      // Update the last position and add 1, so it starts from the next character
      lastIndex = i + 1;
      // Increase the position in the array that we store into
      counter++;
    }

    // If we're at the end of the string (no more commas to stop us)
    if(i == input.length() - 1){
      // Grab the last part of the string from the lastIndex to the end
      DEPTH_EC1[counter] = input.substring(lastIndex, i);
    }
  }
  pressVal1 = DEPTH_EC1[0].toFloat();
  tempVal1 = DEPTH_EC1[1].toFloat();
  ECVal1 = DEPTH_EC1[2].toFloat();
  
  input = "";
  counter = 0;
  lastIndex = 0;

  DepECScan1.print('S');

  DepECScan1.flush();
  DepECScan1.end();

  Serial.println(F("Depth 1, Temp 1 and EC 1: "));
  Serial.print(pressVal1);
  Serial.print(" ");
  Serial.print(tempVal1);
  Serial.print(" ");
  Serial.println(ECVal1);

}

void DepECread2(){
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
  
  DepECScan2.begin(9600);
  DepECScan2.listen();
  
  tout = millis();

  while(millis() - tout < 10000){
    if(DepECScan2.available() != 0){
      //Serial.println("Start");
      bytin = DepECScan2.read();
      if(bytin = 'R'){
        break;
      }
    }
  }

  DepECScan2.print('V');

  DepECScan2.setTimeout(1500);
  DepECScan2.readBytes(data, 20);
  input = data;

  //Serial.println(data);

  for(int i = 0; i<input.length(); i++){
    if(input.substring(i,i+1) == ","){
      // Grab the piece from the last index up to the current position and store it
      DEPTH_EC2[counter] = input.substring(lastIndex, i);
      // Update the last position and add 1, so it starts from the next character
      lastIndex = i + 1;
      // Increase the position in the array that we store into
      counter++;
    }

    // If we're at the end of the string (no more commas to stop us)
    if(i == input.length() - 1){
      // Grab the last part of the string from the lastIndex to the end
      DEPTH_EC2[counter] = input.substring(lastIndex, i);
    }
  }
  pressVal2 = DEPTH_EC2[0].toFloat();
  tempVal2 = DEPTH_EC2[1].toFloat();
  ECVal2 = DEPTH_EC2[2].toFloat();
  
  input = "";
  counter = 0;
  lastIndex = 0;

  DepECScan2.print('S');

  DepECScan2.flush();
  DepECScan2.end();

  Serial.println(F("Depth 2, Temp 2 and EC 2: "));
  Serial.print(pressVal2);
  Serial.print(" ");
  Serial.print(tempVal2);
  Serial.print(" ");
  Serial.println(ECVal2);

}

void DepECread3(){
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
  
  DepECScan3.begin(9600);
  DepECScan3.listen();
  
  tout = millis();

  while(millis() - tout < 10000){
    if(DepECScan3.available() != 0){
      //Serial.println("Start");
      bytin = DepECScan3.read();
      if(bytin = 'R'){
        break;
      }
    }
  }

  DepECScan3.print('V');

  DepECScan3.setTimeout(1500);
  DepECScan3.readBytes(data, 20);
  input = data;

  //Serial.println(data);

  for(int i = 0; i<input.length(); i++){
    if(input.substring(i,i+1) == ","){
      // Grab the piece from the last index up to the current position and store it
      DEPTH_EC3[counter] = input.substring(lastIndex, i);
      // Update the last position and add 1, so it starts from the next character
      lastIndex = i + 1;
      // Increase the position in the array that we store into
      counter++;
    }

    // If we're at the end of the string (no more commas to stop us)
    if(i == input.length() - 1){
      // Grab the last part of the string from the lastIndex to the end
      DEPTH_EC3[counter] = input.substring(lastIndex, i);
    }
  }
  pressVal3 = DEPTH_EC3[0].toFloat();
  tempVal3 = DEPTH_EC3[1].toFloat();
  ECVal3 = DEPTH_EC3[2].toFloat();
  
  input = "";
  counter = 0;
  lastIndex = 0;

  DepECScan3.print('S');

  DepECScan3.flush();
  DepECScan3.end();

  Serial.println(F("Depth 3, Temp 3 and EC 3: "));
  Serial.print(pressVal3);
  Serial.print(" ");
  Serial.print(tempVal3);
  Serial.print(" ");
  Serial.println(ECVal3);

  simCom.listen();
}

void getaverage(){

    ECVal1 = (double) ECSum1 / (double) reps;
    tempVal1 = (double) TempSum1 / (double) reps;
    pressVal1 = (double) PressSum1 / (double) reps;

    ECVal2 = (double) ECSum2 / (double) reps;
    tempVal2 = (double) TempSum2 / (double) reps;
    pressVal2 = (double) PressSum2 / (double) reps;

     ECVal3 = (double) ECSum3 / (double) reps;
    tempVal3 = (double) TempSum3 / (double) reps;
    pressVal3 = (double) PressSum3 / (double) reps;

    ECSum1 = TempSum1 = PressSum1 = ECSum2 = TempSum2 = PressSum2 = ECSum3 = TempSum3 = PressSum3 =0;
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
    
    bool clrVars[10] = {0,0,0,0,0,1,0,0,0,0};
    
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
     sendATcmd(F("AT+CGDCONT=1,\"IP\",\"simbase\""), "OK", 2000);//set IPv4 and apn
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
    
    dataStr = "AT+HTTPPARA=\"URL\",\"www.bosl.com.au/IoT/testing/scripts/WriteMe.php?SiteName=";
    
    dataStr += SITEID;
    dataStr += ".csv";
    dataStr += "&T=";
    dataStr += Press1; //pressure of the low cost sensor --> need to convert to water level based on temperature data
    dataStr += "&EC=";
    dataStr += Temp1; //temperature of the low cost sensor
    dataStr += "&D=";
    dataStr += EC1; //EC data --> need to check the sensor's code to fix
    dataStr += "&B=";
    dataStr += Press2; //pressure of the low cost sensor --> need to convert to water level based on temperature data
    dataStr += "&U=";
    dataStr += Temp2; //temperature of the low cost sensor
    dataStr += "&V=";
    dataStr += EC2; //EC data --> need to check the sensor's code to fix
    dataStr += "&W=";
    dataStr += Press3; //pressure of the low cost sensor --> need to convert to water level based on temperature data
    dataStr += "&X=";
    dataStr += Temp3; //temperature of the low cost sensor
    dataStr += "&Y=";
    dataStr += EC3; //EC data --> need to check the sensor's code to fix
    dataStr += "&Z=";
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
            sendATcmd(F("AT+CSTT=\"simbase\""), "OK",1000);
        }
    
    
    //close open bearer
    sendATcmd(F("AT+SAPBR=2,1"), "OK",1000);
    if (strstr(response, "1,1") == NULL){
        if (strstr(response, "1,3") == NULL){
        sendATcmd(F("AT+SAPBR=0,1"), "OK",1000);
        }
        sendATcmd(F("AT+SAPBR=3,1,\"APN\",\"simbase\""), "OK",1000); //set bearer apn
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
