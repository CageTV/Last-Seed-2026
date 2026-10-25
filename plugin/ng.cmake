# LastSeed.dll on alandtse's CommonLibSSE-NG; included by E:/WorkSpace/ng-build/CMakeLists.txt (see ng_plugin there)
ng_plugin(TARGET LastSeed NAME LastSeed VERSION 1.2.0 ROOT "${CMAKE_CURRENT_LIST_DIR}"
    SOURCES src/main.cpp src/Settings.cpp src/Game.cpp src/Hud.cpp src/Menu.cpp src/AutoStart.cpp src/Spoilage.cpp src/Freshness.cpp src/GridInv.cpp
            src/NativeMcm.cpp src/Hotkeys.cpp src/NativeFoodLists.cpp src/Compat.cpp src/DiseaseWidget.cpp src/Ids.cpp src/Classifier.cpp src/FoodAuto.cpp
    INCLUDES "${CMAKE_CURRENT_LIST_DIR}/src" "${CMAKE_CURRENT_LIST_DIR}/include"
    PCH "${CMAKE_CURRENT_LIST_DIR}/src/PCH.h")
