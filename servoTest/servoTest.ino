#include <math.h>

const uint8_t SAMPLE_RATE = 100;
uint8_t mode;

// number of points in lookup table
const uint16_t sineNPoints = 500;

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
float endFreq = 8;
float freqStep = .5;
uint8_t settleCycles = 2;
uint8_t sampleCycles = 5;

uint8_t stepNumber;
uint8_t jumpFreq;
//uint8_t jumpChange = round(freqSteps*4*sineNPoints/50);

void setup() {

  // Serial Setup
  Serial.begin(115200);
  
  // Setup timer
  cli();
  // Enable pwm output on OC1A and set bits for FAST PWM ICR1 Top mode
  TCCR1A = (1 << COM1A1) | (0 << COM1A0) | (1 << WGM11) | (0 << WGM10);
  // Other bits for FAST PWM and prescaler to 8, for 2MHz
  TCCR1B = (1 << WGM13) | (1 << WGM12) | (0 << CS12) | (1 << CS11) | (0 << CS10);
  // Set input compare register to define top of counter (where it resets and pin goes high)
  // 2M/50 = 40K
  ICR1 = 2000000/SAMPLE_RATE - 1;
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
  
  // enable interrupts
  sei();

  // wait for servo to move to middle
  delay(1000);

  // reset encoder position
  encoderCount = 0;

  // Start the calibration phase
  uint16_t start_position = 2000;
  uint16_t end_position = 4000;
  uint16_t position_step = 250;

  mode = 1;

  OCR1A = start_position-100;
  delay(500);
  Serial.println("calibration1");
  for (uint16_t position=start_position; position<=end_position; position+=position_step){
    // set position, wait and output position info
    OCR1A = position;
    delay(500);
    sprintf(printbuf,"%d,%d\n",position,encoderCount);
    Serial.print(printbuf);
    
  }

  Serial.println("end");
  Serial.flush();

  OCR1A = end_position+100;
  delay(500);
  Serial.println("calibration2");
  for (uint16_t position=end_position; position>=start_position; position-=position_step){
    // set position, wait and output position info
    OCR1A = position;
    delay(500);
    sprintf(printbuf,"%d,%d\n",position,encoderCount);
    Serial.print(printbuf);
    
  }
  Serial.println("end");
  Serial.flush();

  // Step response test
  mode = 2;
  Serial.println("step");
  for (uint16_t position=start_position+position_step; position<=end_position; position+=position_step){
    // Go to start position
    OCR1A = start_position;
    delay(1000);
    OCR1A = position;
    for (uint16_t i=0; i<SAMPLE_RATE; i++){
      while (!sendFlag){}
      sendFlag = false;
      sprintf(printbuf,"%d,%d\n",position,encoderCount);
      Serial.print(printbuf);
    }
  }
  Serial.println("end");

  
  // Start frequency test
  Serial.println("frequency");
  Serial.flush();
  mode = 3;
  position_step = 200;

  // iterate through amplitudes
  for (uint16_t amp = position_step; amp<=1000; amp+=position_step){
    // recalculate lookup table
    for (uint16_t i = 0; i < sineNPoints; i++) {
      float angleRad = M_PI / 2 * (float) i / sineNPoints;
      sineValues[i] = (uint16_t) round(sin(angleRad) * amp);
    }

    for (float freq=startFreq; freq <= endFreq; freq+=freqStep){
      // send servo to middle and reset states
      OCR1A = 3000;
      jumpFreq = 0;
      sineIndex = 0;
      cycleCount = 0;
      delay(500);

      // set frequency and adjust for phase
      uint8_t jump = round(freq*4*sineNPoints/SAMPLE_RATE);      
      // phase correction so that start of sample cycles is at 0 phase
      sineIndex = 4*settleCycles*sineNPoints % jump;
      
      // start motion
      sendFlag = false;
      jumpFreq = jump;

      // collect data until reached end
      while(true){
        // check if interrupt has occured
        if (sendFlag){
          sendFlag = false;
           // Check if reached settle cycles
          if (cycleCount >= settleCycles){
            // check if in sample cycles
            if (cycleCount < settleCycles + sampleCycles){
              // print out the current state
              sprintf(printbuf,"%d,%d,%d,%d\n",amp,(uint8_t)(freq*10),sineValue,encoderCount);
              Serial.print(printbuf);
            } else {
              // Reached end of samples for this frequency
              // Get last datapoint
              sprintf(printbuf,"%d,%d,%d,%d\n",amp,(uint8_t)(freq*10),sineValue,encoderCount);
              Serial.print(printbuf);
              // stop test at this frequency
              break;
            }
          }
        }   
      }
    }
  }
  // Stop servo
  OCR1A = 3000;
  jumpFreq = 0;
  sineIndex = 0;
  cycleCount = 0;
  Serial.println("end");
}

void loop() {
  delay(1);
}

// Interrupt that occurs when timer reaches TOP
// Triggers at 50Hz
ISR(TIMER1_OVF_vect) {
  sendFlag = true;
  switch(mode){
    case 1:
    break;
    case 2:
    break;
    case 3:    
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
    break;
  }
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
