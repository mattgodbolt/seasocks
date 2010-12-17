
DEPS_3RD=deps/3rd

C_SRC=src/main/c
WEB_SRC=src/report

BOOST_OVERRIDE=-I /site/libs/1.1/3rd/x86_64/include/boost-1_36
INCLUDES=-I $(C_SRC) -I $(DEPS_3RD)log4cplus/include $(BOOST_OVERRIDE) 
CPPFLAGS=-DBOOST_ASIO_DISABLE_EVENTFD=1 -g -O0 -m64 -fPIC -pthread -Wreturn-type -W -Werror $(INCLUDES) -std=gnu++0x

ifdef TEAMCITY
BOOST_LIB_OVERRIDE=/site/libs/1.1/3rd/x86_64/lib/
STATIC_LIBS=
else
BOOST_LIB_OVERRIDE=/usr/lib
STATIC_LIBS=
endif

.PHONY: all clean data show-data run-web-server

OBJ_DIR=obj
BIN_DIR=bin
WEB_DIR=web
WEB_PORT=5284

all: $(BIN_DIR)/ws

CPP_SRCS=$(shell find $(C_SRC) -name '*.cpp')	

OBJS=$(patsubst $(C_SRC)/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))

-include $(OBJS:.o=.d)

$(OBJS) : $(OBJ_DIR)/%.o : $(C_SRC)/%.cpp
	@mkdir -p $(dir $@)
	$(CC)  $(CPPFLAGS) -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -c -o "$@" "$<" 

$(BIN_DIR)/ws: $(OBJS)
	mkdir -p $(BIN_DIR)
	g++ $(CPPFLAGS) -lpcap -o $@ $^ $(STATIC_LIBS)

clean:
	rm -rf $(OBJ_DIR) $(BIN_DIR) $(WEB_DIR) *.tar.gz

