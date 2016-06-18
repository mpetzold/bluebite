CC				:= g++
CFLAGS			:= -fPIC -Wall -Wl,-rpath,’$$ORIGIN’
LDFLAGS			:= -shared
RM				:= rm -f
LIBRARY			:= libBluetoothHciSocket.so
SRCFILES		:= BluetoothHciSocket.cpp

OBJFILES 		:= $(SRCFILES:.cpp=.o)

# Set INCLUDES according to location of JAVA_HOME

OS := $(shell uname)

ifeq ($(OS),Linux)

ARCH := $(shell uname -m)

ifeq ($(ARCH),x86_64)

JAVA_HOME		:= $(shell readlink -f /usr/bin/javac | sed "s:bin/javac::")
INCLUDES		:= -I . -I ../include -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
DEPENDENCIES	:= -L../../../lib/linux/x86_64 -luv -lstdc++
OUTPUT			:= ../../../lib/linux/x86_64

else ifneq ($(filter arm%,$(ARCH)),)

JAVA_HOME		:= $(shell readlink -f /usr/bin/javac | sed "s:bin/javac::")
INCLUDES		:= -I . -I ../include -I$(JAVA_HOME)/include -I$(JAVA_HOME)/include/linux
DEPENDENCIES	:= -L../../../lib/linux/arm -luv -lstdc++
OUTPUT			:= ../../../lib/linux/arm

endif

else

$(error OS not supported )

endif

# Execute selected target

all:
	$(CC) $(LDFLAGS) -o $(OUTPUT)/$(LIBRARY) $(CFLAGS) $(INCLUDES) $(SRCFILES) $(DEPENDENCIES) 

clean:
	$(RM) $(OUTPUT)/$(LIBRARY) $(OBJFILES)
	