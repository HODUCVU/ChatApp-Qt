# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/ChatServer_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/ChatServer_autogen.dir/ParseCache.txt"
  "ChatServer_autogen"
  )
endif()
