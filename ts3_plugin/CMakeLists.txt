cmake_minimum_required (VERSION 3.6)

project (TokoVoip LANGUAGES CXX)
add_definitions(-DASIO_STANDALONE -DUSE_STANDALONE_ASIO)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5 COMPONENTS Core Sql REQUIRED)
link_libraries(Qt5::Core Qt5::Sql)

include_directories(
    "src"
    "deps/openssl/include"
    "deps/asio/asio/include"
    "deps/Simple-WebSocket-Server"
    "deps/json/single_include/nlohmann"
    "deps/cpp-httplib"
)

set (TOKOVOIP_SRC
    "src/tokovoip.cpp"
    "src/tokovoip.h"
)
source_group("tokovoip" FILES ${TOKOVOIP_SRC})

# Create named folders for the sources within the .vcproj
# Empty name lists them directly under the .vcproj
#source_group("" FILES ${TOKOVOIP_SRC})

# Turn on the ability to create folders to organize projects (.vcproj)
# It creates "CMakePredefinedTargets" folder by default and adds CMake
# defined projects like INSTALL.vcproj and ZERO_CHECK.vcproj
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Set compiler flags and options.
# Here it is setting the Visual Studio warning level to 4
# set (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /W4")

# Command to output information to the console
# Useful for displaying errors, warnings, and debugging
# message ("cxx Flags:" ${CMAKE_CXX_FLAGS})

# Sub-directories where more CMakeLists.txt exist
include(deps/teamspeak-plugin-radiofx/CMakeLists.txt)

project (TokoVoip LANGUAGES CXX)

link_directories(deps/openssl/)

add_library(TokoVoip SHARED
    ${TOKOVOIP_SRC} ${PLUGIN_SRC} ${TS_QT_CORE} ${TS_QT_VOLUME} ${DSP_FILTERS})

if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "4")
    target_link_libraries(TokoVoip crypt32.lib normaliz.lib libcrypto32MD.lib libssl32MD.lib)
    set_target_properties(TokoVoip PROPERTIES OUTPUT_NAME "tokovoip_win32")
ELSE()
    target_link_libraries(TokoVoip crypt32.lib normaliz.lib libcrypto64MD.lib libssl64MD.lib)
    set_target_properties(TokoVoip PROPERTIES OUTPUT_NAME "tokovoip_win64")
ENDIF()

set_property(TARGET TokoVoip PROPERTY CXX_STANDARD 17)
set_property(TARGET TokoVoip PROPERTY CXX_STANDARD_REQUIRED ON)
