CXX := g++
WINDRES := windres
CXXFLAGS := -std=c++17 -O2 -municode -Wall -DUNICODE -D_UNICODE
LDFLAGS := -municode -mwindows -static -static-libgcc -static-libstdc++
LIBS := -luser32 -lshell32 -lgdi32 -ladvapi32 -lkernel32 -lole32 -luuid

TARGET := KeepAwake.exe
SOURCES := main.cpp Settings.cpp
OBJECTS := $(SOURCES:.cpp=.o)
RESOURCE := app.res

all: $(TARGET)

$(TARGET): $(OBJECTS) $(RESOURCE)
	$(CXX) $(OBJECTS) $(RESOURCE) -o $(TARGET) $(LDFLAGS) $(LIBS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(RESOURCE): app.rc resource.h app.ico
	$(WINDRES) app.rc -O coff -o $(RESOURCE)

clean:
	rm -f $(OBJECTS) $(RESOURCE) $(TARGET)

.PHONY: all clean
