# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\FallDetectionAdmin_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\FallDetectionAdmin_autogen.dir\\ParseCache.txt"
  "FallDetectionAdmin_autogen"
  )
endif()
