cmake_minimum_required(VERSION 3.17)

project(DSRemote C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt5 COMPONENTS Core Widgets Network Gui REQUIRED)

#add_compile_options(-Wextra -Wshadow -Wformat-nonliteral -Wformat-security -Wtype-limits -Wfatal-errors)

add_executable(dsremote
        main.cpp
        about_dialog.cpp
        utils.c
        connection.cpp
        tmc_dev.c
        tmc_lan.c
        tled.cpp
        edflib.c
        signalcurve.cpp
        settings_dialog.cpp
        screen_thread.cpp
        lan_connect_thread.cpp
        read_settings_thread.cpp
        save_data_thread.cpp
        decode_dialog.cpp
        tdial.cpp
        wave_dialog.cpp
        wave_view.cpp
        playback_dialog.cpp

        mainwindow.cpp


        third_party/kiss_fft/kiss_fft.c
        third_party/kiss_fft/kiss_fftr.c

        images.qrc
        )

target_link_libraries(dsremote Qt5::Core Qt5::Widgets Qt5::Network Qt5::Gui pthread bsd)

install(TARGETS dsremote RUNTIME DESTINATION bin)

install(FILES images/r_dsremote.png DESTINATION share/icons)
install(FILES images/r_dsremote.png DESTINATION share/pixmaps)
install(FILES images/r_dsremote.png DESTINATION share/icons/hicolor/48x48/apps)
install(FILES images/r_dsremote.png DESTINATION share/icons/hicolor/48x48/mimetypes)
install(FILES install/dsremote.desktop DESTINATION share/applications)
install(FILES install/30-usbtmc.rules DESTINATION /etc/udev/rules.d)
