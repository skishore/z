BUILD := build
C_FILES := $(wildcard src/*/*.c)
CPP_FILES := $(wildcard src/*/*.cpp) main.cpp
OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.obj))) $(addprefix $(BUILD)/,$(notdir $(C_FILES:.c=.obj)))
EXECUTABLE := $(BUILD)/main

INCLUDES := freetype2 freetype2/config harfbuzz
PRELOADS := data #fonts/default_font.ttf images
VPATH := src:$(subst $(eval) ,:,$(wildcard src/*))

EMCC_OBJ_FILES := $(addprefix $(BUILD)/,$(notdir $(CPP_FILES:.cpp=.o))) $(addprefix $(BUILD)/,$(notdir $(C_FILES:.c=.o)))
HTML := $(BUILD)/main.html
BASE_C_FLAGS := -Wall -g #-O2 -DNDEBUG
BASE_CC_FLAGS := ${BASE_C_FLAGS} -std=c++11 -stdlib=libc++ -I permissive-fov

CC := clang++
C_FLAGS := ${BASE_C_FLAGS}
CC_FLAGS := ${BASE_CC_FLAGS} -Isrc #$(addprefix -I/usr/local/include/,$(INCLUDES))
LD_FLAGS := $(CC_FLAGS) #-lSDL2 -lfreetype -lharfbuzz

EMC_FLAGS := ${BASE_C_FLAGS} #-s USE_SDL=2
EMCC_FLAGS := $(BASE_CC_FLAGS) -Isrc #-s USE_SDL=2 $(addprefix -Icompiled-bytecode/include/,$(INCLUDES))
EMCC_LD_FLAGS := $(EMCC_FLAGS) #compiled-bytecode/lib/freetype2/* compiled-bytecode/lib/harfbuzz/*

all:
	make exe

clean:
	rm -f $(EXECUTABLE) $(BUILD)/*.obj
	rm -f $(HTML) $(HTML).mem $(BUILD)/*.js $(BUILD)/*.json $(BUILD)/*.bmp $(BUILD)/*.data $(BUILD)/*.o
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
	em++ --bind $(EMCC_LD_FLAGS) -o $@ $^ $(addprefix --preload-file ,$(PRELOADS))
	for file in build/*.d; do mv $${file} build/`basename $${file} .d`.emccd; done
	cp index.html build/main.html
	cp static/* build/.
	cp images/* build/.

$(BUILD)/%.o: %.cpp
	emcc $(EMCC_FLAGS) -c -MD -o $@ $<

$(BUILD)/%.o: %.c
	emcc $(EMC_FLAGS) -c -MD -o $@ $<

-include build/*.ccd
-include build/*.emccd
