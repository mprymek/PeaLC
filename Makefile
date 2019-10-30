# format sources
.PHONY: format
format:
	$(MAKE) -C firmware/plc $@
	$(MAKE) -C firmware/slave $@

# test sources before pushing to git
.PHONY: pre-push
pre-push:
	$(MAKE) -C cases $@
	$(MAKE) -C firmware/plc $@
	$(MAKE) -C firmware/slave $@
