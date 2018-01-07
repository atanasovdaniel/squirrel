set(SQSTDLIB_SRC sqstdaux.cpp
                 sqstdblob.cpp
                 sqstdio.cpp
                 sqstdmath.cpp
                 sqstdpackage.cpp
                 sqstdrex.cpp
                 sqstdsquirrelio.cpp
                 sqstdstream.cpp
                 sqstdstreamio.cpp
                 sqstdstring.cpp
                 sqstdsystem.cpp
                 sqstdtextio.cpp)

set(SQSTDLIB_PUBINC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
    $<INSTALL_INTERFACE:${INSTALL_INC_DIR}>)

if(NOT DEFINED DISABLE_DYNAMIC)
  add_library(sqstdlib SHARED ${SQSTDLIB_SRC})
  target_link_libraries(sqstdlib squirrel ${CMAKE_DL_LIBS})
  target_include_directories(sqstdlib PUBLIC ${SQSTDLIB_PUBINC})

  add_library(sqstdlib_pkg INTERFACE)
  if(WIN32)
    target_link_libraries(sqstdlib_pkg INTERFACE sqstdlib)
  else()
    target_include_directories(sqstdlib_pkg INTERFACE ${SQSTDLIB_PUBINC})
  endif()
  
  if(NOT DEFINED SQ_DISABLE_INSTALLER)
    install(TARGETS sqstdlib sqstdlib_pkg EXPORT squirrel
                             RUNTIME DESTINATION ${INSTALL_BIN_DIR}
                             LIBRARY DESTINATION ${INSTALL_LIB_DIR}
                             ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
  endif()
endif()

if(NOT DEFINED DISABLE_STATIC)
  add_library(sqstdlib_static STATIC ${SQSTDLIB_SRC})
  target_link_libraries(sqstdlib_static ${CMAKE_DL_LIBS})
  target_include_directories(sqstdlib_static PUBLIC
                    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../include>
                    $<INSTALL_INTERFACE:${INSTALL_INC_DIR}>)
  if(NOT DEFINED SQ_DISABLE_INSTALLER)
    install(TARGETS sqstdlib_static EXPORT squirrel ARCHIVE DESTINATION ${INSTALL_LIB_DIR})
  endif()
endif()

if(NOT DEFINED SQ_DISABLE_INSTALLDEV)
    set(SQ_PUB_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdaux.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdblob.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdio.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdmath.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdpackage.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdstream.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdstreamio.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdstring.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdsystem.h
                     ${CMAKE_CURRENT_SOURCE_DIR}/../include/sqstdtextio.h)
    install(FILES ${SQ_PUB_HEADERS} DESTINATION ${INSTALL_INC_DIR})
endif()

if(DEFINED LONG_OUTPUT_NAMES)
  if(NOT DEFINED DISABLE_DYNAMIC)
    set_target_properties(sqstdlib PROPERTIES OUTPUT_NAME sqstdlib3)
  endif()

  if(NOT DEFINED DISABLE_STATIC)
    set_target_properties(sqstdlib_static PROPERTIES OUTPUT_NAME sqstdlib3_static)
  endif()
endif()

