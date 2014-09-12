photons
=======

Arduino library for controlling a Proton Pack or anything else connected to TLC 5940 chips.

So it's come to this, you're building a proton pack. Even worse, you're going the Arduino route and are now scouring the internet for help. Congratulations, you've found some. 

What we have here is a library for controlling your proton pack. The setup it works with is as follows: You have an arduino of some kind, and the lights in the pack are controlled by several [TLC 5940](http://www.ti.com/product/tlc5940) chips. The advantage of using these chips over direct wiring is that you can control literally hundreds of LEDs with one chip. You can also use shift registers, but you'll have to solder a bunch of resistors, which is not fun.

The photons library emits events based on switch throws and button presses. In order to use it, you define subclasses of `PackComponent` and implement the callbacks for the event you want. When a button press (or other pack event) happens, your callback is called, and passed a `pack` data structure. You can then manipulate the internal state of your component and turn lights on and off. 

One of the flaws I saw in current pack libraries was overuse of the `delay` function. There was code like this:
```C++
void loop()                     
{
  digitalWrite(25, HIGH);
  digitalWrite(2, HIGH);
  digitalWrite(5, HIGH);
  digitalWrite(20, HIGH);
  digitalWrite(3, HIGH);
  delay(50);            
  digitalWrite(21, HIGH);
  digitalWrite(6, HIGH);
  digitalWrite(3, LOW);
  delay(50);
  digitalWrite(22, HIGH);
  digitalWrite(7, HIGH);
  digitalWrite(3, HIGH);
  delay(50);
  digitalWrite(23, HIGH);
  digitalWrite(8, HIGH);
  digitalWrite(3, LOW);
  delay(50);
  digitalWrite(24, HIGH)
```

Aside from being tedious and difficult to maintain, those delay(50) calls add up over time and create dependencies among the different parts of your pack. This can be frustrating --especially to non-programmers. Photons uses no delays, and each component can decide how often it is called. 

The core part of the library is a `Pack Component`. A `Pack Component` addressses a single TLC chip, and has an offset. Your first TLC chip in the daisy chain has offset 0, the second has offset 1 and so on.  Let's use Photons to cycle two lights. 

The first step is to define the data structure for our lights. You can include the example directly in your sketch.
Our structure looks like this:

```C++
#include <ProtonPack.h>

class Toggler : public PackComponent {
  public:
    Toggler(int offset);
    void onUpdate(Pack pack);
    void reset(Pack pack);
  
  protected:
    int currentLed;
};
```
The first line `#include <ProtonPack.h>`, includes our library. Then we define our component, called Toggler. We listen to two events, `onUpdate` and `reset`.  `onUpdate` is called whenever the pack is ready to refresh our component. `reset` is called when the pack starts up (and a couple of other times) and should reset your component to its initial state. Now let's build the code that makes the lights turn on and off.

```C++
Toggler::Toggler(int offset) : PackComponent(offset) {
  // this is called when you create a Toggler
}

void Toggler::reset(Pack pack) {
  currentLed = 0;
}

void Toggler::onUpdate(Pack pack) {
  setLed(currentLed, 4000);
  
  if ( currentLed == 0 ){ 
    currentLed = 1;
  } else {
    currentLed = 0;
  }
  callAgainIn(1000);
}
```

That was pretty easy! First, we set the current led to a value of 4000 (the TLC allows brightness values between 0 and 4096), and then we switch the current led for the next iteration. Finally, we tell the library to call us again in 1000 milliseconds, which is one second. This is one of the neat things about photons, you tell it when you want to be notified, and it keeps track of the time for you, calling you after that delay. This will cause each light to light up for a second. 

But there's a problem, do you see it? 

You're right, we never turned off the last LED. Let's fix it. 

```c++
void Toggler::onUpdate(Pack pack) {
  setLed(currentLed, 4000);
  
  if ( currentLed == 0 ){ 
    setLed(1, 0); // turns off LED 1
    currentLed = 1;
  } else {
    setLed(0, 0); // turns off LED 0
    currentLed = 0;
  }
  callAgainIn(1000);
}
```

All better. Each light will stay on for one second and then turn off when the next light turns on. 
Go ahead and play with the `callAgainIn` call, make it 50 and see what happens.

Finally, we need to define the rest of the sketch (the Arduino part). That's extremely easy. All we need to do is create our Toggler and give it an offset. 

```C++

Toggler toggler(0); // make our toggler 
ProtonPack pack(8, 7); // make a proton pack, fire button is input 7, activate button is button 8

void setup() {
  pack.addComponent(toggler); // Add the toggler to our pack
  pack.initialize();
}

void loop() {
  pack.update(); // This is what starts the onUpdate callback. 
}

```

That's it! Your pack will now be a really blinky-blinky wonder.

The complete sketch is here:

```c++
#include <ProtonPack.h>

class Toggler : public PackComponent {
  public:
    Toggler(int offset);
    void onUpdate(Pack pack);
    void reset(Pack pack);
  
  protected:
    int currentLed;
};

Toggler::Toggler(int offset) : PackComponent(offset) {
  // this is called when you create a Toggler
}

void Toggler::reset(Pack pack) {
  currentLed = 0;
}

void Toggler::onUpdate(Pack pack) {
  setLed(currentLed, 4000);
  
  if ( currentLed == 0 ){ 
    setLed(1, 0); // turns off LED 1
    currentLed = 1;
  } else {
    setLed(0, 0); // turns off LED 0
    currentLed = 0;
  }
  callAgainIn(1000);
}

/** 
* The sketch starts here.
*
**/
Toggler toggler(0); // make our toggler 
ProtonPack pack(8, 7); // make a proton pack, fire button is input 7, activate button is button 8

void setup() {
  pack.addComponent(toggler); // Add the toggler to our pack
  pack.initialize();
}

void loop() {
  pack.update(); // This is what starts the onUpdate callback. 
}
```
