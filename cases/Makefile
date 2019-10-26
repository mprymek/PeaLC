# path to openscad
openscad = openscad

# make command
make = make

# build dir path
build = ./build

.PHONY: all
all: $(build)/cpu-body.stl $(build)/cpu-top-lid.stl $(build)/bpio-body.stl \
	 $(build)/bpio-top-lid.stl $(build)/patchbox-body.stl $(build)/patchbox-top-lid.stl \
	 $(build)/4relays-holders.stl $(build)/bottom-lid.stl

$(build)/cpu-body.stl: lib/cases.scad
	$(openscad) -o $@ -D make_cpu_body=1 $<

$(build)/cpu-top-lid.stl: lib/cases.scad
	$(openscad) -o $@ -D make_cpu_top_lid=1 $<

$(build)/bpio-body.stl: lib/cases.scad
	$(openscad) -o $@ -D make_bpio_body=1 $<

$(build)/bpio-top-lid.stl: lib/cases.scad
	$(openscad) -o $@ -D make_bpio_top_lid=1 $<

$(build)/patchbox-body.stl: lib/cases.scad lib/Small_Single_Keystone_Jack_Faceplate.stl
	$(openscad) -o $@ -D make_patchbox_body=1 $<

$(build)/patchbox-top-lid.stl: lib/cases.scad
	$(openscad) -o $@ -D make_patchbox_top_lid=1 $<

$(build)/4relays-holders.stl: lib/cases.scad
	$(openscad) -o $@ -D make_4relays_holders=1 $<

$(build)/bottom-lid.stl: lib/cases.scad
	$(openscad) -o $@ -D make_bottom_lid=1 $<
