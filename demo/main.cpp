#include "Gamma.h"

int main(int argc, char* argv[]) {
  Gamma::Window window;

  window.setRenderMode(Gamma::RenderMode::OPENGL);
  window.open();

  return 0;
}