set (TARGET le_imgui)

set (SOURCES "le_imgui.cpp")
set (SOURCES ${SOURCES} "le_imgui.h")

if (${PLUGINS_DYNAMIC})

    add_library(${TARGET} SHARED ${SOURCES})

    # IMPORTANT: --no-gnu-unique
    #            This compiler flag is neccesary as otherwise the library may get compiled
    #            with some symbols exported as UNIQUE, which implicitly makes this library
    #            un-closeable. This means, any calls to `dlclose` on this library, once
    #            loaded, will have no effect, and autoreload for this library will not work
    #            as the first version of the library will remain resident.
    target_compile_options (${TARGET} PUBLIC --no-gnu-unique)
    
    target_compile_definitions(${TARGET}  PUBLIC "PLUGINS_DYNAMIC")
    
else()

    add_library(${TARGET} STATIC ${SOURCES})

    # link against static version of imgui library
    target_link_libraries(${TARGET} PRIVATE imgui)

    add_static_lib( ${TARGET} )

endif()

target_link_libraries(${TARGET} PUBLIC ${LINKER_FLAGS})
source_group(${TARGET} FILES ${SOURCES})
