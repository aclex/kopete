
include_directories( 
${KOPETE_INCLUDES} 
)


########### next target ###############

set(kopete_history_PART_SRCS 
   historyplugin.cpp 
   historydialog.cpp 
   historylogger.cpp 
   converter.cpp 
   historyguiclient.cpp
   historyimport.cpp )

ki18n_wrap_ui(kopete_history_PART_SRCS historyviewer.ui )

kconfig_add_kcfg_files(kopete_history_PART_SRCS historyconfig.kcfgc )


add_library(kopete_history MODULE ${kopete_history_PART_SRCS})


target_link_libraries(kopete_history  KF5::KHtml kopete )

install(TARGETS kopete_history  DESTINATION ${PLUGIN_INSTALL_DIR})


########### next target ###############

set(kcm_kopete_history_PART_SRCS historypreferences.cpp )

ki18n_wrap_ui(kcm_kopete_history_PART_SRCS historyprefsui.ui )

kconfig_add_kcfg_files(kcm_kopete_history_PART_SRCS historyconfig.kcfgc )


add_library(kcm_kopete_history MODULE ${kcm_kopete_history_PART_SRCS})


target_link_libraries(kcm_kopete_history   kopete )

install(TARGETS kcm_kopete_history  DESTINATION ${PLUGIN_INSTALL_DIR})


########### install files ###############

install( FILES historyconfig.kcfg  DESTINATION ${KCFG_INSTALL_DIR})
install( FILES kopete_history.desktop  DESTINATION ${SERVICES_INSTALL_DIR})
install( FILES historyui.rc  historychatui.rc  DESTINATION ${DATA_INSTALL_DIR}/kopete_history)
install( FILES kopete_history_config.desktop  DESTINATION ${SERVICES_INSTALL_DIR}/kconfiguredialog)




