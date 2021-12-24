#include <Wire.h> 
#include <LiquidCrystal_I2C.h>
#include <EEPROM.h>
#include <Time.h>
#include <TimeAlarms.h>

#define DEBUG 0
#define CONFIG_VERSION "PMP4"
#define CONFIG_START 20

#define buttonPin 4

#define POMP1_PIN 9
#define POMP2_PIN 8
#define POMP3_PIN 7
#define POMP4_PIN 6


void(* resetFunc) (void) = 0; //declare reset function @ address 0

struct mnuItem{
	uint8_t next; 
	uint8_t  prev; 
        uint8_t  push; 
        uint8_t  up;
	char *caption;  // Menu caption
};


struct mnuItem Menu[] = {
//  next   prev   push up text
   {1,     42,    2,   0, "Settings        "},       //0
   {32,    0,     1,   0,  "Run Config      "},       //1
//                                                   //32
   {3,     6,     7,   0, "Pomp 1          "},       //2   
   {4,     2,     13,  0, "Pomp 2          "},       //3   
   {5,     3,     19,  0, "Pomp 3          "},       //4   
   {31,    4,     25,  0, "Pomp 4          "},       //5   
// ratio ....                                         31
   {2,    31,     0,   0, "Back            "},       //6      

   {8,     12,    7,   2, "Pomp 1 On/Off   "},       //7   
   {9,     7,     8,   2, "Pomp 1 PW Offset"},       //8   
   {10,     8,     9,  2, "Pomp 1 Frequency"},       //9   
   {11,     9,    10,  2, "Pomp 1 Dosage   "},       //10   
   {12,    10,    11,  2, "Pomp 1 Calibrate"},       //11   
   {7,     11,     2,  2, "Back            "},        //12      

   {14,    18,    13,  3, "Pomp 2 On/Off   "},       //13   
   {15,    13,    14,  3, "Pomp 2 PW Offset"},       //14  
   {16,    14,    15,  3, "Pomp 2 Frequency"},       //15  
   {17,    15,    16,  3, "Pomp 2 Dosage   "},       //16   
   {18,    16,    17,  3, "Pomp 2 Calibrate"},       //17   
   {13,    17,     3,  3, "Back            "},        //18      

   {20,    24,    19,  4, "Pomp 3 On/Off   "},       //19   
   {21,    19,    20,  4, "Pomp 3 PW Offset"},       //20  
   {22,    20,    21,  4, "Pomp 3 Frequency"},       //21  
   {23,    21,    22,  4, "Pomp 3 Dosage   "},       //22   
   {24,    22,    23,  4, "Pomp 3 Calibrate"},       //23   
   {19,    23,     4,  4, "Back            "},        //24      

   {26,    30,    25,  5, "Pomp 4 On/Off   "},       //25   
   {27,    25,    26,  5, "Pomp 4 PW Offset"},       //26  
   {28,    26,    27,  5, "Pomp 4 Frequency"},       //27  
   {29,    27,    28,  5, "Pomp 4 Dosage   "},       //28   
   {30,    28,    29,  5, "Pomp 4 Calibrate"},       //29   
   {25,    29,     5,  5, "Back            "},       //30 
   
   {6,    5,     31,  0,  "Ratio           "},        //31 
   
   
   {42,     1,      33,  0,    "Run Manually    "},        //32
   {34,    41,     33,  32,  "Run Pomp1       "},        //33
   {35,    33,     34,  32,  "Run Pomp2       "},        //34
   {36,    34,     35,  32,  "Run Pomp3       "},        //35
   {37,    35,     36,  32,  "Run Pomp4       "},        //36
   {38,    36,     37,  32,  "Run Pomp1 Dosage"},        //37
   {39,    37,     38,  32,  "Run Pomp2 Dosage"},        //38
   {40,    38,     39,  32,  "Run Pomp3 Dosage"},        //39
   {41,    39,     40,  32,  "Run Pomp4 Dosage"},        //40   
   {33,    40,     32,  32,  "Back            "},        //41
   
   {0,    32,     42,  42,   "                "}        //42   
   
};


uint16_t  pump_counter[4];

uint8_t  debug = DEBUG;
//int touch;
uint8_t pomp = 999;
uint8_t menu_index = 0;
uint8_t prev_menu_index = 0;
//int couter = 0;
uint8_t pomp_pins[4];

int freeram() {
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);


struct StoreStruct {
  // This is for detection if they are your settings
  char version[5];
  int ratio;

