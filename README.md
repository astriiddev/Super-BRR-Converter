# Super-BRR-Converter
Lightweight GUI editor/converter for Super Famicom/Super Nintendo BRR samples in SDL2 for Windows/MacOS/Linux

![image](https://github.com/astriiddev/Super-BRR-Converter/assets/98296288/8cc52a7e-f65b-49c5-9c6e-8b7b526379f7)

Supported input file types: WAV, BRR (SNES/SFC samples), AIF, IFF/8SVX (Amiga Samples), VC (Fairlight CMI samples), mu-law BIN (LM-1/LM-2 drum samples), 8-bit raw PCM.
Supported output file types: WAV, BRR, AIF, IFF/8SVX, mu-law BIN, and 8 or 16-bit raw PCM.

Loading files can be done by dragging files into the window or with the LOAD button. Saving files can be done with the SAVE button
# How To Use
The main goal for this project was to create a simple, easy to use converter for people to easily make their own BRR samples to be used in homebrew SNES/SFC games or homemade SNES/SFC music.

BRR audio samples are a bit of a weird thing and therefore any audio being converted into BRR must meet a few requirements:
1. The sample length (before conversion) must be evenly divisible by 16.
2. If the sample is looped, the length of that loop must also be divisible by 16.
3. The first block of 16 samples must be an amplitude of 0. </br>

SBC will automatically add the block of 0 amplitude samples if not already present. Having a sample length and loop length divisible by 16 requires a bit more work....

Frankly, I find it easiest to find length of samples or loop points that are divisible by 16 if the values of the lengths are in hexadecimal. Why? Because if the smallest value place is 0 in hexadecimal, that length will always be divisible by 16. Therefore the sample length and loop slider values are all set to hex values. </br>
To make it even easier, there's a "BRR SAMPLE SELECT" button in the Options menu which will force the mouse clicks/drags in the waveform area to _always_ land on the start of a 16 sample block. This will also affect the sample start slider and the loop start and end sliders, dragging the sliders while BRR SAMPLE SELECT is engaged will always have them land on the start of a 16 block sample (sample start will be with respect to loop start, loop start with respect to sample start, and loop end with respect to loop start). </br>

And while it's not exactly a _requirement_ of BRR samples, BRR samples are used in SPC song files, which themselves have a 64kb limit. All sample data, song data, and delay buffer RAM must fit into this 64kb space. So it's recommended to keep your BRR samples as short as possible. Resampling the audio data down to a lower sample rate will also decrease file size, while sacrificing sound quality. But hey, lofi is also a vibe.

# BRR Sample Rates
There's a lot of confusion out there about sample rates for BRR samples. Quite a few articles say that BRR samples _must_ be at a sample rate of 8000hz, 16000hz, or 32000hz, due to the SPC700's (SNES audio processor) max sampling rate of 32000hz. Most likely, these articles are treating the 32000hz sample rate like the mixing sample rate that we see in modern audio systems. The SPC700 audio processor is closer to older variable sample rate samplers, such as the Fairlight CMI or the Paula audio processor in the Commodore Amiga, than it is to a modern audio system using 44.1khz or 48khz sample rate. That being said, there's no need to "tune to 500hz" or "tune to 21 cents sharp of B" like these articles suggest. </br>

To more easily get a perfect loop with a loop length that's divisible by 16, it's recommended to resample your audio to your audio's note frequency times a multitude of 16 (recommended 32 or 64 for most samples, 128 for shorter lower frequency samples). To make things easy: if your instrument sample is the note A, resample to 14080hz; if it's the note C, resample to 16744hz. If you're unsure of your sample's pitch, click the PITCH button and Super BRR Converter will do its best to detect a recommended resampling rate. If you're happy with the rate that's in the resample box, click the RESAMPLE button.

# Building
## Windows
Recommended to build with MSYS2/MinGW. </br>
If using MINGW64, install SDL2 with
```
pacman -S mingw-w64-x86_64-SDL2
```
If using UCRT64, install SDL2 with:
```
pacman -S mingw-w64-ucrt-x86_64-SDL2
```
If using CLANG64, install SDL2 with:
```
pacman -S mingw-w64-clang-x86_64-SDL2
```
Navigate in the terminal to whereever you saved the repository and enter:
```
cd windows
make CONFIG=Release
```
Visual Studio project will be uploaded in the future. If you're deadset on building with Visual Studio IDE, the project must be build under C11 and `/experimental:c11atomics` must be added to the compiler command-line. In addition to [including the SDL2 library](https://lazyfoo.net/tutorials/SDL/01_hello_SDL/windows/msvc2019/index.php), `dwmapi.lib` must be added to the linker. Finally `_CRT_SECURE_NO_WARNINGS;_CRT_NONSTDC_NO_DEPRECATE` must be added to the preprocessor definitions.
## MacOS
Install clang with:
```
command xcode-select --install
```
Install the SDL2 framework to `/Library/Frameworks`

Navigate in the terminal to whereever you saved the repository and enter:
```
cd macos
make CONFIG=Release
```
## Linux
Install both SDL2 and GTK+3.0:
```
sudo apt install libgtk-3-dev libsdl2-dev
```
(personally I recommend building SDL2 from source as the SDL2 version from APT is _very_ out of date, but SBC should build under either)

Navigate in the terminal to whereever you saved the repository and enter:
```
cd linux
make CONFIG=Release
```
# The future of this project
Honestly, the main goal of this project, other than to make an easy to use GUI editor for BRR samples, was for me to teach myself how to make a GUI app completely from scratch and learn low-level audio programming without help from a higher level API. A lot of the code I developed for this project made its way into other projects that I've already released (the waveform drawing in this was used in the already released Ami Sampler VST). </br>
I learned a lot about creating widgets from scratch and how to do pseudo-OOP in the C programming language and working on this project really forced me to push my programming abilities. </br>
That being said, I doubt I'll develop this particular project further. I'll do patches and bug fixes when they come up, but I'm not sure I'll add many more features to this project and I'll probably keep it as a lightweight audio editor. </br>
A goal of mine, since before I even started this project, has been to create a module tracker for the SNES so that people can easily create their own SPC songs on the SNES, so I'll take everything I learned from this project and apply it to that one when the time comes. </br>
Until then, thanks for enjoying the Super BRR Converter :)
