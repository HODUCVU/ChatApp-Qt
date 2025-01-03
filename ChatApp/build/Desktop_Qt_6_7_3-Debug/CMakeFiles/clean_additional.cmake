# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/appChatApp_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/appChatApp_autogen.dir/ParseCache.txt"
  "appChatApp_autogen"
  )
endif()
