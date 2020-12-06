#include "Gamma.h"

int main(int argc, char* argv[]) {
  Gamma::Window window;

  window.setRegion({ 200, 200, 640, 480 });
  window.setRenderer(new Gamma::OpenGLRenderer());
  window.open();

  return 0;
}