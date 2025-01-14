#include "editor_widget.h"

#include "base/files/file_reader.h"
#include "gui/renderer/renderer.h"
#include "gui/widget/padding_widget.h"

namespace gui {

EditorWidget::EditorWidget(size_t main_font_id, size_t ui_font_size, size_t panel_close_image_id)
    : main_font_id(main_font_id),
      multi_view(new MultiViewWidget<TextEditWidget>()),
      tab_bar(new TabBarWidget(ui_font_size, kTabBarHeight, panel_close_image_id)) {
    // Leave padding between window title bar and tab.
    auto tab_bar_padding = std::make_unique<PaddingWidget>(Size{0, 3 * 2}, kTabBarColor);
    auto text_view_padding = std::make_unique<PaddingWidget>(Size{0, 2 * 2}, kTextViewColor);

    multi_view->addTab(std::make_unique<TextEditWidget>("", main_font_id));

    addChildStart(std::move(tab_bar_padding));
    addChildStart(std::unique_ptr<TabBarWidget>(tab_bar));
    addChildStart(std::move(text_view_padding));
    setMainWidget(std::unique_ptr<MultiViewWidget<TextEditWidget>>(multi_view));
}

TextEditWidget* EditorWidget::currentWidget() const {
    return multi_view->currentWidget();
}

void EditorWidget::setIndex(size_t index) {
    multi_view->setIndex(index);
    tab_bar->setIndex(index);
}

void EditorWidget::prevIndex() {
    multi_view->prevIndex();
    tab_bar->prevIndex();
}

void EditorWidget::nextIndex() {
    multi_view->nextIndex();
    tab_bar->nextIndex();
}

void EditorWidget::lastIndex() {
    multi_view->lastIndex();
    tab_bar->lastIndex();
}

size_t EditorWidget::getCurrentIndex() {
    return multi_view->getCurrentIndex();
}

void EditorWidget::addTab(std::string_view tab_name, std::string_view text) {
    multi_view->addTab(std::make_unique<TextEditWidget>(text, main_font_id));
    tab_bar->addTab(tab_name);
    layout();
}

void EditorWidget::removeTab(size_t index) {
    multi_view->removeTab(index);
    tab_bar->removeTab(index);
    layout();
}

void EditorWidget::openFile(std::string_view path) {
    std::string contents = base::ReadFile(path);
    addTab(path, contents);
}

// TODO: Refactor this.
void EditorWidget::updateFontId(size_t font_id) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    line_layout_cache.clear();

    main_font_id = font_id;

    for (size_t i = 0; i < multi_view->widgetCount(); ++i) {
        auto* text_view = multi_view->at(i);
        text_view->updateFontId(font_id);
    }
}

}  // namespace gui
