/***************************************************************************

                     Copyright 2008 Gravitech
                        All Rights Reserved

****************************************************************************/

/***************************************************************************
 File Name: I2C_7SEG_Temperature.pde

 Hardware: Arduino Diecimila with 7-SEG Shield

 Description:
   This program reads I2C data from digital thermometer and display it on 7-Segment

 Change History:
   03 February 2008, Gravitech - Created

****************************************************************************/

#include <Wire.h> 
 
#define BAUD (9600)    /* Serial baud define */
#define _7SEG (0x38)   /* I2C address for 7-Segment */
#define THERM (0x49)   /* I2C address for digital thermometer */
#define EEP (0x50)     /* I2C address for EEPROM */
#define RED (3)        /* Red color pin of RGB LED */
#define GREEN (5)      /* Green color pin of RGB LED */
#define BLUE (6)       /* Blue color pin of RGB LED */

#define COLD (23)      /* Cold temperature, drive blue LED (23c) */
#define HOT (26)       /* Hot temperature, drive red LED (27c) */
#define trigPin 8
#define echoPin 2

const byte NumberLookup[16] =   {0x3F,0x06,0x5B,0x4F,0x66,
                                 0x6D,0x7D,0x07,0x7F,0x6F, 
                                 0x77,0x7C,0x39,0x5E,0x79,0x71};

/* Function prototypes */
void Cal_temp (int&, byte&, byte&, bool&);
void Dis_7SEG (int, byte, byte, bool);
void Send7SEG (byte,   byte);
void SerialMonitorPrint (byte, int, bool);
void UpdateRGB (byte);

/***************************************************************************
 Function Name: setup

 Purpose: 
   Initialize hardwares.
****************************************************************************/

const int inputPin = 2;

void setup() 
{ 
  Serial.begin(BAUD);
  Wire.begin();        /* Join I2C bus */
  pinMode(RED, OUTPUT);    
  pinMode(GREEN, OUTPUT);  
  pinMode(BLUE, OUTPUT);
  pinMode(inputPin, INPUT);   
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);  
  delay(500);          /* Allow system to stabilize */
} 

/***************************************************************************
 Function Name: loop

 Purpose: 
   Run-time forever loop.
****************************************************************************/
 
