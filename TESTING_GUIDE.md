# Stepper Motor Hardware Testing Guide

This guide will help you connect and test the stepper motor for the Profiling Float
project. It is written for beginners. Every step is explained clearly. Follow the
steps in order. Do not skip any step.

---

## Table of Contents

- [What Is This Project?](#what-is-this-project)
- [Part 1: Shopping List](#part-1-shopping-list)
- [Part 2: Know Your Parts](#part-2-know-your-parts)
- [Part 3: Wiring Step by Step](#part-3-wiring-step-by-step)
- [Part 4: Software Setup](#part-4-software-setup)
- [Part 5: Testing Step by Step](#part-5-testing-step-by-step)
- [Part 6: Troubleshooting](#part-6-troubleshooting)
- [Part 7: Safety Rules](#part-7-safety-rules)

---

## What Is This Project?

The Profiling Float is a small underwater robot. It sinks down in the water, measures
the water pressure and temperature, then comes back up to the surface.

A **stepper motor** is attached to a syringe. When the motor turns one way, it pushes
the syringe plunger in. This lets water into the float, making it heavier, so it
sinks. When the motor turns the other way, it pulls the plunger out, pushing the
water out, so the float becomes lighter and rises back up.

This guide helps you test that the stepper motor is working correctly before you put
everything inside the float.

---

## Part 1: Shopping List

You need all of these parts before you start. Make sure you have everything first.

| # | Part | What It Looks Like | What It Does |
|---|------|--------------------|--------------|
| 1 | **ESP32 DevKit board** | A small blue or black board with a USB plug on one end | The brain. It tells the motor what to do. |
| 2 | **A4988 stepper driver board** | A tiny board (about 2cm x 2cm) with a small metal chip and a tiny screw on top | Translates the ESP32's tiny signals into big power signals the motor can use. |
| 3 | **Bipolar stepper motor** | A square metal motor with 4 wires coming out of it | The motor that will push and pull the syringe. |
| 4 | **12V DC power supply** | A black box with a plug, like a laptop charger, that outputs 12 volts | Gives power to the motor. The ESP32 alone is too weak to power the motor. |
| 5 | **Breadboard** | A white or cream plastic board with lots of tiny holes in rows | Lets you connect wires together without soldering. |
| 6 | **Jumper wires (male-to-male)** | Colourful thin wires with small metal pins on both ends | Used to connect parts together on the breadboard. |
| 7 | **100 uF electrolytic capacitor** | A small cylinder (looks like a tiny battery) with two wire legs, one leg longer than the other | Protects the A4988 from electrical spikes. Very important. |
| 8 | **2x limit switches** | Small switches with a thin metal arm that clicks when pressed | Tell the ESP32 when the syringe has reached the top or bottom of its travel. |
| 9 | **USB cable (micro-USB or USB-C)** | A phone-charging cable that fits your ESP32 | Connects the ESP32 to your computer for programming and power. |

---

## Part 2: Know Your Parts

### The ESP32 Board

The ESP32 board has two rows of pins along the left and right edges. Each pin has a
name printed on the board next to it. We only use 6 of these pins:

```
         ┌───────────────────┐
         │      [USB]        │
         │                   │
         │  ESP32 DevKit     │
         │                   │
   3V3 ──┤                   ├── GND
         │                   │
         ├──              ───┤
         │                   │
   D14 ──┤                   ├── D13
         │                   │
         ├──              ───┤
         │                   │
         ├──              ───┤
         │                   │
         ├──              ───┤
         │                   │
    D5 ──┤                   ├── D4
         │                   │
         ├──              ───┤
         │                   │
         ├──              ───┤
         │                   │
   VIN ──┤                   ├── GND
         │                   │
         └───────────────────┘

  Pins we use:
    D4  = STEP signal (tells the motor to take one step)
    D5  = DIR signal  (tells the motor which direction to turn)
    D13 = Bottom limit switch
    D14 = Top limit switch
    3V3 = 3.3 volt power output (for the A4988 logic)
    GND = Ground (the "minus" side of the circuit)
    VIN = 5 volt input (not used for motor power)
```

> **Note:** The exact position of each pin depends on your specific ESP32 board.
> Look at the tiny white text printed next to each pin on YOUR board to find them.


### The A4988 Driver Board

The A4988 is a small board with pins on both sides. One side handles the signal from
the ESP32. The other side handles the motor power.

```
     Signal Side              Motor Side
    ┌───────────┐           ┌───────────┐
    │           │           │           │
    │  ENABLE ──┤           ├── VMOT    │  ← 12V power goes here
    │           │           │           │
    │    MS1  ──┤           ├── GND     │  ← 12V ground goes here
    │           │           │           │
    │    MS2  ──┤           ├── 2B      │  ← motor wire
    │           │           │           │
    │    MS3  ──┤    A4988  ├── 2A      │  ← motor wire
    │           │           │           │
    │  RESET ──┤   [screw] ├── 1A      │  ← motor wire
    │           │           │           │
    │  SLEEP ──┤           ├── 1B      │  ← motor wire
    │           │           │           │
    │   STEP ──┤           ├── VDD     │  ← 3.3V logic power
    │           │           │           │
    │    DIR ──┤           ├── GND     │  ← logic ground
    │           │           │           │
    └───────────┘           └───────────┘

  What each pin does:
    VMOT   = Motor power input. Connect your 12V power supply here.
    GND    = Ground. There are two GND pins — one on each side.
    VDD    = Logic power. Connect 3.3V from the ESP32 here.
    STEP   = Every time this pin gets a pulse, the motor moves one step.
    DIR    = HIGH = one direction. LOW = the other direction.
    SLEEP  = Must be HIGH for the driver to work (not sleeping).
    RESET  = Must be HIGH for the driver to work (not reset).
    ENABLE = Must be LOW for the driver to work (enabled).
    1A, 1B = Connect to one coil (pair of wires) of the motor.
    2A, 2B = Connect to the other coil (pair of wires) of the motor.
    MS1/2/3= We leave these unconnected (full-step mode).

  The tiny screw on top sets the maximum current for the motor.
  We will adjust this later.
```


### The Capacitor

The capacitor has **two legs**. One leg is **longer** than the other. This matters!

```
         (+)               (-)
    Longer leg         Shorter leg
         │                 │
         │    ┌───────┐    │
         │    │ 100uF │    │
         │    └───────┘    │
         │                 │

  The longer leg is POSITIVE (+). It goes to 12V (VMOT).
  The shorter leg is NEGATIVE (-). It goes to GND.

  WARNING: If you put the capacitor in backwards, it can pop and
           release smoke. Always double-check which leg is longer.
```


### The Limit Switches

A limit switch has 3 metal tabs at the bottom. They are usually labelled:

```
    ┌─────────────────┐
    │   Limit Switch  │
    │                 │
    │    [click arm]  │
    │                 │
    └──┬─────┬─────┬──┘
       │     │     │
      NC    COM    NO

  COM = Common (the middle one). Connect this to the ESP32 GPIO pin.
  NC  = Normally Closed. Connect this to GND.
  NO  = Normally Open. We do NOT use this one.

  "Normally Closed" means COM and NC are connected together when
  the switch arm is NOT pressed. When the arm IS pressed, they
  disconnect.

  This is exactly what we want:
    - Arm not pressed → GPIO reads LOW (connected to GND through NC-COM)
    - Arm pressed     → GPIO reads HIGH (disconnected, pulled up internally)
```

---

## Part 3: Wiring Step by Step

Read ALL of this section before you start connecting wires. Then go back and do it
one step at a time.

> **IMPORTANT:** Do all wiring with the power OFF. Do not plug in the USB cable or
> the 12V power supply until this section tells you to.


### Phase 1: Place the Parts on the Breadboard

1. Place the **ESP32 board** on the left side of the breadboard, straddling the
   center groove. The USB plug should face outward (away from the breadboard).

2. Place the **A4988 driver** on the right side of the breadboard. Make sure the
   tiny screw faces up (you can see it).

3. Leave space between the ESP32 and A4988 so the wires are not too crowded.


### Phase 2: A4988 Power Wiring (12V Side)

These wires give the motor its power. The motor needs 12 volts. The ESP32 cannot
provide this, so we use a separate power supply.

4. **Capacitor:** Push the capacitor's **longer leg (+)** into the same breadboard
   row as the A4988's **VMOT** pin. Push the **shorter leg (-)** into the same row
   as the A4988's **GND** pin (motor side). The capacitor should be right next to
   these two pins.

5. **12V positive wire:** Connect the **positive (+)** output of your 12V power
   supply to the same row as **VMOT** (where the capacitor's long leg is).

6. **12V ground wire:** Connect the **negative (-)** output of your 12V power
   supply to the same row as the A4988's **GND** (motor side).


### Phase 3: A4988 Logic Wiring (3.3V Side)

These wires give the A4988 chip its "thinking" power. This is separate from the
motor power.

7. **Logic power:** Connect the ESP32's **3V3** pin to the A4988's **VDD** pin.
   Use a red jumper wire.

8. **Logic ground:** Connect the ESP32's **GND** pin to the A4988's **GND** pin
   (signal side). Use a black jumper wire.

9. **IMPORTANT — Connect the grounds together:** The 12V power supply ground
   (step 6) and the ESP32 ground (step 8) **MUST** be connected together.
   Use a black jumper wire to connect the A4988's motor-side GND to the A4988's
   signal-side GND. (They may already be connected through the breadboard if they
   are in the same row. If not, add a wire.)

10. **SLEEP to RESET:** Connect the A4988's **SLEEP** pin to the A4988's **RESET**
    pin. Use a short jumper wire. Then connect the **RESET** pin to the A4988's
    **VDD** pin (which already has 3.3V from step 7). This keeps the driver awake.

11. **ENABLE to GND:** Connect the A4988's **ENABLE** pin to the A4988's **GND**
    (signal side). This turns the driver on. (LOW = on, HIGH = off.)


### Phase 4: ESP32 to A4988 Signal Wires

These two wires carry the step and direction signals from the ESP32 to the A4988.

12. **STEP wire:** Connect ESP32 pin **D4** (GPIO4) to the A4988's **STEP** pin.
    Use a green jumper wire.

13. **DIR wire:** Connect ESP32 pin **D5** (GPIO5) to the A4988's **DIR** pin.
    Use a blue jumper wire.


### Phase 5: A4988 to Motor Wires

Your stepper motor has **4 wires**. These 4 wires form **2 pairs** (called coils).
You need to find which wires belong to the same pair.

**How to find the coil pairs:**

14. Set your multimeter to **resistance mode** (the ohm symbol: Ω).

15. Touch the multimeter probes to two of the motor wires. If the multimeter shows
    a **low number** (between 1 and 10 ohms), those two wires are **one coil pair**.
    If it shows nothing (OL or infinity), try a different combination.

16. Once you know the two pairs, connect them:
    - **Pair 1:** Connect one wire to A4988 **1A** and the other to A4988 **1B**.
    - **Pair 2:** Connect one wire to A4988 **2A** and the other to A4988 **2B**.

> **Tip:** If the motor later turns the wrong direction, just swap the two wires
> of ONE pair (swap 1A and 1B, or swap 2A and 2B). This reverses the direction.


### Phase 6: Limit Switches

17. **Bottom limit switch:** Connect the switch's **COM** tab to ESP32 pin **D13**
    (GPIO13). Connect the switch's **NC** tab to **GND** on the ESP32.

18. **Top limit switch:** Connect the switch's **COM** tab to ESP32 pin **D14**
    (GPIO14). Connect the switch's **NC** tab to **GND** on the ESP32.


### Final Wiring Diagram

```
                          12V Power Supply
                          (+)          (-)
                           |            |
                           |  ┌──100uF──┘  (capacitor: + leg to VMOT,
                           |  |              - leg to GND)
                           |  |
                    ┌──────┴──┴──────────────────────┐
                    │  VMOT         GND (motor side)  │
                    │                                 │
                    │            A4988                 │
                    │                                 │
           ┌───────┤  STEP          1A ├──── Motor wire 1 ─┐
           │  ┌────┤  DIR           1B ├──── Motor wire 2 ─┤ Coil A
           │  │    │                                 │     │
           │  │    │  SLEEP──RESET──VDD          2A ├──── Motor wire 3 ─┐
           │  │    │                             2B ├──── Motor wire 4 ─┤ Coil B
           │  │    │  ENABLE──GND (signal side)      │
           │  │    │            │                     │
           │  │    │  VDD       GND (signal side)     │
           │  │    └──┬─────────┬────────────────────┘
           │  │       │         │
           │  │       │         │
    ┌──────┴──┴───────┴─────────┴────────────────────┐
    │  D4  D5       3V3        GND                   │
    │                                                │
    │                   ESP32                         │
    │                                                │
    │  D13                               D14         │
    └──┬─────────────────────────────────┬───────────┘
       │                                 │
  ┌────┴────┐                       ┌────┴────┐
  │ COM  NC │                       │ COM  NC │
  │ Bottom  │                       │  Top    │
  │ Switch  │                       │ Switch  │
  └─────┬───┘                       └─────┬───┘
        │                                 │
       GND                               GND
```


### Checklist Before Powering On

Go through this list. Check every single item. Do not power on until all are checked.

- [ ] Capacitor is next to VMOT and GND, with the **longer leg on VMOT**
- [ ] 12V (+) is connected to VMOT
- [ ] 12V (-) is connected to GND (motor side)
- [ ] ESP32 3V3 is connected to A4988 VDD
- [ ] ESP32 GND is connected to A4988 GND (signal side)
- [ ] Motor-side GND and signal-side GND are connected together
- [ ] SLEEP is connected to RESET, and RESET is connected to VDD (3.3V)
- [ ] ENABLE is connected to GND
- [ ] ESP32 D4 (GPIO4) is connected to A4988 STEP
- [ ] ESP32 D5 (GPIO5) is connected to A4988 DIR
- [ ] Motor wires: Pair 1 to 1A + 1B, Pair 2 to 2A + 2B
- [ ] Bottom limit switch: COM to D13, NC to GND
- [ ] Top limit switch: COM to D14, NC to GND
- [ ] No loose wires touching each other
- [ ] No bare wire ends touching the metal case of the motor

---

## Part 4: Software Setup

### Step 1: Install PlatformIO

If you haven't already, install PlatformIO. The easiest way:

1. Install **Visual Studio Code** (VS Code) from https://code.visualstudio.com
2. Open VS Code.
3. Click the **Extensions** icon on the left sidebar (it looks like four squares).
4. Search for **"PlatformIO IDE"**.
5. Click **Install**. Wait for it to finish.

### Step 2: Open the Project

1. In VS Code, click **File** > **Open Folder**.
2. Navigate to the `profiling-float-new` folder and click **Open**.

### Step 3: Build the Test Firmware

Open a **Terminal** in VS Code (click **Terminal** > **New Terminal** at the top menu).

Type this command and press Enter:

```bash
~/.platformio/penv/bin/pio run -e wokwi_stepper_auto
```

Wait until you see **SUCCESS** in green text. If you see errors, check that you
opened the correct folder.

### Step 4: Set the A4988 Current Limit

Before turning the motor on for the first time, you must set the current limit on
the A4988. This protects your motor from getting too hot.

1. Plug the **USB cable** into the ESP32 and your computer. (Do NOT turn on 12V yet.)
2. Plug in the **12V power supply**.
3. Find the **tiny screw** on top of the A4988 board.
4. Using a small flat-head screwdriver, slowly turn the screw **counterclockwise**
   (to the left). This lowers the current. Start low to be safe.
5. If you know your motor's rated current, you can measure the voltage on the screw
   pad with a multimeter and use this formula:

```
Current limit = VREF voltage / (8 x 0.068)
```

For example, if your motor is rated at 1.0A, set VREF to about 0.54V.

> **If you don't know your motor's current rating:** Start with the screw turned
> almost fully counterclockwise (lowest setting). You can slowly increase it later
> if the motor is too weak to turn.

### Step 5: Flash the Firmware to the ESP32

1. Find out which USB port the ESP32 is using:

```bash
ls /dev/cu.usb*
```

You should see something like `/dev/cu.usbserial-0001`. If you see nothing, try a
different USB cable (some cables are charge-only and do not carry data).

2. Upload the firmware:

```bash
~/.platformio/penv/bin/pio run -e wokwi_stepper_auto -t upload
```

Wait until you see **SUCCESS**.

### Step 6: Open the Serial Monitor

```bash
~/.platformio/penv/bin/pio device monitor -b 115200
```

You should see text appearing on the screen. This is the ESP32 talking to you.

To exit the serial monitor later, press **Ctrl + ]** (Control key and the right
square bracket key at the same time).

---

## Part 5: Testing Step by Step

Work through these tests in order. Each test builds on the previous one.


### Test 1: LED Blink Test (Check GPIO Output)

**What this tests:** Does the ESP32 actually send signals out of its pins?

**What you need:** Two LEDs and two 220-ohm resistors (or the already-flashed
auto-test firmware).

**How to do it:**

1. The auto-test firmware (`wokwi_stepper_auto`) directly drives GPIO4 and GPIO5.
2. If you have the motor and A4988 connected, the motor should already be moving
   after flashing. If not, temporarily connect an LED + 220 ohm resistor between
   GPIO4 and GND. The LED should blink.

**What success looks like:**
- The serial monitor shows messages like `[CW] DIR=LOW, pulsing STEP 10x...`
  followed by `pulse 1`, `pulse 2`, etc.
- If you have an LED on GPIO4, it blinks on and off.
- If you have an LED on GPIO5, it switches between on and off every few seconds.

**What failure looks like:**
- The serial monitor shows nothing. → Check the USB cable and serial monitor
  baud rate (must be 115200).
- The LED does not light up at all. → Check the LED is the right way around
  (longer leg to GPIO pin, shorter leg to GND through the resistor).


### Test 2: Motor Auto-Test (Motor Spins By Itself)

**What this tests:** Does the full chain work — ESP32 → A4988 → Motor?

**What you need:** Everything wired up from Part 3. The `wokwi_stepper_auto`
firmware should already be flashed from Step 5.

**How to do it:**

1. Make sure the USB cable is plugged in (powers the ESP32).
2. Turn on the 12V power supply.
3. Open the serial monitor if it is not already open:
   ```bash
   ~/.platformio/penv/bin/pio device monitor -b 115200
   ```
4. Watch the motor and the serial monitor.

**What success looks like:**
- The serial monitor shows pulse counts.
- The motor shaft **visibly turns** — 10 steps one way, pause, 10 steps the
  other way, pause, and repeats.
- You can feel the motor vibrating or see the shaft rotating.

**What failure looks like:**

| What you see | What to try |
|---|---|
| Serial shows pulses, but motor does not move at all | Check the 12V power supply is on. Check VMOT and GND wires. Check the capacitor is not backwards. |
| Motor vibrates but does not spin | The motor coil wires are in the wrong order. Swap the two wires on 1A and 1B. |
| Motor only turns one direction | The DIR wire (D5 to DIR) may be loose. Check the connection. |
| Motor is very hot | The A4988 current limit is set too high. Turn the tiny screw counterclockwise to lower it. |
| Nothing happens at all | Check that SLEEP is connected to RESET and to VDD. Check that ENABLE is connected to GND. |


### Test 3: Interactive Debug Test (Serial Commands)

**What this tests:** Can you control the motor step-by-step and read limit switches?

**How to do it:**

1. Flash the interactive debug firmware:
   ```bash
   ~/.platformio/penv/bin/pio run -e wokwi_stepper_debug -t upload
   ```

2. Open the serial monitor:
   ```bash
   ~/.platformio/penv/bin/pio device monitor -b 115200
   ```

3. You should see a welcome message and a `>` prompt.

4. Try these commands (type the command and press Enter):

| Command | What it does | What you should see |
|---------|-------------|-------------------|
| `s` | Show status | Prints step count, speed, and limit switch states |
| `f100` | Step 100 pulses forward (clockwise) | Motor turns, step count increases by 100 |
| `b100` | Step 100 pulses backward (counter-clockwise) | Motor turns the other way, step count decreases by 100 |
| `f1000` | Step 1000 pulses forward | Motor turns longer |
| `v2000` | Set speed to 2000 microseconds (slower) | Next steps will be slower |
| `v600` | Set speed to 600 microseconds (default) | Next steps will be normal speed |
| `z` | Reset step counter to 0 | Step count becomes 0 |

**What success looks like:**
- Typing `f100` makes the motor turn one way. Typing `b100` makes it turn the other way.
- The step count goes up and down correctly.
- Changing speed with `v` makes the motor visibly faster or slower.


### Test 4: Limit Switch Test

**What this tests:** Do the limit switches correctly tell the ESP32 when the motor
has reached the end of its travel?

**How to do it:**

1. With the interactive debug firmware running (Test 3), type `s` and press Enter.

2. Look at the limit switch readings:
   ```
   Bottom lim : clear
   Top limit  : clear
   ```

3. **Press the bottom limit switch** with your finger. Look at the serial monitor.
   You should see:
   ```
   [LIMIT] Bottom: TRIGGERED
   ```

4. **Release** the bottom limit switch. You should see:
   ```
   [LIMIT] Bottom: released
   ```

5. Do the same with the **top limit switch**.

**What success looks like:**
- Pressing a switch shows "TRIGGERED".
- Releasing it shows "released".
- Type `s` to confirm both show correctly.

**What failure looks like:**

| What you see | What to try |
|---|---|
| Switch always shows "AT LIMIT" even when not pressed | The wires are connected to the wrong tabs on the switch. Use COM and NC, not COM and NO. |
| Switch always shows "clear" even when pressed | Check the wires are firmly in the breadboard. Try wiggling them. |
| Pressing bottom shows "Top" or vice versa | The GPIO13 and GPIO14 wires are swapped. Swap them. |


### Test 5: Run-to-Limit Test

**What this tests:** Can the motor run until a limit switch stops it?

**How to do it:**

1. With the interactive debug firmware running, type `s` to check both limit
   switches show "clear" (neither is pressed).

2. Type `r` and press Enter. The motor will start running clockwise (descend
   direction).

3. While the motor is running, **press the bottom limit switch**.

4. The motor should **stop immediately**. The serial monitor should show:
   ```
   [DONE] Bottom reached. count=XXXX
   ```

5. Now type `h` and press Enter. The motor will start running counter-clockwise
   (ascend / home direction).

6. While the motor is running, **press the top limit switch**.

7. The motor should stop. The step count should reset to 0.

**What success looks like:**
- Motor runs continuously until you press the switch.
- Motor stops as soon as the switch is pressed.
- After homing (`h`), the step count is 0.


### Test 6: Full System Test

Once all the above tests pass, you can flash the full firmware:

```bash
~/.platformio/penv/bin/pio run -e esp32 -t upload
```

> **Note:** The full firmware also needs the MS5837 pressure sensor connected to
> GPIO21 (SDA) and GPIO22 (SCL). Without the sensor, the ESP32 will print
> "ERROR: MS5837 sensor not detected. Halting." and stop. This is expected if you
> only have the motor connected.

---

## Part 6: Troubleshooting

### The Serial Monitor Shows Nothing

1. Check you are using the right baud rate: **115200**.
2. Try pressing the **EN** (reset) button on the ESP32 board.
3. Try a different USB cable. Some cables only charge and do not carry data.
4. Make sure the correct serial port is selected. Run `ls /dev/cu.usb*` to find it.

### The Motor Does Not Move At All

1. Is the 12V power supply turned on?
2. Is the capacitor installed? Is it the right way around (long leg to VMOT)?
3. Is SLEEP connected to RESET, and RESET connected to 3.3V (VDD)?
4. Is ENABLE connected to GND?
5. Are the STEP (D4) and DIR (D5) wires firmly connected?
6. Is the A4988 current limit set too low? Slowly turn the screw clockwise to
   increase it a little, then try again.

### The Motor Vibrates But Does Not Turn

This almost always means the motor coil wires are mixed up.

1. Disconnect the motor from the A4988.
2. Use a multimeter (resistance mode) to find the two coil pairs.
3. Connect pair 1 to 1A and 1B.
4. Connect pair 2 to 2A and 2B.
5. If it still vibrates, swap the two wires of just ONE pair (e.g., swap 1A and 1B).

### The Motor Turns the Wrong Direction

Swap the two wires of ONE coil pair. For example, swap the wires on 1A and 1B.
The motor will now turn the other way.

### The Motor Gets Very Hot

1. Turn the tiny screw on the A4988 **counterclockwise** (to the left) to lower
   the current limit.
2. If the motor is still hot, it may need better ventilation or a heat sink on
   the A4988.

### The A4988 Gets Very Hot

1. Check the capacitor is installed correctly on VMOT.
2. Lower the current limit (tiny screw, counterclockwise).
3. Make sure VMOT is not accidentally connected to more than 12V.

### The Limit Switches Are Not Detected

1. Check you are using the **COM** and **NC** tabs (not NO).
2. Make sure the wires go to the correct ESP32 pins: D13 for bottom, D14 for top.
3. Test the switch with a multimeter in continuity mode. Touch the probes to COM
   and NC. It should beep when the arm is NOT pressed, and stop beeping when it IS
   pressed.

### The ESP32 Keeps Restarting

1. This can happen if the motor draws too much current and causes a voltage drop.
   Make sure the 12V supply and the ESP32 USB power are separate.
2. Check for short circuits on the breadboard (bare wires touching each other).

---

## Part 7: Safety Rules

Read these rules before you start. Follow them every time you work on the circuit.

1. **Always wire with power OFF.** Unplug the USB cable and turn off the 12V supply
   before changing any wires.

2. **The capacitor is mandatory.** Never run the A4988 without the 100uF capacitor
   on VMOT. The A4988 can be destroyed instantly without it.

3. **Check the capacitor direction.** Long leg (+) goes to VMOT. Short leg (-) goes
   to GND. A backwards capacitor can pop.

4. **Set the current limit before running the motor.** Start with the screw turned
   counterclockwise (low current). Increase slowly if needed.

5. **Power-on order:**
   - **First:** Plug in the USB cable (ESP32 turns on).
   - **Second:** Turn on the 12V power supply.

6. **Power-off order:**
   - **First:** Turn off the 12V power supply.
   - **Second:** Unplug the USB cable.

7. **Do not touch motor wires while 12V is on.** The A4988 output can produce high
   voltage spikes when switching the motor coils.

8. **Do not disconnect the motor while the driver is running.** This can damage the
   A4988. Always stop the motor (remove power or stop the firmware) before unplugging
   motor wires.

9. **Keep the workspace dry.** Water and electronics do not mix (except when the
   float is fully sealed and waterproofed, which is a separate step).

10. **If something smells like burning, immediately unplug everything.** Check for
    shorts, backwards components, or wrong voltage connections before trying again.

---

*This guide is for the Profiling Float project. For the full project documentation,
see the [README](README.md).*
