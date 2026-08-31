cmake_minimum_required(VERSION 3.28...3.30)

option(SESSION_CREATE_ONCE "Create one FalconM SDK session per Source lifetime" OFF)

set(FALCONM_SDK_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/vendor/media-sdk/arm64")
file(GLOB_RECURSE FALCONM_SDK_HEADERS CONFIGURE_DEPENDS "${FALCONM_SDK_ROOT}/include/*.h")

add_library(
  xbotogo-falconM
  MODULE
  xbotogo-falconM.cpp
  falconm-source.cpp
  falconm-source-bridge.cpp
  falconm-source-bridge.hpp
  falconm-stream.cpp
  falconm-angle-state.hpp
  falconm-time.cpp
  xblog.hpp
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
target_include_directories(xbotogo-falconM PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
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
  COMMAND
    "${CMAKE_COMMAND}" -E copy_if_different "${FALCONM_SDK_LIBRARY}"
    "$<TARGET_FILE_DIR:xbotogo-falconM>/../Frameworks/libmedia_sdk.1.0.0.dylib"
  VERBATIM
)

target_link_libraries(xbotogo-falconM PRIVATE OBS::libobs "-framework CoreVideo" "-framework CoreFoundation")

if(ENABLE_FRONTEND)
  find_package(CURL REQUIRED)
  find_package(Qt6 REQUIRED Core Network Widgets)
  target_sources(
    xbotogo-falconM
    PRIVATE
      device-search/device-info.hpp
      device-search/device-search-dialog.cpp
      device-search/device-search-dialog.hpp
      director/auto-director.cpp
      director/auto-director.hpp
      director/auto-director-policy.hpp
      live/curl-request-worker.cpp
      live/curl-request-worker.hpp
      live/live-stream-config.hpp
      live/live-stream-config-dialog.cpp
      live/live-stream-config-dialog.hpp
      live/live-stream-parser.cpp
      live/live-stream-parser.hpp
      live/live-stream-runtime.cpp
      live/live-stream-runtime.hpp
      live/live-stream-session.cpp
      live/live-stream-session.hpp
      live/live-task-client.cpp
      live/live-task-client.hpp
      runtime/xbotgo-translation.hpp
      runtime/xbotgo-plugin-runtime.cpp
      runtime/xbotgo-plugin-runtime.hpp
      scenes/camera-role.hpp
      scenes/camera-role-scenes.cpp
      scenes/camera-role-scenes.hpp
      ui/auto-director-control-widget.cpp
      ui/auto-director-control-widget.hpp
      ui/falconm-control-widget.cpp
      ui/falconm-control-widget.hpp
      ui/falconm-devices-widget.cpp
      ui/falconm-devices-widget.hpp
      ui/xbotgo-combo-box-control.cpp
      ui/xbotgo-combo-box-control.hpp
      ui/xbotgo-slider-control.cpp
      ui/xbotgo-slider-control.hpp
  )
  target_link_libraries(
    xbotogo-falconM
    PRIVATE CURL::libcurl OBS::frontend-api OBS::idian Qt::Core Qt::Network Qt::Widgets
  )
  set_property(TARGET xbotogo-falconM PROPERTY AUTOMOC ON)
endif()

set_target_properties_obs(xbotogo-falconM PROPERTIES FOLDER plugins)

if(ENABLE_UNIT_TESTS)
  find_package(CURL REQUIRED)
  find_package(Qt6 REQUIRED Core Network)

  add_executable(
    falconm-protocol-test
    falconm-protocol-test.cpp
    falconm-protocol.hpp
    falconm-time.cpp
    falconm-time.hpp
    device-search/device-search-dialog.hpp
  )
  target_include_directories(falconm-protocol-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}")
  target_link_libraries(falconm-protocol-test PRIVATE "-framework CoreFoundation")
  add_test(NAME falconm-protocol-test COMMAND falconm-protocol-test)

  add_executable(
    xbotgo-live-stream-test
    live/live-stream-parser.cpp
    live/live-stream-session.cpp
    live/live-stream-test.cpp
  )
  target_include_directories(xbotgo-live-stream-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/live")
  target_link_libraries(xbotgo-live-stream-test PRIVATE Qt::Core Qt::Network)
  add_test(NAME xbotgo-live-stream-test COMMAND xbotgo-live-stream-test)
  get_filename_component(XBOTGO_QT_FRAMEWORK_DIR "${Qt6_DIR}/../.." ABSOLUTE)
  set_tests_properties(xbotgo-live-stream-test PROPERTIES ENVIRONMENT "DYLD_FRAMEWORK_PATH=${XBOTGO_QT_FRAMEWORK_DIR}")

  add_executable(
    xbotgo-curl-request-worker-test
    live/curl-request-worker.cpp
    live/curl-request-worker.hpp
    live/curl-request-worker-test.cpp
  )
  target_include_directories(xbotgo-curl-request-worker-test PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/live")
  target_link_libraries(xbotgo-curl-request-worker-test PRIVATE CURL::libcurl Qt::Core Qt::Network)
  add_test(NAME xbotgo-curl-request-worker-test COMMAND xbotgo-curl-request-worker-test)
  set_tests_properties(
    xbotgo-curl-request-worker-test
    PROPERTIES ENVIRONMENT "DYLD_FRAMEWORK_PATH=${XBOTGO_QT_FRAMEWORK_DIR}"
  )

  add_executable(
    xbotgo-auto-director-policy-test
    director/auto-director-policy-test.cpp
    director/auto-director-policy.hpp
    scenes/camera-role.hpp
  )
  add_test(NAME xbotgo-auto-director-policy-test COMMAND xbotgo-auto-director-policy-test)
endif()
