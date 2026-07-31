# Safety and responsible use

This repository documents a classroom hardware prototype. It is not a medical device and has not been validated against medical-device, electrical-safety, accessibility, or child-resistance standards.

## Do not rely on the prototype for

- life-critical or time-critical medication;
- confirming that a dose was swallowed or that the correct quantity was taken;
- controlled substances or child-resistant storage;
- unattended use by a person who cannot independently recover from a fault;
- emergency alerts or caregiver notification.

## Safe demonstration checklist

- Test with empty containers or non-medication objects.
- Keep fingers clear of the servo linkage and lid lock.
- Calibrate the servo while its linkage is disconnected.
- Use a regulated supply, a common ground, and insulated connections.
- Confirm the RTC after every power interruption.
- Test each alarm, the lid switch, Reset, and the tamper response before a demonstration.
- Disconnect power if the servo stalls, wiring heats up, or the controller repeatedly resets.

## Engineering work required for real-world use

A practical system would need redundant fault detection, persistent and auditable schedules, battery and clock monitoring, dose-presence sensing, safe manual override, secure access control, enclosure testing, human-factors evaluation, and review by qualified medical and safety professionals.
