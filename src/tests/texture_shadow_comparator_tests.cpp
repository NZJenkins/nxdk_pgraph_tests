#include <pbkit/pbkit.h>

#include <utility>

#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "swizzle.h"
#include "test_host.h"
#include "texture_depth_source_tests.h"

#define SET_MASK(mask, val) (((val) << (__builtin_ffs(mask) - 1)) & (mask))

// NV097_SET_CONTEXT_DMA_ZETA is set to channel 10 by default.
const uint32_t kDefaultDMAZetaChannel = 10;

// Number of small box values to set in the depth texture.
static constexpr int kHorizontal = 15;
static constexpr int kVertical = 15;

static constexpr uint32_t kCompareFuncs[] = {
    NV097_SET_SHADOW_COMPARE_FUNC_NEVER,  NV097_SET_SHADOW_COMPARE_FUNC_GREATER, NV097_SET_SHADOW_COMPARE_FUNC_EQUAL,
    NV097_SET_SHADOW_COMPARE_FUNC_GEQUAL, NV097_SET_SHADOW_COMPARE_FUNC_LESS,    NV097_SET_SHADOW_COMPARE_FUNC_NOTEQUAL,
    NV097_SET_SHADOW_COMPARE_FUNC_LEQUAL, NV097_SET_SHADOW_COMPARE_FUNC_ALWAYS,
};

struct BoxLayoutInfo {
  uint32_t box_width;
  uint32_t box_height;
  uint32_t spacing;
  uint32_t first_box_left;
  uint32_t top;
};

static std::string MakeTestName(const TextureFormatInfo &format, uint32_t depth_format, uint32_t comp_func) {
  char buf[64] = {0};
  sprintf(buf, "%s%s_", format.name, format.xbox_linear ? "_L" : "");

  std::string ret = buf;
  if (depth_format == NV097_SET_SURFACE_FORMAT_ZETA_Z16) {
    ret += "Z16_";
  } else {
    ret += "Z24S8_";
  }

  switch (comp_func) {
    case NV097_SET_SHADOW_COMPARE_FUNC_NEVER:
      ret += "Never";
      break;
    case NV097_SET_SHADOW_COMPARE_FUNC_GREATER:
      ret += "GT";
      break;
    case NV097_SET_SHADOW_COMPARE_FUNC_EQUAL:
      ret += "EQ";
      break;
    case NV097_SET_SHADOW_COMPARE_FUNC_GEQUAL:
      ret += "GE";
      break;
    case NV097_SET_SHADOW_COMPARE_FUNC_LESS:
      ret += "LT";
      break;
    case NV097_SET_SHADOW_COMPARE_FUNC_NOTEQUAL:
      ret += "NE";
      break;
    case NV097_SET_SHADOW_COMPARE_FUNC_LEQUAL:
      ret += "LE";
      break;
    case NV097_SET_SHADOW_COMPARE_FUNC_ALWAYS:
      ret += "Always";
      break;
  }

  return std::move(ret);
}

TextureShadowComparatorTests::TextureShadowComparatorTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture shadow comparator") {
  for (auto comp_func : kCompareFuncs) {
    const TextureFormatInfo &texture_format =
        GetTextureFormatInfo(NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED);
    std::string name = MakeTestName(texture_format, NV097_SET_SURFACE_FORMAT_ZETA_Z16, comp_func);
    tests_[name] = [this, comp_func, name]() {
      Test(NV097_SET_SURFACE_FORMAT_ZETA_Z16, NV097_SET_TEXTURE_FORMAT_COLOR_LU_IMAGE_DEPTH_Y16_FIXED, comp_func, name);
    };
  }
}

void TextureShadowComparatorTests::Initialize() {
  TestSuite::Initialize();
  auto shader = std::make_shared<PrecalculatedVertexShader>(true);
  host_.SetVertexShaderProgram(shader);
}

static BoxLayoutInfo GetExplicitBoxLayout(uint32_t buffer_width, uint32_t buffer_height) {
  const uint32_t box_width = buffer_width / kHorizontal;
  const uint32_t box_height = buffer_height / kVertical;

  BoxLayoutInfo info;
  info.box_width = box_width / 2;
  info.box_height = box_height * 2;
  info.spacing = (box_width - info.box_width) * 2;

  info.first_box_left = (buffer_width / 2) - (info.box_width + info.spacing) * 3 + (info.box_width / 2);
  info.top = buffer_height / 2 - (info.box_width / 2);

  return std::move(info);
}

template <typename T>
static void PrepareTestTexture(uint8_t *memory, uint32_t width, uint32_t height, T default_val, float max_val) {
  static constexpr int kTotal = kHorizontal * kVertical;

  const uint32_t box_width = width / kHorizontal;
  const uint32_t box_height = height / kVertical;

  const uint32_t x_indent = width - (box_width * kHorizontal);
  const uint32_t y_indent = height - (box_height * kVertical);

  const uint32_t pitch = width * sizeof(T);
  auto buffer = reinterpret_cast<T *>(memory);
  for (auto i = 0; i < width * height; ++i) {
    buffer[i] = default_val;
  }

  auto row = buffer;

  float value = 0.0f;
  float val_inc = max_val / static_cast<float>(kTotal);
  auto row_data = new T[width];
  for (auto i = 0; i < width; ++i) {
    row_data[i] = default_val;
  }

  for (auto y = y_indent >> 1; y < kVertical; ++y) {
    auto box_pixel = row_data + (x_indent >> 1);
    for (auto x = 0; x < kHorizontal; ++x) {
      for (auto i = 0; i < box_width; ++i, ++box_pixel) {
        *box_pixel = static_cast<T>(value);
      }
      value += val_inc;
    }

    for (auto y_row = 0; y_row < box_height; ++y_row, row += width) {
      memcpy(row, row_data, pitch);
    }
  }

  delete[] row_data;

  // Place a few small boxes of known values near the middle.
  // Values:
  // {0, 1, default - 1, default, default + 1, max - 1, max}
  auto layout = GetExplicitBoxLayout(width, height);

  uint32_t left = layout.first_box_left;
  uint32_t top = layout.top;
  row = buffer + top * width;

  auto set_box = [&layout](T *row, uint32_t left, T value) {
    T *pixel = row + left;
    for (auto x = 0; x < layout.box_width; ++x) {
      *pixel++ = value;
    }
  };

  for (uint32_t y = top; y < top + layout.box_height; ++y, row += width) {
    auto x = left;
    set_box(row, x, 0);
    x += layout.box_width + layout.spacing;

    set_box(row, x, 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, default_val - 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, default_val);
    x += layout.box_width + layout.spacing;

    set_box(row, x, default_val + 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, static_cast<T>(max_val) - 1);
    x += layout.box_width + layout.spacing;

    set_box(row, x, static_cast<T>(max_val));
  }
}

