#include "base/apple/foundation_util.h"
#include "base/apple/string_conversions.h"
#include "gui/platform/cocoa/impl_cocoa.h"
#include "gui/platform/menu.h"

// This Objective-C class wraps a gui::Menu object, which allows it to be stored in the
// representedObject field of an NSMenuItem.
@interface PtrToMenuAsNSObject : NSObject
- (instancetype)initWithMenu:(gui::Menu*)menu;
+ (gui::Menu*)getFrom:(id)instance;
@end

namespace gui {

Menu::Menu() : pimpl{new Impl{}} {
    pimpl->ns_menu = [[NSMenu alloc] initWithTitle:@""];
    pimpl->menu_controller = [[MenuController alloc] init];
}

Menu::~Menu() {
    // [pimpl->ns_menu release];
    // [pimpl->menu_controller release];
}

void Menu::addItem(std::string_view label) {
    NSString* label_nsstring = base::apple::StringToNSString(label);
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:label_nsstring
                                                  action:@selector(itemSelected:)
                                           keyEquivalent:@""];
    item.target = pimpl->menu_controller;
    item.tag = 42;
    item.representedObject = [[PtrToMenuAsNSObject alloc] initWithMenu:this];
    [pimpl->ns_menu addItem:item];
}

std::optional<size_t> Menu::show(const Point& mouse_pos) const {
    NSPoint point = {static_cast<CGFloat>(mouse_pos.x), static_cast<CGFloat>(mouse_pos.y)};
    bool selected = [pimpl->ns_menu popUpMenuPositioningItem:nil atLocation:point inView:nullptr];
    if (selected) {
        return index;
    } else {
        return std::nullopt;
    }
}

void Menu::setSelectedIndex(size_t index) { this->index = index; }

}  // namespace gui

@implementation MenuController

- (void)itemSelected:(id)sender {
    NSMenuItem* item = base::apple::ObjCCastStrict<NSMenuItem>(sender);
    gui::Menu* menu = [PtrToMenuAsNSObject getFrom:item.representedObject];
    menu->setSelectedIndex(item.tag);
}

@end

@implementation PtrToMenuAsNSObject {
    gui::Menu* _menu;
}

- (instancetype)initWithMenu:(gui::Menu*)menu {
    if ((self = [super init])) {
        _menu = menu;
    }
    return self;
}

+ (gui::Menu*)getFrom:(id)instance {
    return base::apple::ObjCCastStrict<PtrToMenuAsNSObject>(instance)->_menu;
}

@end
