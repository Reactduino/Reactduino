# Reactduino

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

  app.delay(1000, [] () {
      digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  });
});
```

With Reactduino, the developer creates an `app` object and passes to the constructor a start up function. In the example above, a [lambda function](http://en.cppreference.com/w/cpp/language/lambda) is used.

There is no `setup()` or `loop()`, Reactduino will define these for you. All you need to do is tell Reactduino which events you need to watch, and it will dispatch your handlers/callbacks when they occur.

## Why Bother?

Charlie wants to make a simple program which echoes data on the `Serial` port. Their Arduino sketch will looks like this:

```cpp
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
        digitlWrite(LED_BUILTIN, HIGH);
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
