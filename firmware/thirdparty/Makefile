openplc_dir = ./OpenPLC_v3
matiec_dir = $(realpath $(openplc_dir)/utils/matiec_src)
matiec = $(matiec_dir)/iec2c

.PHONY: all
all: fetch patch build

.PHONY: fetch
fetch:
	git submodule update --init

.PHONY: patch
patch:
	cd $(openplc_dir) && git apply $(realpath ./openplc.patch)

.PHONY: build
build: $(matiec)

$(matiec):
	cd $(matiec_dir) && autoreconf -i
	cd $(matiec_dir) && ./configure
	cd $(matiec_dir) && make

.PHONY: clean
clean:
	rm -rf $(openplc_dir)