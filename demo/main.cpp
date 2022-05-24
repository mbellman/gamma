#include "Gamma.h"
#include "DemoScene.h"

int main(int argc, char* argv[]) {
  GmContext* context = Gm_CreateContext();

  Gm_SetRenderMode(context, GmRenderMode::OPENGL);
  Gm_SetScene(context, new DemoScene());

  while (!context->window.closed) {
    float dt = Gm_GetDeltaTime(context);

    Gm_LogFrameStart(context);
    Gm_HandleEvents(context);
    context->scene->updateScene(dt);
    Gm_RenderScene(context);
    Gm_LogFrameEnd(context);
  }

  Gm_DestroyContext(context);

  return 0;
}