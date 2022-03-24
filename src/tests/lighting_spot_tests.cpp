#include "lighting_spot_tests.h"


#include <pbkit/pbkit.h>

#include "../test_host.h"
#include "debug_output.h"
#include "pbkit_ext.h"
#include "shaders/precalculated_vertex_shader.h"
#include "vertex_buffer.h"

static constexpr char kTestName[] = "Spotlight";

LightingSpotTests::LightingSpotTests(TestHost& host, std::string output_dir)
    : TestSuite(host, std::move(output_dir), "Lighting spot") {
  tests_[kTestName] = [this]() { Test(); };
}

static void SetLightAndMaterial() {
  auto p = pb_begin();
  // TODO: Work out what these mean.
  p = pb_push1(p, NV097_SET_SPECULAR_PARAMS, 0xbf34dce5);
  p = pb_push1(p, 0x09e4, 0xc020743f);
  p = pb_push1(p, 0x09e8, 0x40333d06);
  p = pb_push1(p, 0x09ec, 0xbf003612);
  p = pb_push1(p, 0x09f0, 0xbff852a5);
  p = pb_push1(p, 0x09f4, 0x401c1bce);

  p = pb_push1(p, NV097_SET_COLOR_MATERIAL, NV097_SET_COLOR_MATERIAL_ALL_FROM_MATERIAL);
  p = pb_push3(p, NV097_SET_SCENE_AMBIENT_COLOR, 0x0, 0x0, 0x0);
  p = pb_push3(p, NV097_SET_MATERIAL_EMISSION, 0x0, 0x0, 0x0);
  p = pb_push1f(p, NV097_SET_MATERIAL_ALPHA, 1.0f);

  p = pb_push3(p, NV097_SET_LIGHT_AMBIENT_COLOR, 0, 0, 0);
  p = pb_push3f(p, NV097_SET_LIGHT_DIFFUSE_COLOR, 0.0f, 0.66f, 0.0f);
  p = pb_push3(p, NV097_SET_LIGHT_SPECULAR_COLOR, 0, 0, 0);

  // TODO: Verify that local range has no effect on spotlights.
  p = pb_push1f(p, NV097_SET_LIGHT_LOCAL_RANGE, 10.0f);
  p = pb_push3f(p, NV097_SET_LIGHT_LOCAL_POSITION, 0.0f, 0.0f, -1.0f);
  p = pb_push3f(p, NV097_SET_LIGHT_LOCAL_ATTENUATION, 0.0f, 0.0f, 0.0f);
  p = pb_push3f(p, NV097_SET_LIGHT_SPOT_FALLOFF, 0.0f, 0.0f, 0.0f);
  p = pb_push4f(p, NV097_SET_LIGHT_SPOT_DIRECTION, 0.0f, 0.0f, 1.0f, 1.0f);
  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_SPOT);


  /*
D3DLIGHT8 light;
ZeroMemory(&light, sizeof(light));
light.Type = D3DLIGHT_SPOT;
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_ENABLE_MASK<0x3BC> (0x3 {Light0:SPOT, Light1:OFF, Light2:OFF, Light3:OFF, Light4:OFF, Light5:OFF, Light6:OFF, Light7:OFF})

light.Diffuse = D3DXCOLOR(0.0f, 1.0f, 0.0f, 1.0f);
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_DIFFUSE_COLOR@0[0]<0x100C> (0x00000000 => 0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_DIFFUSE_COLOR@0[1]<0x1010> (0x3F800000 => 1.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_DIFFUSE_COLOR@0[2]<0x1014> (0x00000000 => 0.000000)

light.Specular = D3DXCOLOR(1.0f, 0.0f, 0.0f, 1.0f);
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPECULAR_COLOR@0[0]<0x1018> (0x00000000 => 0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPECULAR_COLOR@0[1]<0x101C> (0x00000000 => 0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPECULAR_COLOR@0[2]<0x1020> (0x00000000 => 0.000000)

// Looks like this might be pre-transformed?
light.Position = D3DXVECTOR3(0.0f, 0.0f, -1.0f);
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_LOCAL_POSITION@0[0]<0x105C> (0x00000000 => 0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_LOCAL_POSITION@0[1]<0x1060> (0x00000000 => 0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_LOCAL_POSITION@0[2]<0x1064> (0x40C00000 => 6.000000)

light.Direction = D3DXVECTOR3(0.0f, 0.0f, -1.0f);
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPOT_DIRECTION@0[0]<0x104C> (0x80000000 => -0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPOT_DIRECTION@0[1]<0x1050> (0x80000000 => -0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPOT_DIRECTION@0[2]<0x1054> (0xBFB50564 => -1.414227)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPOT_DIRECTION@0[3]<0x1058> (0xB7B04283 => -0.000021)

light.Range = 100.0f;
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_LOCAL_RANGE@0[0]<0x1024> (0x42C80000 => 100.000000)

Attenuation = 1 / (Att0 + D * Att1 + D^2 * Att2)
light.Attenuation0 = 1.0f;
light.Attenuation1 = 2.0f;
light.Attenuation2 = 4.0f;
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_LOCAL_ATTENUATION@0[0]<0x1068> (0x3F800000 => 1.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_LOCAL_ATTENUATION@0[1]<0x106C> (0x40000000 => 2.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_LOCAL_ATTENUATION@0[2]<0x1070> (0x40800000 => 4.000000)

light.Falloff = 1.0f;
light.Phi = 3.141563f;
light.Theta = 3.141563f * 0.5f;

nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPOT_FALLOFF@0[0]<0x1040> (0x00000000 => 0.000000)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPOT_FALLOFF@0[1]<0x1044> (0xBEFD3B2A => -0.494592)
nv2a_pgraph_method 0: NV20_KELVIN_PRIMITIVE<0x97> -> NV097_SET_LIGHT_SPOT_FALLOFF@0[2]<0x1048> (0x3FBF4ECA => 1.494592)

   */


