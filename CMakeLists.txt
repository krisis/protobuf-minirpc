cmake_minimum_required(VERSION 3.0)
project (MINIRPC)

find_package(PkgConfig)

pkg_check_modules(PROTOC REQUIRED libprotobuf-c)
pkg_check_modules(LIBEVENT REQUIRED libevent)

set(MINIPROTO ${CMAKE_SOURCE_DIR}/rpc.proto)
set(MINIPROTOSRC ${CMAKE_SOURCE_DIR}/rpc.pb-c.c ${CMAKE_SOURCE_DIR}/rpc.pb-c.h)
add_custom_command (
  OUTPUT ${MINIPROTOSRC}
  COMMAND protoc-c --proto_path=${CMAKE_SOURCE_DIR} --c_out=${CMAKE_SOURCE_DIR} ${MINIPROTO}
  )

set(TESTPROTO ${CMAKE_SOURCE_DIR}/calc.proto)
set(TESTPROTOSRC ${CMAKE_SOURCE_DIR}/calc.pb-c.c ${CMAKE_SOURCE_DIR}/calc.pb-c.h)
add_custom_command (
  OUTPUT ${TESTPROTOSRC}
  COMMAND protoc-c --proto_path=${CMAKE_SOURCE_DIR} --c_out=${CMAKE_SOURCE_DIR} ${TESTPROTO}
  )

set(MINIRPCSRC ${MINIPROTOSRC} ${TESTPROTOSRC} ${CMAKE_SOURCE_DIR}/rpc.c ${CMAKE_SOURCE_DIR}/rpc.h)
add_library(pbminirpc SHARED ${MINIRPCSRC})
target_link_libraries(pbminirpc ${PROTOC_LIBRARIES} ${LIBEVENT_LIBRARIES})

set(SERVERSRC ${CMAKE_SOURCE_DIR}/server.c)
set(CLIENTSRC ${CMAKE_SOURCE_DIR}/client.c)

add_executable(server ${SERVERSRC})
target_link_libraries(server pbminirpc)
add_executable(client ${CLIENTSRC})
target_link_libraries(client pbminirpc)
