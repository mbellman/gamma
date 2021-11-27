#include <cmath>

#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  Gm_EnableFlags(GammaFlags::FREE_CAMERA_MODE);

  camera.position.z = -300.0f;
  camera.position.y = 20.0f;

  useSceneFile("./demo/scene.yml");

  addFloor();
  addCenterCubesExhibit();
  addRainbowCubesExhibit();
  addStatuesExhibit();
  addChessExhibit();

  // addMesh("daVinci", 1, Mesh::Model("./demo/assets/models/da-vinci.obj"));
  // addMesh("particles", 10000, Mesh::Particles());

  // addMesh("rabbit", 10, Mesh::Model({
  //   "./demo/assets/models/rabbit-lod1.obj",
  //   "./demo/assets/models/rabbit-lod2.obj"
  // }));

  // mesh("cube").texture = "./demo/assets/images/cat.png";
  // mesh("wall").texture = "./demo/assets/images/cat.png";
  // mesh("cube").texture = "./demo/assets/images/marble.png";
  // mesh("cube").normalMap = "./demo/assets/images/marble-normal-map.png";

  // mesh("plane").type = MeshType::REFLECTIVE;
  // mesh("rabbit").type = MeshType::REFRACTIVE;
  // mesh("cube").type = MeshType::EMISSIVE;

  // auto& particles = mesh("particles").particleSystem;

  // particles.spawn = Vec3f(0.0f, 20.0f, 0.0f);
  // particles.spread = 10.0f;
  // particles.minimumRadius = 0.0f;
  // particles.medianSpeed = 0.1f;
  // particles.speedVariation = 0.1f;
  // particles.medianSize = 10.0f;
  // particles.sizeVariation = 10.0f;
  // particles.deviation = 3.0f;
  // particles.isCircuit = true;

  // particles.path = {
  //   Vec3f(0.0f, 20.0f, 0.0f),
  //   Vec3f(20.0f, -10.0f, -40.0f),
  //   Vec3f(50.0f, 40.0f, 10.0f),
  //   Vec3f(0.0f, 30.0f, 10.0f),
  //   Vec3f(-20.0f, 40.0f, 35.0f),
  //   Vec3f(-40.0f, 15.0f, 25.0f),
  //   Vec3f(-60.0f, 20.0f, -30.0f),
  //   Vec3f(-5.0f, 20.0f, -5)
  // };

  // auto& daVinci = createObjectFrom("daVinci");

  // daVinci.position = Vec3f(0.0f, 20.0f, 0.0f);
  // daVinci.scale = 50.0f;
  // daVinci.color = pVec4(100, 175, 255);

  // commit(daVinci);

  // for (uint32 i = 0; i < 10; i++) {
  //   auto& rabbit = createObjectFrom("rabbit");
  //   float r = (float)i / 10.0f * M_PI * 2.0f;

  //   rabbit.scale = 10.0f;

  //   rabbit.position = Vec3f(
  //     sinf(r) * 60.0f,
  //     -17.0f,
  //     cosf(r) * 60.0f
  //   );

  //   rabbit.rotation = Vec3f(0.0f, -1.0f * i / 10.0f * 3.141592f * 2.0f, 0.0f);

  //   rabbit.color = pVec4(
  //     Vec3f(
  //       i / 10.0f,
  //       sinf(i / 10.0f * 3.141592f),
  //       1.0f - i / 10.0f
  //     )
  //   );

  //   commit(rabbit);
  // }

  // auto& daVinciLight = createLight(LightType::SPOT_SHADOWCASTER);

  // daVinciLight.position = Vec3f(0.0f, 20.0f, 0.0f);
  // daVinciLight.color = Vec3f(1.0f, 0.0f, 0.0f);
  // daVinciLight.radius = 1000.0f;
  // daVinciLight.power = 2.0f;
  // daVinciLight.direction = Vec3f(0.0f, 0.0f, 1.0f);
  // daVinciLight.fov = 80.0f;
  // daVinciLight.isStatic = true;

  // auto& daVinciLight2 = createLight(LightType::SPOT_SHADOWCASTER);

  // daVinciLight2.position = Vec3f(-50.0f, 20.0f, -50.0f);
  // daVinciLight2.color = Vec3f(0.0f, 1.0f, 0.0f);
  // daVinciLight2.radius = 1000.0f;
  // daVinciLight2.power = 2.0f;
  // daVinciLight2.direction = Vec3f(-0.3f, -1.0f, -0.3f);
  // daVinciLight2.fov = 80.0f;
  // daVinciLight2.isStatic = true;

  // auto& daVinciLight3 = createLight(LightType::SPOT_SHADOWCASTER);

  // daVinciLight3.position = Vec3f(50.0f, 20.0f, -50.0f);
  // daVinciLight3.color = Vec3f(0.0f, 0.0f, 1.0f);
  // daVinciLight3.radius = 1000.0f;
  // daVinciLight3.power = 2.0f;
  // daVinciLight3.direction = Vec3f(0.3f, -1.0f, -0.3f);
  // daVinciLight3.fov = 80.0f;
  // daVinciLight3.isStatic = true;

  auto& sunlight = createLight(LightType::DIRECTIONAL_SHADOWCASTER);

  sunlight.direction = Vec3f(-0.3f, -0.5f, 1.0f);
  sunlight.color = Vec3f(1.0f);

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
}

