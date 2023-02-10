#include <math.h>

// number of points in lookup table
const uint16_t sineNPoints = 500;

// Maximum value
float sineMaxValue = 100;

// Array of sinevalues
uint16_t sineValues[sineNPoints];
// Current index
int16_t sineIndex;


// buffer for printing to serial
char printbuf[40];
volatile bool sendFlag;

volatile uint16_t sineValue;
volatile uint8_t quadrant;
volatile uint16_t cycleCount;

// Encoder config
volatile int32_t encoderCount;

// Step sine config, floats are rounded to integer jump steps
float startFreq = .5;
float freqStep = .5;
float freq = startFreq;
uint8_t nSteps = 11;
uint8_t settleCycles = 2;
uint8_t sampleCycles = 5;

uint8_t stepNumber;
uint8_t jumpFreq;
//uint8_t jumpChange = round(freqSteps*4*sineNPoints/50);

void setup() {

  // Serial Setup
  Serial.begin(115200);

  // Generate sine wave lookup table
  for (uint16_t i = 0; i < sineNPoints; i++) {
    float angleRad = M_PI / 2 * (float) i / sineNPoints;
    sineValues[i] = (uint16_t) round(sin(angleRad) * sineMaxValue);
  }
  
  // Setup timer
  cli();
  // Enable pwm output on OC1A and set bits for FAST PWM ICR1 Top mode
  TCCR1A = (1 << COM1A1) | (0 << COM1A0) | (1 << WGM11) | (0 << WGM10);
  // Other bits for FAST PWM and prescaler to 8, for 2MHz
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (0 << CS12) | (1 << CS11) | (0 << CS10);
  // Set input compare register to define top of counter (where it resets and pin goes high)
  // 2M/50 = 40K
  ICR1 = 40000 - 1;
  // Set initial output compare value where pin will be pulled low
  // 1ms pulse = 2000, 2ms pules = 4000;
  OCR1A = 3000;
  // Enable interupts for timer overflow TOV1(when TCNT1 reaches ICR1)
  // Can also use ICR1 interrupt
  TIMSK1 =  (1 << TOIE1) | (0 << ICIE1);
  // Set pin as output
  DDRB |= 1 << PB1;

  // Encoder interrupts, change on either for pin 2 and 3
  EICRA |= (0 << ISC11) | (1 << ISC10) | (0 << ISC01) | (1 << ISC00);
  // Enable pin interrupts
  EIMSK = (1 << INT1) | (1 << INT0);
  // Enable pullup resitor
  PORTD |= (1<<PD2) | (1<<PD3);

  // wait for servo to move to middle
  delay(1000);
  
  // enable interrupts
  sei();

  // Set start frequency
  jumpFreq = round(freq*4*sineNPoints/50);

}

void loop() {
  
  if (sendFlag) {
    // Reset flag
    sendFlag = false;

    // Check if reached settle cycles
    if (cycleCount >= settleCycles){
      // check if in sample cycles
      if (cycleCount < settleCycles + sampleCycles){
        // print out the current state
        sprintf(printbuf,"%d,%d,%d\n",(uint8_t)(freq*10),sineValue,encoderCount);
        Serial.print(printbuf);
      } else {
        // Reached end of samples for this frequency
        // Get last datapoint
        sprintf(printbuf,"%d,%d,%d\n",(uint8_t)(freq*10),sineValue,encoderCount);
        Serial.print(printbuf);
        // send servo to middle and reset states
        OCR1A = 3000;
        jumpFreq = 0;
        sineIndex = 0;
        cycleCount = 0;
        delay(500);

        // go to next frequency
        stepNumber ++;
        // check if test end
        if (stepNumber == nSteps){
          // end of test
          OCR1A = 3000;
          jumpFreq = 0;
          sineIndex = 0;
          cycleCount = 0;
          Serial.println("end");
          Serial.flush();
          while(1){delay(100);}
        }

        // apply new frequency
        freq += freqStep;
       uint8_t jump = round(freq*4*sineNPoints/50);

       // phase correction so that start of sample cycles is at 0 phase
       sineIndex = 4*settleCycles*sineNPoints % jump;

       // start motion
       jumpFreq = jump;
      
      }
    }    
  }
  delay(1);
}

// Interrupt that occurs when timer reaches TOP
// Triggers at 50Hz
ISR(TIMER1_OVF_vect) {
  sendFlag = true;
  if (quadrant == 0) {
    sineIndex += jumpFreq;
    if (sineIndex >= sineNPoints) {
      sineIndex = sineNPoints - (sineIndex - sineNPoints) - 1;
      quadrant = 1;
    }
  } else if (quadrant == 1) {
    sineIndex -= jumpFreq;
    if (sineIndex < 0) {
      sineIndex = abs(sineIndex) - 1;
      quadrant = 2;
    }
  } else if (quadrant == 2) {
    sineIndex += jumpFreq;
    if (sineIndex >= sineNPoints) {
      sineIndex = sineNPoints - (sineIndex - sineNPoints) - 1;
      quadrant = 3;
    }
  } else if (quadrant == 3) {
    sineIndex -= jumpFreq;
    if (sineIndex < 0) {
      sineIndex = abs(sineIndex) - 1;
      quadrant = 0;
      cycleCount ++;
    }
  }

  if (quadrant == 0 || quadrant == 1) {
    sineValue = 3000 + sineValues[sineIndex];
  } else {
    sineValue = 3000 - sineValues[sineIndex];
  }
  OCR1A = sineValue;
}

// Pinchange interrupt for encoder channel A
ISR(INT0_vect) {
  // Read state of pins
  uint8_t portstate = PIND;
  // bitwise xor the pinstates for direction
  bool dir = ((portstate >> PD2) & 1) ^ ((portstate >> PD3) & 1);
  if (dir) {
    encoderCount -= 1;
  } else {
    encoderCount += 1;
  }
}

ISR(INT1_vect) {
  uint8_t portstate = PIND;
  // bitwise xor the pinstates for direction
  bool dir = ((portstate >> PD2) & 1) ^ ((portstate >> PD3) & 1);
  if (dir) {
    encoderCount += 1;
  } else {
    encoderCount -= 1;
  }
}