  int onoff[4];
  int pwr_off[4];
  int offset[4];
  int dosage[4];
  int calib[4];
      
} 
storage = {
  CONFIG_VERSION,
  100,
  // The default values
  {
    1,1,1,0    }
  ,
  {
    5,35,20,20 }
  ,
  {
    60,60,60,1}
  ,
  {
    5,5,2,10}
  ,
  {
    800,800,800,800    }
   
};


//-------------------------------------------


//------------------------------------------
//
// EEPROM


void loadConfig() {
  // To make sure there are settings, and they are YOURS!
  // If nothing is found it will use the default settings.
  if (EEPROM.read(CONFIG_START + 0) == CONFIG_VERSION[0] &&
    EEPROM.read(CONFIG_START + 1) == CONFIG_VERSION[1] &&
    EEPROM.read(CONFIG_START + 2) == CONFIG_VERSION[2] &&
    EEPROM.read(CONFIG_START + 3) == CONFIG_VERSION[3])
    for (unsigned int t=0; t<sizeof(storage); t++)
      *((char*)&storage + t) = EEPROM.read(CONFIG_START + t);
}

void saveConfig() {
  for (unsigned int t=0; t<sizeof(storage); t++)
    EEPROM.write(CONFIG_START + t, *((char*)&storage + t));
}


//----------------------------------------


enum PinAssignments {
  encoderPinA = 2,   // rigth
  encoderPinB = 3,   // left
  Button = 4    
};

volatile unsigned int encoderPos = 30000;  // a counter for the dial
volatile unsigned int button = 0;  // a counter for the dial
volatile unsigned int lastbutton = 0;  // a counter for the dial
unsigned int lastReportedPos = 30000;   // change management
static boolean rotating=false;      // debounce management
// interrupt service routine vars
boolean A_set = false;              
boolean B_set = false;


void setup()
{
  //pumps.
  pinMode(POMP1_PIN, OUTPUT);
  pinMode(POMP2_PIN, OUTPUT);
  pinMode(POMP3_PIN, OUTPUT);
  pinMode(POMP4_PIN, OUTPUT);
  digitalWrite(POMP1_PIN, LOW);
  digitalWrite(POMP2_PIN, LOW);
  digitalWrite(POMP3_PIN, LOW);
  digitalWrite(POMP4_PIN, LOW);

// led.  
  pinMode(13, OUTPUT);
  digitalWrite(13 , HIGH);
  
  
  lcd.begin(16,2);
  
//  lcd.setCursor(0,0);
//  lcd.print("Free RAM");
//  lcd.setCursor(0,1);
//  lcd.print(freeRam());
//  delay(3000);


  lcd_blink();
  
  lcd.backlight(); // finish with backlight on  
// welcome screen LCD.
  lcd.clear();


  pinMode(encoderPinA, INPUT); 
  pinMode(encoderPinB, INPUT); 
  pinMode(Button, INPUT);
 // turn on pullup resistors
  digitalWrite(encoderPinA, HIGH);
  digitalWrite(encoderPinB, HIGH);
  digitalWrite(Button, HIGH);

// encoder pin on interrupt 0 (pin 2)
  attachInterrupt(0, doEncoderA, CHANGE);
// encoder pin on interrupt 1 (pin 3)
  attachInterrupt(1, doEncoderB, CHANGE);

//button
  pinMode(Button, INPUT);
  digitalWrite(Button, HIGH);

// pins
  pomp_pins[0] = POMP1_PIN;
  pomp_pins[1] = POMP2_PIN;
  pomp_pins[2] = POMP3_PIN;
  pomp_pins[3] = POMP4_PIN;
  



  

  Serial.begin(9600);
  Serial.println(F("Starting...."));
  delay(1000);


// load configuration from EEPROM.
  loadConfig();  
 
// check duration.
//   float duration = ((storage.dosage[0] * storage.ratio) / 100)*(float)((float)storage.calib[0]/1000);
//  float duration = ((float)((float)storage.dosage[0] * storage.ratio) / 100)*(float)((float)storage.calib[0]);
//  lcd.clear();
//  lcd.setCursor(0,0);
//  lcd.println(duration);
//  delay(2000);
///


  lcd.clear();
  lcd.setCursor(0,0);
  lcd.println(Menu[menu_index].caption);
  
  
  // setup the power off alarms.
  setup_alarms();


//  lcd.setCursor(0,0);
//  lcd.print("Free RAM");
//  lcd.setCursor(0,1);
//  lcd.print(freeRam());
//  delay(3000);

  
}

