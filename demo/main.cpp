#include "Gamma.h"

static void initScene(_ctx) {
  using namespace Gamma;

  auto& sunlight = createLight(LightType::DIRECTIONAL_SHADOWCASTER);

  sunlight.direction = Vec3f(-0.3f, -0.5f, 1.0f);
  sunlight.color = Vec3f(1.0f);

  // addFloor
  auto& floor = createObjectFrom("floor");

  floor.scale = Gamma::Vec3f(1000.0f, 1.0f, 1000.0f);

  commit(floor);

  // addCenterCubesExhibit
  Vec3f location(0.0f);

  auto& cube1 = createObjectFrom("cat-cube");
  auto& cube2 = createObjectFrom("cat-cube");
  auto& cube3 = createObjectFrom("cat-cube");

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

  commit(cube1);
  commit(cube2);
  commit(cube3);

  // addRainbowCubesExhibit
  const float pi = 3.141592f;
  Vec3f rcLocation = Vec3f(-150.0f, 0.0f, 12.0f);
  auto& ball = createObjectFrom("probe-ball");

  ball.scale = 16.0f;
  ball.position = rcLocation + Vec3f(-10.0f, 35.0f, -10.0f);

  commit(ball);

  addProbe("ball-probe", ball.position);

  auto n_sinf = [](float value) {
    return sinf(value) * 0.5f + 0.5f;
  };

  auto n_cosf = [](float value) {
    return cosf(value) * 0.5f + 0.5f;
  };

  for (int i = 0; i < 4; i++) {
    for (int j = 0; j < 4; j++) {
      auto& cube = createObjectFrom("rainbow-cube");

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

      commit(cube);
    }
  }

  // addStatuesExhibit
  Vec3f sLocation(150.0f, 0.0f, 0.0f);
  auto& lucy = createObjectFrom("lucy");

  lucy.position = sLocation;
  lucy.scale = 10.0f;

  commit(lucy);

  auto& dragon = createObjectFrom("dragon");

  dragon.position = sLocation + Vec3f(40.0f, 0, 0);
  dragon.scale = 10.0f;

  commit(dragon);

  auto& wall1 = createObjectFrom("statue-wall");
  auto& wall2 = createObjectFrom("statue-wall");
  auto& wall3 = createObjectFrom("statue-wall");
  auto& ceiling = createObjectFrom("statue-wall");

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

  commit(wall1);
  commit(wall2);
  commit(wall3);
  commit(ceiling);

  auto& lucyLight = createLight(LightType::SPOT_SHADOWCASTER);

  lucyLight.position = lucy.position + Vec3f(0.0f, 5.0f, -20.0f);
  lucyLight.color = Vec3f(1.0f);
  lucyLight.direction = Vec3f(0.0f, 1.0f, 0.75f);
  lucyLight.fov = 60.0f;
  lucyLight.radius = 200.0f;

  // addChessExhibit
  Vec3f cLocation = Vec3f(0, 0, 250.0f);

  // Create chess board
  auto& chessBoard = createObjectFrom("chess-board");

  chessBoard.position = cLocation + Vec3f(-8.6f, 1.0f, 17.0f);
  chessBoard.scale = 69.0f;

  commit(chessBoard);

  // Create white pawns
  for (uint32 i = 0; i < 8; i++) {
    auto& pawn = createObjectFrom("pawn");

    pawn.scale = 4.5f;
    pawn.position = cLocation + Vec3f(-38.5f + 8.6f * i, 1.0f, -4.5f);

    commit(pawn);
  }

  // Create white rooks
  auto& wRook1 = createObjectFrom("rook");
  auto& wRook2 = createObjectFrom("rook");

  wRook1.position = cLocation + Vec3f(-38.5f, 1.0f, -13.5f);
  wRook1.scale = 4.5f;

  wRook2.position = cLocation + Vec3f(21.7f, 1.0f, -13.5f);
  wRook2.scale = 4.5f;

  commit(wRook1);
  commit(wRook2);

  // Create white knights
  auto& wKnight1 = createObjectFrom("knight");
  auto& wKnight2 = createObjectFrom("knight");

  wKnight1.position = cLocation + Vec3f(-29.9f, 1.0f, -13.5f);
  wKnight1.scale = 4.5f;
  wKnight1.rotation = Vec3f(0, pi, 0);

  wKnight2.position = cLocation + Vec3f(13.1f, 1.0f, -13.5f);
  wKnight2.scale = 4.5f;
  wKnight2.rotation =  Vec3f(0, pi, 0);

  commit(wKnight1);
  commit(wKnight2);

  // Create white bishops
  auto& wBishop1 = createObjectFrom("bishop");
  auto& wBishop2 = createObjectFrom("bishop");

  wBishop1.position = cLocation + Vec3f(-21.3f, 1.0f, -13.5f);
  wBishop1.scale = 4.5f;
  wBishop1.rotation = Vec3f(0, pi, 0);

  wBishop2.position = cLocation + Vec3f(4.5f, 1.0f, -13.5f);
  wBishop2.scale = 4.5f;
  wBishop2.rotation =  Vec3f(0, pi, 0);

  commit(wBishop1);
  commit(wBishop2);

  // Create white king + queen
  auto& wKing = createObjectFrom("king");
  auto& wQueen = createObjectFrom("queen");

  wKing.position = cLocation + Vec3f(-4.1f, 1.0f, -13.5f);
  wKing.scale = 4.5f;

  wQueen.position = cLocation + Vec3f(-12.7f, 1.0f, -13.5f);
  wQueen.scale = 4.5f;

  commit(wKing);
  commit(wQueen);

  // Create black pawns
  for (uint32 i = 0; i < 8; i++) {
    auto& pawn = createObjectFrom("pawn");

    pawn.scale = 4.5f;
    pawn.position = cLocation + Vec3f(-38.5f + 8.6f * i, 1.0f, 38.6f);
    pawn.color = pVec4(50, 50, 50);

    commit(pawn);
  }

  // Create black rooks
  auto& bRook1 = createObjectFrom("rook");
  auto& bRook2 = createObjectFrom("rook");

  bRook1.position = cLocation + Vec3f(-38.5f, 1.0f, 47.25f);
  bRook1.scale = 4.5f;
  bRook1.color = pVec4(50, 50, 50);

  bRook2.position = cLocation + Vec3f(21.7f, 1.0f, 47.25f);
  bRook2.scale = 4.5f;
  bRook2.color = pVec4(50, 50, 50);

  commit(bRook1);
  commit(bRook2);

  // Create black knights
  auto& bKnight1 = createObjectFrom("knight");
  auto& bKnight2 = createObjectFrom("knight");

  bKnight1.position = cLocation + Vec3f(-29.9f, 1.0f, 47.25f);
  bKnight1.scale = 4.5f;
  bKnight1.color = pVec4(50, 50, 50);

  bKnight2.position = cLocation + Vec3f(13.1f, 1.0f, 47.25f);
  bKnight2.scale = 4.5f;
  bKnight2.color = pVec4(50, 50, 50);

  commit(bKnight1);
  commit(bKnight2);

  // Create black bishops
  auto& bBishop1 = createObjectFrom("bishop");
  auto& bBishop2 = createObjectFrom("bishop");

  bBishop1.position = cLocation + Vec3f(-21.3f, 1.0f, 47.25f);
  bBishop1.scale = 4.5f;
  bBishop1.color = pVec4(50, 50, 50);

  bBishop2.position = cLocation + Vec3f(4.5f, 1.0f, 47.25f);
  bBishop2.scale = 4.5f;
  bBishop2.color = pVec4(50, 50, 50);

  commit(bBishop1);
  commit(bBishop2);

  // Create black king + queen
  auto& bKing = createObjectFrom("king");
  auto& bQueen = createObjectFrom("queen");

  bKing.position = cLocation + Vec3f(-4.1f, 1.0f, 47.25f);
  bKing.scale = 4.5f;
  bKing.color = pVec4(50, 50, 50);

  bQueen.position = cLocation + Vec3f(-12.7f, 1.0f, 47.25f);
  bQueen.scale = 4.5f;
  bQueen.color = pVec4(50, 50, 50);

  commit(bKing);
  commit(bQueen);
}

