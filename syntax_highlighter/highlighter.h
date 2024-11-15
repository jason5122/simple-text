#pragma once

#include "syntax_highlighter/language.h"
#include <memory>
#include <string_view>

namespace highlight {

class Highlighter {
public:
    static Highlighter& instance();
    Highlighter(const Highlighter&) = delete;
    Highlighter& operator=(const Highlighter&) = delete;

    const Language& getLanguage(std::string_view name);

private:
    Highlighter();
    ~Highlighter() = default;

    struct impl;
    std::unique_ptr<impl> pimpl;
};

}  // namespace highlight