void loop()
{
  
  Alarm.delay(10);

  blink_13_led();
  


  if (menu_index == 42)
  {
  lcd.setCursor(0,0);
  lcd.print(F("P1="));       
  if (storage.onoff[0] == 1)
    {lcd.print(pump_counter[0]);}
    else {lcd.print("OFF");}

  lcd.setCursor(7,0);
  lcd.print(F(" P2="));
  if (storage.onoff[1] == 1)
    {lcd.print(pump_counter[1]);}
    else {lcd.print("OFF");}
    
  lcd.setCursor(0,1);
  lcd.print(F("P3="));       
  if (storage.onoff[2] == 1)
    {lcd.print(pump_counter[2]);}
    else {lcd.print("OFF");}

  lcd.setCursor(7,1);
  lcd.print(F(" P4="));
  if (storage.onoff[3] == 1)
    {lcd.print(pump_counter[3]);}
    else {lcd.print("OFF");}
  }

  
  rotating = true;  // reset the debouncer

//  lastbutton = button;
// read button
//  if (digitalRead(Button) == LOW)
//  { button = 1;}
//  else
//  { button = 0;}




  if (lastReportedPos != encoderPos) {
   
//   Serial.println(encoderPos);
//   Serial.print(encoderPos);
//   Serial.print(" ");
//   Serial.print(lastReportedPos);
   
//   Serial.print(" ");
//   Serial.println(encoderPos - lastReportedPos);

   if ( (encoderPos - lastReportedPos) == 1)
   {
       prev_menu_index = menu_index;
       menu_index = Menu[menu_index].next;
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.println(Menu[menu_index].caption);
       
//       Serial.println(">>>");
   }
   else
   {
       prev_menu_index = menu_index;
       menu_index = Menu[menu_index].prev;     
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.println(Menu[menu_index].caption);
//       Serial.println("<<<");       
   }
   
   
// set the pump.
       if (Menu[menu_index].up == 2)
       { pomp = 0; }  
       if (Menu[menu_index].up == 3)
       { pomp = 1; }  
       if (Menu[menu_index].up == 4)
       { pomp = 2; }  
       if (Menu[menu_index].up == 5)
       { pomp = 3; } 

// display 2nd line information.
       second_line();


  //  musi byc.
    lastReportedPos = encoderPos;
   
  }
   

   if (digitalRead(Button) == LOW )
   {
   button = 1;
   }
   else
   {
   button = 0;
   }
  
    

   if ( button != lastbutton && lastbutton == 1)
   {
          
//     couter = couter + 1;
//     Serial.println(couter);
    
    delay(500);
    
//    menu.moveRight();
//    menu.use();

       prev_menu_index = menu_index;
       menu_index = Menu[menu_index].push;     
       lcd.clear();
       lcd.setCursor(0,0);
       lcd.println(Menu[menu_index].caption);

       if (Menu[menu_index].up == 2)
       { pomp = 0; }  
       if (Menu[menu_index].up == 3)
       { pomp = 1; }  
       if (Menu[menu_index].up == 4)
       { pomp = 2; }  
       if (Menu[menu_index].up == 5)
       { pomp = 3; }  
       
       
       second_line();
       

// wcisniecie opcji....

       if ( prev_menu_index == menu_index )
       {

       
// pomp setting ON / OFF
    if ( menu_index == 7 || menu_index == 13 || menu_index == 19 || menu_index == 25)
       {
        int difference;
        int new_value;
        new_value = storage.onoff[pomp];
        difference = encoderPos - new_value; 
        do    {
            lcd.setCursor(0,1);
            if (new_value == 1 ) 
            {
              lcd.println("ON");    
            }
            else
            {
              lcd.println("OFF");                
            }
            new_value = encoderPos - difference;       
            if (new_value < 0) {new_value = 0;}
            if (new_value > 1) {new_value = 1;}
                     
            } while ( digitalRead(Button) != LOW );
    
          storage.onoff[pomp] = new_value;    
          saveConfig();    
          lcd_blink();    
          delay(500);    
          menu_index = Menu[menu_index].up;                
       }  
       
// pomp power on offset 
    if ( menu_index == 8 || menu_index == 14 || menu_index == 20 || menu_index == 26)
       {
        int difference;
        int new_value;
        new_value = storage.pwr_off[pomp];
        difference = encoderPos - new_value; 
        do    {
            lcd.setCursor(0,1);
            lcd.println(new_value);    
            new_value = encoderPos - difference;       
            if (new_value < 0) {new_value = 0;}
            if (new_value > 360) {new_value = 360;}
                     
            } while ( digitalRead(Button) != LOW );
    
          storage.pwr_off[pomp] = new_value;    
          saveConfig();    
          lcd_blink();    
          delay(500);    
          menu_index = Menu[menu_index].up;                
       }         


// pomp offset / frequency 
    if ( menu_index == 9 || menu_index == 15 || menu_index == 21 || menu_index == 27)
       {
        int difference;
        int new_value;
        new_value = storage.offset[pomp];
        difference = encoderPos - new_value; 
        do    {
            lcd.setCursor(0,1);
            lcd.println(new_value);    
            new_value = encoderPos - difference;       
            if (new_value < 0) {new_value = 0;}
            if (new_value > 1200) {new_value = 1200;}
                     
            } while ( digitalRead(Button) != LOW );
    
          storage.offset[pomp] = new_value;    
          saveConfig();    
          lcd_blink();    
          delay(500);    
          menu_index = Menu[menu_index].up;                
       }         
       

// pomp dosage 
    if ( menu_index == 10 || menu_index == 16 || menu_index == 22 || menu_index == 28)
       {
        int difference;
        int new_value;
        new_value = storage.dosage[pomp];
        difference = encoderPos - new_value; 
        do    {
            lcd.setCursor(0,1);
            lcd.println(new_value);    
            new_value = encoderPos - difference;       
            if (new_value < 0) {new_value = 0;}
            if (new_value > 300) {new_value = 300;}
                     
            } while ( digitalRead(Button) != LOW );
    
          storage.dosage[pomp] = new_value;    
          saveConfig();    
          lcd_blink();    
          delay(500);    
          menu_index = Menu[menu_index].up;                
       } 


   if ( menu_index == 11 || menu_index == 17 || menu_index == 23 || menu_index == 29)
       {
        int difference;
        int new_value;
        new_value = storage.calib[pomp];
        difference = encoderPos - new_value; 
        do    {
            lcd.setCursor(0,1);
            lcd.println(new_value);    
            new_value = encoderPos - difference;       
            if (new_value < 0) {new_value = 0;}
            if (new_value > 2000) {new_value = 2000;}
                     
            } while ( digitalRead(Button) != LOW );
    
          storage.calib[pomp] = new_value;    
          saveConfig();    
          lcd_blink();    
          delay(500);    
          menu_index = Menu[menu_index].up;                
       } 


// pomp run manually 
    if ( menu_index == 33 || menu_index == 34 || menu_index == 35 || menu_index == 36)
       {
        if ( menu_index == 33 ) { pomp = 0;}
        if ( menu_index == 34 ) { pomp = 1;}
        if ( menu_index == 35 ) { pomp = 2;}        
        if ( menu_index == 36 ) { pomp = 3;} 
        
        
        pomp_start(pomp);
         do    {
           
           lcd_blink();
                     
            } while ( digitalRead(Button) != LOW );
        pomp_stop(pomp);    
          delay(500);    
          menu_index = Menu[menu_index].up;                
       } 



// pomp run manually with dosage. 
    if ( menu_index == 37 || menu_index == 38 || menu_index == 39 || menu_index == 40)
       {
        if ( menu_index == 37 ) { pomp = 0; P1Run();}
        if ( menu_index == 38 ) { pomp = 1; P2Run();}
        if ( menu_index == 39 ) { pomp = 2; P3Run();}        
        if ( menu_index == 40 ) { pomp = 3; P4Run();} 
           
          delay(500);    
          menu_index = Menu[menu_index].up;                
       } 


// ratio setting.
    if ( menu_index == 31) 
       {
        int difference;
        int new_value;
        new_value = storage.ratio;
        difference = encoderPos - new_value; 
        do    {
            lcd.setCursor(0,1);
            lcd.print(new_value);
            lcd.println(" %");    
            new_value = encoderPos - difference;       
            if (new_value < 0) {new_value = 0;}
            if (new_value > 200) {new_value = 200;}
                     
            } while ( digitalRead(Button) != LOW );
    
          storage.ratio = new_value;    
          saveConfig();    
          lcd_blink();    
          delay(500);    
          menu_index = Menu[menu_index].up;                
       } 
       


// run pumps and config again.....
    if ( menu_index == 1) 
       {
          lcd_blink();    
          lcd_blink();    
          lcd_blink();    
          delay(500);    
          
          // soft reset.
          resetFunc();  //call reset

       } 


       
       prev_menu_index = 0;
       }
       
   }
  
  lastbutton = button;


 
  
  
}



