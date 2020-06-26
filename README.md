# pcbmodelgen
Convert KiCAD PCB files to models for import in openEMS 

## Description

You can use this software to convert KiCAD PCB files to Octave function files.
Then call from Octave script to import model into openEMS structures.
  pcbmodelgen also generates Octave function file with model mesh lines. You can
use them as is or modify before adding to simulation structure.

## Installation

### Dependencies

Usage dependencies
- AppCSXCAD
- OpenEMS

Build dependencies
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

OpenEMS should be installed and the octave paths should be configured in `~/.octaverc` as in:

```
addpath('/usr/share/octave/packages/openems-0.0.35/')
addpath('/usr/local/share/CSXCAD/matlab/')
```

Check out examples directory.

Inside each example folder execute the make command as in:
```
# To run everything just type
make

# To generate the mesh of the Kicad's PCB
make run

# To simulate the generated mesh with openEMS
make sim

# To clean the generated files
make clean
```

These steps will show model using the AppCSXCAD and after exit from its 3D viewer, the next step is to run the simulation with openEMS and present results.

### Windows users:

On `Windows 10` it has been tested with the WLS Linux Ubuntu App, downloaded from Microsoft Store.
The AppCSXCAD and the openEMS must be available on PATH environment variable so pcbgenmod can find required tools.
