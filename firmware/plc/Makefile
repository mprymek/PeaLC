# platformio path
pio = pio

openplc_dir = ../thirdparty/OpenPLC_v3
matiec_dir_abs = $(realpath $(openplc_dir)/utils/matiec_src)
matiec = $(matiec_dir_abs)/iec2c

plc_prog_src_dir = plc-prog-src
plc_program_abs = $(realpath $(plc_program))

.PHONY: all
all: build

.PHONY: check-plc_program
check-plc_program:
	@if [ ! "$(plc_program)" ]; then \
		/bin/echo -e '\nERROR: you must provide plc_program variable, for instance:\n\n$$ make build plc_program=st/blink.st\n\n'; \
		return 1; \
	fi

.PHONY: prog-build
prog-build: check-plc_program prog-clean
	cd $(plc_prog_src_dir) && $(matiec) -I $(matiec_dir_abs)/lib $(plc_program_abs)

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
	$(pio) run -e esp32dev

.PHONY: upload
upload:
	$(pio) run -e esp32dev -t upload

.PHONY: clean
clean: prog-clean
	$(pio) run -e esp32dev -t clean
