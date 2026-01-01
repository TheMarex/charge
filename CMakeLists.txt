cmake_minimum_required(VERSION 3.14)
project(charge C CXX)

option(ENABLE_SANITIZER "Use memory sanitizer for Debug build" OFF)
option(ENABLE_STATISTICS "Enable debug statistics such as heap operation counts" OFF)
option(ENABLE_MEMORY_STATISTICS "Enable debug memory statistics such as label usage" OFF)
option(ENABLE_CCACHE "Speed up incremental rebuilds via ccache" ON)
option(ENABLE_JEMALLOC "Use JeMalloc instead of glibc malloc for speedup" ON)
option(ENABLE_STATIC_STDLIBCXX "Compile everything statically for protable binaries" OFF)

if (ENABLE_SANITIZER)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
endif()
if (ENABLE_STATISTICS)
    add_compile_definitions(CHARGE_ENABLE_STATISTICS)
endif()
if (ENABLE_MEMORY_STATISTICS)
    add_compile_definitions(CHARGE_ENABLE_MEMORY_STATISTICS)
endif()
if (ENABLE_STATIC_STDLIBCXX)
    add_link_options(-static-libstdc++ -static-libgcc)
endif()

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set up include directories as an interface library
add_library(charge_includes INTERFACE)
target_include_directories(charge_includes INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include/
)
target_include_directories(charge_includes SYSTEM INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/libosmium/include
)

# Add compile options and definitions
add_compile_options(-fdiagnostics-color=always)
add_compile_definitions(TEST_DIR="${CMAKE_CURRENT_SOURCE_DIR}/test")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# prefix compilation with ccache by default if available and on clang or gcc
if(ENABLE_CCACHE AND (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU"))
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    message(STATUS "Using ccache to speed up incremental builds")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache)
    set(ENV{CCACHE_CPP2} "true")
  endif()
endif()

find_package (Threads REQUIRED)
find_package (ZLIB REQUIRED)

set(DEFAULT_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} ${ZLIB_LIBRARIES})

if (ENABLE_JEMALLOC)
    find_package (JeMalloc)
    if (JEMALLOC_FOUND)
        message(STATUS "Using JeMalloc at ${JEMALLOC_LIBRARIES}.")
        set(DEFAULT_LIBRARIES ${JEMALLOC_LIBRARIES} ${DEFAULT_LIBRARIES})
    else()
        message(STATUS "JeMalloc not found falling back to default glibc.")
    endif()
endif()

message(STATUS "Using libraries ${DEFAULT_LIBRARIES}")

add_executable(osm2graph src/preprocessing/osm2graph.cpp)
target_link_libraries(osm2graph PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(geojson2charger src/preprocessing/geojson2charger.cpp)
target_link_libraries(geojson2charger PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(graph2turngraph src/preprocessing/graph2turngraph.cpp)
target_link_libraries(graph2turngraph PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_library(STATISTICS OBJECT src/common/statistics.cpp)
target_link_libraries(STATISTICS PRIVATE charge_includes)
add_library(SIGNALS OBJECT src/common/signal_handler.cpp)
target_link_libraries(SIGNALS PRIVATE charge_includes)
add_library(OPTIONS OBJECT src/common/options.cpp)
target_link_libraries(OPTIONS PRIVATE charge_includes)


add_executable(dijkstra_experiment src/experiments/dijkstra.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS> $<TARGET_OBJECTS:SIGNALS>)
target_link_libraries(dijkstra_experiment PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(mc_dijkstra_experiment src/experiments/mc_dijkstra.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS> $<TARGET_OBJECTS:SIGNALS>)
target_link_libraries(mc_dijkstra_experiment PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(mcc_dijkstra_experiment src/experiments/mcc_dijkstra.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS> $<TARGET_OBJECTS:SIGNALS>)
target_link_libraries(mcc_dijkstra_experiment PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(fp_dijkstra_experiment src/experiments/fp_dijkstra.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS> $<TARGET_OBJECTS:SIGNALS>)
target_link_libraries(fp_dijkstra_experiment PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(fpc_dijkstra_experiment src/experiments/fpc_dijkstra.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS> $<TARGET_OBJECTS:SIGNALS>)
target_link_libraries(fpc_dijkstra_experiment PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(generate_queries src/experiments/generate_queries.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS> $<TARGET_OBJECTS:SIGNALS>)
target_link_libraries(generate_queries PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(generate_ranks src/experiments/generate_ranks.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS> $<TARGET_OBJECTS:SIGNALS>)
target_link_libraries(generate_ranks PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(routed src/server/charge.cpp src/server/routed.cpp $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS>)
target_link_libraries(routed PRIVATE charge_includes ${DEFAULT_LIBRARIES})

# Tests
add_executable(common_tests
    test/common/graph_transform_test.cpp
    test/common/functions.cpp
    test/common/minimize_combined_function_test.cpp
    test/common/minimize_composed_function_test.cpp
    test/common/compose_function_test.cpp
    test/common/adj_graph_test.cpp
    test/common/lazy_clear_vector_test.cpp
    test/common/dijkstra_test.cpp
    test/common/mc_dijkstra_test.cpp
    test/common/mcc_dijkstra_test.cpp
    test/common/fp_dijkstra_test.cpp
    test/common/fpc_dijkstra_test.cpp
    test/common/to_geojson_test.cpp
    test/common/path_test.cpp
    test/common/convex_test.cpp
    test/common/csv_test.cpp
    test/common/lower_envelop_test.cpp
    test/common/intersection_test.cpp
    test/common/critical_point_test.cpp
    test/common/roots_test.cpp
    test/common/adapter_iter_test.cpp
    test/common/sink_iter_test.cpp
    test/common/domination_test.cpp
    test/common/nearest_neighbour_test.cpp
    test/common/common.cpp
    $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS>)
target_link_libraries (common_tests PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(server_tests
    src/server/charge.cpp
    test/server/charge_test.cpp
    test/server/http_test.cpp
    test/server/server.cpp
    $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS>)
target_link_libraries (server_tests PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(ev_tests
    test/ev/ev.cpp
    test/ev/phem_test.cpp
    test/ev/graph_transform_test.cpp
    test/ev/node_potential_test.cpp
    test/ev/charging_model_test.cpp
    $<TARGET_OBJECTS:STATISTICS> $<TARGET_OBJECTS:OPTIONS>)
target_link_libraries (ev_tests PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_executable(preprocessing_tests
    test/preprocessing/import_osm_test.cpp
    test/preprocessing/srtm_test.cpp
    test/preprocessing/preprocessing.cpp)
target_link_libraries (preprocessing_tests PRIVATE charge_includes ${DEFAULT_LIBRARIES})

add_custom_target(tests DEPENDS
    common_tests
    server_tests
    ev_tests
    preprocessing_tests
)


