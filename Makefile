BUILD := build
CPP_FILES := $(wildcard src/*/*.cpp) main.cpp
OBJ_FILES := $(patsubst src/%.cpp, $(BUILD)/%.obj, $(CPP_FILES))
EXECUTABLE := $(BUILD)/main

INCLUDES := freetype2 freetype2/config harfbuzz
PRELOADS := data #images
VPATH := src:$(subst $(eval) ,:,$(wildcard src/*))

EMCC_OBJ_FILES := $(OBJ_FILES:.obj=.o)
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
	make html

clean:
	rm -rf $(BUILD)

exe: $(BUILD) $(EXECUTABLE)

html: $(BUILD) $(HTML)
	# Uncomment this line to regenerate the static image files.
	cp images/*.png meteor/public/.

$(BUILD):
	mkdir -p $(BUILD)

$(EXECUTABLE):	$(OBJ_FILES)
	$(CC) $(LD_FLAGS) -o $@ $^

$(BUILD)/%.obj: %.cpp
	@mkdir -p $(dir $@)
	$(CC) $(CC_FLAGS) -c -MD -o $@ $<
	@mv $(patsubst %.obj,%.d,$@) $(addprefix $(BUILD)/, $(subst /,_,$(patsubst $(BUILD)/%.obj,%.ccd,$@)))

$(HTML):	$(EMCC_OBJ_FILES)
	em++ --bind $(EMCC_LD_FLAGS) -o $@ $^ $(addprefix --preload-file ,$(PRELOADS))
	# Override Meteor scope guards for the Module global object.
	sed -i '.bak' 's/var Module/var Module = window.Module/g' $(BUILD)/main.js
	rm $(BUILD)/main.js.bak
	# Move the newly compiled files into Meteor-controlled directories.
	cp $(BUILD)/main.js meteor/client/bin/main.js
	cp $(BUILD)/main.data meteor/public/main.data

$(BUILD)/%.o: %.cpp
	@mkdir -p $(dir $@)
	emcc $(EMCC_FLAGS) -c -MD -o $@ $<
	@mv $(patsubst %.o,%.d,$@) $(addprefix $(BUILD)/, $(subst /,_,$(patsubst $(BUILD)/%.o,%.emccd,$@)))

-include build/*.ccd
-include build/*.emccd
