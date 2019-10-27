# path to openscad
openscad = openscad

# make command
make = make

# build dir path
build = ./build

# images dir path
img = ./doc/img

main_parts := cpu-body bpio-body patchbox-body 4relays-holders
lids := cpu-top-lid bpio-top-lid patchbox-top-lid bottom-lid

parts := $(main_parts) $(lids)

stls := $(foreach part,$(parts),$(build)/$(part).stl)
pngs := $(foreach part,$(main_parts),$(img)/$(part).png) \
		$(img)/cpu-top-lid.png $(img)/bottom-lid.png

.PHONY: all
all: $(stls) $(pngs)

.PHONY: clean
clean:
	-rm $(stls) $(pngs)

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

camera1 := --camera=10,20,25,50,0,295,250

$(img)/cpu-body.png: lib/cases.scad
	$(openscad) -o $@ -D make_cpu_body=1 -D show_cpu_pcb=0 $(camera1) $<

$(img)/bpio-body.png: lib/cases.scad
	$(openscad) -o $@ -D make_bpio_body=1 -D show_bpio_pcb=0 $(camera1) $<

$(img)/patchbox-body.png: lib/cases.scad lib/Small_Single_Keystone_Jack_Faceplate.stl
	$(openscad) -o $@ -D make_patchbox_body=1 -D show_patchbox_pcb=0 $(camera1) $<

camera2 := --camera=12,10,8,27,0,305,208

$(img)/4relays-holders.png: lib/cases.scad
	$(openscad) -o $@ -D make_4relays_holders=1 $(camera2) $<

camera3 := --camera=10,20,25,50,0,295,200

$(img)/cpu-top-lid.png: lib/cases.scad
	$(openscad) -o $@ -D make_cpu_top_lid=1 $(camera3) $<

$(img)/bottom-lid.png: lib/cases.scad
	$(openscad) -o $@ -D make_bottom_lid=1 $(camera3) $<
