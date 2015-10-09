
set(TEMP_DIR "${CMAKE_BINARY_DIR}/temp")

file(WRITE ${TEMP_DIR}/TestUnorderedMap.cpp 
"#include <unordered_map>\n
int main(void)
{
    std::unordered_map<int, int> a;
    a.emplace(2, 3);
    
    return 0;
}\n\n"
)

try_compile(HAVE_UNORDERED_MAP_EMPLACE ${TEMP_DIR} 
                    ${TEMP_DIR}/TestUnorderedMap.cpp
                    COMPILE_DEFINITIONS -std=c++11
                    )

if( HAVE_UNORDERED_MAP_EMPLACE )
    set(HAVE_UNORDERED_MAP_EMPLACE 1)
else()
    set(HAVE_UNORDERED_MAP_EMPLACE 0)
endif()

