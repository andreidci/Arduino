/* Raw IR commander
 test 
 */
 
#include <LiquidCrystal.h>
#include "IR_RemoteCodes.h"


#define IRpin_PIN      PIND
#define IRpin          2
#define LEDpin         4
#define RELAYpin       7
// the maximum pulse we'll listen for - 65 milliseconds is a long time
#define MAXPULSE 65000
#define NUMPULSES 40
// what our timing resolution should be, larger is better
// as its more 'precise' - but too large and you wont get
// accurate timing
#define RESOLUTION 20 
// What percent we will allow in variation to match the same code
#define FUZZINESS 20
// Minimum length of any IR signal to be read
#define MINSIGNALLNG 30
// we will store up to 100 pulse pairs (this is -a lot-)
uint16_t pulses[NUMPULSES][2];  // pair is high and low pulse 
uint8_t currentpulse = 0; // index for pulses we're storing

LiquidCrystal lcd(8, 7, 13, 12, 11, 10);

int Ridx=0;
int lightread[10] = {0,0,0,0,0,0,0,0,0,0};
int medianLight=0;
uint16_t valoareIluminare=0;
uint8_t LightMode=0;
//int SensorStable=0;
int timer =0 ;
int current_millis_value=0;
int previous_millis_value=0;


// FIRST CYCLE
void setup() {
    // Init Sequence
  Serial.begin(9600);
  pinMode(LEDpin,OUTPUT);
  pinMode(RELAYpin,OUTPUT); 
  
  Serial.println("Ready to decode IR!");
  valoareIluminare = analogRead(0);
} 


// LOOP CYCLE
void loop() {
  int numberpulses;
  numberpulses = listenForIR();
  timer=0;
  Serial.print("Heard ");
  Serial.print(numberpulses);
  Serial.println("-pulse long IR signal");
 //valoareIluminare = analogRead(0);
 //Serial.println(valoareIluminare, DEC); 

 // Luminosity Sensor Reading / Averaging
  lightread[Ridx] = analogRead(0);
 
  Ridx++;
  if (Ridx == 10) {
    Ridx=0;
    for(int i=0;i<10;i++) {
      medianLight+=lightread[i];
    }
  valoareIluminare=medianLight/10;
  medianLight=0;
  }  
  Serial.print("Luminozitate: ");
  Serial.println(valoareIluminare, DEC); 

 
 // Remote commands 
  if (IRcompare(numberpulses, RemoteOnOff,sizeof(RemoteOnOff)/4)) {
    Serial.println("ON / OFF button pressed");
    digitalWrite(LEDpin,HIGH);
    Serial.print("LightMode : ");
    Serial.println(LightMode,DEC);
    if (LightMode!=0) 
         LightMode=0;
    else LightMode=1;
    Serial.print("LightMode : ");
    Serial.println(LightMode,DEC);
  }

  if (IRcompare(numberpulses, RemoteA,sizeof(RemoteA)/4)) {
    Serial.println(" A button pressed");
   /* digitalWrite(LEDpin,HIGH);
    delay(200);
    digitalWrite(LEDpin,LOW);
    delay(200);
    digitalWrite(LEDpin,HIGH);
    delay(200); */
    Serial.print("LightMode : ");
    Serial.println(LightMode,DEC);
    if (LightMode==2) 
      LightMode=1;
    else LightMode=2;
    //if (LightMode==1) LightMode=2;
    Serial.print("LightMode : ");
    Serial.println(LightMode,DEC); 
    Serial.print("Valoare iluminare : ");
    Serial.println(valoareIluminare,DEC);
  }
  
  digitalWrite(LEDpin,LOW);
  Serial.println("Waiting new command");
  
    switch (LightMode) {
      // Lights OFF 
      case 0:
        digitalWrite(RELAYpin,LOW);
       // SensorStable=0;
      break;
      // Light ON
      case 1:
        digitalWrite(RELAYpin,HIGH);
       // SensorStable=0;
      break;
    // Automatic Mode
    case 2:
      if (valoareIluminare <= 410 ) { // &&  SensorStable>=) {
       // if (SensorStable <= 20) 
       //   SensorStable++;
       // else 
          digitalWrite(RELAYpin,LOW);
      }
      else {
        digitalWrite(RELAYpin,HIGH);
      //  SensorStable=0;
      }      
      break;
    default: 
      digitalWrite(RELAYpin,LOW);
    //  SensorStable=0;
      // if nothing else matches, do the default
  }

}


