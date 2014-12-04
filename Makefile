BUILD := build
C_FILES := $(wildcard *.c)
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.obj))) $(addprefix $(BUILD)/,$(notdir $(C_FILES:.c=.obj)))
EXECUTABLE := $(BUILD)/main

EMCC_OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.o))) $(addprefix $(BUILD)/,$(notdir $(C_FILES:.c=.o)))
HTML := $(BUILD)/main.html
BASE_C_FLAGS := -Wall
BASE_CC_FLAGS := ${BASE_C_FLAGS} -std=c++11 -stdlib=libc++

CC := clang++
C_FLAGS := ${BASE_C_FLAGS}
CC_FLAGS := ${BASE_CC_FLAGS} -I/usr/local/include/freetype2 -I/usr/local/include/freetype2/config -I/usr/local/include/harfbuzz
LD_FLAGS := $(CC_FLAGS) -lSDL2 -lfreetype -lharfbuzz

EMC_FLAGS := ${BASE_C_FLAGS} -s USE_SDL=2
EMCC_FLAGS := $(BASE_CC_FLAGS) -s USE_SDL=2 -Icompiled-bytecode/include -Icompiled-bytecode/include/freetype2 -Icompiled-bytecode/include/freetype2/config -Icompiled-bytecode/include/harfbuzz
EMCC_LD_FLAGS := $(EMCC_FLAGS) compiled-bytecode/lib/freetype/* compiled-bytecode/lib/harfbuzz/*

all:
	make exe

clean:
	rm -f $(EXECUTABLE) $(BUILD)/*.obj
	rm -f $(HTML) $(BUILD)/*.js $(BUILD)/*.data $(BUILD)/*.o
	rm -f $(BUILD)/*.d $(BUILD)/*.ccd $(BUILD)/*.emccd
	rmdir -p $(BUILD)

exe: $(BUILD) $(EXECUTABLE)

html: $(BUILD) $(HTML)

$(BUILD):
	mkdir -p $(BUILD)

$(EXECUTABLE):	$(OBJ_FILES)
	$(CC) $(LD_FLAGS) -o $@ $^

$(BUILD)/%.obj: %.cpp
	$(CC) $(CC_FLAGS) -c -MD -o $@ $<
	for file in build/*.d; do mv $${file} build/`basename $${file} .d`.ccd; done

$(BUILD)/%.obj: %.c
	clang $(C_FLAGS) -c -MD -o $@ $<

$(HTML):	$(EMCC_OBJ_FILES)
	emcc $(EMCC_LD_FLAGS) -o $@ $^ --preload-file data --preload-file fonts/default_font.ttf --preload-file images
	for file in build/*.d; do mv $${file} build/`basename $${file} .d`.emccd; done
	cp index.html build/main.html

$(BUILD)/%.o: %.cpp
	emcc $(EMCC_FLAGS) -c -MD -o $@ $<

$(BUILD)/%.o: %.c
	emcc $(EMC_FLAGS) -c -MD -o $@ $<

-include build/*.ccd
-include build/*.emccd
