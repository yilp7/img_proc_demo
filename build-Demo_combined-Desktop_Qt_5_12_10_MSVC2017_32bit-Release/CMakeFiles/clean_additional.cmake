# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Release")
  file(REMOVE_RECURSE
  "CMakeFiles\\Demo_param_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Demo_param_autogen.dir\\ParseCache.txt"
  "CMakeFiles\\Demo_release_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\Demo_release_autogen.dir\\ParseCache.txt"
  "Demo_param_autogen"
  "Demo_release_autogen"
  )
endif()
