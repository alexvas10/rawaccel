set(GUI_SOURCES
    main.cpp
    MainWindow.cpp
    AccelGUI.cpp
    Calculator.cpp
    SettingsManager.cpp
    IpcClient.cpp
    AccelTypePanel.cpp
    Options/OptionWidget.cpp
    Options/LockableOption.cpp
    Options/LutEditor.cpp
    Charts/ChartXY.cpp
    Charts/AccelCharts.cpp
)

set(GUI_HEADERS
    MainWindow.hpp
    AccelGUI.hpp
    Calculator.hpp
    SettingsManager.hpp
    IpcClient.hpp
    AccelTypePanel.hpp
    Options/OptionWidget.hpp
    Options/LockableOption.hpp
    Options/LutEditor.hpp
    Layouts/LayoutBase.hpp
    Charts/ChartXY.hpp
    Charts/AccelCharts.hpp
)

qt6_wrap_cpp(GUI_MOC_SOURCES ${GUI_HEADERS})

add_executable(rawaccel-gui ${GUI_SOURCES} ${GUI_MOC_SOURCES})

target_include_directories(rawaccel-gui PRIVATE
    ${RAWACCEL_COMMON_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/..        # linux/ — allows #include "shared/compat.hpp"
    ${CMAKE_CURRENT_SOURCE_DIR}/../shared # allows #include "compat.hpp" directly
    ${CMAKE_CURRENT_SOURCE_DIR}           # gui/ — allows #include "AccelGUI.hpp" etc.
)

target_link_libraries(rawaccel-gui PRIVATE
    Qt6::Widgets
    Qt6::Charts
    Qt6::Network
    nlohmann_json::nlohmann_json
    pthread
)

target_compile_options(rawaccel-gui PRIVATE
    -Wall -Wextra -Wno-unused-parameter -Wno-changes-meaning
)

set_target_properties(rawaccel-gui PROPERTIES
    AUTOMOC OFF
)

install(TARGETS rawaccel-gui DESTINATION /usr/local/bin)
