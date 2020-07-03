pcb = test_pcb.kicad_pcb
config = pcbmodelgen.json

all: run sim

# Generate octave script files with model and mesh grid
run:
	pcbmodelgen -p $(pcb) -c $(config) -m kicad_pcb_model.m -g kicad_pcb_mesh.m

# Run simulation using generated model and mesh
sim:
	octave --silent --persist simulation_script.m

clean:
	rm -rf tmp/
	rm -rf kicad_pcb_mesh.m
	rm -rf kicad_pcb_model.m
	rm -f *.png
	rm -f fp-info-cache
	rm -rf *.kicad_pcb-bak
	rm -rf octave-workspace
