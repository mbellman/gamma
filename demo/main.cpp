#include "Gamma.h"
#include "DemoScene.h"
// #include "DemoController.h"

// #include "benchmarks/object_management.h"
// #include "benchmarks/matrix_multiplication.h"

int main(int argc, char* argv[]) {
  // Gamma::Window window;

  // window.setRenderMode(Gamma::RenderMode::OPENGL);
  // window.setController(new DemoController());
  // window.open();

  GmContext* context = Gm_CreateContext();

  Gm_SetRenderMode(context, GmRenderMode::OPENGL);
  Gm_SetScene(context, new DemoScene());

  while (!context->window.closed) {
    Gm_HandleEvents(context);

    // update logic

    Gm_RenderScene(context);
  }

  Gm_DestroyContext(context);

  // benchmark_matrix_multiplication();
  // benchmark_object_management();

  return 0;
}