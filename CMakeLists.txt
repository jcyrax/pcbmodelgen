#pcbmodelgen
cmake_minimum_required(VERSION 3.5)

project(pcbmodelgen)

#Dependencies
find_path(TINYXML2_INCLUDE_DIR
  NAMES tinyxml2.h
  PATHS ${TINYXML2_PKGCONF_INCLUDE_DIRS}
)

find_library(TINYXML2_LIBRARY
  NAMES tinyxml2
  PATHS ${TINYXML2_PKGCONF_LIBRARY_DIRS}
)

find_path(TCLAP_INCLUDE_DIR
  NAMES tclap
  PATHS ${TCLAP_PKGCONF_INCLUDE_DIRS}
)

message("TinyXML2 include path: " ${TINYXML2_INCLUDE_DIR})
message("TinyXML2 library: " ${TINYXML2_LIBRARY})
message("TCLAP include path: " ${TCLAP_INCLUDE_DIR})

if((TINYXML2_INCLUDE_DIR MATCHES TINYXML2_INCLUDE_DIR-NOTFOUND) OR (TINYXML2_LIBRARY MATCHES TINYXML2_LIBRARY-NOTFOUND))
    message("TinyXML2 not found")
endif()

if(TCLAP_INCLUDE_DIR MATCHES TCLAP_INCLUDE_DIR-NOTFOUND)
    message("TCLAP library not found")
endif()


set( SRC
    source/ems.cpp
    source/ems_prims.cpp
    source/kicadtoems_config.cpp
    source/kicadtoems_ui.cpp
    source/main.cpp
    source/srecs.cpp
    source/json/jsoncpp.cpp
    )

SET( INCLUDE_DIRS
     source/
     ${TINYXML2_INCLUDE_DIR}
     ${TCLAP_INCLUDE_DIR}
    )


include_directories(${INCLUDE_DIRS})

SET(GCC_COVERAGE_COMPILE_FLAGS "-std=c++11")

SET( CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} ${GCC_COVERAGE_COMPILE_FLAGS}" )

add_executable(pcbmodelgen ${SRC})

target_link_libraries(pcbmodelgen ${TINYXML2_LIBRARY})

install(TARGETS pcbmodelgen
        RUNTIME DESTINATION bin)














