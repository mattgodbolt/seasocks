
DEPS_3RD=deps/3rd

C_SRC=src/main/c
WEB_SRC=src/report

INCLUDES=-I $(C_SRC) -I $(DEPS_3RD)log4cplus/include
CPPFLAGS=-g -O0 -m64 -fPIC -pthread -Wreturn-type -W -Werror $(INCLUDES) -std=gnu++0x

STATIC_LIBS=-levent

.PHONY: all clean data show-data run-web-server

OBJ_DIR=obj
BIN_DIR=bin
WEB_DIR=web
WEB_PORT=5284

all: $(BIN_DIR)/seasocks

CPP_SRCS=$(shell find $(C_SRC) -name '*.cpp')	

OBJS=$(patsubst $(C_SRC)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))

-include $(OBJS:.o=.d)

$(OBJS) : $(OBJ_DIR)/%.o : $(C_SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CC)  $(CPPFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<" 

$(BIN_DIR)/seasocks: $(OBJS)
	mkdir -p $(BIN_DIR)
	g++ $(CPPFLAGS) -lpcap -o $@ $^ $(STATIC_LIBS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(WEB_DIR) *.tar.gz
