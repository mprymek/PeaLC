include ../../config.mk

envs = \
	uno \
	pro16MHzatmega328 \
	bluepill_f103c8

.PHONY: build
build:
	$(pio) run

.PHONY: upload
upload:
	$(pio) run -t upload

.PHONY: clean
clean:
	$(pio) run -t clean

.PHONY: format
format:
	find include lib src test \
		-iname *.h -o -iname *.cpp -o -iname *.c \
	| xargs $(clang-format) -style=file -i

.PHONY: pre-push
pre-push: format
	$(pio) run $(foreach env,$(envs),-e $(env) )
