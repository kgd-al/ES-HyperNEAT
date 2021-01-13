cmake_minimum_required(VERSION 3.4)

################################################################################
## General settings
################################################################################


project("ES-HyperNEAT")

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic")#"-ftime-report")

set(CMAKE_CXX_STANDARD 17)

set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

################################################################################
## Multi-configuration installation path
################################################################################

if (${CMAKE_INSTALL_PREFIX} MATCHES "^$ENV{HOME}")
    string(TOLOWER ${CMAKE_BUILD_TYPE} BUILD_TYPE)
    string(APPEND CMAKE_INSTALL_PREFIX "/${BUILD_TYPE}")
    set(CMAKE_PREFIX_PATH ${CMAKE_INSTALL_PREFIX})
endif()
message("> Install path is ${CMAKE_INSTALL_PREFIX}/")

################################################################################
## Dependencies
################################################################################

find_package(APOGeT REQUIRED)
include_directories(${APOGeT_INCLUDE_DIRS})
message("> APOGeT found at " ${APOGeT_DIR})
message("  > Core libraries " "${APOGeT_LIBRARIES}")
list(APPEND CORE_LIBS ${APOGeT_LIBRARIES})
message("  > Gui libraries " "${APOGeT_GUI_LIBRARIES}")
list(APPEND GUI_LIBS ${APOGeT_GUI_LIBRARIES})

list(APPEND KGD_DEFINITIONS ${APOGeT_KGD_DEFINITIONS})

find_package(PkgConfig)
pkg_search_module(gvc libgvc libcgraph libcdt)
if (${gvc_FOUND})
    add_definitions(-DWITH_GVC)
    include_directories(${gvc_INCLUDE_DIRS})
    list(APPEND CORE_LIBS ${gvc_LIBRARIES})
endif()

################################################################################
## Process additional pre-processor definitions
################################################################################
add_definitions(${KGD_DEFINITIONS})
message("Using additionnal pre-processor definitions: ${KGD_DEFINITIONS}")

################################################################################
## Source files (simulation)
################################################################################

FUNCTION(PREPEND output prefix)
   SET(listVar "")
   FOREACH(f ${${output}})
      LIST(APPEND listVar "${prefix}/${f}")
   ENDFOREACH(f)
   SET(${output} "${listVar}" PARENT_SCOPE)
ENDFUNCTION(PREPEND)

set(CORE_SRC
    "genotype/es-hyperneat.cpp"
    "phenotype/cppn.cpp"
    "phenotype/ann.cpp"
)
PREPEND(CORE_SRC "src")

set(MISC_SRC
    "fixed_size_string.hpp"
    "gvc_wrapper.cpp"
)
PREPEND(MISC_SRC "src/misc")

add_library(SIMU_OBJS OBJECT ${CORE_SRC} ${MISC_SRC})

################################################################################
## Target (cppn test)
################################################################################
if (WITH_TESTS)
    add_executable(
      cppn_test
      $<TARGET_OBJECTS:SIMU_OBJS>
      "src/tests/cppn.cpp")
    target_link_libraries(cppn_test ${CORE_LIBS})
endif()

################################################################################
## Target (evaluation)
################################################################################
# add_executable(
#   evaluator
#   $<TARGET_OBJECTS:SIMU_OBJS>
#   "src/evaluator.cpp")
# target_link_libraries(evaluator ${SIMU_LIBS} ${APOGeT_LIBRARIES})

################################################################################
## Target (timelines explorer)
################################################################################
# find_package(OpenMP)
# if (OPENMP_FOUND)
#     message("> OpenMP Found.")
#     message("  > OpenMP flags are " ${OpenMP_CXX_LIBRARIES})
#   add_executable(
#     timelines
#     $<TARGET_OBJECTS:SIMU_OBJS>
#     "src/timelines.cpp")
#   target_link_libraries(timelines ${SIMU_LIBS} ${APOGeT_LIBRARIES}
#                         OpenMP::OpenMP_CXX)
# endif()

