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

TextEditWidget* EditorWidget::current_widget() const {
    return multi_view->current_widget();
}

void EditorWidget::set_index(size_t index) {
    multi_view->set_index(index);
    tab_bar->set_index(index);
}

void EditorWidget::prev_index() {
    multi_view->prev_index();
    tab_bar->prev_index();
}

void EditorWidget::next_index() {
    multi_view->next_index();
    tab_bar->next_index();
}

void EditorWidget::last_index() {
    multi_view->last_index();
    tab_bar->last_index();
}

size_t EditorWidget::get_current_index() {
    return multi_view->index();
}

void EditorWidget::add_tab(std::string_view tab_name, std::string_view text) {
    multi_view->addTab(std::make_unique<TextEditWidget>(text, main_font_id));
    tab_bar->add_tab(tab_name);
    layout();
}

void EditorWidget::remove_tab(size_t index) {
    multi_view->removeTab(index);
    tab_bar->remove_tab(index);
    layout();
}

void EditorWidget::open_file(std::string_view path) {
    std::string contents = base::ReadFile(path);
    add_tab(path, contents);
}

// TODO: Refactor this.
void EditorWidget::update_font(size_t font_id) {
    auto& line_layout_cache = Renderer::instance().getLineLayoutCache();
    line_layout_cache.clear();

    main_font_id = font_id;

    for (size_t i = 0; i < multi_view->count(); ++i) {
        auto* text_view = multi_view->at(i);
        text_view->update_font_id(font_id);
    }
}

}  // namespace gui
