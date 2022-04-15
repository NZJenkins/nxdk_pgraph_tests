#include "texture_signed_component_tests.h"

#include <SDL.h>
#include <pbkit/pbkit.h>

#include <memory>
#include <utility>

#include "debug_output.h"
#include "shaders/perspective_vertex_shader.h"
#include "shaders/pixel_shader_program.h"
#include "test_host.h"
#include "texture_format.h"
#include "vertex_buffer.h"

static int GenerateTestSurface(SDL_Surface **test_surface, int width, int height);

TextureSignedComponentTests::TextureSignedComponentTests(TestHost &host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Texture signed component tests") {

  auto add_test = [this](uint32_t texture_format, uint32_t signed_flags) {
    const TextureFormatInfo &texture_format_info = GetTextureFormatInfo(texture_format);
    std::string name = MakeTestName(texture_format_info, signed_flags);
    tests_[name] = [this, texture_format_info, signed_flags, name]() {
      Test(texture_format_info, signed_flags, name);
    };
  };

  for (auto i = 0; i <= 0x0F; ++i) {
    add_test(NV097_SET_TEXTURE_FORMAT_COLOR_SZ_A8R8G8B8, i);
  }
}

void TextureSignedComponentTests::Initialize() {
  TestSuite::Initialize();
  CreateGeometry();

  auto shader = std::make_shared<PerspectiveVertexShader>(host_.GetFramebufferWidth(), host_.GetFramebufferHeight());
  shader->SetLightingEnabled(false);
  host_.SetVertexShaderProgram(shader);
  host_.SetTextureStageEnabled(0, true);
  host_.SetShaderStageProgram(TestHost::STAGE_2D_PROJECTIVE);

  PixelShaderProgram::LoadTexturedPixelShader();
}

void TextureSignedComponentTests::CreateGeometry() {
  std::shared_ptr<VertexBuffer> buffer = host_.AllocateVertexBuffer(6);
  buffer->DefineBiTri(0, -0.75, 0.75, 0.75, -0.75, 0.1f);
  buffer->Linearize(static_cast<float>(host_.GetMaxTextureWidth()), static_cast<float>(host_.GetMaxTextureHeight()));
}

void TextureSignedComponentTests::Test(const TextureFormatInfo &texture_format, uint32_t signed_flags, const std::string &test_name) {
  host_.SetTextureFormat(texture_format);

  SDL_Surface *test_surface;
  int update_texture_result =
      GenerateTestSurface(&test_surface, (int)host_.GetMaxTextureWidth(), (int)host_.GetMaxTextureHeight());
  ASSERT(!update_texture_result && "Failed to generate SDL surface");

  const bool signed_alpha = signed_flags & 0x01;
  const bool signed_red = signed_flags & 0x02;
  const bool signed_green = signed_flags & 0x04;
  const bool signed_blue = signed_flags & 0x08;

  auto stage = host_.GetTextureStage(0);
  stage.SetFilter(0, TextureStage::K_QUINCUNX, TextureStage::MIN_BOX_LOD0, TextureStage::MAG_BOX_LOD0, signed_alpha, signed_red, signed_green, signed_blue);

  update_texture_result = host_.SetTexture(test_surface);
  SDL_FreeSurface(test_surface);
  ASSERT(!update_texture_result && "Failed to set texture");

  host_.PrepareDraw(0xFE202020);
  host_.DrawArrays();

  pb_print("%s\n", test_name.c_str());
  pb_print("Signed: %s%s%s%s\n", signed_alpha ? "A" : "", signed_red ? "R" : "", signed_green ? "G" : "", signed_blue ? "B" : "");
  pb_draw_text_screen();

  host_.FinishDraw(allow_saving_, output_dir_, test_name);
}

std::string TextureSignedComponentTests::MakeTestName(const TextureFormatInfo &texture_format, uint32_t signed_flags) {
  std::string test_name = texture_format.name;
  if (texture_format.xbox_linear) {
    test_name += "_L";
  }

  char buf[16] = {0};
  sprintf(buf, "_0x%04X", signed_flags);
  test_name += buf;

  return std::move(test_name);
}

static int GenerateTestSurface(SDL_Surface **test_surface, int width, int height) {
  *test_surface = SDL_CreateRGBSurfaceWithFormat(0, width, height, 32, SDL_PIXELFORMAT_RGBA8888);
  if (!(*test_surface)) {
    return 1;
  }

  if (SDL_LockSurface(*test_surface)) {
    SDL_FreeSurface(*test_surface);
    *test_surface = nullptr;
    return 2;
  }

  auto pixels = static_cast<uint32_t *>((*test_surface)->pixels);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x, ++pixels) {
      int x_normal = static_cast<int>(static_cast<float>(x) * 255.0f / static_cast<float>(width));
      int y_normal = static_cast<int>(static_cast<float>(y) * 255.0f / static_cast<float>(height));
      *pixels = SDL_MapRGBA((*test_surface)->format, y_normal, x_normal, 255 - y_normal, x_normal + y_normal);
    }
  }

  SDL_UnlockSurface(*test_surface);

  return 0;
}
