#include "app/cocoa/impl_cocoa.h"
#include "base/apple/foundation_util.h"
#include "menu.h"

#include "util/std_print.h"

@interface WeakPtrToMenuAsNSObject : NSObject
+ (instancetype)weakPtrForModel:(app::Menu*)model;
+ (app::Menu*)getFrom:(id)instance;
- (instancetype)initWithModel:(app::Menu*)model;
@end

namespace app {

Menu::Menu() : pimpl{new impl{}} {
    pimpl->ns_menu = [[NSMenu alloc] initWithTitle:@""];
    pimpl->menu_controller = [[MenuController alloc] init];
}

Menu::~Menu() {
    [pimpl->ns_menu release];
}

void Menu::addItem(ItemType type) {
    NSMenuItem* item = [[NSMenuItem alloc] initWithTitle:@"TODO: Change this"
                                                  action:@selector(itemSelected:)
                                           keyEquivalent:@""];
    item.target = pimpl->menu_controller;
    item.tag = 42;
    item.representedObject = [WeakPtrToMenuAsNSObject weakPtrForModel:this];
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

void Menu::setSelectedIndex(size_t index) {
    this->index = index;
}

}  // namespace app

@implementation MenuController

- (void)itemSelected:(id)sender {
    NSMenuItem* item = base::apple::ObjCCastStrict<NSMenuItem>(sender);
    app::Menu* menu = [WeakPtrToMenuAsNSObject getFrom:item.representedObject];
    menu->setSelectedIndex(item.tag);
}

@end

@implementation WeakPtrToMenuAsNSObject {
    app::Menu* _model;
}

+ (instancetype)weakPtrForModel:(app::Menu*)model {
    return [[WeakPtrToMenuAsNSObject alloc] initWithModel:model];
}

+ (app::Menu*)getFrom:(id)instance {
    return [base::apple::ObjCCastStrict<WeakPtrToMenuAsNSObject>(instance) menu];
}

- (instancetype)initWithModel:(app::Menu*)model {
    if ((self = [super init])) {
        _model = model;
    }
    return self;
}

- (app::Menu*)menu {
    return _model;
}

@end
