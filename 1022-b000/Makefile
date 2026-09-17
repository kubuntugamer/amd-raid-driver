CXX = g++
CXXFLAGS = -O3 -shared -fPIC -fvisibility=hidden -I. -DVBOX -DVBOX_OSE -DRT_OS_LINUX -DRT_ARCH_AMD64

TARGET = linux.amd64/libmy_plugin.so
SRC = my_plugin.cpp

all: $(TARGET)

$(TARGET): $(SRC)
	@mkdir -p linux.amd64
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -rf linux.amd64 debug.log

.PHONY: all clean
