cmake_minimum_required(VERSION 3.28...3.30)

option(SESSION_CREATE_ONCE "Create one FalconM SDK session per Source lifetime" OFF)

set(FALCONM_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/media-sdk/arm64")
file(GLOB_RECURSE FALCONM_SDK_HEADERS CONFIGURE_DEPENDS
  "${FALCONM_SDK_ROOT}/include/*.h"
)

add_library(xbotogo-falconM MODULE
  xbotogo-falconM.cpp
  falconm-source.cpp
  falconm-stream.cpp
  falconm-angle-state.hpp
  falconm-time.cpp
  falconm-log.hpp
  falconm.hpp
  falconm-stream.hpp
  falconm-time.hpp
  falconm-protocol.hpp
  protocol/falcon-request.hpp
  protocol/falcon-calibration.hpp
  protocol/falcon-camera-requests.hpp
  protocol/falcon-mode-requests.hpp
  protocol/falcon-time-requests.hpp
  protocol/falcon-protocol-parser.hpp
  protocol/falcon-event.hpp
  protocol/falcon-events.hpp
  ${FALCONM_SDK_HEADERS}
)

set(FALCONM_SDK_LIBRARY "${FALCONM_SDK_ROOT}/lib/libmedia_sdk.1.0.0.dylib")

target_include_directories(xbotogo-falconM SYSTEM PRIVATE "${FALCONM_SDK_ROOT}/include")
target_link_libraries(xbotogo-falconM PRIVATE "${FALCONM_SDK_LIBRARY}")
set_property(TARGET xbotogo-falconM PROPERTY BUILD_RPATH "@loader_path/../Frameworks")
target_link_options(xbotogo-falconM PRIVATE "LINKER:-rpath,@loader_path/../Frameworks")

if(SESSION_CREATE_ONCE)
  target_compile_definitions(xbotogo-falconM PRIVATE SESSION_CREATE_ONCE)
endif()

add_custom_command(
  TARGET xbotogo-falconM
  POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:xbotogo-falconM>/../Frameworks"
  COMMAND "${CMAKE_COMMAND}" -E copy_if_different
          "${FALCONM_SDK_LIBRARY}"
          "$<TARGET_FILE_DIR:xbotogo-falconM>/../Frameworks/libmedia_sdk.1.0.0.dylib"
  VERBATIM
)

target_link_libraries(xbotogo-falconM PRIVATE
  OBS::libobs
  "-framework CoreVideo"
  "-framework CoreFoundation"
)

if(ENABLE_FRONTEND)
  find_package(Qt6 REQUIRED Widgets)
  target_compile_definitions(xbotogo-falconM PRIVATE XBOTGO_DEVICE_DISCOVERY XBOTGO_FRONTEND_API)
  target_link_libraries(xbotogo-falconM PRIVATE OBS::frontend-api OBS::xbotgo-device-discovery Qt::Widgets)
endif()

set_target_properties_obs(xbotogo-falconM PROPERTIES FOLDER plugins)

if(ENABLE_UNIT_TESTS)
  add_executable(falconm-protocol-test
    falconm-protocol-test.cpp
    falconm-protocol.hpp
    falconm-time.cpp
    falconm-time.hpp
  )
  target_include_directories(falconm-protocol-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(falconm-protocol-test PRIVATE "-framework CoreFoundation")
  add_test(NAME falconm-protocol-test COMMAND falconm-protocol-test)
endif()
