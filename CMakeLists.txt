cmake_minimum_required(VERSION 3.4)

################################################################################
## General settings
################################################################################

project("ES-HyperNEAT")
message("\n####################################################################"
    "############\n## CMakeFile for ${PROJECT_NAME}")

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
set(LIB_BASE "${CMAKE_INSTALL_PREFIX}/lib/kgd")
message("> Install path is ${CMAKE_INSTALL_PREFIX}/")

################################################################################
## Dependencies
################################################################################

find_package(APOGeT REQUIRED)
include_directories(${APOGeT_INCLUDE_DIRS})
message("> APOGeT found at " ${APOGeT_DIR})
list(APPEND CORE_LIBS ${APOGeT_LIBRARIES})
message("  > Core libraries " "${CORE_LIBS}")
list(APPEND GUI_LIBS ${APOGeT_GUI_LIBRARIES})
message("  > Gui libraries " "${GUI_LIBS}")

list(APPEND KGD_DEFINITIONS ${APOGeT_KGD_DEFINITIONS})

if (${NO_GVC})
    message("Not searching for gvc")
else()
    find_package(PkgConfig)
    pkg_search_module(gvc libgvc libcgraph libcdt)
    if (${gvc_FOUND})
        list(APPEND KGD_DEFINITIONS -DWITH_GVC)
        include_directories(${gvc_INCLUDE_DIRS})
        list(APPEND CORE_LIBS ${gvc_LIBRARIES})
    elseif(NOT ${CLUSTER_BUILD})
        message(FATAL_ERROR
                "gvc is essential in graphic mode. Please install it properly")
    endif()
endif()

################################################################################
## Process additional pre-processor definitions
################################################################################
add_definitions(${KGD_DEFINITIONS})
message("Using additionnal pre-processor definitions: ${KGD_DEFINITIONS}")

################################################################################
## Source files (core-library)
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

add_library(eshn-core STATIC ${CORE_SRC} ${MISC_SRC})
target_link_libraries(eshn-core ${CORE_LIBS})
list(APPEND NEW_CORE_LIBS ${LIB_BASE}/$<TARGET_FILE_NAME:eshn-core>)

################################################################################
## Target (cppn test)
################################################################################
if (WITH_TESTS)
    add_executable(
      cppn_test
      "src/tests/cppn.cpp")
  target_link_libraries(cppn_test ${CORE_LIBS} eshn-core)
endif()

if (NOT CLUSTER_BUILD)

    ############################################################################
    ## Shut gcc9/Qt deprecated-copy QVariant-related warnings
    if(CMAKE_CXX_COMPILER_VERSION VERSION_GREATER "9")
        add_compile_options(-Wno-deprecated-copy)
    endif()

    ############################################################################
    ## Qt library (additional dependencies)
    ############################################################################

    list(APPEND QT_COMPONENTS 3DCore 3DRender 3DInput 3DExtras)
    list(APPEND QT_LIBS Qt5::3DCore Qt5::3DRender Qt5::3DInput Qt5::3DExtras)

    find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
    message("Qt found at " ${Qt5_DIR})
    message("  Components > " ${QT_COMPONENTS})
    list(APPEND GUI_LIBS ${QT_LIBS})
    message("        Libs > " ${GUI_LIBS})

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

        "ann/neuronstate.cpp"

        "ann/2d/node.cpp"
        "ann/2d/edge.cpp"
        "ann/2d/viewer.cpp"

        "ann/3d/node.cpp"
        "ann/3d/edge.cpp"
        "ann/3d/viewer.cpp"

        "es_hyperneatpanel.cpp"
        "config.cpp"
    )
    PREPEND(GUI_SRC "src/gui")

    add_library(eshn-gui STATIC ${GUI_SRC})
    target_link_libraries(eshn-gui eshn-core ${CORE_LIBS} ${GUI_LIBS})
    target_include_directories(eshn-gui PRIVATE
        $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_INCLUDE_DIRECTORIES>)
    target_compile_definitions(eshn-gui PRIVATE
        $<TARGET_PROPERTY:Qt5::Widgets,INTERFACE_COMPILE_DEFINITIONS>)
    set_property(TARGET eshn-gui PROPERTY POSITION_INDEPENDENT_CODE ON)
    set_property(TARGET eshn-gui PROPERTY AUTOMOC ON)
    list(APPEND NEW_GUI_LIBS ${LIB_BASE}/$<TARGET_FILE_NAME:eshn-gui>)



################################################################################
## Target (gui test)
################################################################################
    if (WITH_TESTS)
        add_executable(
          cppn_gui_test
          "src/tests/gui_cppn.cpp"
        )
        set_property(TARGET cppn_gui_test PROPERTY AUTOMOC ON)
        target_link_libraries(cppn_gui_test ${CORE_LIBS} eshn-core
                                            ${GUI_LIBS} eshn-gui)
    endif()

endif()


#################################################################################
### Additional flags
#################################################################################


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

option(NO_GVC "Whether to prevent looking for the gvc library" OFF)
message("> Disable gvc: " ${NO_GVC})

option(WITH_TESTS "Whether or not to build all test targets" OFF)
message("> Building tests: " ${WITH_TESTS})

