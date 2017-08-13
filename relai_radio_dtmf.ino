/*
 * Author: Jérôme LOYET
 * Date: July 2017
 * Subject: Controlling a led strip from a HAM radio using DTMF and signal detection
 * 
 * = HOW IT WORKS =
 * - Use the DTMF lib to detect DTMF codes received from a HAM radio (baofeng UV-B2 or any appliance that can output signal with DTMF through a jack)
 * - The DTMF lib has been modified to return wheter a signal without DTMF has been detected (source https://forum.arduino.cc/index.php?topic=121540.0)
 * - if signal is detected then UP() the relay
 * - if DTMF code is '0', down() (and sleep for 2 seconds to avoid to up the relay imediately because of signal presence)
 * - if DTMF code is '*', lower brightness
 * - if DTMF code is '#', higher brightness
 * - for any other code, just up() the relay
 * - Each time the relay is up(), a 5 minutes timer is reset
 * - After 5 minutes being UP the relay is shutdown
 * 
 * = SCHEMATICS =
 * == SPEAKER SCHEMATICS ==
 * Ref: https://i.stack.imgur.com/XZddX.jpg
 * ┌=sleeve=|=ring=|=tip=
 * │
 * ├[SPRK-]-|-ring-|-[SPRK+]-
 * |
 * 
 * 
 * 
 * [VCC=5V+]------┐
 *                │
 *              [R1=1k]
 *                │
 * SPK+ ----------┼----[ARDUINO A0]
 *                │
                [R2=1k]
 *                │
 * SPK- ----------┤
 *                │
 * [GND]----------┘
 *              
 *              
 *              
 * == RELAY SCHEMATICS ==
 * 
 * [VBAT+]----------┐
 *                  │+
 *                [LED]
 *                  │-
 * [IRF540: DRAIN]--┘
 *                   -[IFR540: GATE]┬-[R=1k]-----[ARDUINO D11]
 * [IRF540: SOURCE]-┐              │
 *                  │             [R=10k]
 * [VBAT-]----------┤              │
 * [GND]------------┴--------------┘
 * 
 * 
 * == ARDUINO SCHEMATICS ==
 * 
 * [ARDUINO GND]---------[VBAT-]
 * [ARDUINO VIN]---------[VBAT+]
 * [ARDUINO A0]----------[SPK+]
 * [ARDUINO D11]---------[R=1k]--[IFR540: GATE]
 * 
 */

/*
 * HEADER
 */
#define SERIAL_ENABLE 1
#define SERIAL_SPEED 115200 // The speed of the Serial Line

#define SIGNAL_THRESHOLD 10000 // The sum of N sample centered by 512 (funtion dtmf.detect())
#define LED_DOWN_DELAY 2000 // 2 second delay after a manual shutdown if used with signal detection, 0 otherwise
#define SHUTDOWN_TIMEOUT 300000 // 5 minutes en ms pour déterminer quand arrêter le relai
#define BRIGHTNESS_ENABLE 1
#define BRIGHTNESS_STEP 16 // step to navigate between 0 and 255
#define BRIGHTNESS_INIT 128 // brightness at startup (max 255)

#define PIN_RADIO A0
#define PIN_RELAY 11
#define PIN_LED LED_BUILTIN
/*
 * DTMF
 */
#include <DTMF.h>
#define DTMF_N 128.0
#define DTMF_SAMPLING_RATE 8900.0 // avec l'arduino nano mesuré à 4.45 kHz sur D4 (donc x2 = 8900.0)
#define DTMF_SAMPLING_MIDDLE 512 // valeur sans signal au millieu de pont de diode, avec 5V et un pont de diode symétrique on a 512


/*
 * Global Variables
 */
char active = LOW; // to track the relay state
float d_mags[8];   // dtmf tracking struct
#if BRIGHTNESS_ENABLE
int brightness = BRIGHTNESS_INIT; // track brightness
#endif
unsigned long last_up = 0; // last time the relay has been activated
DTMF dtmf = DTMF(DTMF_N,DTMF_SAMPLING_RATE);; // declare DTMF instance

/*
 * SETUP
 */