#add_executable(
#    pve
#    $<TARGET_OBJECTS:SIMU_OBJS>
#    "src/naturalisation/common.h"
#    "src/naturalisation/common.cpp"
#    "src/naturalisation/pvesimulation.h"
#    "src/naturalisation/pvesimulation.cpp"
#    "src/naturalisation/pve.cpp")
#target_link_libraries(pve ${APOGeT_LIBRARIES})

#add_executable(
#    pvp
#    $<TARGET_OBJECTS:SIMU_OBJS>
#    "src/naturalisation/common.h"
#    "src/naturalisation/common.cpp"
#    "src/naturalisation/pvpsimulation.h"
#    "src/naturalisation/pvpsimulation.cpp"
#    "src/naturalisation/pvp.cpp")
#target_link_libraries(pvp ${APOGeT_LIBRARIES})

################################################################################
## Targets (other)
################################################################################

#option(AUTO_CALIBRATION_TOOL "Whether or not to build the (slighlty) automatic calibration tool" OFF)
#message("> Building automatic calibration tool: " ${AUTO_CALIBRATION_TOOL})
#if (AUTO_CALIBRATION_TOOL)
#  add_executable(auto-calibrator
#                 $<TARGET_OBJECTS:SIMU_OBJS>
#                 "src/misc/autocalibrator.cpp")
#  target_link_libraries(auto-calibrator ${APOGeT_LIBRARIES})
#endif()

#option(ANALYZER_TOOL "Whether or not to build the analyzer tool" OFF)
#message("> Building analyzer tool: " ${ANALYZER_TOOL})
#if (ANALYZER_TOOL)
#  add_executable(analyzer $<TARGET_OBJECTS:SIMU_OBJS> "src/misc/analyzer.cpp")
#  target_link_libraries(analyzer ${APOGeT_LIBRARIES})
#endif()

#option(SAVE_EQUAL_ASSERTER "Whether or not to build the tool for save equality assertion" OFF)
#if (SAVE_EQUAL_ASSERTER)
#  add_executable(save-equal-assert
#                 $<TARGET_OBJECTS:SIMU_OBJS>
#                 "src/misc/saveequalasserter.cpp")
#  target_link_libraries(save-equal-assert ${APOGeT_LIBRARIES})
#endif()

#option(BUILD_TESTS "Whether or not to build test tools" OFF)
#if (BUILD_TESTS)
#  add_executable(test-cgp
#                 $<TARGET_OBJECTS:SIMU_OBJS>
#                 "src/cgp/minicgp_test.cpp")
#  target_link_libraries(test-cgp ${APOGeT_LIBRARIES})
#endif()

