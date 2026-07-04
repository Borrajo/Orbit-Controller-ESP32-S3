# Hackman3D Orbit Controller - Tuning Guide

This guide explains how to adjust the **dead zones**, **sensitivity**, and **reactivity** of the Hackman3D Orbit Controller if your controller feels too sensitive, too slow, unstable, or if it produces unwanted movements.

All settings are located near the top of the firmware file:

```cpp
// SETTINGS / PARAMÈTRES
const int DEADZONE_INPUT  = 40;
const int DEADZONE_OUTPUT = 45;
const int CENTER_SAMPLES  = 100;
const int SMOOTH_DIVISOR  = 5;

// SENSITIVITY / SENSIBILITÉ
const float GAIN_TX = 1.3;
const float GAIN_TY = 1.3;
const float GAIN_TZ = 2.3;

const float GAIN_RX = 1.8;
const float GAIN_RY = 1.8;
const float GAIN_RZ = 2.0;

const float ROTATION_PRIORITY = 0.65;
const bool ENABLE_DOMINANT_AXIS_FILTER = false;
```

After changing a value, upload the firmware again with Arduino IDE and test the controller in your 3D/CAD software.

---

## 1. Important startup calibration note

The controller calibrates itself every time it starts.

When plugging in the Orbit Controller:

1. Place it on a stable surface.
2. Do not touch the knob.
3. Wait about one second before using it.

If the knob is touched during startup, the center position may be wrong and the controller may drift or move by itself.

If this happens, unplug the USB cable and plug it back in without touching the knob.

---

## 2. Input dead zone

```cpp
const int DEADZONE_INPUT = 40;
```

The input dead zone removes small electrical noise directly after reading the joysticks.

### Increase this value if:

- the controller moves by itself;
- small vibrations create movement;
- an axis reacts even when you are not touching the knob;
- rotation sometimes creates unwanted translation.

### Decrease this value if:

- the controller needs too much force before reacting;
- small movements are ignored;
- the controller feels less precise around the center.

### Recommended range

```cpp
30 to 70
```

### Example

More stable, less sensitive around center:

```cpp
const int DEADZONE_INPUT = 55;
```

More sensitive around center:

```cpp
const int DEADZONE_INPUT = 30;
```

---

## 3. Output dead zone

```cpp
const int DEADZONE_OUTPUT = 45;
```

The output dead zone removes very small final movement values before sending them to the computer.

This is useful to stop tiny unwanted movements after smoothing.

### Increase this value if:

- there is still movement after releasing the knob;
- the camera slowly drifts;
- the movement does not fully stop;
- the controller feels unstable at rest.

### Decrease this value if:

- very small movements are difficult to control;
- the controller feels like it has a delay before starting;
- fine navigation is not precise enough.

### Recommended range

```cpp
25 to 80
```

### Example

More stable stop:

```cpp
const int DEADZONE_OUTPUT = 60;
```

More precise small movements:

```cpp
const int DEADZONE_OUTPUT = 30;
```

---

## 4. Smoothing / reactivity

```cpp
const int SMOOTH_DIVISOR = 5;
```

This setting controls how smooth or reactive the controller feels.

A higher value makes movement smoother, but slower.
A lower value makes movement faster, but less filtered.

### Increase this value if:

- movement feels too nervous;
- the camera jumps too quickly;
- movements are not smooth enough.

### Decrease this value if:

- the controller feels too slow;
- there is too much delay;
- the movement feels soft or delayed.

### Recommended range

```cpp
3 to 8
```

### Examples

More reactive:

```cpp
const int SMOOTH_DIVISOR = 3;
```

Balanced:

```cpp
const int SMOOTH_DIVISOR = 5;
```

Smoother but slower:

```cpp
const int SMOOTH_DIVISOR = 7;
```

---

## 5. Translation sensitivity

```cpp
const float GAIN_TX = 1.3;
const float GAIN_TY = 1.3;
const float GAIN_TZ = 2.3;
```

These values control translation sensitivity:

- `GAIN_TX` = left / right movement
- `GAIN_TY` = forward / backward movement
- `GAIN_TZ` = up / down movement

### Increase a value if:

- that axis feels too slow;
- you need to push too far to get movement;
- movement is not strong enough.

### Decrease a value if:

- that axis is too fast;
- it is difficult to make small movements;
- it triggers too easily.

### Recommended range

```cpp
1.0 to 3.0
```

### Example

If vertical movement is too strong:

```cpp
const float GAIN_TZ = 1.8;
```

If left/right movement is too slow:

```cpp
const float GAIN_TX = 1.6;
```

---

## 6. Rotation sensitivity

```cpp
const float GAIN_RX = 1.8;
const float GAIN_RY = 1.8;
const float GAIN_RZ = 2.0;
```

These values control rotation sensitivity:

- `GAIN_RX` = rotate around X
- `GAIN_RY` = rotate around Y
- `GAIN_RZ` = twist around Z

### Increase a value if:

- rotation feels too slow;
- you need too much movement to rotate;
- the axis lacks response.

### Decrease a value if:

- rotation is too fast;
- rotation is difficult to control;
- rotation triggers unwanted movement.

### Recommended range

```cpp
1.2 to 3.0
```

### Example

If Z rotation is too fast:

```cpp
const float GAIN_RZ = 1.6;
```

If rotation feels too slow:

```cpp
const float GAIN_RX = 2.1;
const float GAIN_RY = 2.1;
```

---

## 7. Rotation priority

```cpp
const float ROTATION_PRIORITY = 0.65;
```

