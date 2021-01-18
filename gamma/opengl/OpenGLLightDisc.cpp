#include "math/matrix.h"
#include "opengl/OpenGLLightDisc.h"
#include "system/camera.h"
#include "system/Window.h"

#include "glew.h"

namespace Gamma {
  const static uint32 DISC_CORNERS = 10;

  enum GLBuffer {
    VERTEX,
    DISC
  };

  enum GLAttribute {
    VERTEX_POSITION,
    DISC_OFFSET,
    DISC_SCALE,
    DISC_LIGHT
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

    // Buffer disc vertices
    // @TODO

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

    glEnableVertexAttribArray(GLAttribute::DISC_LIGHT);
    glVertexAttribPointer(GLAttribute::DISC_LIGHT, sizeof(Light), GL_FLOAT, GL_FALSE, sizeof(Disc), (void*)offsetof(Disc, light));
    glVertexAttribDivisor(GLAttribute::DISC_LIGHT, 1);
  }

  void OpenGLLightDisc::destroy() {
    // @TODO
  }

  void OpenGLLightDisc::draw(const std::vector<Light>& lights) {
    // @TODO avoid reallocating/freeing the disc array on each draw
    Disc* discs = new Disc[lights.size()];

    auto& camera = *Camera::active;
    float aspectRatio = (float)Window::size.width / (float)Window::size.height;
    Matrix4f projection = Matrix4f::projection(Window::size, 90.0f, 1.0f, 10000.0f);

    Matrix4f view = (
      Matrix4f::rotation(camera.orientation.toVec3f()) *
      Matrix4f::translation(camera.position.invert())
    );

    for (uint32 i = 0; i < lights.size(); i++) {
      auto& light = lights[i];
      auto& disc = discs[i];
      Vec3f localLightPosition = view * light.position;

      disc.light = light;

      if (localLightPosition.z > 0.0f) {
        // Define disc attributes for lights in front of the camera
        Vec3f screenLightPosition = (projection * localLightPosition) / localLightPosition.z;

        disc.offset.x = screenLightPosition.x;
        disc.offset.y = screenLightPosition.y;
        disc.scale.x = light.radius / localLightPosition.z * aspectRatio;
        disc.scale.y = light.radius / localLightPosition.z;
      } else {
        // Define disc attributes for lights behind the camera
        disc.offset = Vec2f(0.0f);
        // @TODO determine scale based on light type + distance of light behind camera
        disc.scale = Vec2f(2.0f);
      }
    }

    glBindBuffer(GL_ARRAY_BUFFER, buffers[GLBuffer::DISC]);
    glBufferData(GL_ARRAY_BUFFER, sizeof(Disc) * lights.size(), discs, GL_DYNAMIC_DRAW);

    glBindVertexArray(vao);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, lights.size());

    delete[] discs;
  }
}