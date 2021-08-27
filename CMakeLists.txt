cmake_minimum_required(VERSION 3.16)
project(renamer)

enable_testing()

add_executable(renamer renamer.cc)
target_link_libraries(renamer glog)

add_library(hoge SHARED hoge.c)

add_test(
  NAME rename_hoge_test
  COMMAND ./renamer libhoge.so --output libhoge.so.renamed
  WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set_tests_properties(rename_hoge_test PROPERTIES ENVIRONMENT
                                                 "GLOG_logtostderr=1")
