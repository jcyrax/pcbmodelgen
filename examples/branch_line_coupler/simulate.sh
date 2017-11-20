#!/bin/bash

pcb_file=pcb.kicad_pcb
config_file=config.json

# Generate octave script files with model and mesh grid
pcbmodelgen -p $pcb_file -c $config_file -m pcb_model.m -g pcb_mesh_grid.m

# Run simulation using generated model and mesh
octave --persist simulation_script.m
