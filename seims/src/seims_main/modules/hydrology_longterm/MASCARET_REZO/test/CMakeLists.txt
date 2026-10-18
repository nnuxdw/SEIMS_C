cmake_minimum_required(VERSION 3.10)
project(NodeReachNetworkTest CXX)

add_executable(node_reach_test ../NodeReachNetwork.cpp test_node_reach.cpp)
target_include_directories(node_reach_test PRIVATE ..)
set_property(TARGET node_reach_test PROPERTY CXX_STANDARD 14)

enable_testing()
add_test(NAME node_reach_network COMMAND node_reach_test
    ${CMAKE_CURRENT_SOURCE_DIR}/network.csv
    ${CMAKE_CURRENT_SOURCE_DIR}/boundary.csv
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_network.csv
    ${CMAKE_CURRENT_SOURCE_DIR}/reverse_boundary.csv)
