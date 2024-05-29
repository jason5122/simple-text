#pragma once

#include <memory>
#include <pango/pangocairo.h>

namespace font {

template <typename T, auto fn> struct Deleter {
    void operator()(T* ptr) {
        fn(ptr);
    }
};

template <typename T> using GObjectPtr = std::unique_ptr<T, Deleter<T, g_object_unref>>;
template <typename T, auto fn> using UniquePtrDeleter = std::unique_ptr<T, Deleter<T, fn>>;

using PangoFontDescriptionPtr =
    UniquePtrDeleter<PangoFontDescription, pango_font_description_free>;
using PangoFontMetricsPtr = UniquePtrDeleter<PangoFontMetrics, pango_font_metrics_unref>;
using CairoSurfacePtr = UniquePtrDeleter<cairo_surface_t, cairo_surface_destroy>;
using CairoContextPtr = UniquePtrDeleter<cairo_t, cairo_destroy>;

}
