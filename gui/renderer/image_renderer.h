#pragma once

#include "app/types.h"
#include "gui/renderer/atlas.h"
#include "gui/renderer/opengl_types.h"
#include "gui/renderer/shader.h"
#include "util/non_copyable.h"
#include <vector>

#include <filesystem>
namespace fs = std::filesystem;

namespace gui {

class ImageRenderer : util::NonCopyable {
public:
    static constexpr size_t kPanelClose2xIndex = 0;
    static constexpr size_t kFolderOpen2xIndex = 1;
    static constexpr size_t kStanfordBunny = 2;

    ImageRenderer();
    ~ImageRenderer();
    ImageRenderer(ImageRenderer&& other);
    ImageRenderer& operator=(ImageRenderer&& other);

    app::Size getImageSize(size_t image_index);
    void addImage(size_t image_index, const app::Point& coords, const Rgba& color);
    void flush(const app::Size& screen_size);

private:
    static constexpr size_t kBatchMax = 0x10000;

    Shader shader_program;
    GLuint vao = 0;
    GLuint vbo_instance = 0;
    GLuint ebo = 0;

    Atlas atlas;
    struct AtlasImage {
        Vec2 rect_size;
        Vec4 uv;
    };
    std::vector<AtlasImage> image_atlas_entries;

    bool loadPng(size_t index, fs::path file_name);

    struct InstanceData {
        Vec2 coords;
        Vec2 rect_size;
        Vec4 uv;
        Rgba color;
    };
    std::vector<InstanceData> instances;
};

static_assert(!std::is_copy_constructible_v<ImageRenderer>);
static_assert(!std::is_trivially_copy_constructible_v<ImageRenderer>);

}  // namespace gui
