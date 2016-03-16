include(CheckIncludeFileCXX)
include(CheckCXXCompilerFlag)
set(TEMP_DIR "${CMAKE_BINARY_DIR}/temp")

# C++0x / C++11
CHECK_CXX_COMPILER_FLAG("-std=c++1y" COMPILER_SUPPORTS_CXX1Y)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)
if (COMPILER_SUPPORTS_CXX1Y)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1y")
elseif (COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif ()

# atomic
CHECK_INCLUDE_FILE_CXX(atomic HAS_ATOMIC)
if (NOT HAS_ATOMIC)
    message(FATAL_ERROR "Seasocks requires a C++ compiler with <atomic>")
endif ()

# override support
file(WRITE ${TEMP_DIR}/TestOverride.cpp
        "struct A { virtual ~A() {} virtual void a() {} };\n
        struct B : A { void a() override {} };\n
        int main() { return 0; }"
)

try_compile(HAVE_OVERRIDE ${TEMP_DIR}
        ${TEMP_DIR}/TestOverride.cpp
        COMPILE_DEFINITIONS
)

if (NOT HAVE_OVERRIDE)
    message(FATAL_ERROR "Seasocks requires a C++ compiler with support for 'override'")
endif ()

# emplace support
file(WRITE ${TEMP_DIR}/TestUnorderedMap.cpp
        "#include <unordered_map>\n
        int main() {
            std::unordered_map<int, int> a;
            a.emplace(2, 3);
            return 0;
        }\n"
)

try_compile(HAVE_UNORDERED_MAP_EMPLACE ${TEMP_DIR}
        ${TEMP_DIR}/TestUnorderedMap.cpp
        COMPILE_DEFINITIONS
)

if (HAVE_UNORDERED_MAP_EMPLACE)
    set(HAVE_UNORDERED_MAP_EMPLACE 1)
else()
    set(HAVE_UNORDERED_MAP_EMPLACE 0)
endif ()
