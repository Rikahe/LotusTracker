cmake_minimum_required(VERSION 3.10)
project(LotusTracker
	VERSION 1.3.1
)

set(CMAKE_CXX_STANDARD 11)

find_package(Qt5 COMPONENTS Core Gui Network Widgets REQUIRED)
include_directories(
	${Qt5_INCLUDE_DIRS}
	${Qt5Core_INCLUDE_DIRS}
	${Qt5Gui_INCLUDE_DIRS}
	${Qt5Network_INCLUDE_DIRS}
	${Qt5Widgets_INCLUDE_DIRS}
)


set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

find_package(Qt5Keychain REQUIRED)
include_directories(${QTKEYCHAIN_INCLUDE_DIRS})


include_directories(include)

configure_file (
    "${PROJECT_SOURCE_DIR}/src/config.h.in"
    "${PROJECT_BINARY_DIR}/config.h"
)

###############################################################################
set(HEADERS 
	src/api/lotusapi.h
	src/api/requestdata.h
	src/api/rqtparsedeckposside.h
	src/api/rqtplayerdeck.h
	src/api/rqtplayerdeckpublish.h
	src/api/rqtplayerdeckupdate.h
	src/api/rqtupdateplayerinventory.h
	src/api/rqtupdateplayercollection.h
	src/api/rqtuploadmatch.h
	src/api/rqtuploadeventresult.h
	src/api/rqtuploadplayermatch.h
	src/api/untappedapi.h
	src/entity/card.h
	src/entity/deck.h
	src/entity/deckarch.h
	src/entity/user.h
	src/entity/matchinfo.h
	src/entity/matchplayer.h
	src/entity/matchstatediff.h
	src/entity/matchzone.h
	src/entity/matchzonetransfer.h
	src/entity/opponentinfo.h
	src/mtg/mtgarena.h
	src/mtg/mtgalogparser.h
	src/mtg/mtgalogwatcher.h
	src/mtg/mtgamatch.h
	src/mtg/mtgcards.h
	src/mtg/mtgdecksarch.h
	src/ui/cardblinkinfo.h
	src/ui/deckoverlaybase.h
	src/ui/deckoverlaydraft.h
	src/ui/deckoverlayopponent.h
	src/ui/deckoverlayplayer.h
	src/ui/preferencesscreen.h
	src/ui/startscreen.h
	src/ui/tababout.h
	src/ui/tabgeneral.h
	src/ui/tablogs.h
	src/ui/taboverlay.h
	src/ui/trayicon.h
	src/utils/appsecure.h
	src/utils/appsettings.h
	src/utils/influxdb.hpp
	src/utils/logger.h
	src/utils/lotusexception.h
	src/utils/metrics.h
	src/utils/linuxwindowfinder.h
	src/updater/sparkleupdater.h
	src/credentials.h
	src/ganalytics.h
	src/lotustracker.h
	src/macros.h
	src/transformations.h
	src/urls.h
)

set(SOURCES 
	src/api/lotusapi.cpp
	src/api/untappedapi.cpp
	src/main.cpp
	src/ganalytics.cpp
	src/lotustracker.cpp
	src/transformations.cpp
	src/entity/deck.cpp
	src/entity/matchinfo.cpp
	src/mtg/mtgarena.cpp
	src/mtg/mtgalogparser.cpp
	src/mtg/mtgalogwatcher.cpp
	src/mtg/mtgamatch.cpp
	src/mtg/mtgcards.cpp
	src/mtg/mtgdecksarch.cpp
	src/ui/deckoverlaybase.cpp
	src/ui/deckoverlaydraft.cpp
	src/ui/deckoverlayopponent.cpp
	src/ui/deckoverlayplayer.cpp
	src/ui/preferencesscreen.cpp
	src/ui/startscreen.cpp
	src/ui/tababout.cpp
	src/ui/tabgeneral.cpp
	src/ui/tablogs.cpp
	src/ui/taboverlay.cpp
	src/ui/trayicon.cpp
	src/utils/appsecure.cpp
	src/utils/appsettings.cpp
	src/utils/logger.cpp
	src/utils/metrics.cpp
	src/utils/linuxwindowfinder.cpp
) 

set (UIS
	src/ui/decktrackerbase.ui
	src/ui/preferences.ui
	src/ui/start.ui
	src/ui/tababout.ui
	src/ui/tabgeneral.ui
	src/ui/tablogs.ui
	src/ui/taboverlay.ui
)

set (RESOURCES
	res/resources.qrc
)

add_executable(LotusTracker
    ${HEADERS}
	${SOURCES}
	${UIS}
	${RESOURCES}
)

target_link_libraries(LotusTracker 
	${Qt5Core_LIBRARIES}
	${Qt5Gui_LIBRARIES}
	${Qt5Network_LIBRARIES}
	${Qt5Widgets_LIBRARIES}

	${QTKEYCHAIN_LIBRARIES}
)