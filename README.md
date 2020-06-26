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
# Dependencies
sudo apt-get install libtinyxml2-dev libtclap-dev

# Build and install 
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


## Json Fields

Description of some fields in pcbmodelgen config .json

| Field | Description |
| --- | --- |
| pcb_metal_zero_thick | If this is true, then top and bottom copper has zero thickness. This allows for less mesh lines and faster simulation, but you must be careful how mesh lines are positioned otherwise you can get invalid results. |
| corner_approximation | Impacts number of mesh lines and so the simulation time. |
| insert_automatic_mesh | This controls automatic mesh line generation. You don't have to use it, you can generate the lines as needed manually or by some other automation process. |
| manual_mesh | Insert your manual mesh line positions here. They will be inserted before automatic line generation. |
| min_cell_size, max_cell_size | Sets needed cell size boundaries for simulation. This relates to your test signal bandwidth. Make as large as possible for used test signal frequency to minimize mesh line count. |
| pcb_z_lines | will insert this amount of mesh lines between top and bottom copper layers, before performing automatic mesh line generation. This is needed because there is no geometry on the inside of PCB. |
| smooth_mesh_lines, smth_neighbor_size_diff | This tries to make neighboring mesh cells not differ by more than smth_neighbor_size_diff times. This is needed for FTDT simulation. You can't have abrupt change in cell sizes - this can make your simulation invalid. |
| SimulationBox | Sets simulation domain region. You need to place this around your geometry with sufficient distance. |
| use_box_fill | Fill simulation domain with specified material. |

One thing to notice regarding mesh line generation. Is is not always successful given needed parameters. If you request large min size and small difference between neighboring cells it will fail to comply and report errors you mentioned. This is also highly dependent on pcb geometry. As each geometry point contributes to mesh lines. This in itself doesn't mean that you can't use the results, but you need to evaluate resulting mesh and decide for yourself if given mesh is good enough for your usage case. You can explore generated kicad_pcb_mesh.m to see all warnings about non conforming mesh sizes.

Also if you are using `pcb_metal_zero_thick` you need to adjust mesh lines accordingly. For that there is `boundary_one_third_rule`.
