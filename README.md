### MidiPlayer

MidiPlayer is a cross-platform application for exploring sound synthesis and designing custom instruments from scratch, controllable via any MIDI device.
It also supports SoundFount files for working with more complex and realistic sounds.

https://github.com/user-attachments/assets/70f87e5a-f1b6-432a-90b1-c0006a99af0e

## Compile

> [!NOTE]
> Precompiled binaries are available in the [Releases section](https://github.com/B-Bischoff/MidiPlayer/releases/tag/0.1.0).

To build the project from sources, first clone the repository with its dependencies, then use CMake and a C++ compiler to compile it.

```bash
git clone --recurse-submodules git@github.com:B-Bischoff/MidiPlayer.git
cd MidiPlayer
./start.sh
```

The `start.sh` script will run CMake to configure the build and then compile the project.

If you prefer to build manually without the script, run:
```bash
cmake -S . -B build
make -C build -j $(nproc)
```

## Dependencies

The following apt packages are required to build this project: `xorg-dev libglu1-mesa-dev libasound2-dev libglib2.0-dev cmake build-essential`

- `xorg-dev libglu1-mesa-dev libasound2-dev`: Required libraries and headers for audio and graphics.
- `cmake build-essential` Build system generate, C++ compiler and other essential build tools.

> [!NOTE]
> This project requires a C++ compiler with support for **C++17** (or later) and CMake version **3.5** or higher.

## Usage

### 🎛️ Node Editor
Once you created a new instrument you can:
- Select node(s) using the left mouse button
- Move around using the right mouse button (also works in the graph windows)
- Zoom using the mouse wheel
- Delete selected node(s) by pressing `Delete` key
- Use the `F` key to focus on the selected node(s)
- Change a node's input pin type to slider by right-clicking on it
- Copy, cut and paste nodes using `Ctrl + C / X / V`

https://github.com/user-attachments/assets/845c15df-0b34-4216-9b31-05a6e5093528

### 💾 Save and Load Instrument

Json file located in `resources/instruments` at the same path than MidiPlayer executable are read at startup.
Instruments can be selected and loaded to the node editor from the "Stored instruments" window.

JSON files located in the `resources/instruments` directory, at the same path as the MidiPlayer executable, are read at startup.
Instruments can be selected and loaded into the node editor from the "Stored Instruments" window.

https://github.com/user-attachments/assets/8a5d08dd-687c-4a23-8380-d6fbcc8e83de

### 🔊 Audio Output Settings

Audio settings can be configured from the "Settings" window:

- Select output audio device
- Choose sample rate: 44100 Hz or 48000 Hz
- Set channel mode: mono or stereo
- Adjust audio latency (time between audio generation and upload)

### 🎹 MIDI Device Usage

MIDI device can be selected and used from within the "Settings" window.
The computer keyboard can also be used as MIDI inputs using the following keys:
```
Keyboard keys layout:
    S   D       G   H         BLACK KEYS
  Z   X   C   V   B   N   ,    WHITE KEYS

Piano mapping:
Piano:   C  C# D  D# E  F  F# G  G# A  B
Keys:    Z  S  X  D  C  V  G  B  H  N  ,

Use 'O' and 'P' to respectively go up and down one octave
```

### 🪟 UI features

Windows can be **docked** and **resized** to create custom layouts.

Layouts are automatically saved between sessions, you can delete the `imgui.ini` file to reset everything.

https://github.com/user-attachments/assets/92963b4f-c2f6-4fc8-97cc-39880a892c97

## Nodes

Below is a description of the available nodes, along with details about their inputs and behaviors.

> [!TIP]
> **Multiple links can be connected to the same input**. The final value will be the **sum of all connected links**.

<details>
<summary><strong>Master</strong></summary>

Node from which audio goes to the system sound.
This node is created by default and cannot be removed or duplicated.

The **Master** node sends audio to the system's output device.
It is created by default and **cannot be removed or duplicated**.

**Input:**
`input` — Audio signal (*Any value*).
The signal is **clamped between** `-1` and `1` before being passed to the audio output.

---
</details>

<details>
<summary><strong>Oscillator</strong></summary>

Generates a periodic signal that oscillates between `-1` and `1`.

**Inputs:**
- `freq` — Frequency in Hertz. Defines how many cycles occur per second. (*Any value*)
- `phase` — Phase offset applied to the waveform. (*Any value*)

**UI settings:**

Oscillator shape can be selected from the following waveforms:

- **Sin** — Smooth sine wave oscillation between `-1` and `1`.
- **Square** — Alternates sharply between `-1` and `1`, creating a binary-like signal.
- **Triangle** — Linearly ramps between `-1` and `1` in a triangular shape.
- **Saw Dig** — A digital-style sawtooth wave that ramps linearly up to `1` and drops abruptly to `-1`.
- **White Noise** — Produces random values between `-1` and `1`.
- **Pink Noise** — Noise signal with equal energy per octave, resulting in a warmer, more balanced sound with more emphasis on lower frequencies.
- **Brownian Noise** — Also known as red or Brown noise. It produces a deeper, smoother noise, emphasizing low frequencies even more than pink noise.

---
</details>

<details>
<summary><strong>Multiply</strong></summary>

Multiply and outputs input A with input B.

If input A or B is unplugged, its value is `0`.

**Inputs:**
- `input A` — Audio signal (*Any value*).
- `input B` — Audio signal (*Any value*).
---
</details>

<details>
<summary><strong>Keyboard frequency</strong></summary>

Converts **MIDI note events** (triggered by key presses) into corresponding **frequencies in Hertz**.

This node outputs the base frequency associated with each MIDI note.

A reference table mapping MIDI note numbers to note names and frequencies can be found [here](https://inspiredacoustics.com/en/MIDI_note_numbers_and_center_frequencies).

---
</details>

<details>
<summary><strong>ADSR envelope</strong></summary>

An ADSR envelope defines how the intensity (or amplitude) of its input evolves from the moment a key is pressed until it is released.
It's composed of four phases:

The envelope describe four parameters or phases:
1. Attack : How quickly the sound reaches its maximum intensity after the key is pressed.
2. Decay : How quickly the sound drops from the peak to the sustain level.
3. Sustain : The constant intensity level held as long as the key is pressed.
4. Release : How quickly the sound fades to silence after the key is released.

**Inputs:**
- `input` — Audio signal affected by the envelope  (*Any value*).
- `trigger` — Any **non-zero** value will trigger the envelope.

**UI settings:**

`Edit ADSR` : Modify the shape of the ADSR node using a graphic.

---
</details>

<details>
<summary><strong>Low Pass Filter</strong></summary>

Attenuates **high-frequency components** from the input signal.
As the cutoff value approaches `0`, more high frequencies are removed, resulting in a smoother sound.

**Inputs:**
- `input` — Audio signal (*Any value*).
- `cutoff` — Smoothing factor between `0` and `1` (*Out of range values are clamped*).
- `resonance` —Emphasizes frequencies near the cutoff. Ranges from `0` (no resonance) to `1` (strong peak at cutoff) (*Out of range values are clamped*).

---
</details>

<details>
<summary><strong>High Pass Filter</strong></summary>

Attenuates **low-frequency components** from the input signal.
As the cutoff value approaches `1`, more high frequencies are removed, resulting in a bright sound.

**Inputs:**
- `input` — Audio signal (*Any value*).
- `cutoff` — Smoothing factor between `0` and `1` (*Out of range values are clamped*).
- `resonance` —Emphasizes frequencies near the cutoff. Ranges from `0` (no resonance) to `1` (strong peak at cutoff) (*Out of range values are clamped*).

---
</details>

<details>
<summary><strong>Comb filter</strong></summary>

Applies a delayed version of the signal back into itself.

**Inputs:**
- `input` — Audio signal (*Any value*).
- `delay samples` — Number of samples before the signal is fed back (*Negative values are clamped to `0`*).
- `feedback` — Amount of the delayed signal fed back into the input. Ranges from `0` (no feedback) to `1` (maximum feedback). (*Out-of-range values are clamped*)

---
</details>

<details>
<summary><strong>Overdrive</strong></summary>

Applies a soft clipping effect to simulate analog-style distortion.

- `input` — Audio signal (*Any value*).
- `drive` — Controls the amount of distortion applied. Higher values increase the signal gain before clipping, resulting in a more aggressive, saturated sound.
---
</details>

<details>
<summary><strong>SoundFontPlayer</strong></summary>

Play the content of a SoundFont file in response to incoming MIDI note and velocity events.

**UI settings:**

Provides a file browser for selecting a SoundFont file (`.sf2`) to load and use for playback.

---
</details>


<details>
<summary><strong>Number</strong></summary>

Output a constant value.

**UI settings:**

Value: The number to output, can be any value.

</details>