//  p = pb_push1(p, NV097_SET_LIGHT_LOCAL_RANGE, 0x7149f2ca);  // 1e30 == infinite
//  p = pb_push3(p, NV097_SET_LIGHT_INFINITE_HALF_VECTOR, 0, 0, 0);
//  p = pb_push3f(p, NV097_SET_LIGHT_INFINITE_DIRECTION, 0.0f, 0.0f, 1.0f);
//  p = pb_push1(p, NV097_SET_LIGHT_ENABLE_MASK, NV097_SET_LIGHT_ENABLE_MASK_LIGHT0_INFINITE);

  pb_end(p);
}

void LightingSpotTests::Initialize() {
  TestSuite::Initialize();

  host_.SetVertexShaderProgram(nullptr);
  CreateGeometry();
  host_.SetXDKDefaultViewportAndFixedFunctionMatrices();

  auto p = pb_begin();
  p = pb_push1(p, NV097_SET_LIGHTING_ENABLE, true);
  p = pb_push1(p, NV097_SET_SPECULAR_ENABLE, true);

  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_SPECULAR), 0);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_DIFFUSE), 0xFFFFFFFF);
  p = pb_push1(p, NV097_SET_VERTEX_DATA4UB + (4 * NV2A_VERTEX_ATTR_BACK_SPECULAR), 0);
  pb_end(p);

  SetLightAndMaterial();
}

void LightingSpotTests::CreateGeometry() {
  static constexpr float kLeft = -2.75f;
  static constexpr float kRight = 2.75f;
  static constexpr float kTop = 1.75f;
  static constexpr float kBottom = -1.75f;

  static constexpr uint32_t kHQuads = 8;
  static constexpr uint32_t kVQuads = 8;
  static constexpr uint32_t kNumQuads = kHQuads * kVQuads;

  auto buffer = host_.AllocateVertexBuffer(6 * kNumQuads);

  static constexpr float kSubQuadWidth = (kRight - kLeft) / (float)kHQuads;
  static constexpr float kSubQuadHeight = (kTop - kBottom) / (float)kVQuads;

  static constexpr float z = 1.0f;
  uint32_t index = 0;

  float top = kTop;
  for (uint32_t qy = 0; qy < kVQuads; ++qy) {
    float left = kLeft;
    for (uint32_t qx = 0; qx < kHQuads; ++qx) {
      buffer->DefineBiTri(index++, left, top, left + kSubQuadWidth, top - kSubQuadHeight, z);
      left += kSubQuadWidth;
    }
    top -= kSubQuadHeight;
  }
}

void LightingSpotTests::Test() {
  static constexpr uint32_t kBackgroundColor = 0xFF303030;
  host_.PrepareDraw(kBackgroundColor);

  host_.DrawArrays(host_.POSITION | host_.NORMAL);

  host_.FinishDraw(allow_saving_, output_dir_, kTestName);
}