void loop() 
{ 
  int Decimal;
  byte Temperature_H, Temperature_L, counter, counter2;
  bool IsPositive;
  /* Configure 7-Segment to 12mA segment output current, Dynamic mode, 
     and Digits 1, 2, 3 AND 4 are NOT blanked */
     
  Wire.beginTransmission(_7SEG);   
  byte val = 0; 
  Wire.write(val);
  val = B01000111;
  Wire.write(val);
  Wire.endTransmission();
  
  /* Setup configuration register 12-bit */
     
  Wire.beginTransmission(THERM);  
  val = 1;  
  Wire.write(val);
  val = B01100000;
  Wire.write(val);
  Wire.endTransmission();
  
  /* Setup Digital THERMometer pointer register to 0 */
     
  Wire.beginTransmission(THERM); 
  val = 0;  
  Wire.write(val);
  Wire.endTransmission();
  
  /* Test 7-Segment */
  for (counter=0; counter<8; counter++)
  {
    Wire.beginTransmission(_7SEG);
    Wire.write(1);
    for (counter2=0; counter2<4; counter2++)
    {
      Wire.write(1<<counter);
    }
    Wire.endTransmission();
    delay (250);
  }

  int fahrenheitFlag = 0;                          /* The sign for display in Fahrenheit degree.*/
  int acceptStream = 0;                            /* The sign used for accepting data stream from server */
  double outdoor = 0;                              /* Outdoor temperature received from server */
  double decimal_Digit = 0;
  int outdoorDisplay = 0;                          /* To decide whether displaying the outdoor temperature or not */  
  int motionValue = 0;                             /* The value for motion sensor, 0 represents no motion, 1 represents motion detected */
  
  while (1)
  {

    motionValue = digitalRead(inputPin);      /* read from motion sensor */
    
    // <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
    int duration, distance;
  
    /* Sending ultra sonic */
    digitalWrite(trigPin, HIGH);
    //delayMicroseconds(1000);
    digitalWrite(trigPin, LOW);
  
    /*  Recv the sonic & calculate the length */
    distance = pulseIn(echoPin, HIGH) / 2;
    distance = distance / 29.1;
//    Serial.print(distance);
//    Serial.println(" cm distance");
  
    if(distance < 50){ // Something is close
      motionValue = 1;
    }
    else{              // Nothing is close
      motionValue = 0;
    }
    //>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>

    Wire.requestFrom(THERM, 2);
    Temperature_H = Wire.read();
    Temperature_L = Wire.read();
    /* Calculate temperature */
    Cal_temp (Decimal, Temperature_H, Temperature_L, IsPositive);

    int bytesIn = Serial.read();
    
    if( bytesIn == 35 ){     /*   # means the outdoor data stream is transferring */
      acceptStream = 1;
      outdoor = 0;
      decimal_Digit = 10;
      continue;
    }
    if( bytesIn == 42 ){    /*    * means the outdoor data stream transferring ends */
      acceptStream = 0;
      decimal_Digit = 0;
      continue;
    } 
      if ( acceptStream != 1 ) {   /* the status not receiving outdoor temperature data */
        if ( bytesIn == 48 ) {                 //  when incoming integer is 0, the bytesIn is 48, which has been converted from byte to integer.
          fahrenheitFlag = 0;
          //Serial.println("Now displaying the temperature in Celsius.\n");
        } else if ( bytesIn == 49 ){
          fahrenheitFlag = 1;
          //Serial.println("Now displaying the temperature in Fahrenheit.\n");
        } else if ( bytesIn == 50 ) {           // when incoming integer is 2, the Arduino gets into stand-by mode.
           //Serial.println( "Now Stand-By Mode.\n" );
           fahrenheitFlag = 2;
          // UpdateRGB ((byte));
           continue;
        } else if ( bytesIn == 51 ) {           // when incoming integer is 3, the Arduino displays the outdoor temperature
          outdoorDisplay = 1 - outdoorDisplay;  // To ensure the success of displaying, the incoming total message should be the sign "3" after the temperature stream, like #23.19*3
        }
      }
      else {
        if ( bytesIn == 46 ) continue;
        outdoor += double( ( bytesIn - 48) * decimal_Digit );
        decimal_Digit = decimal_Digit / 10;
        continue;
      }
      
    //}

    /* Update RGB LED.*/
    if ( fahrenheitFlag != 2)   UpdateRGB (Temperature_H);

    /* send motionValue to server */
    Serial.print(motionValue);
    Serial.print(" ");
    
    /* Display temperature on the serial monitor. 
       Comment out this line if you don't use serial monitor.*/
    SerialMonitorPrint (Temperature_H, Decimal, IsPositive, fahrenheitFlag);

    if ( fahrenheitFlag ) {
       toFahrenheit(Decimal, Temperature_H);         /* change the temperature data into Fahrenheit */
    }
    
    if ( fahrenheitFlag == 2) {
        standByMode();
        delay (1000); 
        continue;
    }

    if ( outdoorDisplay == 0 ) {
      /* Display temperature on the 7-Segment */
      Dis_7SEG (Decimal, Temperature_H, Temperature_L, IsPositive, fahrenheitFlag);
    } else {
      byte outdoorTemperH = (byte)((int)outdoor);
      int outdoorDec = ( outdoor - (double)((int)outdoor) ) * 10000;
      if (fahrenheitFlag) {
          toFahrenheit(outdoorDec, outdoorTemperH);
      }
      
      Dis_7SEG (outdoorDec, outdoorTemperH, Temperature_L, IsPositive, fahrenheitFlag);  // display outdoor temperature on Arduino 
    }
    delay (1000);        /* Take temperature read every 1 second */
  }
} 

/***************************************************************************
 Function Name: Cal_temp

 Purpose: 
   Calculate temperature from raw data.
****************************************************************************/
void Cal_temp (int& Decimal, byte& High, byte& Low, bool& sign)
{
  if ((High&B10000000)==0x80)    /* Check for negative temperature. */
    sign = 0;
  else
    sign = 1;
    
  High = High & B01111111;      /* Remove sign bit */
  Low = Low & B11110000;        /* Remove last 4 bits */
  Low = Low >> 4; 
  Decimal = Low;
  Decimal = Decimal * 625;      /* Each bit = 0.0625 degree C */
  
  if (sign == 0)                /* if temperature is negative */
  {
    High = High ^ B01111111;    /* Complement all of the bits, except the MSB */
    Decimal = Decimal ^ 0xFF;   /* Complement all of the bits */
  }  
}

