set(DAEMON_SOURCES
    main.cpp
    SettingsLoader.cpp
    VirtualMouse.cpp
    MouseDevice.cpp
    DeviceManager.cpp
    IpcServer.cpp
)

add_executable(rawacceld ${DAEMON_SOURCES})

target_include_directories(rawacceld PRIVATE
    ${RAWACCEL_COMMON_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..        # linux/ — allows #include "shared/compat.hpp"
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared # allows #include "compat.hpp" directly
    ${LIBEVDEV_INCLUDE_DIRS}
)

target_link_libraries(rawacceld PRIVATE
    ${LIBEVDEV_LIBRARIES}
    nlohmann_json::nlohmann_json
    pthread
)

target_compile_options(rawacceld PRIVATE
    ${LIBEVDEV_CFLAGS_OTHER}
    -Wall -Wextra -Wno-unused-parameter -Wno-changes-meaning
)

install(TARGETS rawacceld DESTINATION /usr/local/bin)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/../install/rawaccel.service
        DESTINATION /etc/systemd/system)
