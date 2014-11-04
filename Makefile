BUILD := build
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.obj)))
EXECUTABLE := $(BUILD)/main

EMCC_OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.o)))
HTML := $(BUILD)/main.html

CC := clang++
CC_FLAGS := -Wall -std=c++11 -stdlib=libc++
LD_FLAGS := $(CC_FLAGS)

all:
	make exe && make html

clean:
	rm -f $(EXECUTABLE) $(BUILD)/*.obj
	rm -f $(HTML) $(BUILD)/*.js $(BUILD)/*.data $(BUILD)/*.o
	rm -f $(BUILD)/*.ccd $(BUILD)/*.emccd
	rmdir -p $(BUILD)

exe: $(EXECUTABLE)

html: $(HTML)

$(EXECUTABLE):	$(OBJ_FILES)
	mkdir -p $(BUILD)
	$(CC) $(LD_FLAGS) -o $@ $^

$(BUILD)/%.obj: %.cpp
	mkdir -p $(BUILD)
	$(CC) $(CC_FLAGS) -c -MD -o $@ $<
	for file in build/*.d; do mv $${file} build/`basename $${file} .d`.ccd; done

$(HTML):	$(EMCC_OBJ_FILES)
	mkdir -p $(BUILD)
	emcc $(LD_FLAGS) -o $@ $^
	for file in build/*.d; do mv $${file} build/`basename $${file} .d`.emccd; done

$(BUILD)/%.o: %.cpp
	mkdir -p $(BUILD)
	emcc $(CC_FLAGS) -c -MD -o $@ $<

-include build/*.ccd
-include build/*.emccd