/***************************************************************************
 Function Name: Dis_7SEG

 Purpose: 
   Display number on the 7-segment display.
****************************************************************************/
void Dis_7SEG (int Decimal, byte High, byte Low, bool sign, int fahrenheitFlag)
{
  byte Digit = 4;                 /* Number of 7-Segment digit */
  byte Number;                    /* Temporary variable hold the number to display */
  
  if (sign == 0)                  /* When the temperature is negative */
  {
    Send7SEG(Digit,0x40);         /* Display "-" sign */
    Digit--;                      /* Decrement number of digit */
  }
  
  if (High > 99)                  /* When the temperature is three digits long */
  {
    Number = High / 100;          /* Get the hundredth digit */
    Send7SEG (Digit,NumberLookup[Number]);     /* Display on the 7-Segment */
    High = High % 100;            /* Remove the hundredth digit from the TempHi */
    Digit--;                      /* Subtract 1 digit */    
  }
  
  if (High > 9)
  {
    Number = High / 10;           /* Get the tenth digit */
    Send7SEG (Digit,NumberLookup[Number]);     /* Display on the 7-Segment */
    High = High % 10;            /* Remove the tenth digit from the TempHi */
    Digit--;                      /* Subtract 1 digit */
  }
  
  Number = High;                  /* Display the last digit */
  Number = NumberLookup [Number]; 
  if (Digit > 1)                  /* Display "." if it is not the last digit on 7-SEG */
  {
    Number = Number | B10000000;
  }
  Send7SEG (Digit,Number);  
  Digit--;                        /* Subtract 1 digit */
  
  if (Digit > 0)                  /* Display decimal point if there is more space on 7-SEG */
  {
    Number = Decimal / 1000;
    Send7SEG (Digit,NumberLookup[Number]);
    Digit--;
  }

  if (Digit > 0)                 /* Display "c" if there is more space on 7-SEG */
  {
    if( fahrenheitFlag ){
      Send7SEG (Digit,0x71);     /* Display "F" if required to display Fahrenheit degree */
      Digit--;
    } else {
      Send7SEG (Digit,0x58);
      Digit--;
    }
  }
  
  if (Digit > 0)                 /* Clear the rest of the digit */
  {
    Send7SEG (Digit,0x00);    
  }  
}

/***************************************************************************
 Function Name: Send7SEG

 Purpose: 
   Send I2C commands to drive 7-segment display.
****************************************************************************/

void Send7SEG (byte Digit, byte Number)
{
  Wire.beginTransmission(_7SEG);
  Wire.write(Digit);
  Wire.write(Number);
  Wire.endTransmission();
}

/***************************************************************************
 Function Name: UpdateRGB

 Purpose: 
   Update RGB LED according to define HOT and COLD temperature. 
****************************************************************************/

void UpdateRGB (byte Temperature_H)
{
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);        /* Turn off all LEDs. */
  
  if (Temperature_H <= COLD)
  {
    digitalWrite(BLUE, HIGH);
  }
  else if (Temperature_H >= HOT)
  {
    digitalWrite(RED, HIGH);
  }
  else 
  {
    digitalWrite(GREEN, HIGH);
  }
}

/***************************************************************************
 Function Name: SerialMonitorPrint

 Purpose: 
   Print current read temperature to the serial monitor.
****************************************************************************/
void SerialMonitorPrint (byte Temperature_H, int Decimal, bool IsPositive, int fahrenheitFlag )
{
    Serial.print("The temperature is ");
    if (!IsPositive)
    {
      Serial.print("-");
    }
    Serial.print(Temperature_H, DEC);
    Serial.print(".");
    Serial.print(Decimal, DEC);
    Serial.print(" degree C");
    
    Serial.print("\n\n");
}


void toFahrenheit(int& Decimal, byte& Temperature_H){
    double temperature;
    temperature = (int)Temperature_H + (double) Decimal / 10000;
    temperature = temperature * 1.8 + 32;
    Decimal = ( temperature - (int)temperature) * 10000;
    int tem = (int)temperature;
    Temperature_H = (byte)tem;
}

void standByMode(){
  Send7SEG( 4, 0x40 );
  Send7SEG( 3, 0x40 );
  Send7SEG( 2, 0x40 );
  Send7SEG( 1, 0x40 );
  digitalWrite(RED, LOW);
  digitalWrite(GREEN, LOW);
  digitalWrite(BLUE, LOW);
}
