#include "widget.h"

namespace gui {

Widget::Widget(std::shared_ptr<renderer::Renderer> renderer) : renderer{std::move(renderer)} {}

}
