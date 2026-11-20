cmake_minimum_required(VERSION 3.16.0)
list(APPEND EXTRA_COMPONENT_DIRS "common/components")
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smart_fan)
