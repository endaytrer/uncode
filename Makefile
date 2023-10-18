CC = gcc
LD = gcc

DEPS := gtk4 gl glew freetype2

CFLAGS := -std=c11 -Wall -Wextra -g -O2
CFLAGS += $(shell pkg-config --cflags $(DEPS))
LIBS := $(shell pkg-config --libs $(DEPS))

SHADERS := shaders
SHADER_FILES = $(wildcard $(SHADERS)/*.glsl)
SHADER_C = $(patsubst $(SHADERS)/%.glsl, $(SHADERS)/%.c, $(SHADER_FILES))

SRC := src
C_FILES := $(wildcard $(SRC)/*.c)
C_HDRS := $(wildcard $(SRC)/*.h)
OBJS := objs
C_OBJS := $(patsubst $(SRC)/%.c, $(OBJS)/%.o, $(C_FILES))
C_OBJS += $(patsubst $(SHADERS)/%.glsl, $(OBJS)/%.o, $(SHADER_FILES))

EXE = uncode

.PHONY: all clean

all: $(EXE)

$(EXE): $(C_OBJS)
	$(LD) $(LDFLAGS) -o $@ $^ $(LIBS)

$(OBJS)/%.o: $(SRC)/%.c $(C_HDRS) | $(OBJS)
	$(CC) $(CFLAGS) -c -o $@ $<

$(OBJS):
	mkdir -p $(OBJS)

$(OBJS)/%.o: $(SHADERS)/%.c
	$(CC) $(CFLAGS) -c -o $@ $^

$(SHADERS)/%.c: $(SHADERS)/%.glsl
	xxd -i $^ > $@

clean:
	rm -f $(EXE) \
		$(OBJS)/*.o \
		$(SHADERS)/*.c