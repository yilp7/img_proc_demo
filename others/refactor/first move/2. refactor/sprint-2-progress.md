# Sprint 2 Progress: Leaf Module Extraction — MyWidget Breakup

## Task 2.1: Extract `Display` to `widgets/display.h/cpp`
Status: DONE

Moved Display class (declaration + 8 method implementations) from mywidget.h/cpp to display.h/cpp. display.h includes util.h.

## Task 2.2: Extract `TitleBar` to `widgets/titlebar.h/cpp`
Status: DONE

Moved InfoLabel, TitleButton, and TitleBar classes from mywidget.h/cpp to titlebar.h/cpp. titlebar.h includes util.h and forward-declares Preferences and ScanConfig (task 2.5). titlebar.cpp includes visual/preferences.h and visual/scanconfig.h for object creation in TitleBar::setup().

## Task 2.3: Extract `StatusBar` + `StatusIcon` to `widgets/statusbar.h/cpp`
Status: DONE

Moved StatusIcon and StatusBar classes from mywidget.h/cpp to statusbar.h/cpp. statusbar.h includes util.h.

## Task 2.4: Extract `FloatingWindow` to `widgets/floatingwindow.h/cpp`
Status: DONE

Moved FloatingWindow class from mywidget.h/cpp to floatingwindow.h/cpp. floatingwindow.h includes display.h (FloatingWindow owns a Display*).

## Task 2.5: Remove TitleBar → Preferences/ScanConfig header dependency
Status: DONE

titlebar.h uses forward declarations (`class Preferences; class ScanConfig;`) instead of full includes. Full includes moved to titlebar.cpp. mywidget.h no longer includes preferences.h or scanconfig.h — it includes display.h, titlebar.h, statusbar.h, floatingwindow.h instead.

## Task 2.6: Remove THREAD → WIDGETS dependency
Status: DONE

controlportthread.h: replaced `#include "widgets/mywidget.h"` with `#include "widgets/statusbar.h"` (for StatusIcon) and `class ScanConfig;` forward declaration (for TCUThread). Note: controlportthread.h/cpp are commented out in CMakeLists.txt (not part of active build).

## Task 2.7: Update CMakeLists.txt with new files
Status: DONE

Added 8 new files to SOURCE_FILES: display.h/cpp, titlebar.h/cpp, statusbar.h/cpp, floatingwindow.h/cpp. Kept mywidget.h/cpp.

## Approach

mywidget.h becomes a thin aggregate header that includes display.h, titlebar.h, statusbar.h, floatingwindow.h and retains the remaining small widget classes: Ruler, AnimationLabel, Tools, Coordinate, IndexLabel, MiscSelection. This avoids any changes to .ui files (user_panel.ui and preferences.ui reference widgets/mywidget.h for promoted widgets).

Classes extracted per file:
- display.h/cpp: Display
- titlebar.h/cpp: InfoLabel, TitleButton, TitleBar
- statusbar.h/cpp: StatusIcon, StatusBar
- floatingwindow.h/cpp: FloatingWindow

Remaining in mywidget.h/cpp: Ruler, AnimationLabel, Tools, Coordinate, IndexLabel, MiscSelection

Files created: display.h, display.cpp, titlebar.h, titlebar.cpp, statusbar.h, statusbar.cpp, floatingwindow.h, floatingwindow.cpp
Files modified: mywidget.h, mywidget.cpp, controlportthread.h, CMakeLists.txt, userpanel.h

Build fix: userpanel.h previously got widget types transitively through controlportthread.h → mywidget.h. After task 2.6 broke that chain, added explicit `#include "widgets/mywidget.h"` to userpanel.h.

## Build Verification
Status: PASS