// Interrupt on A changing state
void doEncoderA(){
  // debounce
  if ( rotating ) delay (1);  // wait a little until the bouncing is done

  // Test transition, did things really change? 
  if( digitalRead(encoderPinA) != A_set ) {  // debounce once more
    A_set = !A_set;

    // adjust counter + if A leads B
    if ( A_set && !B_set ) 
      encoderPos += 1;

    rotating = false;  // no more debouncing until loop() hits again
  }
}

// Interrupt on B changing state, same as A above
void doEncoderB(){
  if ( rotating ) delay (1);
  if( digitalRead(encoderPinB) != B_set ) {
    B_set = !B_set;
    //  adjust counter - 1 if B leads A
    if( B_set && !A_set ) 
      encoderPos -= 1;

    rotating = false;
  }
}



void lcd_blink()
{
  // ------- Quick 3 blinks of backlight  -------------
  for(int i = 0; i< 2; i++)
  {
    lcd.backlight();
    delay(200);
    lcd.noBacklight();
    delay(200);
  }
  lcd.backlight(); // finish with backlight on  


}


void second_line()
{
// display on 2nd line information.
    if ( menu_index == 7 || menu_index == 13 || menu_index == 19 || menu_index == 25)
    {       
       lcd.setCursor(0,1);
       if ( storage.onoff[pomp] == 1 )
       {
         lcd.println("ON");   
       } else {
         lcd.println("OFF");            
       }     
       
    }
    
    if ( menu_index == 8 || menu_index == 14 || menu_index == 20 || menu_index == 26)
       {
                lcd.setCursor(0,1);
                lcd.println(storage.pwr_off[pomp]);
       }

    if ( menu_index == 9 || menu_index == 15 || menu_index == 21 || menu_index == 27)
       {
         
                lcd.setCursor(0,1);
                lcd.println(storage.offset[pomp]);
         
       }
       
   if ( menu_index == 10 || menu_index == 16 || menu_index == 22 || menu_index == 28)
       {
         
                lcd.setCursor(0,1);
                lcd.println(storage.dosage[pomp]);       
         
       }

   if ( menu_index == 11 || menu_index == 17 || menu_index == 23 || menu_index == 29)
       {
         
                lcd.setCursor(0,1);
                lcd.println(storage.calib[pomp]);       
         
       }
       
   if ( menu_index == 31 )
       {
         
                lcd.setCursor(0,1);
                lcd.print(storage.ratio);
                lcd.println(" %");
      
       }


   if ( menu_index == 0 && debug == 1)
       {
         
                lcd.setCursor(0,1);
                lcd.print(freeram());
                lcd.println(" bytes");
      
       }


}


