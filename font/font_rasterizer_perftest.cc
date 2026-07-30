#include "base/rand_util.h"
#include "build/build_config.h"
#include "font/font_rasterizer.h"
#include <gtest/gtest.h>

#if BUILDFLAG(IS_WIN)
#include "base/win/scoped_com_initializer.h"
#include <memory>
#endif

namespace font {

// FontRasterizer::instance() constructs WIC/Direct2D/DirectWrite factories,
// which require COM to be initialized on the calling thread. Bracket the suite
// so the apartment is up for exactly these tests and no others.
class FontRasterizerTest : public ::testing::Test {
#if BUILDFLAG(IS_WIN)
protected:
    static void SetUpTestSuite() {
        com_ = std::make_unique<base::win::ScopedCOMInitializer>();
    }
    static void TearDownTestSuite() { com_.reset(); }

    static std::unique_ptr<base::win::ScopedCOMInitializer> com_;
#endif
};

#if BUILDFLAG(IS_WIN)
std::unique_ptr<base::win::ScopedCOMInitializer> FontRasterizerTest::com_;
#endif

// We should move the rasterized bitmap data *directly* into OpenGL. Manually creating the buffer
// via rearranging pixels will be too slow.
TEST_F(FontRasterizerTest, RasterizePerformance) {
    auto& rasterizer = FontRasterizer::instance();
    size_t font_id = rasterizer.add_system_font(32);

    auto layout = rasterizer.layout_line(font_id, "a");
    uint32_t glyph_id = layout.glyphs[0].glyph_id;

    for (int i = 0; i < 10000; ++i) {
        auto rglyph = rasterizer.rasterize(font_id, glyph_id);
        EXPECT_GT(rglyph.width, 0);
        EXPECT_GT(rglyph.height, 0);
    }
}

// TODO: Find a way to randomly generate valid UTF-8 and re-enable.
// TEST(FontRasterizerTest, LineLayoutPerformance) {
//     auto& rasterizer = FontRasterizer::instance();
//     size_t font_id = rasterizer.add_system_font(32);

//     for (int i = 0; i < 1000; ++i) {
//         std::string str = base::rand_bytes_as_string(100);  // TODO: Use valid UTF-8.
//         auto layout = rasterizer.layout_line(font_id, str);
//     }
// }

}  // namespace font
