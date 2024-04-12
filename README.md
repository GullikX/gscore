# gscore - OpenGL-based music composition software

In gscore, a score is made up of small building blocks, creatively called *blocks*. Blocks are akin to classes in programming. Each block is defined once and can then be instantiated any number of times within the score. Blocks can also be shared across different instrument tracks. Change the definition of a block and all instances are updated accordingly. No need to copy and paste stuff.

gscore has two different editing modes, one for modifying block definitions and one for adding block instances to the score. These are called *edit mode* and *object mode*, respectively. Yes, this is inspired by Blender, and yes, you switch between the modes using `tab`.

gscore uses human-readable XML format for its project files, easy to edit by hand or check into a version control system.


## Default keybindings

### Global

* `b` Select current block, or create new one if one with the provided name does not exist
* `c` Set color of the current block
* `r` Rename current block
* `t` Set score tempo (BPM)
* `w` Write score to file
* `q` Quit application
* `tab` Switch between edit mode and object mode
* `ctrl+tab` Toggle between the two most recent blocks
* `space` Play score/block starting from the beginning
* `ctrl+space` Play score/block starting from the mouse cursor
* `shift+space` Play score/block with repeat, can be combined with `ctrl`

### Object mode

* `i` Set instrument/synth program for the hovered track
* `n` Toggle midi note-off events/sustain pedal for the hovered track
* `v` Set velocity of the hovered track
* `ctrl+up` Decrement the number of instrument tracks
* `ctrl+down` Increment the number of instrument tracks
* `ctrl+left` Decrement the score length by one block
* `ctrl+right` Increment tho score length by one block
* `left mouse button` Add instance of the current block to the score
* `right mouse button` Remove block instance from the score
* `middle mouse button` Pick the hovered block and set as current, pick hovered instrument for edit mode
* `ctrl+scoll wheel` Adjust the velocity of the hovered block instance

### Edit mode

* `k` Set key signature for the background highlighting
* `i` Set instrument/synth program to use inside edit mode
* `n` Toggle midi note-off events/sustain pedal inside edit mode
* `ctrl+up` Transpose the view upwards one octave
* `ctrl+down` Transpose the view downwards one octave
* `left mouse button` Place note, hold and drag for longer note
* `right mouse button` Remove hovered note, hold and drag to remove multiple notes
* `middle mouse button` Preview note at mouse position, hold and drag to preview multiple notes
* `ctrl+scoll wheel` Adjust the velocity of the hovered note


## Configuration

gscore is configured by editing the file `config.h` and recompiling.


## Notes

* Think of gscore as a midi editor, not a DAW. There are no effects, filters or any other kind of audio processing. Export the scores using midi format and import them into another application for additional tinkering.
* gscore only supports sf2 soundfonts for audio, not vst's or anything fancy (TODO maybe?).
* For a list of downloadable soundfonts, check out the FluidSynth wiki: https://github.com/FluidSynth/fluidsynth/wiki/SoundFont
* Unused block definitions and empty tracks are automatically discarded when saving a score. Instantiate any blocks you want to keep!
* Changing the instrument while editing block definitions in edit mode only affects the sound inside of edit mode.
* Project files created by gscore has a `metadata` tag inside of them. You can put whatever you want in there and it will be preserved when reading/saving the file.
* Use `shift+enter` when entering values with dmenu to prevent auto-complete.


## Dependencies

* dmenu
* libfluidsynth 1.x
* glew
* glfw
* libX11
* libXrandr
* libxml2
* xprop

### Optional, for midi exporter

* python3
    * mido


## Installing and running

Installing:

```
gmake && sudo gmake install
```

Running:

```
export GSCORE_SOUNDFONTS=/path/to/soundfont1.sf2:/path/to/soundfont2.sf2:...
gscore projectfile.gsx
```

Keep the terminal window open and visible to receive helpful status messages.

Export a project file as midi:

```
gscore-export-midi projectfile.gsx
```


## Links

* Main repo: https://github.com/GullikX/gscore
* Repo mirrors:
    * https://bitbucket.org/Gullik/gscore
    * https://git.sr.ht/~gullik/gscore
* Build service: https://builds.sr.ht/~gullik/gscore