void DemoScene::destroy() {}

void DemoScene::update(float dt) {
  // useLodByDistance(mesh("rabbit"), 100.0f);
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

  for (auto& cube : mesh("cat-cube").objects) {
    cube.rotation += Vec3f(0.5f * dt);

    commit(cube);
  }

  for (auto& l : mesh("lucy").objects) {
    l.rotation.y += dt;

    commit(l);
  }

  // auto& wall = *mesh("wall").objects.begin();

  // uint8 g = uint8((0.5f * sinf(getRunningTime() * 3.0f) + 0.5f) * 1.0f);
  // uint8 b = uint8((0.5f * cosf(getRunningTime() * 2.0f) + 0.5f) * 255.0f);

  // wall.color = pVec4(255, g, b);
  // wall.position.z = 0.0f + (1.0f + sinf(getRunningTime() * 0.2f)) * 0.5f * 200.0f;

  // commit(wall);
}

void DemoScene::addFloor() {
  auto& floor = createObjectFrom("floor");

  floor.scale = Vec3f(1000.0f, 1.0f, 1000.0f);

  commit(floor);
}

void DemoScene::addCenterCubesExhibit() {
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
}

void DemoScene::addRainbowCubesExhibit() {
  const float pi = 3.141592f;
  Vec3f location = Vec3f(-150.0f, 0.0f, 12.0f);
  auto& ball = createObjectFrom("probe-ball");

  ball.scale = 16.0f;
  ball.position = location + Vec3f(-10.0f, 35.0f, -10.0f); 

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

      cube.position = location + Vec3f(
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
}

void DemoScene::addStatuesExhibit() {
  Vec3f location(150.0f, 0.0f, 0.0f);
  auto& lucy = createObjectFrom("lucy");

  lucy.position = location;
  lucy.scale = 10.0f;

  commit(lucy);

  auto& dragon = createObjectFrom("dragon");

  dragon.position = location + Vec3f(40.0f, 0, 0);
  dragon.scale = 10.0f;

  commit(dragon);

  auto& wall1 = createObjectFrom("statue-wall");
  auto& wall2 = createObjectFrom("statue-wall");
  auto& wall3 = createObjectFrom("statue-wall");
  auto& ceiling = createObjectFrom("statue-wall");

  wall1.position = location + Vec3f(-17.0f, 25.0f, -7.5f);
  wall1.scale = Vec3f(3.0f, 25.0f, 40.0f);
  wall1.color = pVec4(255, 0, 0);

  wall2.position = location + Vec3f(17.0f, 25.0f, -7.5f);
  wall2.scale = Vec3f(3.0f, 25.0f, 40.0f);
  wall2.color = pVec4(0, 255, 0);

  wall3.position = location + Vec3f(0, 25.0f, 28.0f);
  wall3.scale = Vec3f(70.0f, 25.0f, 3.0f);
  wall3.color = pVec4(0, 0, 255);

  ceiling.position = location + Vec3f(0, 53.0f, -7.5f);
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
  // lucyLight.isStatic = true;

  // auto& lucyLight2 = createLight(LightType::POINT);

  // lucyLight2.position = lucy.position + Vec3f(0.0f, 30.0f, 10.0f);
  // lucyLight2.color = Vec3f(1.0f);
  // lucyLight2.radius = 50.0f;
}

void DemoScene::addChessExhibit() {
  Vec3f location = Vec3f(0, 0, 250.0f);

  // Create chess board
  auto& chessBoard = createObjectFrom("chess-board");

  chessBoard.position = location + Vec3f(-8.6f, 1.0f, 17.0f);
  chessBoard.scale = 69.0f;

  commit(chessBoard);

  // Create white pawns
  for (uint32 i = 0; i < 8; i++) {
    auto& pawn = createObjectFrom("pawn");

    pawn.scale = 4.5f;
    pawn.position = location + Vec3f(-38.5f + 8.6f * i, 1.0f, -4.5f);

    commit(pawn);
  }

  // Create white rooks
  auto& wRook1 = createObjectFrom("rook");
  auto& wRook2 = createObjectFrom("rook");

  wRook1.position = location + Vec3f(-38.5f, 1.0f, -13.5f);
  wRook1.scale = 4.5f;

  wRook2.position = location + Vec3f(21.7f, 1.0f, -13.5f);
  wRook2.scale = 4.5f;

  commit(wRook1);
  commit(wRook2);

  // Create white knights
  auto& wKnight1 = createObjectFrom("knight");
  auto& wKnight2 = createObjectFrom("knight");

  wKnight1.position = location + Vec3f(-29.9f, 1.0f, -13.5f);
  wKnight1.scale = 4.5f;
  wKnight1.rotation = Vec3f(0, M_PI, 0);

  wKnight2.position = location + Vec3f(13.1f, 1.0f, -13.5f);
  wKnight2.scale = 4.5f;
  wKnight2.rotation =  Vec3f(0, M_PI, 0);

  commit(wKnight1);
  commit(wKnight2);

  // Create white bishops
  auto& wBishop1 = createObjectFrom("bishop");
  auto& wBishop2 = createObjectFrom("bishop");

  wBishop1.position = location + Vec3f(-21.3f, 1.0f, -13.5f);
  wBishop1.scale = 4.5f;
  wBishop1.rotation = Vec3f(0, M_PI, 0);

  wBishop2.position = location + Vec3f(4.5f, 1.0f, -13.5f);
  wBishop2.scale = 4.5f;
  wBishop2.rotation =  Vec3f(0, M_PI, 0);

  commit(wBishop1);
  commit(wBishop2);

  // Create white king + queen
  auto& wKing = createObjectFrom("king");
  auto& wQueen = createObjectFrom("queen");

  wKing.position = location + Vec3f(-4.1f, 1.0f, -13.5f);
  wKing.scale = 4.5f;

  wQueen.position = location + Vec3f(-12.7f, 1.0f, -13.5f);
  wQueen.scale = 4.5f;

  commit(wKing);
  commit(wQueen);

  // Create black pawns
  for (uint32 i = 0; i < 8; i++) {
    auto& pawn = createObjectFrom("pawn");

    pawn.scale = 4.5f;
    pawn.position = location + Vec3f(-38.5f + 8.6f * i, 1.0f, 38.6f);
    pawn.color = pVec4(50, 50, 50);

    commit(pawn);
  }

  // Create black rooks
  auto& bRook1 = createObjectFrom("rook");
  auto& bRook2 = createObjectFrom("rook");

  bRook1.position = location + Vec3f(-38.5f, 1.0f, 47.25f);
  bRook1.scale = 4.5f;
  bRook1.color = pVec4(50, 50, 50);

  bRook2.position = location + Vec3f(21.7f, 1.0f, 47.25f);
  bRook2.scale = 4.5f;
  bRook2.color = pVec4(50, 50, 50);

  commit(bRook1);
  commit(bRook2);

  // Create black knights
  auto& bKnight1 = createObjectFrom("knight");
  auto& bKnight2 = createObjectFrom("knight");

  bKnight1.position = location + Vec3f(-29.9f, 1.0f, 47.25f);
  bKnight1.scale = 4.5f;
  bKnight1.color = pVec4(50, 50, 50);

  bKnight2.position = location + Vec3f(13.1f, 1.0f, 47.25f);
  bKnight2.scale = 4.5f;
  bKnight2.color = pVec4(50, 50, 50);

  commit(bKnight1);
  commit(bKnight2);

  // Create black bishops
  auto& bBishop1 = createObjectFrom("bishop");
  auto& bBishop2 = createObjectFrom("bishop");

  bBishop1.position = location + Vec3f(-21.3f, 1.0f, 47.25f);
  bBishop1.scale = 4.5f;
  bBishop1.color = pVec4(50, 50, 50);

  bBishop2.position = location + Vec3f(4.5f, 1.0f, 47.25f);
  bBishop2.scale = 4.5f;
  bBishop2.color = pVec4(50, 50, 50);

  commit(bBishop1);
  commit(bBishop2);

  // Create black king + queen
  auto& bKing = createObjectFrom("king");
  auto& bQueen = createObjectFrom("queen");

  bKing.position = location + Vec3f(-4.1f, 1.0f, 47.25f);
  bKing.scale = 4.5f;
  bKing.color = pVec4(50, 50, 50);

  bQueen.position = location + Vec3f(-12.7f, 1.0f, 47.25f);
  bQueen.scale = 4.5f;
  bQueen.color = pVec4(50, 50, 50);

  commit(bKing);
  commit(bQueen);
}