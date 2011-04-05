C_SRC=src/main/c

INCLUDES=-I $(C_SRC) -Iinclude -Llib
CPPFLAGS=-g -O2 -m64 -fPIC -pthread -Wreturn-type -W -Werror $(INCLUDES) -std=gnu++0x

STATIC_LIBS= 
APP_LIBS=

.PHONY: all clean

OBJ_DIR=obj
BIN_DIR=bin

FIG_DEP=.fig-up-to-date
UNAME_R:=$(shell uname -r)
ifeq "" "$(findstring el5,$(UNAME_R))"
  PLATFORM=ubuntu
  CC=g++
else
  PLATFORM=redhat
  GCC_DIR=/site/apps/gcc-4.5.0
  CC=$(GCC_DIR)/bin/g++
  GCC_LIB_PATH=$(GCC_DIR)/lib64
endif

$(FIG_DEP): package.fig
	rm -rf lib include
	fig -u --config $(PLATFORM) && touch $@

all: $(BIN_DIR)/seasocks $(BIN_DIR)/libseasocks.so $(BIN_DIR)/libseasocks.a

fig: $(FIG_DEP)

CPP_SRCS=$(shell find $(C_SRC) -name '*.cpp')	

OBJS=$(patsubst $(C_SRC)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))

-include $(OBJS:.o=.d)

obj/app/main.o : src/app/c/main.cpp $(FIG_DEP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<" 

$(OBJS) : $(OBJ_DIR)/%.o : $(C_SRC)/%.cpp $(FIG_DEP)
	@mkdir -p $(dir $@)
	$(CC) $(CPPFLAGS) -fPIC -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<" 

$(BIN_DIR)/seasocks: $(OBJS) obj/app/main.o
	mkdir -p $(BIN_DIR)
	$(CC) $(CPPFLAGS) -o $@ $^ $(STATIC_LIBS) $(APP_LIBS)

$(BIN_DIR)/libseasocks.so: $(OBJS)
	mkdir -p $(BIN_DIR)
	$(CC) -shared $(CPPFLAGS) -o $@ $^ $(STATIC_LIBS)

$(BIN_DIR)/libseasocks.a: $(OBJS)
	mkdir -p $(BIN_DIR)
	-rm -f $(BIN_DIR)/libseasocks.a
	ar cq $@ $^

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) *.tar.gz
