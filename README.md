# pops2cue

[POPS2CUE](#pops2cue-v10) / [CUE2POPS](#cue2pops-v23) reconstructed source code.

Note: I couldn't find the source code for these tools and I wanted to build them on macOS, so this is a reverse-engineered version based on a Linux binary disassembly (using [Ghidra](https://github.com/NationalSecurityAgency/ghidra)).

## CUE2POPS v2.3

```
BIN/CUE to POPStarter VCD conversion tool v2.3
Last modified : 2016/11/03

Hello! I convert BIN+CUE disc images to the POPS format.
The BIN has to be a raw MODE2/2352 dump.
The CUE has to be a standard ASCII cuesheet.
Exotic files like NRG, CCD, ISO, GI... are not supported.
Splitted dumps (with separate tracks) are not supported.
High density dumps (aka merged/combined discs) are supported.
Old extra commands gap++, gap--, vmode, trainer, whatever, no longer exist.

Usage:
./cue2pops "input_file" "output_file"

input_file must be an ASCII cuesheet (CUE) which points to a Mode 2 Form 1 CD-ROM image.
output_file is the filename of the VCD you want to create (optional).
```

## POPS2CUE v1.0

```
POPS2CUE v1.0, a POPS VCD to BIN+CUE converter

Usage :
./pops2cue <input_VCD> <option1> <option2>

Options are :
-nobin      : Do not save the disc image (saves the cuesheet only)
-noindex00  : Do not save INDEX 00 entries in the cuesheet

This program accepts VCDs that were made with CUE2POPS v2.0
```
