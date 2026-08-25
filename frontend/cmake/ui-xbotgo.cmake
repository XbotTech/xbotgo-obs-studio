target_sources(
        obs-studio
        PRIVATE
        xbotgo/components/XBotGoSliderControl.cpp
        xbotgo/components/XBotGoSliderControl.hpp
        xbotgo/dialogs/XBotGoLiveStreamConfigDialog.cpp
        xbotgo/dialogs/XBotGoLiveStreamConfigDialog.hpp
        xbotgo/dialogs/XBotGoSliderControlDemoDialog.cpp
        xbotgo/dialogs/XBotGoSliderControlDemoDialog.hpp
        xbotgo/models/XBotGoLiveStreamConfig.hpp
        xbotgo/services/XBotGoLiveStreamProvider.cpp
        xbotgo/services/XBotGoLiveStreamProvider.hpp
)

target_link_libraries(obs-studio PRIVATE OBS::xbotgo-device-discovery)
