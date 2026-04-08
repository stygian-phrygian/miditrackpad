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
./build/apps/miditrackpad # <--- binary here
```

## Usage
```
# likely requires sudo (root privileges), as we access evdev input devices
./miditrackpad
    -l # list all evdev input and midi output devices
    -t <timeout> # how long poll() should wait
    -i <evdev-input-device-id|device-path>
    -o <midi-output-device-id
    -c <midi-channel> # [0-15]
```
