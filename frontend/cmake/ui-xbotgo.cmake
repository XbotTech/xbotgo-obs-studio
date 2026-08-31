target_sources(
        obs-studio
        PRIVATE
        xbotgo/components/XBotGoComboBoxControl.cpp
        xbotgo/components/XBotGoComboBoxControl.hpp
        xbotgo/components/XBotGoSliderControl.cpp
        xbotgo/components/XBotGoSliderControl.hpp
        xbotgo/director/XBotGoAutoDirector.cpp
        xbotgo/director/XBotGoAutoDirector.hpp
        xbotgo/director/XBotGoAutoDirectorPolicy.hpp
        xbotgo/dialogs/XBotGoLiveStreamConfigDialog.cpp
        xbotgo/dialogs/XBotGoLiveStreamConfigDialog.hpp
        xbotgo/dialogs/XBotGoSliderControlDemoDialog.cpp
        xbotgo/dialogs/XBotGoSliderControlDemoDialog.hpp
        xbotgo/models/XBotGoLiveStreamConfig.hpp
        xbotgo/scenes/XBotGoCameraRoleScenes.cpp
        xbotgo/scenes/XBotGoCameraRoleScenes.hpp
        xbotgo/services/XBotGoLiveStreamProvider.cpp
        xbotgo/services/XBotGoLiveStreamProvider.hpp
        xbotgo/sources/XBotGoFalconMSource.cpp
        xbotgo/sources/XBotGoFalconMSource.hpp
        xbotgo/sources/XBotGoSourceObserver.hpp
)

if(ENABLE_UNIT_TESTS)
  add_executable(
    xbotgo-auto-director-policy-test
    xbotgo/director/XBotGoAutoDirectorPolicyTest.cpp
  )
  target_include_directories(xbotgo-auto-director-policy-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(xbotgo-auto-director-policy-test PRIVATE OBS::libobs)
  add_test(NAME xbotgo-auto-director-policy-test COMMAND xbotgo-auto-director-policy-test)
  set_target_properties_obs(xbotgo-auto-director-policy-test PROPERTIES FOLDER tests)
endif()