void TextureShadowComparatorTests::Test(uint32_t depth_format, uint32_t texture_format, uint32_t shadow_comp_function,
                                        const std::string &name) {
  host_.PrepareDraw(0xFE112233);

  uint32_t default_value;
  switch (depth_format) {
    case NV097_SET_SURFACE_FORMAT_ZETA_Z16:
      default_value = 0x7FFF;
      PrepareTestTexture<uint16_t>(host_.GetTextureMemory(), host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                   default_value, 65535.0f);
      break;

    case NV097_SET_SURFACE_FORMAT_ZETA_Z24S8:
      default_value = 0x7FFFFF00;
      PrepareTestTexture<uint32_t>(host_.GetTextureMemory(), host_.GetFramebufferWidth(), host_.GetFramebufferHeight(),
                                   default_value, 16777215.0f);
      break;
  }

  // Render a quad using the zeta buffer as a shadow map applied to the diffuse color.

  // The texture map is used as a color source and will either be 0xFFFFFFFF or 0x00000000 for any given texel.
  host_.SetFinalCombiner0Just(TestHost::SRC_TEX0);
  host_.SetFinalCombiner1Just(TestHost::SRC_DIFFUSE, true);

  auto &stage = host_.GetTextureStage(0);
  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_SHADOW_COMPARE_FUNC, shadow_comp_function);
  pb_end(p);

  stage.SetFormat(GetTextureFormatInfo(texture_format));
  host_.SetShaderStageProgram(TestHost::STAGE_3D_PROJECTIVE);
  stage.SetFormat(GetTextureFormatInfo(texture_format));
  stage.SetTextureDimensions(1, 1, 1);
  stage.SetImageDimensions(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  host_.SetTextureStageEnabled(0, true);
  host_.SetupTextureStages();

  {
    const float kLeft = 0.0f;
    const float kRight = host_.GetFramebufferWidthF() - kLeft;
    const float kTop = 100.0f;
    const float kBottom = host_.GetFramebufferHeightF() - kTop;

    const auto tex_depth = static_cast<float>(default_value);
    const float z = 1.5f;
    host_.Begin(TestHost::PRIMITIVE_QUADS);
    host_.SetDiffuse(0xFF2277FF);
    host_.SetTexCoord0(0.0f, 0.0f, tex_depth, 1.0f);
    host_.SetVertex(kLeft, kTop, z, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), 0.0f, tex_depth, 1.0f);
    host_.SetVertex(kRight, kTop, z, 1.0f);

    host_.SetTexCoord0(host_.GetFramebufferWidthF(), host_.GetFramebufferHeightF(), tex_depth, 1.0f);
    host_.SetVertex(kRight, kBottom, z, 1.0f);

    host_.SetTexCoord0(0.0, host_.GetFramebufferHeightF(), tex_depth, 1.0f);
    host_.SetVertex(kLeft, kBottom, z, 1.0f);
    host_.End();
  }

  // Render tiny triangles in the center of each explicit value box to make them visible regardless of the comparison.
  {
    host_.SetFinalCombiner0Just(TestHost::SRC_DIFFUSE);
    host_.SetShaderStageProgram(TestHost::STAGE_NONE);
    host_.SetTextureStageEnabled(0, false);
    host_.SetupTextureStages();

    auto layout = GetExplicitBoxLayout(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
    static constexpr uint32_t kMarkerColors[] = {
        0xFFFFAA00, 0xFFFF7700, 0xFFCC5500, 0xFF00FF00, 0xFF00AACC, 0xFF005577, 0xFF002266,
    };
    const float left_offset = static_cast<float>(layout.box_width) / 4.0f;
    const float mid_offset = left_offset * 2.0f;
    const float right_offset = mid_offset + left_offset;
    float left = static_cast<float>(layout.first_box_left);

    const float top_offset = static_cast<float>(layout.box_height) / 4.0f;
    const float bottom_offset = top_offset * 2.0f;
    const float top = static_cast<float>(layout.top) + top_offset;
    const float bottom = static_cast<float>(layout.top) + bottom_offset;

    for (auto color : kMarkerColors) {
      host_.Begin(TestHost::PRIMITIVE_TRIANGLES);
      host_.SetDiffuse(color);
      host_.SetVertex(left + left_offset, bottom, 0.0f, 1.0f);

      host_.SetVertex(left + mid_offset, top, 0.0f, 1.0f);

      host_.SetVertex(left + right_offset, bottom, 0.0f, 1.0f);

      left += layout.spacing + layout.box_width;
      host_.End();
    }
  }

  pb_print("%s\n", name.c_str());
  pb_print("Ref, edges, center: 0x%X\n", default_value);
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, name);
}
