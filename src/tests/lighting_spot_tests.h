#ifndef NXDK_PGRAPH_TESTS_SRC_TESTS_LIGHTING_SPOT_TESTS_H_
#define NXDK_PGRAPH_TESTS_SRC_TESTS_LIGHTING_SPOT_TESTS_H_

#include <memory>
#include <vector>

#include "test_suite.h"

class TestHost;
class VertexBuffer;

// Tests behavior of spot lights.
class LightingSpotTests : public TestSuite {
 public:
  LightingSpotTests(TestHost& host, std::string output_dir);

  void Initialize() override;

 private:
  void CreateGeometry();
  void Test();
};

#endif  // NXDK_PGRAPH_TESTS_SRC_TESTS_LIGHTING_SPOT_TESTS_H_
