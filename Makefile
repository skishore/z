BUILD := build
CPP_FILES := $(wildcard *.cpp)
OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.obj)))
EXECUTABLE := $(BUILD)/main

EMCC_OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.o)))
HTML := $(BUILD)/main.html
BASE_FLAGS := -Wall -std=c++11 -stdlib=libc++

CC := clang++
CC_FLAGS := ${BASE_FLAGS} -I/usr/local/include/freetype2 -I/usr/local/include/freetype2/config -I/usr/local/include/harfbuzz
LD_FLAGS := $(CC_FLAGS) -lSDL2 -lfreetype -lharfbuzz

EMCC_FLAGS := $(BASE_FLAGS) -s USE_SDL=2 -Icompiled-bytecode/include -Icompiled-bytecode/include/freetype2 -Icompiled-bytecode/include/freetype2/config -Icompiled-bytecode/include/harfbuzz
EMCC_LD_FLAGS := $(EMCC_FLAGS) compiled-bytecode/lib/freetype/* compiled-bytecode/lib/harfbuzz/* compiled-bytecode/lib/SDL_ttf/*

all:
	make exe

clean:
	rm -f $(EXECUTABLE) $(BUILD)/*.obj
	rm -f $(HTML) $(BUILD)/*.js $(BUILD)/*.data $(BUILD)/*.o
	rm -f $(BUILD)/*.d $(BUILD)/*.ccd $(BUILD)/*.emccd
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
	emcc $(EMCC_LD_FLAGS) -o $@ $^ --preload-file data --preload-file fonts/default_font.ttf --preload-file images
	for file in build/*.d; do mv $${file} build/`basename $${file} .d`.emccd; done
	cp index.html build/main.html

$(BUILD)/%.o: %.cpp
	mkdir -p $(BUILD)
	emcc $(EMCC_FLAGS) -c -MD -o $@ $<

-include build/*.ccd
-include build/*.emccd
