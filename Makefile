BUILD := build
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.o)))
EXECUTABLE := $(BUILD)/main
HTML := $(BUILD)/main.html

CC_FLAGS := -Wall -std=c++11 -stdlib=libc++
LD_FLAGS := $(CC_FLAGS)

#CC := clang++
#TARGET := $(EXECUTABLE)
CC := emcc
TARGET := $(HTML)

all: $(TARGET)

clean:
	rm -f $(BUILD)/*.o
	rm -f $(BUILD)/*.d
	rm -f $(EXECUTABLE)
	rmdir -p $(BUILD)

exe: $(EXECUTABLE)

$(HTML):	$(OBJ_FILES)
	mkdir -p $(BUILD)
	emcc $(LD_FLAGS) -o $@ $^

$(EXECUTABLE):	$(OBJ_FILES)
	mkdir -p $(BUILD)
	$(CC) $(LD_FLAGS) -o $@ $^

$(BUILD)/%.o: %.cpp
	mkdir -p $(BUILD)
	$(CC) $(CC_FLAGS) -c -MD -o $@ $<

-include build/*.d
