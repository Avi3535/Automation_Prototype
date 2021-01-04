// Include necessary libraries
#include<Ethernet.h>
#include<EthernetUdp.h>
#include<SPI.h>
#include <LiquidCrystal.h>
#include <Keypad.h>

// Global declaration
byte mac[] = {0xA8, 0x61, 0x0A, 0xAE, 0x83, 0xCA};
char ip;
unsigned int localPort = 5000;
char packetBuffer[UDP_TX_PACKET_MAX_SIZE];
String datReq;
int packetSize;
EthernetUDP Udp;

const byte dir = 6, spd = 7; // Motor direction and speed pin to Arduino.

const byte chA = 19, chB = 20, chZ = 21; // Encoder pins to Arduino.
volatile int encval = 4; // To store encoder value.
byte dirval = 0; // To store direction value.
byte vessel[3]; // To take numeric input from the keypad.
byte ij = 0; // counter of vessel array.
int stopval; // Stopvalue of from keypad input.

const int pot = A0; // Potentiometer input to Arduino.
volatile int val;

const byte rs = 31, en = 30, d4 = 37, d5 = 36, d6 = 35, d7 = 34; // LCD pins to Arduino.
LiquidCrystal lcd(rs, en, d4, d5, d6, d7); // Initialize LCD.
byte li = 8; // Counter for LCD (not used till now).

const byte numRows= 4; // Keypad Number of Rows.
const byte numCols= 4; // Keypad Number of Cols.

// Each button of the Keypad.
char keymap[numRows][numCols]= { {'1', '2', '3', 'A'},

{'4', '5', '6', 'B'},

{'7', '8', '9', 'C'},

{'*', '0', '#', 'D'} };

byte rowPins[numRows] = {40,41,42,43}; //Rows 0 to 3.
byte colPins[numCols]= {44,45,46,47}; //Columns 0 to 3.
volatile char keypressed;

// Make a class.
Keypad myKeypad= Keypad(makeKeymap(keymap), rowPins, colPins, numRows, numCols);

int tog_fl = 1; // continuous

void check_input();

// ISR for Encoder Z Phase - Priority 1.
void ftr(){
  encval = 0;
  // Serial.println("Channel A");
  // Serial.println("START at 0"); 
  }

// ISR for encoder value.
void sre(){
  if (encval > 360) encval = 1;
  if (encval < 0) encval = 359;
  if(digitalRead(chB) == LOW) encval++;
  else encval--;
  Serial.println(encval);
  }

//Current Position Print on LCD
void curr_pos(){
  lcd.setCursor(10, 1);
  lcd.print("P:");
  lcd.print(encval);
  lcd.print("  ");
  }

// Initialize the motor
void initialize_motor(){
    encval = 4;
    digitalWrite(dir, LOW);
    analogWrite(spd, val);
    while(!(encval >= 359 || (encval >= 0 && encval <=3))){
    }
    encval = 0;
    digitalWrite(spd, 0);
    delay(500); // Wait for 2 secs, can reduce the delay.
    curr_pos();
  }

