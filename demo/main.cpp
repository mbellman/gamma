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

  GmWindow* window = Gm_CreateWindow();

  Gm_SetRenderMode(window, GmRenderMode::OPENGL);

  GmGameContext* context = Gm_CreateGameContext(new DemoScene());

  while (!window->closed) {
    Gm_HandleEvents(window, context);

    // update logic

    Gm_RenderScene(window, context);
  }

  Gm_DestroyWindow(window);
  Gm_DestroyGameContext(context);

  // benchmark_matrix_multiplication();
  // benchmark_object_management();

  return 0;
}