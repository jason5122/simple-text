#include "gui/platform/cocoa/menu/app_menu_bridge.h"
#include "gui/platform/cocoa/menu/app_menu_cocoa_controller.h"

AppMenuBridge::AppMenuBridge()
    : controller([[AppMenuCocoaController alloc] initWithBridge:this]),
      menu_root([[NSMenu alloc] init]) {
    menu_root.delegate = controller;
}

AppMenuBridge::~AppMenuBridge() { menu_root.delegate = nil; }

NSMenu* AppMenuBridge::menu() const { return menu_root; }