void enc_pos(){
  digitalWrite(dir, dirval);
  analogWrite(spd, val);    
  while (encval != stopval){
  curr_pos();
    }
  
  //lcd.print(encval);
  digitalWrite(spd, 0);
  delay(50);
  lcd.setCursor(0,0);
  lcd.print("                ");
  curr_pos();
  delay(50);
  
   
  }

  void keypd_C(){
      String SdatReq;
      vessel[0] = 0;
      vessel[1] = 0;
      vessel[2] = 0;
      lcd.setCursor(0,0);
      lcd.print("EnterVal : ");
      while(1){
        keypressed = myKeypad.getKey();
        packetSize = Udp.parsePacket();
        
        if (keypressed != NO_KEY || packetSize > 0) {
          Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
          String datReq(packetBuffer);   
          if( keypressed == 'C' || datReq == "C"){
          if (keypressed == 'C') stopval = atoi(vessel);
          else stopval = SdatReq.toInt();
          Serial.println(SdatReq); 
          lcd.setCursor(0,0);
          if (stopval > 360 || stopval <= 0) 
          {lcd.print("Input Not Found ");
          delay(1000);
          lcd.print("                ");
          Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
          Udp.print("Invalid Input");
          Udp.endPacket();
          break;
            }
          Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
          Udp.print("....");
          Udp.endPacket();
          lcd.print("Val:");
          lcd.print(stopval);
          lcd.print("    ");
          enc_pos();
          ij = 0;
          break;
          }
          else if(keypressed == '1' || keypressed == '2' || keypressed == '3' || keypressed == '4' || keypressed == '5' || keypressed == '6'|| keypressed == '7'|| keypressed == '8'|| keypressed == '9'|| keypressed == '0'){
            if(ij < 3){
              lcd.setCursor(ij+8,0);
              lcd.print(keypressed);
              vessel[ij] = keypressed;
              ij++;
              }
          }
          else {
          SdatReq = datReq; 
          Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
          Udp.print("Value Entered:  ");
          Udp.print(datReq);
          Udp.endPacket();

          }
          delay(10);
          memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);  
        }                  
        }
    
    }
  

void setup() {
  // put your setup code here, to run once:
  // put your setup code here, to run once:
  pinMode(dir, OUTPUT); // Motor direction as OP.
  pinMode(spd, OUTPUT); // Motor speed as OP.

  pinMode(chA, INPUT_PULLUP); // Enc (Reading) ChA as IN_PullUP (to avoid pulses).
  pinMode(chB, INPUT_PULLUP);
  pinMode(chZ, INPUT_PULLUP); // Enc (Zero Val) ChZ as IN_PullUP (to avoid pulses).
  pinMode(A2, INPUT);

  attachInterrupt(digitalPinToInterrupt(chA), sre, FALLING); // Go to this function when interrupt occurs.
  attachInterrupt(digitalPinToInterrupt(chZ), ftr, FALLING); // Go to this function when interrupt occurs.

  lcd.begin(16, 2); // Initialize the LCD

  // Print on the display the direction the otor is going to rotate.
  lcd.setCursor(0, 0);
  lcd.print("Initializing...");

  val = 130;
  
  initialize_motor(); // Motor to position 0
  
  Ethernet.init(10);  // Most Arduino shields
  Serial.begin(9600);

  Serial.println("Initialize Ethernet with DHCP:");
  if (Ethernet.begin(mac) == 0) {
    Serial.println("Failed to configure Ethernet using DHCP");
    if (Ethernet.hardwareStatus() == EthernetNoHardware) {
      Serial.println("Ethernet shield was not found.  Sorry, can't run without hardware. :(");
    } else if (Ethernet.linkStatus() == LinkOFF) {
      Serial.println("Ethernet cable is not connected.");
    }
    // no point in carrying on, so do nothing forevermore:
    while (true) {
      delay(1);
    }
  }
  // print your local IP address:
  
  Serial.print("My IP address: ");
  Serial.println(Ethernet.localIP());
  
  Ethernet.begin(mac, Ethernet.localIP());
  Udp.begin(localPort);
  delay(500);

  lcd.setCursor(0, 0);
  lcd.print("Completed       ");
  delay(500);
  lcd.setCursor(0, 0);
  lcd.print("                ");
  lcd.setCursor(0, 1);
  lcd.print("D:");
  lcd.print(dirval);
  
  lcd.setCursor(4, 1);
  lcd.print("S:");
  lcd.print(val);
  curr_pos();
  
  Serial.println("START");
}

