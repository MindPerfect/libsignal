
file(GLOB libSrcs *.cc)
file(GLOB libHdrs *.h *.hh *.hpp)

# Target Library name
set(tgt lss)


add_library(${tgt} STATIC)
# https://cmake.org/cmake/help/latest/command/target_sources.html
# https://stackoverflow.com/questions/10487256/cmake-how-to-properly-copy-static-librarys-header-file-into-usr-include
target_sources(${tgt}
    PRIVATE ${libSrcs}
    PUBLIC FILE_SET HEADERS FILES ${libHdrs}
)
set_target_properties(${tgt} PROPERTIES LINKER_LANGUAGE CXX)

#target_include_directories(${tgt} PUBLIC ${sql3_inc})
#target_link_libraries(${tgt} PUBLIC spdlog::spdlog TBB::tbb sql3lib CLI11::CLI11 absl::strings absl::str_format absl::time absl::time_zone absl::civil_time)
target_link_libraries(${tgt} absl::base absl::log absl::strings)


install(
  TARGETS lss
  EXPORT ${TARGETS_EXPORT_NAME}
  ARCHIVE  DESTINATION ${INSTALL_LIBDIR} COMPONENT lib
  FILE_SET HEADERS DESTINATION ${INSTALL_INCDIR}
)
