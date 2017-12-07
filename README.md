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
### Build
#### Linux
Download package and execute from root:
```
sudo apt-get install libtinyxml2-dev libtclap-dev
mkdir build
cd build
cmake ../
make
sudo make install
```

## Usage
Check out examples directory.
Execute from examples directory:
```
./generate_model.sh
```
for basic example of usage. It will show model and after
exit from 3D viewer openEMS will run the simulation and present results.
