
include_directories( 
${KOPETE_INCLUDES} 
)


########### next target ###############

set(kopete_contactnotes_PART_SRCS contactnotesplugin.cpp contactnotesedit.cpp )

add_library(kopete_contactnotes MODULE ${kopete_contactnotes_PART_SRCS})


target_link_libraries(kopete_contactnotes KF5::KIOCore kopete )

install(TARGETS kopete_contactnotes  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES kopete_contactnotes.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES contactnotesui.rc  DESTINATION ${DATA_INSTALL_DIR}/kopete_contactnotes)




