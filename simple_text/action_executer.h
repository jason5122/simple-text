#pragma once

#include "config/key_bindings.h"
#include "simple_text/editor_window.h"
#include "simple_text/simple_text.h"

inline void ExecuteAction(config::Action& action, SimpleText& app, EditorWindow& window) {
    using config::Action;

    switch (action) {
    case Action::kNone:
        break;

    case Action::kNewWindow:
        app.createWindow();
        break;

    case Action::kCloseWindow:
        window.close();
        break;

    case Action::kNewTab:
        window.createTab(fs::path{});
        window.redraw();
        break;

    case Action::kCloseTab:
        window.closeCurrentTab();
        break;

    case Action::kPreviousTab:
        window.selectPreviousTab();
        break;

    case Action::kNextTab:
        window.selectNextTab();
        break;

    case Action::kSelectTab1:
        window.selectTabIndex(0);
        break;
    case Action::kSelectTab2:
        window.selectTabIndex(1);
        break;
    case Action::kSelectTab3:
        window.selectTabIndex(2);
        break;
    case Action::kSelectTab4:
        window.selectTabIndex(3);
        break;
    case Action::kSelectTab5:
        window.selectTabIndex(4);
        break;
    case Action::kSelectTab6:
        window.selectTabIndex(5);
        break;
    case Action::kSelectTab7:
        window.selectTabIndex(6);
        break;
    case Action::kSelectTab8: {
        window.selectTabIndex(7);
        break;
    }

    case Action::kSelectLastTab:
        window.selectLastTab();
        break;

    case Action::kToggleSideBar:
        window.toggleSideBar();
        break;
    }
}

inline void ExecuteGuiAction(gui::Action action, EditorTab* tab) {
    using gui::Action;

    switch (action) {
    case Action::kNone:
        break;

    case Action::kMoveForwardByCharacter:
        tab->moveCaret(EditorTab::MovementType::kCharacters, true, false);
        break;

    case Action::kMoveBackwardByCharacter:
        tab->moveCaret(EditorTab::MovementType::kCharacters, false, false);
        break;

    case Action::kLeftDelete:
        tab->backspace();
        break;
    }
}
