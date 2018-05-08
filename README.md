# Reactduino

![C++](https://img.shields.io/badge/langauge-C++-blue.svg)
[![license: MIT](https://img.shields.io/badge/license-MIT-brightgreen.svg)](https://opensource.org/licenses/MIT)

By [AndrewCarterUK ![(Twitter)](http://i.imgur.com/wWzX9uB.png)](https://twitter.com/AndrewCarterUK)

An asynchronous programming library for the Arduino platform.

## Blink

If you have worked with Arduino before, it is likely that you will have come across the [blink sketch](https://www.arduino.cc/en/tutorial/blink). This is a simple program that flashes an LED every second, and it looks something like this:

```cpp
#include <Arduino.h>

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
}

void loop() {
  digitalWrite(LED_BUILTIN, HIGH);
  delay(1000);
  digitalWrite(LED_BUILTIN, LOW);
  delay(1000);
}
```

Using Reactduino, the sketch can be rewritten to the following:

```cpp
#include <Reactduino.h>

Reactduino app([] () {
  pinMode(LED_BUILTIN, OUTPUT);

  app.repeat(1000, [] () {
      static bool state = false;
      digitalWrite(LED_BUILTIN, state = !state);
  });
});
```

With Reactduino, the developer creates an `app` object and passes to the constructor a start up function. In the example above, a [lambda function](http://en.cppreference.com/w/cpp/language/lambda) is used.

There is no `setup()` or `loop()`, Reactduino will define these for you. All you need to do is tell Reactduino which events you need to watch, and it will dispatch your handlers/callbacks when they occur.

## Why Bother?

Charlie wants to make a simple program which echoes data on the `Serial` port. Their Arduino sketch will looks like this:

```cpp
#include <Arduino.h>

void setup()
{
    Serial.begin(9600);
}

void loop()
{
    if (Serial.available() > 0) {
        Serial.write(Serial.read());
    }

    yield();
}
```

This works, but Charlie decides that they would like to blink the built-in LED every time it processes data. Now, their sketch looks like this:

```cpp
#include <Arduino.h>

void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    if (Serial.available() > 0) {
        Serial.write(Serial.read());

        digitalWrite(LED_BUILTIN, HIGH);
        delay(20);
        digitalWrite(LED_BUILTIN, LOW);
    }

    yield();
}
```

The problem with this sketch is that whilst the LED is blinking, Charlie's program is not relaying data from the Serial port. The longer Charlie blinks the LED for, the slower the rate of transfer.

To solve this problem, Charlie refactors their code to look something like this:

```cpp
#include <Arduino.h>

uint32_t start;
bool blink = false;

void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
}

void loop()
{
    if (Serial.available() > 0) {
        Serial.write(Serial.read());

        blink = true;
        start = millis();
        digitalWrite(LED_BUILTIN, HIGH);
    }

    if (blink && millis() - start > 1000) {
        blink = false;
        digitalWrite(LED_BUILTIN, LOW);
    }

    yield();
}
```

This solves Charlie's problem, but it's quite verbose. Using Reactduino, Charlie can write the same script like this:

```c++
#include <Reactduino.h>

Reactduino app([] () {
  Serial.begin(9600);
  pinMode(LED_BUILTIN, OUTPUT);

  app.onAvailable(&Serial, [] () {
    static reaction led_off = INVALID_REACTION;

    Serial.write(Serial.read());
    digitalWrite(LED_BUILTIN, HIGH);

    app.free(led_off); // Cancel previous timer (if set)

    led_off = app.delay(1000, [] () { digitalWrite(LED_BUILTIN, LOW); });
  });
});
```

## API

### Event Registration Functions

All of the registration functions return a `reaction`. This is either a number which can be used to enable, disable or free the event, or `INVALID_REACTION`.

```cpp
reaction app.delay(uint32_t t, react_callback cb);
```

Delay the executation of a callback by `t` milliseconds.

```cpp
reaction app.repeat(uint32_t t, react_callback cb);
```

Repeatedly execute a callback every `t` milliseconds.

```cpp
reaction app.onAvailable(Stream *stream, react_callback cb);
```

Execute a callback when there is data available to read on an input stream (such as `&Serial`).

```cpp
reaction app.onInterrupt(uint8_t number, react_callback cb, int mode);
```

Execute a callback when an interrupt number fires. This uses the same API as the `attachInterrupt()` Arduino function.

```cpp
reaction app.onPinRising(uint8_t pin, react_callback cb);
```

Execute a callback when a rising voltage is detected on a pin. Make sure the pin provided is an interrupt pin.

```cpp
reaction app.onPinFalling(uint8_t pin, react_callback cb);
```

Execute a callback when a falling voltage is detected on a pin. Make sure the pin provided is an interrupt pin.

```cpp
reaction app.onPinChange(uint8_t pin, react_callback cb);
```

Execute a callback when a changing voltage is detected on a pin. Make sure the pin provided is an interrupt pin.

```cpp
reaction app.onPinRisingNoInt(uint8_t pin, react_callback cb);
```

Same as `onPinRising`, but without using interrupts. Useful for attaching a change callback to a pin 
not triggering interrupts.

```cpp
reaction app.onPinFallingNoInt(uint8_t pin, react_callback cb);
```

Same as `onPinFalling`, but without using interrupts. Useful for attaching a change callback to a pin 
not triggering interrupts.

```cpp
reaction app.onPinChangeNoInt(uint8_t pin, react_callback cb);
```

Same as `onPinChange`, but without using interrupts. Useful for attaching a change callback to a pin 
not triggering interrupts.
```cpp
reaction app.onTick(react_callback cb);
```

Execute a callback on every tick of the event loop.

### Management functions

These functions are used to enable, disable and free events registered with the functions above.

```cpp
void app.enable(reaction r);
void app.disable(reaction r);
void app.free(reaction r);
```

### Examples

- [`Complex`](examples/complex.cpp): Demonstrates several different reaction types for controlling a servo, monitoring inputs and blinking an LED.
