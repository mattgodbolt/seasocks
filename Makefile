default: all
C_SRC=src/main/c
TEST_SRC=src/test/c
APPS_SRC=src/app/c

VERSION_STRING=SeaSocks/$(or $(VERSION),unversioned) ($(shell git rev-parse HEAD))

INCLUDES=-I $(C_SRC) -Iinclude -Llib
CPPFLAGS=-g -O2 -m64 -fPIC -pthread -Wreturn-type -Wall -Werror $(INCLUDES) -std=c++0x '-DSEASOCKS_VERSION_STRING="$(VERSION_STRING)"'

STATIC_LIBS=
APP_LIBS=

.PHONY: all clean run test clobber

OBJ_DIR=obj
BIN_DIR=bin

FIG_DEP=.fig-up-to-date
UNAME_R:=$(shell uname -r)
export LIBRARY_PATH=/usr/lib/x86_64-linux-gnu/
ifeq "" "$(findstring el5,$(UNAME_R))"
  GCC_DIR=/site/apps/gcc-4.7.2-drw.patched.1
  OS_VERSION=redhat5
  GCC_VERSION=472
else
  GCC_DIR=/site/apps/gcc-4.7.2-drw.patched.1.rh5
  OS_VERSION=redhat6
  GCC_VERSION=472
endif
CC=$(GCC_DIR)/bin/g++
GCC_LIB_PATH=$(GCC_DIR)/lib64
export LD_LIBRARY_PATH=$(GCC_LIB_PATH)

$(FIG_DEP): package.fig
	fig --update-if-missing --log-level warn --config build
	touch $@

CPP_SRCS=$(shell find $(C_SRC) -name '*.cpp')
TEST_SRCS=$(shell find $(TEST_SRC) -name '*.cpp')
APPS_CPP_SRCS=$(shell find $(APPS_SRC) -name '*.cpp')
TARGETS=$(patsubst $(APPS_SRC)/%.cpp,$(BIN_DIR)/%,$(APPS_CPP_SRCS))

apps: $(TARGETS)
all: apps $(BIN_DIR)/libseasocks.so $(BIN_DIR)/libseasocks.a test

debug:
	echo $($(DEBUG_VAR))

fig: $(FIG_DEP)

OBJS=$(patsubst $(C_SRC)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))
TEST_OBJS=$(patsubst $(TEST_SRC)/%.cpp,$(OBJ_DIR)/%.o,$(TEST_SRCS))
APPS_OBJS=$(patsubst $(APPS_SRC)/%.cpp,$(OBJ_DIR)/%.o,$(APPS_CPP_SRCS))
ALL_OBJS=$(OBJS) $(APPS_OBJS) $(TEST_OBJS)
GEN_OBJS=$(OBJ_DIR)/embedded.o

-include $(ALL_OBJS:.o=.d)

$(APPS_OBJS) : $(OBJ_DIR)/%.o : $(APPS_SRC)/%.cpp $(FIG_DEP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"

$(OBJS) : $(OBJ_DIR)/%.o : $(C_SRC)/%.cpp $(FIG_DEP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"

$(TEST_OBJS) : $(OBJ_DIR)/%.o : $(TEST_SRC)/%.cpp $(FIG_DEP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"

$(TARGETS) : $(BIN_DIR)/% : $(OBJ_DIR)/%.o $(OBJS) $(GEN_OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) $(CPPFLAGS) -o $@ $^ $(STATIC_LIBS) $(APP_LIBS)

$(BIN_DIR)/libseasocks.so: $(OBJS) $(GEN_OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) -shared $(CPPFLAGS) -o $@ $^ $(STATIC_LIBS)

$(BIN_DIR)/libseasocks.a: $(OBJS) $(GEN_OBJS)
	mkdir -p $(BIN_DIR)
	-rm -f $(BIN_DIR)/libseasocks.a
	ar cq $@ $^

EMBEDDED_CONTENT:=$(shell find src/main/web -type f)
$(OBJ_DIR)/embedded.o: scripts/gen_embedded.py $(EMBEDDED_CONTENT) $(FIG_DEP) src/main/c/internal/Embedded.h
	@mkdir -p $(dir $@)
	scripts/gen_embedded.py $(EMBEDDED_CONTENT) | $(CC) $(CPPFLAGS) -x c++ -c -o "$@" -

run: $(BIN_DIR)/ws_test
	$(BIN_DIR)/ws_test

$(BIN_DIR)/tests: $(TEST_OBJS) $(BIN_DIR)/libseasocks.a
	$(CC) $(CPPFLAGS) -I $(TEST_SRC) -o $@ $^ -lgmock -lgtest

.tests-pass: $(BIN_DIR)/tests
	@rm -f .tests-pass
	$(BIN_DIR)/tests
	@touch .tests-pass

test: .tests-pass

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) *.tar.gz .tests-pass

clobber: clean
	rm -rf lib include $(FIG_DEP)

publish: all
	fig --publish seasocks/$(VERSION).$(OS_VERSION).gcc$(GCC_VERSION)

publish-local: all
	fig --publish-local seasocks/local