static void updateScene(_ctx, float dt) {
  using namespace Gamma;

  useFrustumCulling({
    "pawn",
    "dragon",
    "lucy",
    "rook",
    "knight",
    "bishop",
    "king",
    "queen"
  });

  useLodByDistance(150.0f, {
    "dragon",
    "pawn"
  });

  useLodByDistance(100.0f, {
    "lucy",
    "rook",
    "knight",
    "bishop",
    "king",
    "queen"
  });

  for (auto& cube : context->scene.meshMap["cat-cube"]->objects) {
    cube.rotation += Vec3f(0.5f * dt);

    commit(cube);
  }

  auto& lucy = context->scene.meshMap["lucy"]->objects[0];

  lucy.rotation.y += dt;

  commit(lucy);
}

int main(int argc, char* argv[]) {
  GmContext* context = Gm_CreateContext();

  Gm_SetRenderMode(context, GmRenderMode::OPENGL);

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

  initScene(context);

  camera.position.z = -300.0f;
  camera.position.y = 20.0f;

  while (!context->window.closed) {
    float dt = Gm_GetDeltaTime(context);

    Gm_LogFrameStart(context);
    Gm_HandleEvents(context);

    // @hack @todo remove this
    Gamma::Camera::active = &context->scene.camera;

    Gm_HandleFreeCameraMode(context, dt);
    updateScene(context, dt);

    Gm_RenderScene(context);
    Gm_LogFrameEnd(context);
  }

  Gm_DestroyContext(context);

  return 0;
}