if (NOT CLUSTER_BUILD)

    if (NOT S{NO_MULTIMEDIA})
        list(APPEND QT_COMPONENTS Multimedia)
        list(APPEND QT_LIBS Qt5::Multimedia)
    endif()

    find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
    message("Qt found at " ${Qt5_DIR} " with components ${QT_COMPONENTS}")

    ############################################################################
    ## Shut gcc9/Qt deprecated-copy QVariant-related warnings
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "9")
        add_compile_options(-Wno-deprecated-copy)
    endif()

    ############################################################################
    ## Sources files (visualizer)
    ############################################################################

    set(GUI_SRC
        "gvcqtinterface.cpp"
        "graphviewer.cpp"
        "cppn/node.cpp"
        "cppn/edge.cpp"
        "cppn/viewer.cpp"
        "cppn/outputsummary.cpp"
        "ann/node.cpp"
        "ann/edge.cpp"
        "ann/viewer.cpp"
        "es_hyperneatpanel.cpp"
    )
    PREPEND(GUI_SRC "src/gui")

    add_library(GUI_OBJS OBJECT ${GUI_SRC})
    target_include_directories(GUI_OBJS PRIVATE $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(GUI_OBJS PRIVATE $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_COMPILE_DEFINITIONS>)
    target_include_directories(GUI_OBJS PRIVATE $<TARGET_PROPERTY:Qt5::Multimedia,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(GUI_OBJS PRIVATE $<TARGET_PROPERTY:Qt5::Multimedia,INTERFACE_COMPILE_DEFINITIONS>)
    set_property(TARGET GUI_OBJS PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET GUI_OBJS PROPERTY AUTOMOC ON)


##################################################################################
#### Target (watchmaker)
##################################################################################
    if (WITH_SONGMAKER)
        set(SOUND_SRC
            "endlessbuffer.cpp"
            "generator.cpp"
            "visualizer.cpp"
            )
        PREPEND(SOUND_SRC "src/watchmaker/sound")

        add_executable(
          watchmaker
          $<TARGET_OBJECTS:SIMU_OBJS>
          $<TARGET_OBJECTS:GUI_OBJS>

          "src/watchmaker/bwwindow.cpp"
          "src/watchmaker/main.cpp"
        )
        set_property(TARGET watchmaker PROPERTY AUTOMOC ON)
        target_link_libraries(watchmaker ${CORE_LIBS}
    #                                     ${APOGeT_LIBRARIES} ${APOGeT_GUI_LIBRARIES}
                                         ${QT_LIBS})
    endif()


################################################################################
## Target (gui test)
################################################################################
    if (WITH_TESTS)
        add_executable(
          cppn_gui_test
          $<TARGET_OBJECTS:SIMU_OBJS>
          $<TARGET_OBJECTS:GUI_OBJS>
          "src/tests/gui_cppn.cpp"
        )
        set_property(TARGET cppn_gui_test PROPERTY AUTOMOC ON)
        target_link_libraries(cppn_gui_test ${QT_LIBS} ${CORE_LIBS}
                                            ${APOGeT_LIBRARIES}
                                            ${APOGeT_GUI_LIBRARIES})
    endif()


##################################################################################
#### Targets (Watchmaker)
##################################################################################

#    add_executable(
#      watchmaker
#      $<TARGET_OBJECTS:SIMU_OBJS>
#      $<TARGET_OBJECTS:VISU_OBJS>
#      "src/misc/watchmaker_classes.h"
#      "src/misc/watchmaker_classes.cpp"
#      "src/watchmaker.cpp"
#    )
#    set_property(TARGET watchmaker PROPERTY AUTOMOC ON)
#    target_link_libraries(watchmaker ${SIMU_LIBS}
#                                     ${APOGeT_LIBRARIES} ${APOGeT_GUI_LIBRARIES}
#                                     ${QT_LIBS} ${MultiNEAT_LIBS})

##################################################################################
#### Targets (Visu tool)
##################################################################################

#    add_executable(
#        visu-tool
#        $<TARGET_OBJECTS:SIMU_OBJS>
#        $<TARGET_OBJECTS:VISU_OBJS>
#        "src/visualuator.cpp"
#    )
#    set_property(TARGET visu-tool PROPERTY AUTOMOC ON)
#    target_link_libraries(visu-tool ${SIMU_LIBS}
#                                    ${APOGeT_LIBRARIES} ${APOGeT_GUI_LIBRARIES}
#                                    ${QT_LIBS} ${MultiNEAT_LIBS})

endif()

################################################################################
## Target (tester may be with gui)
################################################################################
#if (CLUSTER_BUILD)
#    add_executable(
#        tester
#        $<TARGET_OBJECTS:SIMU_OBJS>
#        "src/tests/tester.cpp")
#    target_link_libraries(tester ${SIMU_LIBS} ${APOGeT_LIBRARIES})
#else()
#    add_executable(
#        tester
#        $<TARGET_OBJECTS:SIMU_OBJS>
#        $<TARGET_OBJECTS:VISU_OBJS>
#        "src/tests/tester.cpp")
#    target_link_libraries(tester ${SIMU_LIBS} ${APOGeT_LIBRARIES}
#                                 ${APOGeT_GUI_LIBRARIES}
#                                 ${QT_LIBS} ${MultiNEAT_LIBS})
#endif()

#################################################################################
### Additional flags
#################################################################################


#message("")


option(WITH_DEBUG_INFO "Sets whether to maximize debug data collection" OFF)
message("> With debug info " ${WITH_DEBUG_INFO})
if(WITH_DEBUG_INFO)
    add_definitions(-DWITH_DEBUG_INFO)

    # Enable small memory error detector (fuse=gold fixes linker errors)
#    set(ASAN "-fsanitize=thread")
    set(ASAN "-fsanitize=address -fsanitize=undefined")
    string(APPEND CMAKE_CXX_FLAGS " -g ${ASAN} -fno-omit-frame-pointer")
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        string(APPEND CMAKE_CXX_FLAGS " -fuse-ld=gold")
    endif()
endif()

option(CLUSTER_BUILD "Whether or not building on a cluster
                      (i-e no gui and local linkage)" ON)
message("> Cluster building mode is " ${CLUSTER_BUILD})
if(CLUSTER_BUILD)
    add_definitions(-DCLUSTER_BUILD)
endif()

option(WITH_TESTS "Whether or not to build all test targets" OFF)
message("> Building tests: " ${WITH_TESTS})

option(WITH_SONGMAKER "Whether or not to build songmaker" OFF)
message("> SongMaker: " ${WITH_SONGMAKER})

#option(UPDATE_DEPENDENCIES "Whether or not to keep dependency data up-to-date
#                            (implies extra compilation time)" ON)
#message("> Auto updating dependencies " ${UPDATE_DEPENDENCIES})

#if (${CMAKE_BUILD_TYPE} STREQUAL "Debug")
#    option(DEBUG_COLLISIONS "Add CPU/RAM cost to view collision tests" OFF)
#    message("> Debugging collisions: " ${DEBUG_COLLISIONS})
#    if (DEBUG_COLLISIONS)
#        add_definitions(-DDEBUG_COLLISIONS)
#    endif()
#endif()

#message("> Build type is " ${CMAKE_BUILD_TYPE})
#message("> Compile flags are " ${CMAKE_CXX_FLAGS})
#message("")


####################################################################################################
## Export configuration
####################################################################################################

#set(DEPENDENCY_GENERATOR ${APOGeT_DIR}/generate_dependency_data.sh)
#set(DEPENDENCY_FILE "${CMAKE_CURRENT_BINARY_DIR}/reus.dependency")
#set(DEPENDENCY_DATA ${PROJECT_SOURCE_DIR}/src/config/dependencies_autogenerated.cpp)
#if (UPDATE_DEPENDENCIES)
#    add_custom_target(dependencies ALL DEPENDS ${DEPENDENCY_FILE} ${DEPENDENCY_DATA})
#    add_custom_command(
#        OUTPUT ${DEPENDENCY_FILE}
#        COMMAND ${DEPENDENCY_GENERATOR}
#            ${DEPENDENCY_FILE} ReusWorld ${CMAKE_BUILD_TYPE}
#        COMMENT "Generating dependency data"
#        DEPENDS always_check
#    )
#    add_custom_command(
#        OUTPUT ${DEPENDENCY_DATA}
#        COMMAND ${PROJECT_SOURCE_DIR}/scripts/format_dependencies_data.sh
#                ${DEPENDENCY_DATA} ${CMAKE_CURRENT_BINARY_DIR} ${APOGeT_DIR}
#        COMMENT "Formatting dependency data"
#        DEPENDS always_check
#    )
#    add_custom_command(OUTPUT always_check COMMAND cmake -E echo)
#endif()
