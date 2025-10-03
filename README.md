# sap

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

When you drag and drop an audio file into `sap` it will add it to the queue
