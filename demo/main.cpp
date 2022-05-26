#include "Gamma.h"
// #include "DemoScene.h"

static void setupScene(GmContext* context) {
  using namespace Gamma;

  auto& sunlight = Gm_CreateLight(context, LightType::DIRECTIONAL_SHADOWCASTER);

  sunlight.direction = Vec3f(-0.3f, -0.5f, 1.0f);
  sunlight.color = Vec3f(1.0f);

  // addFloor
  auto& floor = Gm_CreateObjectFrom(context, "floor");

  floor.scale = Gamma::Vec3f(1000.0f, 1.0f, 1000.0f);

  Gm_Commit(context, floor);

  // addCenterCubesExhibit
  Vec3f location(0.0f);

  auto& cube1 = Gm_CreateObjectFrom(context, "cat-cube");
  auto& cube2 = Gm_CreateObjectFrom(context, "cat-cube");
  auto& cube3 = Gm_CreateObjectFrom(context, "cat-cube");

  cube1.position = location + Vec3f(-50.0f, 35.0f, 0.0f);
  cube1.scale = 20.0f;
  cube1.rotation = Vec3f(1.5f, 0.7f, 2.1f);
  cube1.color = pVec4(255, 50, 10);

  cube2.position = location + Vec3f(0.0f, 35.0f, 0.0f);
  cube2.scale = 10.0f;
  cube2.rotation = Vec3f(0.3f, 1.1f, 0.8f);
  cube2.color = pVec4(10, 255, 50);

  cube3.position = location + Vec3f(30.0f, 35.0f, 0.0f);
  cube3.scale = 5.0f;
  cube3.rotation = Vec3f(0.9f, 2.5f, 3.1f);
  cube3.color = pVec4(10, 50, 255);

  Gm_Commit(context, cube1);
  Gm_Commit(context, cube2);
  Gm_Commit(context, cube3);

  // addRainbowCubesExhibit
  const float pi = 3.141592f;
  Vec3f rcLocation = Vec3f(-150.0f, 0.0f, 12.0f);
  auto& ball = Gm_CreateObjectFrom(context, "probe-ball");

  ball.scale = 16.0f;
  ball.position = rcLocation + Vec3f(-10.0f, 35.0f, -10.0f);

  Gm_Commit(context, ball);

  Gm_AddProbe(context, "ball-probe", ball.position);

  auto n_sinf = [](float value) {
    return sinf(value) * 0.5f + 0.5f;
  };

  auto n_cosf = [](float value) {
    return cosf(value) * 0.5f + 0.5f;
  };

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      auto& cube = Gm_CreateObjectFrom(context, "rainbow-cube");

      cube.position = rcLocation + Vec3f(
        20.0f * (i - 2.0f),
        8.0f,
        20.0f * (j - 2.0f)
      );

      cube.scale = 8.0f;

      cube.color = pVec4(
        Vec3f(
          n_sinf(i / 3.0f * pi - pi * 0.5f),
          n_cosf(i / 3.0f * pi),
          n_sinf(j / 3.0f * pi - pi * 0.5f)
        )
      );

      Gm_Commit(context, cube);
    }
  }

  // addStatuesExhibit
  Vec3f sLocation(150.0f, 0.0f, 0.0f);
  auto& lucy = Gm_CreateObjectFrom(context, "lucy");

  lucy.position = sLocation;
  lucy.scale = 10.0f;

  Gm_Commit(context, lucy);

  auto& dragon = Gm_CreateObjectFrom(context, "dragon");

  dragon.position = sLocation + Vec3f(40.0f, 0, 0);
  dragon.scale = 10.0f;

  Gm_Commit(context, dragon);

  auto& wall1 = Gm_CreateObjectFrom(context, "statue-wall");
  auto& wall2 = Gm_CreateObjectFrom(context, "statue-wall");
  auto& wall3 = Gm_CreateObjectFrom(context, "statue-wall");
  auto& ceiling = Gm_CreateObjectFrom(context, "statue-wall");

  wall1.position = sLocation + Vec3f(-17.0f, 25.0f, -7.5f);
  wall1.scale = Vec3f(3.0f, 25.0f, 40.0f);
  wall1.color = pVec4(255, 0, 0);

  wall2.position = sLocation + Vec3f(17.0f, 25.0f, -7.5f);
  wall2.scale = Vec3f(3.0f, 25.0f, 40.0f);
  wall2.color = pVec4(0, 255, 0);

  wall3.position = sLocation + Vec3f(0, 25.0f, 28.0f);
  wall3.scale = Vec3f(70.0f, 25.0f, 3.0f);
  wall3.color = pVec4(0, 0, 255);

  ceiling.position = sLocation + Vec3f(0, 53.0f, -7.5f);
  ceiling.scale = Vec3f(70.0f, 3.0f, 40.0f);
  ceiling.color = pVec4(255, 255, 255);

  Gm_Commit(context, wall1);
  Gm_Commit(context, wall2);
  Gm_Commit(context, wall3);
  Gm_Commit(context, ceiling);

  auto& lucyLight = Gm_CreateLight(context, LightType::SPOT_SHADOWCASTER);

  lucyLight.position = lucy.position + Vec3f(0.0f, 5.0f, -20.0f);
  lucyLight.color = Vec3f(1.0f);
  lucyLight.direction = Vec3f(0.0f, 1.0f, 0.75f);
  lucyLight.fov = 60.0f;
  lucyLight.radius = 200.0f;
}

int main(int argc, char* argv[]) {
  GmContext* context = Gm_CreateContext();

  Gm_SetRenderMode(context, GmRenderMode::OPENGL);
  // Gm_SetScene(context, new DemoScene());

  using namespace Gamma;

  auto& input = context->scene.input;
  auto& camera = context->scene.camera;

  input.on<MouseMoveEvent>("mousemove", [&](const MouseMoveEvent& event) {
    if (SDL_GetRelativeMouseMode()) {
      camera.orientation.pitch += event.deltaY / 1000.0f;
      camera.orientation.yaw += event.deltaX / 1000.0f;
    }
  });

  input.on<MouseButtonEvent>("mousedown", [&](const MouseButtonEvent& event) {
    if (!SDL_GetRelativeMouseMode()) {
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
  });

  input.on<Key>("keyup", [&](Key key) {
    if (key == Key::ESCAPE) {
      SDL_SetRelativeMouseMode(SDL_FALSE);
    }

    if (key == Key::V) {
      if (Gm_IsFlagEnabled(GammaFlags::VSYNC)) {
        Gm_DisableFlags(GammaFlags::VSYNC);
      } else {
        Gm_EnableFlags(GammaFlags::VSYNC);
      }
    }
  });

  Gm_UseSceneFile(context, "./demo/scene.yml");

  setupScene(context);

  camera.position.z = -300.0f;
  camera.position.y = 20.0f;

  while (!context->window.closed) {
    float dt = Gm_GetDeltaTime(context);

    Gm_LogFrameStart(context);
    Gm_HandleEvents(context);

    // context->scene_deprecated->updateScene(dt);

    // @hack @todo remove this
    Gamma::Camera::active = &context->scene.camera;

    Gm_HandleFreeCameraMode(context, dt);

    Gm_RenderScene(context);
    Gm_LogFrameEnd(context);
  }

  Gm_DestroyContext(context);

  return 0;
}