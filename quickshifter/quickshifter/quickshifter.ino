/*
1. sense hold and release? i.e. is the rider holding the shift lever down? only act on a rising edge.
2. low-pass filter the incoming signal - 'debounce' it.
3. how to set the shift time? - app? bluetooth? button(s)?
4. different shift time for each gear? again, how to program?
5. don't shift below certain RPM. how to set it? diff for each gear?
6. how long is the execution loop if we have displays and stuff? need a max of 1ms/1000us. Is this achievable?
7. update gear position indicator at 10hz? use timer to update it. kick the timer on every shift
8. also update on every shift
9. neutral light? need to display N as well. reliably. only go there after a second (half a second?) so we don't go N between shifts.
10. use a timer to stop the shift cut?
11. RC filter on the sensor voltage, then analogue interrupt on arduino? versus sampling and filtering in software
*/

#include <EEPROM.h>

// general constants
const unsigned long MAX_LONG = (2^32) - 1;

// project-specific constants
const short MAX_GEARS = 8;
const int ANALOGUE_PIN = 3;  // the voltage from the divider connected to the shift-rod
const int QS_PIN = 13;  // the io pin to the transistors that cut the ignition signal
const int TIMING_PIN = 12;  // strobe a pin to measure response time with an oscilloscope
const int EE_ADDRESS_GEAR_TABLE = 0;

// mutable globals
unsigned long trigger_stop_time = -1;  // when must we stop?
int qs_time = 1000;  // how long should we cut ignition? in ms.
int current_rpm = -1;
float current_speed = -1.0;
float gear_ratios[MAX_GEARS]; // = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
bool timing_pin = 0;

int timer1_counter;

void setup()
{
  Serial.begin(115200);
  pinMode(QS_PIN, OUTPUT);
  gear_load_ratios();

  // timers for things?
  OCR0A = 0xAF;
  TIMSK0 |= _BV(OCIE0A);

  /*
   * http://www.hobbytronics.co.uk/arduino-timer-interrupts
  */
  // initialize timer1 
  noInterrupts();           // disable all interrupts
  TCCR1A = 0;
  TCCR1B = 0;

  // Set timer1_counter to the correct value for our interrupt interval
  //timer1_counter = 64911;   // preload timer 65536-16MHz/256/100Hz
  //timer1_counter = 64286;   // preload timer 65536-16MHz/256/50Hz
  timer1_counter = 34286;   // preload timer 65536-16MHz/256/2Hz
  
  TCNT1 = timer1_counter;   // preload timer
  TCCR1B |= (1 << CS12);    // 256 prescaler 
  TIMSK1 |= (1 << TOIE1);   // enable timer overflow interrupt
  interrupts();             // enable all interrupts
}

void loop()
{
//  val = analogRead(ANALOGUE_PIN);
//  Serial.println(val);
  int millis_now = millis();

  if (trigger_stop_time == -1)  // we're not triggered at the moment
  {
    if (trigger())
    {
      unsigned long time_to_max = MAX_LONG - millis_now;
      trigger_stop_time = time_to_max < qs_time ? qs_time - time_to_max : millis_now + qs_time;
      digitalWrite(QS_PIN, HIGH);
    }
  }
  else  // we're already triggered - should we stop?
  {
    if (millis_now > trigger_stop_time)
    {
      digitalWrite(QS_PIN, LOW);
      trigger_stop_time = -1;
    }
  }

  // toggle the timing pin - check this with a scope to see if our main loop is fast enough
  digitalWrite(TIMING_PIN, timing_pin);
  timing_pin = !timing_pin;
}

void filter_signal()
{
  
}

bool trigger()
{
  // can only trigger above a certain RPM
  return false;
}

/*
 * Update the RPM reading
*/
int update_rpm()
{
  
}

/*
 * Load the gear ratios from EEPROM
*/
void gear_load_ratios()
{
  int ctr = 0;
  float ratio = 0.0;
  int ee_address = EE_ADDRESS_GEAR_TABLE;
  for (ctr = 0; ctr < 8; ctr++)
  {
    EEPROM.get(ee_address, ratio);
    gear_ratios[ctr] = ratio;
    ee_address += sizeof(float);
  }
}

/*
 * Write the gear ratios to EEPROM
*/
void gear_write_ratios()
{
  int ctr = 0;
  int ee_address = EE_ADDRESS_GEAR_TABLE;
  for (ctr = 0; ctr < 8; ctr++)
  {
    EEPROM.put(ee_address, gear_ratios[ctr]);
    ee_address += sizeof(float);
  }
}

/*
 * What gear are we in? Dividing speed by rpm gives a ratio that will be constant for each gear.
 * Can learn them over time. Higher ratio, higher gear. Keep updating the table.
*/
short update_gear()
{
  float road_speed = speed_read();
  int rpm = current_rpm;
  float match_ratio = rpm / road_speed;
  int ctr = 0;
  int ratio = 0;
  bool matched = 0;
  for (ctr = 0; ctr < MAX_GEARS; ctr++)
  {
    // gear_ratios[ctr] higher or lower than or equal to match_ratio?
  }
}

float speed_read()
{
  // is this just an ADC reading? how is the speed indicated to the speedo?
  return 0.0;
}

SIGNAL(TIMER1_COMPA_vect) 
{
  // this would be for doing a comparison to a different voltage
  // could do it with a PWM output into an analogue LPF for the comparison voltage?
  // but that's not filtering the input. Another analogue LPF?
  // or all in software?
}

ISR(TIMER1_OVF_vect)        // interrupt service routine 
{
  TCNT1 = timer1_counter;   // preload timer
  // digitalWrite(ledPin, digitalRead(ledPin) ^ 1);
}

// end

