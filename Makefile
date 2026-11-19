BUNDLE  = loudmax-ladspa.lv2
DEST   ?= $(HOME)/.lv2
CFLAGS ?= -O2
CFLAGS += -Wall -Wextra -fPIC -fvisibility=hidden -Ithird_party/ladspa

# Optional: path to la_LoudMax64.so to copy into the bundle, e.g.
#   make install LOUDMAX_SO=~/Downloads/LoudMax/la_LoudMax64.so
LOUDMAX_SO ?=

all: $(BUNDLE)/loudmax_lv2.so $(BUNDLE)/manifest.ttl

$(BUNDLE)/loudmax_lv2.so: src/loudmax_lv2.c third_party/ladspa/ladspa.h
	@mkdir -p $(BUNDLE)
	$(CC) $(CFLAGS) $(LDFLAGS) -shared $< -o $@ -ldl

$(BUNDLE)/manifest.ttl $(BUNDLE)/loudmax.ttl &: src/gen_ttl.py
	@mkdir -p $(BUNDLE)
	python3 -I src/gen_ttl.py $(BUNDLE)

install: all
	mkdir -p $(DEST)
	rm -rf $(DEST)/$(BUNDLE)
	cp -r $(BUNDLE) $(DEST)/
ifneq ($(LOUDMAX_SO),)
	cp $(LOUDMAX_SO) $(DEST)/$(BUNDLE)/la_LoudMax64.so
endif

uninstall:
	rm -rf $(DEST)/$(BUNDLE)

test/test_host: test/test_host.c
	$(CC) -O2 -Wall $< -o $@ -ldl -lm

# Needs la_LoudMax64.so to be findable (bundle dir, LADSPA_PATH, ~/.ladspa, ...)
test: all test/test_host
	./test/test_host $(BUNDLE)

validate: all
	for f in $(BUNDLE)/*.ttl; do serdi -i turtle $$f >/dev/null || exit 1; done
	@echo "Turtle OK"

clean:
	rm -rf $(BUNDLE) test/test_host

.PHONY: all install uninstall test validate clean