option(ESHN_WITH_DISTANCE "Whether CPPN inputs include connection length" OFF)
message("> CPPN distance: ${CPPN_WITH_DISTANCE}")
if (ESHN_WITH_DISTANCE)
    add_definitions(-DESHN_WITH_DISTANCE)
    list(APPEND KGD_DEFINITIONS -DESHN_WITH_DISTANCE)
endif()

set(ESHN_SUBSTRATE_DIMENSION "2" CACHE STRING
    "Dimension of the ANN substrate (2 or 3)")
set_property(CACHE ESHN_SUBSTRATE_DIMENSION PROPERTY STRINGS 2 3)
message("> Substrate dimension: ${ESHN_SUBSTRATE_DIMENSION}")
add_definitions(-DESHN_SUBSTRATE_DIMENSION=${ESHN_SUBSTRATE_DIMENSION})
list(APPEND KGD_DEFINITIONS -DESHN_SUBSTRATE_DIMENSION=${ESHN_SUBSTRATE_DIMENSION})

set(ESHN_ANN_TYPE "Float" CACHE STRING "Neuron implementation")
set_property(CACHE ESHN_ANN_TYPE PROPERTY STRINGS Float Spiking)
message("> ANN Type: ${ESHN_ANN_TYPE}")
add_definitions(-DESHN_ANN_TYPE=${ESHN_ANN_TYPE})
list(APPEND KGD_DEFINITIONS -DESHN_ANN_TYPE=${ESHN_ANN_TYPE})

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


###################################################################################################
# Export configuration
###################################################################################################

install(TARGETS eshn-core ARCHIVE DESTINATION lib/kgd)
if (NOT CLUSTER_BUILD)
    install(TARGETS eshn-gui ARCHIVE DESTINATION lib/kgd)
endif()

install(
    DIRECTORY "${CMAKE_SOURCE_DIR}/src/"
    DESTINATION include/kgd/eshn
    FILES_MATCHING
        PATTERN "*.h"
        PERMISSIONS
            OWNER_EXECUTE OWNER_READ
            GROUP_EXECUTE GROUP_READ
        PATTERN "*.hpp"
        PERMISSIONS
            OWNER_EXECUTE OWNER_READ
            GROUP_EXECUTE GROUP_READ
)

if (GENERATE_HELP)
    install(
        DIRECTORY "${CMAKE_SOURCE_DIR}/docs/"
        DESTINATION docs/apt
    )
endif()

set(CONFIG "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake")
file(GENERATE
    OUTPUT ${CONFIG}
    CONTENT
"# CMake configuration settings for project ${PROJECT_NAME} (${CMAKE_BUILD_TYPE})

set(${PROJECT_NAME}_INCLUDE_DIRS
    ${CMAKE_INSTALL_PREFIX}/include)

set(${PROJECT_NAME}_LIBRARIES
    $<JOIN:${NEW_CORE_LIBS};${CORE_LIBS},
    >)

$<$<NOT:$<BOOL:${CLUSTER_BUILD}>>:
set(Qt5_DIR ${Qt5_DIR})
set(QT_COMPONENTS ${QT_COMPONENTS})
find_package(Qt5 REQUIRED COMPONENTS ${QT_COMPONENTS})
message(\"Qt found at \" ${Qt5_DIR})

set(${PROJECT_NAME}_GUI_LIBRARIES
    $<JOIN:${NEW_GUI_LIBS};${GUI_LIBS},
    >)
>

set(${PROJECT_NAME}_KGD_DEFINITIONS ${KGD_DEFINITIONS})
set(DEPENDENCY_GENERATOR
    ${CMAKE_INSTALL_PREFIX}/cmake/generate_dependency_data.sh)

set(${PROJECT_NAME}_BUILD_TYPE ${CMAKE_BUILD_TYPE})
if(NOT \${${PROJECT_NAME}_BUILD_TYPE} STREQUAL \${CMAKE_BUILD_TYPE})
    message(FATAL_ERROR \"Mismatching build version!
        Linking against '\${${PROJECT_NAME}_BUILD_TYPE}' while using
        '\${CMAKE_BUILD_TYPE}'\")
endif()

set(${PROJECT_NAME}_CLUSTER_BUILD ${CLUSTER_BUILD})
if (\${${PROJECT_NAME}_CLUSTER_BUILD} STREQUAL ON
    AND \${CLUSTER_BUILD} STREQUAL OFF)
    message(FATAL_ERROR \"Mismatching cluster build values!
        ${PROJECT_NAME} was built in cluster mode\")
endif()
")
install(FILES ${CONFIG} DESTINATION cmake)

set(DEPENDENCY_FILE "${CMAKE_CURRENT_BINARY_DIR}/eshn.dependency")
add_custom_target(dependencies ALL DEPENDS ${DEPENDENCY_FILE})
add_custom_command(
    OUTPUT ${DEPENDENCY_FILE} always_check
    COMMAND ${DEPENDENCY_GENERATOR}
        ${DEPENDENCY_FILE} ESHN ${CMAKE_BUILD_TYPE}

    COMMENT "Generating dependency data"
)
install(FILES ${DEPENDENCY_FILE} DESTINATION cmake)
