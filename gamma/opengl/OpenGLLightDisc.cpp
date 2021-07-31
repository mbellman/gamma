#include <cmath>

#include "math/constants.h"
#include "math/matrix.h"
#include "opengl/OpenGLLightDisc.h"
#include "system/camera.h"
#include "system/console.h"
#include "system/Window.h"

#include "glew.h"

namespace Gamma {
  constexpr static uint32 DISC_SLICES = 16;

  enum GLBuffer {
    VERTEX,
    DISC
  };

  enum GLAttribute {
    VERTEX_POSITION,
    DISC_OFFSET,
    DISC_SCALE,
    DISC_LIGHT_POSITION,
    DISC_LIGHT_RADIUS,
    DISC_LIGHT_COLOR,
    DISC_LIGHT_POWER
  };

  struct Disc {
    Vec2f offset;
    Vec2f scale;
    Light light;
  };

  void OpenGLLightDisc::init() {
    glGenVertexArrays(1, &vao);
    glGenBuffers(2, &buffers[0]);
    glBindVertexArray(vao);

    // Create the vertices for each slice of the disc
    Vec2f vertexPositions[DISC_SLICES * 3];

    for (uint32 i = 0; i < DISC_SLICES; i++) {
      constexpr static float sliceAngle = 360.0f / (float)DISC_SLICES;
      uint32 index = i * 3;

      // Add center vertex
      vertexPositions[index] = Vec2f(0.0f);

      // Add corners
      const float a1 = i * sliceAngle * DEGREES_TO_RADIANS;
      const float a2 = (i + 1) * sliceAngle * DEGREES_TO_RADIANS;

      vertexPositions[index + 1] = Vec2f(sinf(a1), cosf(a1));
      vertexPositions[index + 2] = Vec2f(sinf(a2), cosf(a2));
    }

    // Buffer disc vertices
    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::VERTEX]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Vec2f) * DISC_SLICES * 3, vertexPositions, GL_STATIC_DRAW);

    // Define disc vertex attributes
    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::VERTEX]);

    glEnableVertexAttribArray(GLAttribute::VERTEX_POSITION);
    glVertexAttribPointer(GLAttribute::VERTEX_POSITION, 2, GL_FLOAT, GL_FALSE, sizeof(Vec2f), (void*)0);

    // Define disc instance attributes
    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::DISC]);

    glEnableVertexAttribArray(GLAttribute::DISC_OFFSET);
    glVertexAttribPointer(GLAttribute::DISC_OFFSET, 2, GL_FLOAT, GL_FALSE, sizeof(Disc), (void*)offsetof(Disc, offset));
    glVertexAttribDivisor(GLAttribute::DISC_OFFSET, 1);

    glEnableVertexAttribArray(GLAttribute::DISC_SCALE);
    glVertexAttribPointer(GLAttribute::DISC_SCALE, 2, GL_FLOAT, GL_FALSE, sizeof(Disc), (void*)offsetof(Disc, scale));
    glVertexAttribDivisor(GLAttribute::DISC_SCALE, 1);

    glEnableVertexAttribArray(GLAttribute::DISC_LIGHT_POSITION);
    glVertexAttribPointer(GLAttribute::DISC_LIGHT_POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Disc), (void*)(offsetof(Disc, light) + offsetof(Light, position)));
    glVertexAttribDivisor(GLAttribute::DISC_LIGHT_POSITION, 1);

    glEnableVertexAttribArray(GLAttribute::DISC_LIGHT_RADIUS);
    glVertexAttribPointer(GLAttribute::DISC_LIGHT_RADIUS, 1, GL_FLOAT, GL_FALSE, sizeof(Disc), (void*)(offsetof(Disc, light) + offsetof(Light, radius)));
    glVertexAttribDivisor(GLAttribute::DISC_LIGHT_RADIUS, 1);

    glEnableVertexAttribArray(GLAttribute::DISC_LIGHT_COLOR);
    glVertexAttribPointer(GLAttribute::DISC_LIGHT_COLOR, 3, GL_FLOAT, GL_FALSE, sizeof(Disc), (void*)(offsetof(Disc, light) + offsetof(Light, color)));
    glVertexAttribDivisor(GLAttribute::DISC_LIGHT_COLOR, 1);

    glEnableVertexAttribArray(GLAttribute::DISC_LIGHT_POWER);
    glVertexAttribPointer(GLAttribute::DISC_LIGHT_POWER, 1, GL_FLOAT, GL_FALSE, sizeof(Disc), (void*)(offsetof(Disc, light) + offsetof(Light, power)));
    glVertexAttribDivisor(GLAttribute::DISC_LIGHT_POWER, 1);
  }

  void OpenGLLightDisc::destroy() {
    // @todo
  }

  void OpenGLLightDisc::draw(const std::vector<Light>& lights) {
    // @todo avoid reallocating/freeing the disc array on each draw
    Disc* discs = new Disc[lights.size()];
    auto& camera = *Camera::active;
    float aspectRatio = (float)Window::size.width / (float)Window::size.height;
    Matrix4f projection = Matrix4f::projection(Window::size, 90.0f * 0.5f, 1.0f, 10000.0f);

    Matrix4f view = (
      Matrix4f::rotation(camera.orientation.toVec3f().invert()) *
      Matrix4f::translation(camera.position.invert())
    );

    for (uint32 i = 0; i < lights.size(); i++) {
      auto& light = lights[i];
      auto& disc = discs[i];
      Vec3f localLightPosition = view * light.position;

      disc.light = light;

      if (localLightPosition.z > 0.0f) {
        // Light source in front of the camera
        Vec3f screenLightPosition = (projection * localLightPosition) / localLightPosition.z;
        // @todo use a more sophisticated radius-of-influence formula for this
        float scaleFactor = light.power * 1.2f;

        disc.offset = Vec2f(screenLightPosition.x, screenLightPosition.y);
        disc.scale.x = scaleFactor * light.radius / localLightPosition.z;
        disc.scale.y = scaleFactor * light.radius / localLightPosition.z * aspectRatio;
      } else {
        // Light source behind the camera
        float scale = localLightPosition.magnitude() < light.radius ? 2.0f : 0.0f;

        disc.offset = Vec2f(0.0f);
        disc.scale = Vec2f(scale);
      }
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::DISC]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Disc) * lights.size(), discs, GL_DYNAMIC_DRAW);

    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, DISC_SLICES * 3, lights.size());

    delete[] discs;
  }
}