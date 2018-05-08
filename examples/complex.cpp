#include <Reactduino.h>
#include <Servo.h>

/*
 * This example demonstates:
 * - several reactions :
 *   + servo motion control
 *   + LED blinking
 *   + input monitoring
 *
 * Attach a hobby R/C analog servo to pin SERVO_PIN and a push button to
 * BUTTON_PIN.
 *
 * The servo will start sweeping when the program starts. Depressing the button
 * will toggle the motion on or off.
 *
 * At the same time, the LED will blink quickly when the servo is moving, and 
 * slowly when it is stopped.
 *
 * In addition to demonstrating several types of reaction used in the same
 * application, this show also how reactions can be removed and added on the
 * fly. This is used to change the blinking period of the LED.
 */

#define SERVO_PIN       11
#define BUTTON_PIN      8

// servo motion increment
#define MOVE_INCREMENT  1           

// LED blink periods (ms)
#define BLINK_DELAY_MOVING          200     // when the servo is moving
#define BLINK_DELAY_STOPPED         500     // when the servo is stopped

/*
 * Servo related data
 */
Servo servo ;               // the servo instance
bool servo_activated;       // the flag telling if it is activated or not

/*
 * LED blink reaction reference
 */
reaction r_blink;


/*
 * LED blink reaction callback
 */
void blink() {
  static bool state = false;
  digitalWrite(LED_BUILTIN, state = !state);
}


/*
 * Servo motion control reaction callback
 */
void move_servo() {
    static uint8_t angle = 45;
    static int8_t increment = MOVE_INCREMENT;

    if (!servo_activated)
        return;

    servo.write(angle);

    angle += increment;
    if (angle > 135) {
        angle = 135;
        increment = -MOVE_INCREMENT;
    } else if (angle < 45) {
        angle = 45;
        increment = MOVE_INCREMENT;
    }
}


/*
 * I/O monitoring callback for the push button
 */
void button_cb() {
    static uint32_t last_time;          // last execution time for debouncing

    uint32_t now = millis();
    if (now - last_time >= 100) {       // ignore calls closer than 100ms (debouncing)
        Serial.println("button pressed");
        last_time = now;

        // toggle the servo state by attachong or detaching it
        if (servo_activated) {
            Serial.println("deactivating servo");
            servo.detach();
        } else {
            Serial.println("activating servo");
            servo.attach(SERVO_PIN);
        }
        servo_activated = !servo_activated;

        /*
         * blink frequency update to reflect the servo current activation state
         */

        // delete the current blink reaction
        app.free(r_blink);
        
        // create a new one with the relevant period and save its reference
        r_blink = app.repeat(
            servo_activated ? BLINK_DELAY_MOVING : BLINK_DELAY_STOPPED,  
            blink
        );
    }
}

/*
 * Application initialization
 */
void app_main() {
    // usual I/O configuration
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);

    Serial.begin(9600);         // for debug trace

    // activate the servo from app start
    servo.attach(SERVO_PIN);
    servo_activated = true;

    /*
     * Reactions registration
     */

    // periodic LED blink (keep a reference for being able to edit it afterwards)
    r_blink = app.repeat(BLINK_DELAY_MOVING, blink);

    // periodic servo motion control
    app.repeat(20, move_servo);
    
    // input monitoring for the push button
    app.onPinFallingNoInt(BUTTON_PIN, button_cb); 
}

// let's start the reactor with our application
Reactduino app(app_main);

