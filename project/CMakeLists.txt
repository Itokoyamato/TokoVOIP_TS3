cmake_minimum_required (VERSION 3.6)

project (RadioFx LANGUAGES CXX)

find_package(Qt5 COMPONENTS Gui Widgets REQUIRED)
link_libraries(Qt5::Gui Qt5::Widgets)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

include_directories(
    "src"
    "deps/DSPFilters/shared/DSPFilters/include"
)

set (DSP_FILTERS
    "deps/DSPFilters/shared/DSPFilters/source/Bessel.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Biquad.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Butterworth.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Cascade.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/ChebyshevI.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/ChebyshevII.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Custom.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Design.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Documentation.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Elliptic.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Filter.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Legendre.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/Param.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/PoleFilter.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/RBJ.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/RootFinder.cpp"
    "deps/DSPFilters/shared/DSPFilters/source/State.cpp"
)
source_group("DSPFilters" FILES ${DSP_FILTERS})

set (PLUGIN_SRC
    "src/config_radio.h"
    "src/config_radio.cpp"
    "src/config_radio.ui"
    "src/config_radio_groupbox.h"
    "src/config_radio_groupbox.cpp"
    "src/config_radio_groupbox.ui"
    "src/dsp_radio.h"
    "src/dsp_radio.cpp"
    "src/mod_radio.h"
    "src/mod_radio.cpp"
    "src/plugin.h"
    "src/plugin.cpp"
    "src/plugin_qt.h"
    "src/plugin_qt.cpp"
    "src/settings_radio.h"
    "src/settings_radio.cpp"

    "radiofx.qrc"
)
source_group("plugin" FILES ${PLUGIN_SRC})

# Create named folders for the sources within the .vcproj
# Empty name lists them directly under the .vcproj
#source_group("" FILES ${PLUGIN_SRC})

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
#add_subdirectory(app)
#add_subdirectory (math)
set(WITH_VOLUME true)
set(WITH_VOLUME_WIDGETS true)
#add_subdirectory(../teamspeak-plugin-qt-common common_out)
include(deps/teamspeak-plugin-qt-common/CMakeLists.txt)

add_library(RadioFx SHARED
    ${PLUGIN_SRC} ${TS_QT_CORE} ${TS_QT_VOLUME} ${DSP_FILTERS})