void disp_time()
{
 Serial.print(F(" ["));
 Serial.print(hour());
 Serial.print(F(":"));
 Serial.print(minute()); 
 Serial.print(F(":"));
 Serial.print(second()); 
 Serial.print(F("] free ram = "));
 Serial.println(freeram());
}

void P1PowerOffFinished()
{
 disp_time();
 Serial.println(F("Pomp1 POFF")); 
 
 int count_sec;  
 count_sec = storage.offset[0]*60;
 Alarm.timerRepeat(count_sec, P1Run);  
 P1Run();
 pump_counter[0] = storage.offset[0];  
}

void P2PowerOffFinished()
{
 disp_time();
 Serial.println(F("Pomp2 POFF")); 
 
 int count_sec;  
 count_sec = storage.offset[1]*60;
 Alarm.timerRepeat(count_sec, P2Run);   
 P2Run();
 pump_counter[1] = storage.offset[1];  
}

void P3PowerOffFinished()
{
 disp_time();
 Serial.println(F("Pomp3 POFF")); 
 
 int count_sec;  
 count_sec = storage.offset[2]*60;
 Alarm.timerRepeat(count_sec, P3Run);  
 P3Run();
 pump_counter[2] = storage.offset[2]; 
}

void P4PowerOffFinished()
{
 disp_time();
 Serial.println(F("Pomp4 POFF")); 
 
 int count_sec;  
 count_sec = storage.offset[3]*60;
 Alarm.timerRepeat(count_sec, P4Run);  
 P4Run();
 pump_counter[3] = storage.offset[3];  
}


