# sap

<p align="center">
  <img src="https://github.com/user-attachments/assets/667d2d2d-e0e6-4f12-adab-115624e289d1" />
</p>

<p align="center">simple audio player and visualizer</p>

## Build-time dependencies
- C compiler
- pkg-config
- sdl2-dev
- sdl2_mixer-dev

You might need to download additional packages for decoding audio files (e.g., `opusfile`)

## Installing
```
git clone https://github.com/tranarchy/sap
cd sap
./build.sh
sudo ./build.sh install
```

## Usage

When `sap` is ran without arguments it will search for audio files in `$HOME/Music` and in it's sub dirs

`sap [audio_file ...]` is going to add all given audio files into the queue

`sap [dir ...]` is going to add all given dirs and it's sub dirs to the file selection window

You can drag and drop audio files and directories into sap

## Controls

- SPACE - Play / Pause 
- RIGHT ARROW - Skip 5 seconds
- SHIFT+RIGHT ARROW - Skip 10 seconds
- CTRL+RIGHT ARROW - Skip to next song
- LEFT ARROW - Go back 5 seconds
- SHIFT+LEFT ARROW - Go back 10 seconds
- CTRL+LEFT ARROW - Go back to previous song
- UP ARROW - increase volume
- DOWN ARROW - decrease volume

Middle clicking on entries in the queue will delete them from the queue

Right clicking on a dropped down dir in the File selection window will add the entire dir's content to the queue