void loop() {
  // put your main code here, to run repeatedly:
  checkinput();
  packetSize = Udp.parsePacket();

  if(packetSize > 0){
    Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
    String datReq(packetBuffer);

    if(datReq == "A"){
      Serial.println("A");
      dirval ^= 1;
      lcd.setCursor(0, 1);
      lcd.print("D:");
      lcd.print(dirval);
      
      Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
      Udp.print("Direction Toggled  ");
      Udp.print(dirval);
      Udp.endPacket();
      }
    else if(datReq == "C"){
      Serial.println("C");
      Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
      Udp.print("Enter Value : ");
      Udp.endPacket();
      keypd_C();
      }
    else if(datReq == "B"){
      Serial.println("B");
      Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
      Udp.print("Enter Speed : ");
      Udp.endPacket();
      while(1){
        packetSize = Udp.parsePacket();

        if(packetSize > 0){
        Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
        String datReq(packetBuffer);
        if (datReq == "B") break;
        int buf_val = datReq.toInt();
        if (buf_val > 12 && buf_val < 256){
        val = buf_val;
        lcd.setCursor(4, 1);
        lcd.print("S:");
        lcd.print(val);
        lcd.print(" ");
        Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
        Udp.print("Speed Entered : ");
        Udp.endPacket();
        Serial.println(val);
        break;
        }
      Udp.beginPacket(Udp.remoteIP(),Udp.remotePort());
      Udp.print("Speed Changed");
      Udp.endPacket();
        }
      }
    }
      else if (datReq == "*"){
      
      digitalWrite(dir, HIGH);
      tog_fl *= (-1); 
      if(tog_fl == 1) analogWrite(spd, 0);
      else analogWrite(spd, val);
      curr_pos();
      }
      else if  (datReq == "#"){
      
      digitalWrite(dir, LOW);
      tog_fl *= (-1); 
      if(tog_fl == 1) analogWrite(spd, 0);
      else analogWrite(spd, val);
      curr_pos();
      }
    delay(10);
    memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
}
}

// Input from Keypad.
void checkinput(){
  keypressed = myKeypad.getKey(); // Get ip from Keypad.

  if ((keypressed != NO_KEY) || (packetSize > 0)) // Enter this loop when a key is pressed.

  {
    if ((keypressed == 'A') || (datReq == "A")){ // If A is pressed toggle the direction and print on the LCD.
      
      dirval ^= 1;
      lcd.setCursor(0, 1);
      lcd.print("D:");
      lcd.print(dirval);
      
      }
    else if ((keypressed == 'B') || ((datReq == "B"))){ 
      while(1){
      val = analogRead(A2);
      Serial.print(val);
      Serial.print("     ");
      if (val <= 50) val = 0;
      else val = map(val,50,1023,12,255);
      lcd.setCursor(4, 1);
      lcd.print("S:");
      lcd.print(val);
      lcd.print(" ");
      Serial.println(val);
      packetSize = Udp.parsePacket();  
      if (keypressed != NO_KEY || packetSize > 0 ) {
      Udp.read(packetBuffer, UDP_TX_PACKET_MAX_SIZE);
      String datReq(packetBuffer);
      char keypressed = myKeypad.getKey(); // Get ip from Keypad.
      
      if ((keypressed != NO_KEY && keypressed == 'B') || (packetSize > 0 && datReq == "B")) break;
      
        }
      }
      
      
      delay(10);
      memset(packetBuffer, 0, UDP_TX_PACKET_MAX_SIZE);
      }
    
      
    else if ((keypressed == 'C') || ((datReq == "C"))){ // If user plans to go to a specific pos first press C.
      keypd_C();      
      }
    else if (keypressed == '*' || (datReq == "*")){
      
      digitalWrite(dir, HIGH);
      tog_fl *= (-1); 
      if(tog_fl == 1) analogWrite(spd, 0);
      else analogWrite(spd, val);
      curr_pos();
      }
      else if (keypressed == '#' || (datReq == "#")){
      
      digitalWrite(dir, LOW);
      tog_fl *= (-1); 
      if(tog_fl == 1) analogWrite(spd, 0);
      else analogWrite(spd, val);
      curr_pos();
      }
    else if (keypressed == '0' || (datReq == "0")){
      lcd.setCursor(0,0);
      lcd.print("RESET");
      initialize_motor();
      delay(100);
      lcd.setCursor(0,0);
      lcd.print("                ");
      }

   
  }
}
  
  
