#include "Gamma.h"
#include "DemoController.h"

int main(int argc, char* argv[]) {
  Gamma::Window window;

  window.setRenderMode(Gamma::RenderMode::OPENGL);
  window.setController(new DemoController());
  window.open();

  return 0;
}