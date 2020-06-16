
PCB_FILE = pcb.kicad_pcb

CONFIG_FILE = config.json

all: run sim

# Generate octave script files with model and mesh grid
run:
	pcbmodelgen -p $(PCB_FILE) -c $(CONFIG_FILE) -m kicad_pcb_model.m -g kicad_pcb_mesh.m

# Run simulation using generated model and mesh
sim:
	octave --persist simulation_script.m

clean:
	rm -rf tmp/
	rm -f *.png
	rm -f fp-info-cache
	rm -rf kicad_pcb_mesh.m
	rm -rf kicad_pcb_model.m
	rm -rf *.kicad_pcb-bak






# yardbird.kicad_pcb
# yardbird.pro