This setting helps prevent a rotation from being interpreted as a translation.

The lower the value, the more priority rotation gets over translation.

### Lower this value if:

- rotating sometimes creates unwanted left/right movement;
- rotating sometimes creates unwanted forward/backward movement;
- rotation and translation feel mixed together.

### Increase this value if:

- translation is cancelled too easily;
- it is difficult to make translation movements;
- rotation takes priority too often.

### Recommended range

```cpp
0.45 to 1.00
```

### Examples

More rotation priority:

```cpp
const float ROTATION_PRIORITY = 0.50;
```

Less rotation priority:

```cpp
const float ROTATION_PRIORITY = 0.85;
```

---

## 8. Dominant axis filter

```cpp
const bool ENABLE_DOMINANT_AXIS_FILTER = false;
```

This setting controls whether the firmware keeps only the strongest movement axis.

When this value is `false`, several axes can be sent at the same time.
This allows combined movements such as moving and rotating together.

When this value is `true`, only the strongest axis is kept and the others are cancelled.
This can make the controller easier to control, but it also prevents natural multi-axis movement.

### Set this value to false if:

- you want smooth multi-axis navigation;
- diagonal or combined movements feel blocked;
- moving and rotating at the same time feels difficult.

### Set this value to true if:

- unwanted diagonal movements are too frequent;
- the controller feels difficult to control;
- you prefer one movement direction at a time.

### Example

Allow several axes at the same time:

```cpp
const bool ENABLE_DOMINANT_AXIS_FILTER = false;
```

Keep only the strongest axis:

```cpp
const bool ENABLE_DOMINANT_AXIS_FILTER = true;
```

---

## 9. Center calibration samples

```cpp
const int CENTER_SAMPLES = 100;
```

This value controls how many readings are used during startup calibration.

A higher value may give a more stable center but increases startup time.

### Increase this value if:

- the controller sometimes starts with a bad center;
- the neutral position is not consistent;
- your joystick readings are noisy at startup.

### Decrease this value if:

- startup feels too long.

### Recommended range

```cpp
50 to 200
```

### Example

More stable startup calibration:

```cpp
const int CENTER_SAMPLES = 150;
```

---

## 10. Common problems and suggested fixes

### The controller moves by itself

Try:

```cpp
const int DEADZONE_INPUT  = 50;
const int DEADZONE_OUTPUT = 60;
```

Also make sure you do not touch the knob during USB connection.

---

### The controller is too slow

Try:

```cpp
const int SMOOTH_DIVISOR = 3;
```

You can also slightly increase the gain values.

---

### The controller is too nervous or too sensitive

Try:

```cpp
const int SMOOTH_DIVISOR = 6;
const int DEADZONE_INPUT = 50;
```

You can also lower the gain of the axis that feels too strong.

---

### Rotation creates unwanted translation

Try:

```cpp
const float ROTATION_PRIORITY = 0.50;
const int DEADZONE_INPUT = 50;
```

If the issue is still present, reduce translation gains slightly:

```cpp
const float GAIN_TX = 1.1;
const float GAIN_TY = 1.1;
```

---

### Multi-axis movement feels blocked

Try:

```cpp
const bool ENABLE_DOMINANT_AXIS_FILTER = false;
```

This allows several axes to be sent at the same time.

---

### Too many diagonal movements

Try:

```cpp
const bool ENABLE_DOMINANT_AXIS_FILTER = true;
```

This keeps only the strongest movement axis and cancels the others.

---

### Small movements are not detected

Try:

```cpp
const int DEADZONE_INPUT  = 30;
const int DEADZONE_OUTPUT = 30;
```

Be careful: values that are too low may cause drift.

---

### Vertical movement is too strong

Try lowering `GAIN_TZ`:

```cpp
const float GAIN_TZ = 1.8;
```

---

### Z rotation is too strong

Try lowering `GAIN_RZ`:

```cpp
const float GAIN_RZ = 1.6;
```

---

## 11. Recommended tuning method

Change only one setting at a time.

Recommended order:

1. Start with `DEADZONE_INPUT`.
2. Adjust `DEADZONE_OUTPUT`.
3. Adjust `SMOOTH_DIVISOR`.
4. Adjust translation and rotation gains.
5. Adjust `ENABLE_DOMINANT_AXIS_FILTER` if multi-axis movement feels blocked or too loose.
6. Adjust `ROTATION_PRIORITY` only if rotation and translation are mixed.

After each change:

1. Upload the firmware again.
2. Unplug and reconnect the controller.
3. Do not touch the knob during startup.
4. Test in your 3D/CAD software.
5. Repeat if needed.

---

## 12. Safe default values

If tuning goes wrong, you can return to these default values:

```cpp
const int DEADZONE_INPUT  = 40;
const int DEADZONE_OUTPUT = 45;
const int CENTER_SAMPLES  = 100;
const int SMOOTH_DIVISOR  = 5;

const float GAIN_TX = 1.3;
const float GAIN_TY = 1.3;
const float GAIN_TZ = 2.3;

const float GAIN_RX = 1.8;
const float GAIN_RY = 1.8;
const float GAIN_RZ = 2.0;

const float ROTATION_PRIORITY = 0.65;
const bool ENABLE_DOMINANT_AXIS_FILTER = false;
```

---

## 13. Final note

Small hardware differences, joystick tolerances, soldering, wire routing, and printed part tolerances can affect the final feel of the controller.

There is no single perfect setting for everyone. The values above are safe starting points, and each user can fine-tune the controller to match their own build and preferred navigation style.