void P1Run()
{ 
 uint32_t start_pmp;
 start_pmp = now();
 float duration = ((float) storage.dosage[0] * (float) storage.ratio) / 100;
 duration = (duration * (float)storage.calib[0]) / 1000;
 
 Serial.print(F("P1 ON")); disp_time();
     pomp_start(0);
 while ((uint32_t)(start_pmp + duration) >= now() );
    pomp_stop(0);
 Serial.print(F("P1 OFF")); disp_time();

 pump_counter[0] = storage.offset[0];
 
}

void P2Run()
{
 uint32_t start_pmp;
 start_pmp = now();
 float duration = ((float) storage.dosage[1] * (float) storage.ratio) / 100;
 duration = (duration * (float)storage.calib[1]) / 1000;

 Serial.print(F("P2 ON"));   disp_time();
     pomp_start(1);
 while ((uint32_t)(start_pmp + duration) >= now() );
    pomp_stop(1);
 Serial.print(F("P2 OFF"));     disp_time();
 
 pump_counter[1] = storage.offset[1]; 
}

void P3Run()
{
 uint32_t start_pmp;
 start_pmp = now();
 float duration = ((float) storage.dosage[2] * (float) storage.ratio) / 100;
 duration = (duration * (float)storage.calib[2]) / 1000;

 Serial.print(F("P3 ON"));   disp_time();
     pomp_start(2);
 while ((uint32_t)(start_pmp + duration) >= now() );
    pomp_stop(2);
 Serial.print(F("P3 OFF"));     disp_time();
 
 pump_counter[2] = storage.offset[2]; 
}

void P4Run()
{
 uint32_t start_pmp;
 start_pmp = now();
 float duration = ((float) storage.dosage[3] * (float) storage.ratio) / 100;
 duration = (duration * (float)storage.calib[3]) / 1000;

 Serial.print(F("P4 ON"));   disp_time();
     pomp_start(3);
 while ((uint32_t)(start_pmp + duration) >= now() );
    pomp_stop(3);
 Serial.print(F("P4 OFF"));     disp_time();
 
 pump_counter[3] = storage.offset[3]; 
}


void setup_alarms()
{
  int count_sec;  
  

// setup counters for pump delay.
  pump_counter[0] = storage.pwr_off[0];
  pump_counter[1] = storage.pwr_off[1];
  pump_counter[2] = storage.pwr_off[2];
  pump_counter[3] = storage.pwr_off[3];

// run timer for couters every 1 minute.
 Alarm.timerRepeat(60, refresh_couters);  


  
  if (storage.onoff[0] == 1)
  {
  count_sec = storage.pwr_off[0]*60;
  Alarm.timerOnce(count_sec, P1PowerOffFinished);  
  Serial.println(F("Pomp1 started")); 
  }

  if (storage.onoff[1] == 1)
  {
  count_sec = storage.pwr_off[1]*60;
  Alarm.timerOnce(count_sec, P2PowerOffFinished);  
  Serial.println(F("Pomp2 started")); 
  }
  
  if (storage.onoff[2] == 1)
  {
  count_sec = storage.pwr_off[2]*60;
  Alarm.timerOnce(count_sec, P3PowerOffFinished);  
  Serial.println(F("Pomp3 started")); 
  }

  if (storage.onoff[3] == 1)
  {
  count_sec = storage.pwr_off[3]*60;
  Alarm.timerOnce(count_sec, P4PowerOffFinished);  
  Serial.println(F("Pomp4 started")); 
  }

  Serial.print(F("Free RAM = "));
  Serial.println(freeram());
 
  
}

void refresh_couters()
{
  pump_counter[0] = pump_counter[0] - 1;
  pump_counter[1] = pump_counter[1] - 1;
  pump_counter[2] = pump_counter[2] - 1;
  pump_counter[3] = pump_counter[3] - 1;
  if (pump_counter[0] < 0) {pump_counter[0] = 0;}
  if (pump_counter[1] < 0) {pump_counter[1] = 0;}  
  if (pump_counter[2] < 0) {pump_counter[2] = 0;}
  if (pump_counter[3] < 0) {pump_counter[3] = 0;}  

  lcd.clear();
}


void pomp_start(int pomp)
{
  digitalWrite(pomp_pins[pomp], HIGH);
}

void pomp_stop(int pomp)
{
  digitalWrite(pomp_pins[pomp], LOW);
}

void blink_13_led()
{
 if ((second() % 2 ) == 1)
 {
  digitalWrite(13 , HIGH);
  lcd.setCursor(15,0);
  lcd.print('.');
 }
 else
 {
  digitalWrite(13 , LOW); 
  lcd.setCursor(15,0);
  lcd.print(' ');
 }

}
