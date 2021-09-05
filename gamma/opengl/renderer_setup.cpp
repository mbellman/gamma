#include "opengl/renderer_setup.h"
#include "system/flags.h"

namespace Gamma {
  void Gm_InitRendererResources(RendererBuffers& buffers, RendererShaders& shaders, const Area<uint32>& internalResolution) {
    // Initialize buffers
    buffers.gBuffer.init();
    buffers.gBuffer.setSize(internalResolution);
    buffers.gBuffer.addColorAttachment(ColorFormat::RGBA);    // (RGB) Albedo, (A) Depth
    buffers.gBuffer.addColorAttachment(ColorFormat::RGBA16);  // (RGB) Normal, (A) Specularity
    buffers.gBuffer.addColorAttachment(ColorFormat::RGBA);    // (RGB) Accumulated effects color, (A) Depth
    buffers.gBuffer.addDepthStencilAttachment();
    buffers.gBuffer.bindColorAttachments();

    buffers.post.init();
    buffers.post.setSize(internalResolution);
    buffers.post.addColorAttachment(ColorFormat::RGBA);  // (RGB) Color, (A) Depth
    buffers.gBuffer.shareDepthStencilAttachment(buffers.post);
    buffers.post.bindColorAttachments();

    buffers.reflections.init();
    buffers.reflections.setSize({ internalResolution.width, internalResolution.height });
    buffers.reflections.addColorAttachment(ColorFormat::RGBA);
    buffers.gBuffer.shareDepthStencilAttachment(buffers.reflections);
    buffers.reflections.bindColorAttachments();

    // @todo consider whether we need a half-size reflections buffer
    // buffers.reflections.init();
    // buffers.reflections.setSize({ internalResolution.width / 2, internalResolution.height /2 });
    // buffers.reflections.addColorAttachment(ColorFormat::RGBA);
    // buffers.reflections.bindColorAttachments();

    // Initialize shaders
    shaders.geometry.init();
    shaders.geometry.vertex("./gamma/opengl/shaders/geometry.vert.glsl");
    shaders.geometry.fragment("./gamma/opengl/shaders/geometry.frag.glsl");
    shaders.geometry.link();

    shaders.copyDepth.init();
    shaders.copyDepth.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.copyDepth.fragment("./gamma/opengl/shaders/copy-depth.frag.glsl");
    shaders.copyDepth.link();

    shaders.pointLight.init();
    shaders.pointLight.vertex("./gamma/opengl/shaders/instanced-light.vert.glsl");
    shaders.pointLight.fragment("./gamma/opengl/shaders/point-light-without-shadow.frag.glsl");
    shaders.pointLight.link();

    shaders.pointShadowcaster.init();
    shaders.pointShadowcaster.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.pointShadowcaster.fragment("./gamma/opengl/shaders/point-light-with-shadow.frag.glsl");
    shaders.pointShadowcaster.link();

    shaders.pointShadowcasterView.init();
    shaders.pointShadowcasterView.vertex("./gamma/opengl/shaders/point-light-view.vert.glsl");
    shaders.pointShadowcasterView.geometry("./gamma/opengl/shaders/point-light-view.geom.glsl");
    shaders.pointShadowcasterView.fragment("./gamma/opengl/shaders/point-light-view.frag.glsl");
    shaders.pointShadowcasterView.link();

    shaders.directionalLight.init();
    shaders.directionalLight.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.directionalLight.fragment("./gamma/opengl/shaders/directional-light-without-shadow.frag.glsl");
    shaders.directionalLight.link();

    shaders.directionalShadowcaster.init();
    shaders.directionalShadowcaster.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.directionalShadowcaster.fragment("./gamma/opengl/shaders/directional-light-with-shadow.frag.glsl");
    shaders.directionalShadowcaster.link();

    shaders.directionalShadowcasterView.init();
    shaders.directionalShadowcasterView.vertex("./gamma/opengl/shaders/directional-light-view.vert.glsl");
    shaders.directionalShadowcasterView.fragment("./gamma/opengl/shaders/directional-light-view.frag.glsl");
    shaders.directionalShadowcasterView.link();

    // @todo define remaining shadowcaster shaders

    shaders.skybox.init();
    shaders.skybox.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.skybox.fragment("./gamma/opengl/shaders/skybox.frag.glsl");
    shaders.skybox.link();

    shaders.copyFrame.init();
    shaders.copyFrame.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.copyFrame.fragment("./gamma/opengl/shaders/copy-frame.frag.glsl");
    shaders.copyFrame.link();

    // @todo define different SSR quality levels
    shaders.reflections.init();
    shaders.reflections.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.reflections.fragment("./gamma/opengl/shaders/reflections.frag.glsl");
    shaders.reflections.link();

    shaders.reflectionsDenoise.init();
    shaders.reflectionsDenoise.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    shaders.reflectionsDenoise.fragment("./gamma/opengl/shaders/reflections-denoise.frag.glsl");
    shaders.reflectionsDenoise.link();

    shaders.refractiveGeometry.init();
    shaders.refractiveGeometry.vertex("./gamma/opengl/shaders/geometry.vert.glsl");
    shaders.refractiveGeometry.fragment("./gamma/opengl/shaders/refractive-geometry.frag.glsl");
    shaders.refractiveGeometry.link();

    shaders.refractivePrepass.init();
    shaders.refractivePrepass.vertex("./gamma/opengl/shaders/geometry.vert.glsl");
    shaders.refractivePrepass.fragment("./gamma/opengl/shaders/refractive-prepass.frag.glsl");
    shaders.refractivePrepass.link();

    #if GAMMA_DEVELOPER_MODE
      shaders.gBufferDev.init();
      shaders.gBufferDev.vertex("./gamma/opengl/shaders/quad.vert.glsl");
      shaders.gBufferDev.fragment("./gamma/opengl/shaders/dev/g-buffer.frag.glsl");
      shaders.gBufferDev.link();

      shaders.directionalShadowMapDev.init();
      shaders.directionalShadowMapDev.vertex("./gamma/opengl/shaders/quad.vert.glsl");
      shaders.directionalShadowMapDev.fragment("./gamma/opengl/shaders/dev/directional-shadow-map.frag.glsl");
      shaders.directionalShadowMapDev.link();
    #endif
  }

  void Gm_DestroyRendererResources(RendererBuffers& buffers, RendererShaders& shaders) {
    buffers.gBuffer.destroy();
    buffers.reflections.destroy();
    buffers.post.destroy();

    shaders.geometry.destroy();
    shaders.copyDepth.destroy();
    shaders.pointLight.destroy();
    shaders.directionalLight.destroy();
    shaders.directionalShadowcaster.destroy();
    shaders.directionalShadowcasterView.destroy();
    shaders.skybox.destroy();
    shaders.copyFrame.destroy();
    shaders.reflections.destroy();
    shaders.reflectionsDenoise.destroy();
    shaders.refractiveGeometry.destroy();
    shaders.refractivePrepass.destroy();

    #if GAMMA_DEVELOPER_MODE
      shaders.gBufferDev.destroy();
      shaders.directionalShadowMapDev.destroy();
    #endif
  }
}