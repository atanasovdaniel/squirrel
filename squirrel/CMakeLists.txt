set(SQUIRREL_SRC sqapi.cpp
                 sqbaselib.cpp
                 sqclass.cpp
                 sqcompiler.cpp
                 sqdebug.cpp
                 sqfuncstate.cpp
                 sqlexer.cpp
                 sqmem.cpp
                 sqobject.cpp
                 sqstate.cpp
                 sqtable.cpp
                 sqvm.cpp)

set(SQUIRREL_PUBINC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${INSTALL_INC_DIR}>)

if(NOT DEFINED DISABLE_DYNAMIC)
  add_library(squirrel SHARED ${SQUIRREL_SRC})
  target_include_directories(squirrel PUBLIC ${SQUIRREL_PUBINC})

  add_library(squirrel_pkg INTERFACE)
  if(WIN32)
    target_link_libraries(squirrel_pkg INTERFACE squirrel)
  else()
    target_include_directories(squirrel_pkg INTERFACE ${SQUIRREL_PUBINC})
  endif()
  
  if(NOT DEFINED SQ_DISABLE_INSTALLER)
    install(TARGETS squirrel squirrel_pkg EXPORT squirrel
                         RUNTIME DESTINATION ${INSTALL_BIN_DIR}
                         LIBRARY DESTINATION ${INSTALL_LIB_DIR}
                         ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
  endif()
endif()

if(NOT DEFINED DISABLE_STATIC)
  add_library(squirrel_static STATIC ${SQUIRREL_SRC})
  target_include_directories(squirrel_static PUBLIC
                         $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                         $<INSTALL_INTERFACE:${INSTALL_INC_DIR}>)
  if(NOT DEFINED SQ_DISABLE_INSTALLER)
    install(TARGETS squirrel_static EXPORT squirrel ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
  endif()
endif()

if(NOT DEFINED SQ_DISABLE_INSTALLDEV)
    set(SQ_PUB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqconfig.h
                       ${CMAKE_CURRENT_SOURCE_DIR}/../include/squirrel.h)
    install(FILES ${SQ_PUB_HEADERS} DESTINATION ${INSTALL_INC_DIR})
endif()

if(DEFINED LONG_OUTPUT_NAMES)
  if(NOT DEFINED DISABLE_DYNAMIC)
    set_target_properties(squirrel PROPERTIES OUTPUT_NAME squirrel3)
  endif()

  if(NOT DEFINED DISABLE_STATIC)
    set_target_properties(squirrel_static PROPERTIES OUTPUT_NAME squirrel3_static)
  endif()
endif()
