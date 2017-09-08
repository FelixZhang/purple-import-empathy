#!/usr/bin/make -f

CC=gcc
LIBS=purple glib-2.0

PKG_CONFIG=pkg-config
CFLAGS+=-DPURPLE_PLUGINS $(shell $(PKG_CONFIG) --cflags $(LIBS))
CFLAGS+=-fPIC -DPIC
CFLAGS+=-Wall -g -O0 -Werror
LDLIBS+=$(shell pkg-config --libs $(LIBS))

PLUGIN_DIR_PURPLE	=  $(shell $(PKG_CONFIG) --variable=plugindir purple)
DATA_ROOT_DIR_PURPLE	=  $(shell $(PKG_CONFIG) --variable=datarootdir purple)

# generate .d files when compiling
CPPFLAGS+=-MMD

OBJECTS=import-empathy.o
TARGET= import-empathy.so

all: $(TARGET)
clean:
	rm -f $(OBJECTS) $(OBJECTS:.o=.d) $(TARGET)

install:
	mkdir -p $(DESTDIR)$(PLUGIN_DIR_PURPLE)
	install -m 664 $(TARGET) $(DESTDIR)$(PLUGIN_DIR_PURPLE)

$(TARGET): $(OBJECTS)
	$(LINK.o) -shared $^ $(LOADLIBES) $(LDLIBS) -o $@

-include $(OBJECTS:.o=.d)
