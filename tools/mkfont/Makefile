INSTALLDIR = $(N64_INST)
CFLAGS += -std=gnu99 -O2 -Wall -Werror -I../../include
CFLAGS += -Wno-error=deprecated-declarations -Wno-typedef-redefinition
CFLAGS += -Wno-error=unknown-pragmas -Wno-unused-result 

all: mkfont

mkfont:
	@echo "    [TOOL] mkfont"
	$(CC) $(CFLAGS) -MMD mkfont.c -o mkfont -lm

install: mkfont
	install -m 0755 mkfont $(INSTALLDIR)/bin

.PHONY: clean install

clean:
	rm -rf mkfont

include $(wildcard *.d)
