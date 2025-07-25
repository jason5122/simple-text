#pragma once

@class NSMenu;
@class AppMenuCocoaController;

class AppMenuBridge {
public:
    AppMenuBridge();
    ~AppMenuBridge();
    AppMenuBridge(const AppMenuBridge&) = delete;
    AppMenuBridge& operator=(const AppMenuBridge&) = delete;

    NSMenu* menu() const;

private:
    AppMenuCocoaController* __strong controller;
    NSMenu* __strong menu_root;
};
