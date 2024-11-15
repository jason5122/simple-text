#include "highlighter.h"

#include "third_party/rapidhash/rapidhash.h"
#include <wasm.h>

namespace highlight {

struct Highlighter::impl {
    impl() : engine{wasm_engine_new()} {}
    ~impl() {
        wasm_engine_delete(engine);
    }

    wasm_engine_t* engine;
    std::unordered_map<uint64_t, Language> languages;
};

Highlighter& Highlighter::instance() {
    static Highlighter highlighter;
    return highlighter;
}

const Language& Highlighter::getLanguage(std::string_view name) {
    uint64_t hash = rapidhash(name.data(), name.length());
    auto& languages = pimpl->languages;
    if (auto it = languages.find(hash); it != languages.end()) {
        return it->second;
    } else {
        Language language{pimpl->engine, name};
        language.load();
        auto inserted = languages.emplace(hash, std::move(language));
        return inserted.first->second;
    }
}

Highlighter::Highlighter() : pimpl{new impl{}} {}

}  // namespace highlight
