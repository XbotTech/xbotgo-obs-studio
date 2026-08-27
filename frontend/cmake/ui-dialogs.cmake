if(NOT TARGET OBS::idian)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/qt/idian" "${CMAKE_BINARY_DIR}/shared/qt/idian")
endif()

target_link_libraries(obs-studio PRIVATE OBS::idian)

if(NOT TARGET OBS::properties-view)
  add_subdirectory("${CMAKE_SOURCE_DIR}/shared/properties-view" "${CMAKE_BINARY_DIR}/shared/properties-view")
endif()

target_link_libraries(obs-studio PRIVATE OBS::properties-view)

target_sources(
  obs-studio
  PRIVATE
    dialogs/LogUploadDialog.cpp
    dialogs/LogUploadDialog.hpp
    dialogs/NameDialog.cpp
    dialogs/NameDialog.hpp
    dialogs/OAuthLogin.cpp
    dialogs/OAuthLogin.hpp
    dialogs/OBSAbout.cpp
    dialogs/OBSAbout.hpp
    dialogs/OBSBasicAdvAudio.cpp
    dialogs/OBSBasicAdvAudio.hpp
    dialogs/OBSBasicFalconMDevices.cpp
    dialogs/OBSBasicFalconMDevices.hpp
    dialogs/OBSBasicFalconMControl.cpp
    dialogs/OBSBasicFalconMControl.hpp
    dialogs/OBSBasicFilters.cpp
    dialogs/OBSBasicFilters.hpp
    dialogs/OBSBasicInteraction.cpp
    dialogs/OBSBasicInteraction.hpp
    dialogs/OBSBasicProperties.cpp
    dialogs/OBSBasicProperties.hpp
    dialogs/OBSBasicSourceSelect.cpp
    dialogs/OBSBasicSourceSelect.hpp
    dialogs/OBSBasicTransform.cpp
    dialogs/OBSBasicTransform.hpp
    dialogs/OBSBasicVCamConfig.cpp
    dialogs/OBSBasicVCamConfig.hpp
    dialogs/OBSLogViewer.cpp
    dialogs/OBSLogViewer.hpp
    dialogs/OBSMissingFiles.cpp
    dialogs/OBSMissingFiles.hpp
    dialogs/OBSRemux.cpp
    dialogs/OBSRemux.hpp
    dialogs/OBSWhatsNew.cpp
    dialogs/OBSWhatsNew.hpp
)

if(ENABLE_UNIT_TESTS)
  add_executable(
    xbotgo-falconm-control-test
    dialogs/OBSBasicFalconMControlTest.cpp
  )
  target_include_directories(xbotgo-falconm-control-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/dialogs")
  target_link_libraries(xbotgo-falconm-control-test PRIVATE OBS::libobs Qt6::Widgets)
  add_test(NAME xbotgo-falconm-control-test COMMAND xbotgo-falconm-control-test)
  set_target_properties_obs(xbotgo-falconm-control-test PROPERTIES FOLDER tests)
endif()