//KGO: added size of compare sample. Only compare the minimum of the two
boolean IRcompare(int numpulses, int Signal[], int refsize) {
  if (numpulses<=MINSIGNALLNG)
    return false;
  int count = min(numpulses,refsize);
  Serial.print("count set to: ");
  Serial.println(count);
  for (int i=0; i< count-1; i++) {
    int oncode = pulses[i][1] * RESOLUTION / 10;
    int offcode = pulses[i+1][0] * RESOLUTION / 10;
    
#ifdef DEBUG    
    Serial.print(oncode); // the ON signal we heard
    Serial.print(" - ");
    Serial.print(Signal[i*2 + 0]); // the ON signal we want 
#endif     
    // check to make sure the error is less than FUZZINESS percent
    if ( abs(oncode - Signal[i*2 + 0]) <= (Signal[i*2 + 0] * FUZZINESS / 100)) {
#ifdef DEBUG
      Serial.print(" (ok)");
#endif
    } else {
#ifdef DEBUG
      Serial.print(" (x)");
#endif
      // we didn't match perfectly, return a false match
      return false;
    }
    
    
#ifdef DEBUG
    Serial.print("  \t"); // tab
    Serial.print(offcode); // the OFF signal we heard
    Serial.print(" - ");
    Serial.print(Signal[i*2 + 1]); // the OFF signal we want 
#endif    
    
    if ( abs(offcode - Signal[i*2 + 1]) <= (Signal[i*2 + 1] * FUZZINESS / 100)) {
#ifdef DEBUG
      Serial.print(" (ok)");
#endif
    } else {
#ifdef DEBUG
      Serial.print(" (x)");
#endif
      // we didn't match perfectly, return a false match
      return false;
    }
    
#ifdef DEBUG
    Serial.println();
#endif
  }
  // Everything matched!
  return true;
}


int listenForIR(void) {
  currentpulse = 0;
  timer=0;
  //timer<2000
  while (1) {
    current_millis_value = millis(); 
    timer+=current_millis_value - previous_millis_value;
    previous_millis_value = current_millis_value;
    Serial.print("Timer : ");
    Serial.println(timer,DEC);
    uint16_t highpulse, lowpulse;  // temporary storage timing
    highpulse = lowpulse = 0; // start out with no pulse length
  
//  while (digitalRead(IRpin)) { // this is too slow!
    while (IRpin_PIN & (1 << IRpin)) {
       // pin is still HIGH
       // count off another few microseconds
       highpulse++;
       delayMicroseconds(RESOLUTION);

       // If the pulse is too long, we 'timed out' - either nothing
       // was received or the code is finished, so print what
       // we've grabbed so far, and then reset
       
       // KGO: Added check for end of receive buffer
       if (((highpulse >= MAXPULSE) && (currentpulse != 0))|| currentpulse == NUMPULSES) {
         return currentpulse;
       }
    }
    // we didn't time out so lets stash the reading
    pulses[currentpulse][0] = highpulse;
  
    // same as above
    while (! (IRpin_PIN & _BV(IRpin))) {
       // pin is still LOW
       lowpulse++;
       delayMicroseconds(RESOLUTION);
        // KGO: Added check for end of receive buffer
        if (((lowpulse >= MAXPULSE)  && (currentpulse != 0))|| currentpulse == NUMPULSES) {
         return currentpulse;
       }
    }
    pulses[currentpulse][1] = lowpulse;

    // we read one high-low pulse successfully, continue!
    currentpulse++;
    //timer++;
  }
}
void printpulses(void) {
  Serial.println("\n\r\n\rReceived: \n\rOFF \tON");
  for (uint8_t i = 0; i < currentpulse; i++) {
    Serial.print(pulses[i][0] * RESOLUTION, DEC);
    Serial.print(" usec, ");
    Serial.print(pulses[i][1] * RESOLUTION, DEC);
    Serial.println(" usec");
  }
  
  // print it in a 'array' format
  Serial.println("int IRsignal[] = {");
  Serial.println("// ON, OFF (in 10's of microseconds)");
  for (uint8_t i = 0; i < currentpulse-1; i++) {
    Serial.print("\t"); // tab
    Serial.print(pulses[i][1] * RESOLUTION / 10, DEC);
    Serial.print(", ");
    Serial.print(pulses[i+1][0] * RESOLUTION / 10, DEC);
    Serial.println(",");
  }
  Serial.print("\t"); // tab
  Serial.print(pulses[currentpulse-1][1] * RESOLUTION / 10, DEC);
  Serial.print(", 0};");
}

