# pcbmodelgen
Convert KiCAD PCB files to models for import in openEMS 

## Description
  You can use this software to convert KiCAD PCB files to Octave function files.
Then call from Octave script to import model into openEMS structures.
  pcbmodelgen also generates Octave function file with model mesh lines. You can
use them as is or modify before adding to simulation structure.

## Installation
### Dependencies
1) TinyXML2 https://github.com/leethomason/tinyxml2 (packages available)
2) TCLAP http://tclap.sourceforge.net/ (packages available)
3) Boost (format) 
### Build
Installation using cmake. 
1) Create build directory.
2) cmake <CMakeLists.txt>
3) make
4) make install

## Usage
Check out examples directory.

