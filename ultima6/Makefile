# MinGW build of the stand-alone Ultima 6 engine (src/).
#
#   mingw32-make            build build/u6.exe
#   mingw32-make test       build and run the unit tests (tests/)
#   mingw32-make clean
#
# Variables:
#   DEBUG=1    -O0 -g
#   WARN=0     silence compiler warnings (on by default)

ifeq ($(origin CXX),default)
CXX := g++
endif
BUILD := build
SRC   := src
TARGET := $(BUILD)/u6.exe

ENGINE_SRCS := engine/game_id ultima6/game martian_dreams/game savage_empire/game files/data_file files/tile_data files/world_data files/lzw files/map_data files/obj_block files/lib_n files/shape files/tile_set files/converse files/portrait files/picture files/music engine/opl2 engine/sfx engine/music_player engine/framebuffer engine/backend engine/backend_win32 ultima6/session ultima6/magic ultima6/creatures ultima6/combat ultima6/front_end ultima6/movement ultima6/actors ultima6/commands ultima6/text_window ultima6/conversation ultima6/talk ultima6/describe ultima6/ui ultima6/stats_view ultima6/equipment ultima6/world ultima6/map_renderer ultima6/gypsy ultima6/ending ultima6/use ultima6/moongate ultima6/spells ultima6/game_tables ultima6/transfer martian_dreams/tables savage_empire/tables
SRCS := $(ENGINE_SRCS) main

CXXFLAGS := -std=c++17 -I$(SRC)
ifeq ($(DEBUG),1)
CXXFLAGS += -O0 -g
else
CXXFLAGS += -O2
endif
ifneq ($(WARN),0)
CXXFLAGS += -Wall -Wextra
endif

LIBS :=
ifeq ($(OS),Windows_NT)
LIBS := -lgdi32 -luser32 -lwinmm
endif

OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(SRCS)))
ENGINE_OBJS := $(addprefix $(BUILD)/,$(addsuffix .o,$(ENGINE_SRCS)))
TESTS := $(basename $(notdir $(wildcard tests/test_*.cpp)))
TEST_EXES := $(addprefix $(BUILD)/,$(addsuffix .exe,$(TESTS)))

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LIBS)

$(BUILD)/%.o: $(SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -MMD -MP -c -o $@ $<

$(BUILD)/test_%.exe: tests/test_%.cpp $(ENGINE_OBJS)
	@mkdir -p $(BUILD)
	$(CXX) $(CXXFLAGS) -o $@ $< $(ENGINE_OBJS) $(LIBS)

test: $(TEST_EXES)
	@for t in $(TEST_EXES); do echo "== $$t"; ./$$t || exit 1; done

clean:
	rm -rf $(BUILD)

-include $(OBJS:.o=.d)

.PHONY: all test clean
