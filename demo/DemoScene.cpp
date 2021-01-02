#include "DemoScene.h"
#include "Gamma.h"

using namespace Gamma;

void DemoScene::init() {
  addMesh("cube", Gm_CreateCube(), 10);

  auto& cube = createObjectFrom("cube");

  cube.position = Vec3f(0.0f, 0.0f, 200.0f);
  cube.scale = 20.0f;
  cube.rotation = 0.0f;

  transform(cube);

  store("cube", cube);

  // @TODO fix void* -> Event& casting issues
  // so pointers need not be used
  input.on<MouseMoveEvent*>("mousemove", [=](MouseMoveEvent* event) {
    if (SDL_GetRelativeMouseMode()) {
      camera.orientation.pitch += event->deltaY / 1000.0f;
      camera.orientation.yaw += event->deltaX / 1000.0f;
    }
  });

  input.on<MouseButtonEvent*>("mousedown", [=](MouseButtonEvent* event) {
    if (!SDL_GetRelativeMouseMode()) {
      SDL_SetRelativeMouseMode(SDL_TRUE);
    }
  });

  input.on<Key*>("keyup", [=](Key* key) {
    if (*key == Key::ESCAPE) {
      SDL_SetRelativeMouseMode(SDL_FALSE);
    }
  });
}

void DemoScene::destroy() {
  // @TODO
}

void DemoScene::update(float dt) {
  float movementSpeed = 100.0f * dt;

  if (input.isKeyHeld(Key::A)) {
    camera.position += camera.orientation.getLeftDirection() * movementSpeed;
  } else if (input.isKeyHeld(Key::D)) {
    camera.position += camera.orientation.getRightDirection() * movementSpeed;
  }

  if (input.isKeyHeld(Key::W)) {
    camera.position += camera.orientation.getDirection() * movementSpeed;
  } else if (input.isKeyHeld(Key::S)) {
    camera.position += camera.orientation.getDirection().invert() * movementSpeed;
  }

  auto& cube = get("cube");

  cube.rotation.x += 0.5f * dt;
  cube.rotation.y += dt;
  cube.rotation.z += 0.84f * dt;

  transform(cube);
}