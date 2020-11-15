include ../../config.mk

openplc_dir = ../thirdparty/OpenPLC_v3
matiec_dir_abs = $(realpath $(openplc_dir)/utils/matiec_src)
matiec = $(matiec_dir_abs)/iec2c

plc_prog_src_dir = plc-prog-src
st_prog_abs = $(realpath $(st_prog))

envs = esp32dev lolin32

.PHONY: all
all: build

# Build ST program if applicable.
.PHONY: st-prog-build
prog-build: prog-clean
	if [ -r "$(st_prog_abs)" ]; then \
		cd $(plc_prog_src_dir) && $(matiec) -I $(matiec_dir_abs)/lib $(st_prog_abs); \
	fi

.PHONY: prog-clean
prog-clean:
	-rm $(plc_prog_src_dir)/*.c $(plc_prog_src_dir)/*.h $(plc_prog_src_dir)/*.csv
# make dummy files to make PLC mockup possible
	touch \
		$(plc_prog_src_dir)/Config0.c \
		$(plc_prog_src_dir)/Res0.c \
		$(plc_prog_src_dir)/LOCATED_VARIABLES.h

.PHONY: build
build: prog-build
	$(pio) run

.PHONY: upload
upload: build
	$(pio) run -t upload

.PHONY: clean
clean: prog-clean
	$(pio) run -t clean

.PHONY: monitor
monitor:
	$(pio) device monitor

.PHONY: format
format:
	find include lib src test \
		-type f \( -iname *.h -o -iname *.cpp -o -iname *.c \) \
	| xargs $(clang-format) -style=file -i

.PHONY: pre-push
pre-push: format
	$(pio) run $(foreach env,$(envs),-e $(env) )
