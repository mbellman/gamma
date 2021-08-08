#include "Gamma.h"
#include "DemoController.h"

// #include "benchmarks/object_management.h"
// #include "benchmarks/matrix_multiplication.h"

int main(int argc, char* argv[]) {
  Gamma::Window window;

  window.setRenderMode(Gamma::RenderMode::OPENGL);
  window.setController(new DemoController());
  window.open();

  // benchmark_matrix_multiplication();
  // benchmark_object_management();

  return 0;
}