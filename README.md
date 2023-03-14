# X11 Audio Visualizer
App that will visualize audio from a Pulseaudio source.

## Dependencies
* X11
* PulseAudio
* fftw3

## Installation
Before building the program ensure you have installed all dependencies.

Once you have installed the dependencies, you can compile the program.

```bash
gcc -o AudioVisualize visualize.c -lX11 -lpulse -lpulse-simple -lfftw3 -lm 
```
## Usage
To use the program, run the compiled binary, including the name of your Pulseaudio source as a parameter.
```bash
./AudioVisualize [SOURCE]
```
If no source is included it will use your default audio source.
You can find sources by using:
```bash
pactl list short sources
```