void setup(){

  /* set pins mode */
  pinMode(PIN_RELAY, OUTPUT);
  pinMode(PIN_LED, OUTPUT);

  /* initialize PINS to LOW */
  digitalWrite(PIN_RELAY, LOW);
  digitalWrite(PIN_LED, LOW);

  /* start serial */
#if SERIAL_ENABLE
  Serial.begin(SERIAL_SPEED);
  delay(500); // wait 500ms for initializing
#endif
  if (Serial) {
    Serial.print("radio pin initial value =");
    Serial.print(analogRead(PIN_RADIO));
    Serial.println("");
    Serial.println("*** ~~~ RUNNING ~~~ ***");
  }
}



/*
 * Main Loop
 */
void loop()
{
  /* Get DTMF_N samples from analog pin */
  dtmf.sample(PIN_RADIO);
  /* Detect DTMF frequencies and return sum of the N last samples centered by DTMF_SAMPLING_MIDDLE */
  long sig = abs(dtmf.detect(d_mags,DTMF_SAMPLING_MIDDLE)); // abs() as we are searching for deviation from 0
  sig = sig > SIGNAL_THRESHOLD ? 1 : 0;

  /* Read DTMF char */
  char read_char = dtmf.button(d_mags,1800.);
  if (Serial) {
    if (sig && 0) {
      Serial.print("Signal=");Serial.print(sig);
    }
    if (read_char) {
      Serial.print(" Char=");Serial.println(read_char);
    }
  }

  /* get current time */
  unsigned long now = millis();

  /* action depending on received DTMF code */
  switch(read_char) {

    /* switch UP */
    case '1': 
    case '2': 
    case '3': 
    case '4': 
    case '5': 
    case '6': 
    case '7': 
    case '8': 
    case '9': 
    case 'A': 
    case 'B': 
    case 'C': 
    case 'D': 
      up(now);
      break;

#if BRIGHTNESS_ENABLE
    /* brightness DOWN */
    case '*': 
      inc_brightness(-BRIGHTNESS_STEP);
      break;

    /* brightness UP */
    case '#': 
      inc_brightness(+BRIGHTNESS_STEP);
      break;
#else 
    case '*': 
    case '#': 
#endif
    /* switch DOWN */
    case '0': 
      down(LED_DOWN_DELAY); //down with a small delay
      break;
    /* normally do nothing */
    default:
      if (read_char > 0) up(now);
      else if (sig) up(now);
      break;
  }



  /* handle expiration */
  if (now > last_up + SHUTDOWN_TIMEOUT) { // timeout has expires
    down(0); // shutdown immediately
    if (Serial) {
      Serial.println("TIMEOUT expired");
    }
  }
}

/*
 * inc_brightness
 */
#if BRIGHTNESS_ENABLE
void inc_brightness(int inc) {
  if (brightness == 255 && inc < 0) brightness = 256;
  brightness += inc;
  if (brightness < abs(inc)) brightness = abs(inc); // min to BRIGHTNESS_STEP
  if (brightness > 255) brightness = 255; // max to 255
  if (active) {
    analogWrite(PIN_RELAY, brightness);
  }
  if (Serial) {
    Serial.print("Set brightness to ");
    Serial.println(brightness);
  }
}
#endif

/*
 * UP
 */
void up(unsigned long now) {
  /* if not active */
  if (!active) {
    /* light up PIN_LED */
    digitalWrite(PIN_LED, HIGH);
    /* active PIN_RELAY with right brightness */
#if BRIGHTNESS_ENABLE
    analogWrite(PIN_RELAY, brightness);
#else
    digitalWrite(PIN_RELAY, HIGH);
#endif
    /* remember state */
    active = HIGH;
  }
  /* update last time relay has been activated */
  last_up = now;
  if (Serial) {
    Serial.println("RELAY UP !!!");
  }
}

/*
 * DOWN
 */
void down(unsigned int _delay) {
  if (active) {
    /* turn off PIN_LED */
    digitalWrite(PIN_LED, LOW);
    /* turn off PIN_RELAY */
    digitalWrite(PIN_RELAY, LOW);
    /* remember state */
    active = LOW;
    /* reset relay timer */
    last_up = 0;
  }
  if (Serial) {
    Serial.print("RELAY down !!!");
  }

  /* if a delay has been passed */
  if (_delay) {
    if (Serial) {
      Serial.print("with delay of ");
      Serial.print(_delay);
      Serial.print(" ms");
    }
    /* delay for _delay milliseconds */
    delay(_delay);
  }
  if (Serial) {
    Serial.println("");
  }
}

