
file(GLOB tgtSrcs *.cc)
#file(GLOB tgtHdrs *.h *.hh *.hpp)
#message(STATUS "tgtSrcs: ${tgtSrcs}")
set(tgt lss_test)

add_executable(${tgt})
add_test(NAME ${tgt} COMMAND ${tgt})

target_sources(${tgt} PRIVATE ${tgtSrcs})
target_include_directories(${tgt}
  PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}
    ${PrjSrc}
    ${PrjSrc}/std23
)
target_link_libraries(${tgt} PUBLIC absl::base absl::log absl::strings GTest::gtest)


