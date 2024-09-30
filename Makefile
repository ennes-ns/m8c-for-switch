# Nintendo Switch Compilation Setup
# Switch-specific compiler and tools from devkitPRO
DEVKITPRO ?= /c/devkitPro
DEVKITA64 ?= $(DEVKITPRO)/devkitA64
PORTLIBS ?= $(DEVKITPRO)/portlibs
LIBNX ?= $(DEVKITPRO)/libnx

CC = $(DEVKITA64)/bin/aarch64-none-elf-gcc
CXX = $(DEVKITA64)/bin/aarch64-none-elf-g++

# Compilation and linking flags
CFLAGS = -g -O2 -Wall -march=armv8-a -D__SWITCH__ -fPIE \
         -I$(PORTLIBS)/switch/include/SDL2 \
         -I$(LIBNX)/include \
         -I$(PORTLIBS)/switch/include \
         -DSDL_DISABLE_STATIC_ASSERT

LDFLAGS = -L$(PORTLIBS)/switch/lib \
          -L$(LIBNX)/lib \
          -specs=$(LIBNX)/switch.specs \
          -lSDL2 -lGLESv2 -lnx -lm -pie -march=armv8-a

# Source files and object files
SRCS = src/main.c src/input.c src/audio.c src/render.c src/config.c \
       src/slip.c src/gamecontrollers.c src/fx_cube.c src/serial.c \
       src/inline_font.c src/command.c src/ini.c
	   
OBJS = $(SRCS:.c=.o)

# Output name for the executable
TARGET = m8c_switch.elf

# Build rules
all: $(TARGET)

# Linking the final ELF binary for Switch
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# Compiling individual source files into object files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Cleaning up compiled files
clean:
	rm -f $(OBJS) $(TARGET)

# Optional: Packaging for Switch homebrew
package: $(TARGET)
	nxlink $(TARGET)
