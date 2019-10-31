include ../../config.mk

openplc_dir = ../thirdparty/OpenPLC_v3
matiec_dir_abs = $(realpath $(openplc_dir)/utils/matiec_src)
matiec = $(matiec_dir_abs)/iec2c

plc_prog_src_dir = plc-prog-src
plc_program_abs = $(realpath $(plc_program))

envs = esp32dev lolin32

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
	$(pio) run

.PHONY: upload
upload:
	$(pio) run -t upload

.PHONY: clean
clean: prog-clean
	$(pio) run -t clean

.PHONY: format
format:
	find include lib src test \
		-type f \( -iname *.h -o -iname *.cpp -o -iname *.c \) \
	| xargs $(clang-format) -style=file -i

.PHONY: pre-push
pre-push: format
	$(pio) run $(foreach env,$(envs),-e $(env) )
