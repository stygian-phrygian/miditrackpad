# MIDI TrackPad
Transform your Apple Trackpad into a midi device (only Linux)

## Requirements
* Linux
* Apple TrackPad
* evdev — `sudo apt install libevdev-dev`
* portmidi — `sudo apt install libportmidi-dev`

### Building
```
git clone <this repo>
cd <this repo>
cmake -S . -B build
cmake --build build
./build/apps/miditrackpad # <--- executable here
```

## Usage
1. Run the executable (NB. requires root privileges, eg. sudo)
2. Touch the trackpad (emits MIDI messages)

Currently, the device's axes translate as follows:
* x-axis is pitch bend
* y-axis is cc1 (mod wheel)
* z-axis (pressure) is channel aftertouch

Command line flags permit tweaking:
```
./miditrackpad
    -l # list all evdev input and midi output devices
    -t <timeout-ms> # how long poll() should wait
    -i <evdev-input-device-id|device-path>
    -o <midi-output-device-id>
    -c <midi-channel> # [0-15]
```

Examples:
```
./miditrackpad -l
```

```
./miditrackpad -i /dev/input/event12 -o2 -c0
# or equivalently
./miditrackpad -i12 -o2 -c0
```

