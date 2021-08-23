#include "opengl/renderer_helpers.h"
#include "system/flags.h"

namespace Gamma {
  void Gm_InitDeferredRenderPath(DeferredPath& deferred, const Area<uint32>& internalResolution) {
    // Initialize buffers
    deferred.buffers.gBuffer.init();
    deferred.buffers.gBuffer.setSize(internalResolution);
    deferred.buffers.gBuffer.addColorAttachment(ColorFormat::RGBA);    // (RGB) Albedo, (A) Depth
    deferred.buffers.gBuffer.addColorAttachment(ColorFormat::RGBA16);  // (RGB) Normal, (A) Specularity
    deferred.buffers.gBuffer.addColorAttachment(ColorFormat::RGBA);    // (RGB) Accumulated effects color, (A) Depth
    deferred.buffers.gBuffer.addDepthStencilAttachment();
    deferred.buffers.gBuffer.bindColorAttachments();

    deferred.buffers.post.init();
    deferred.buffers.post.setSize(internalResolution);
    deferred.buffers.post.addColorAttachment(ColorFormat::RGBA);  // (RGB) Color, (A) Depth
    deferred.buffers.gBuffer.shareDepthStencilAttachment(deferred.buffers.post);
    deferred.buffers.post.bindColorAttachments();

    deferred.buffers.reflections.init();
    deferred.buffers.reflections.setSize({ internalResolution.width, internalResolution.height });
    deferred.buffers.reflections.addColorAttachment(ColorFormat::RGBA);
    deferred.buffers.gBuffer.shareDepthStencilAttachment(deferred.buffers.reflections);
    deferred.buffers.reflections.bindColorAttachments();

    // deferred.buffers.reflections.init();
    // deferred.buffers.reflections.setSize({ internalResolution.width / 2, internalResolution.height /2 });
    // deferred.buffers.reflections.addColorAttachment(ColorFormat::RGBA);
    // deferred.buffers.reflections.bindColorAttachments();

    // Initialize shaders
    deferred.shaders.geometry.init();
    deferred.shaders.geometry.vertex("./gamma/opengl/shaders/deferred/geometry.vert.glsl");
    deferred.shaders.geometry.fragment("./gamma/opengl/shaders/deferred/geometry.frag.glsl");
    deferred.shaders.geometry.link();

    deferred.shaders.copyDepth.init();
    deferred.shaders.copyDepth.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    deferred.shaders.copyDepth.fragment("./gamma/opengl/shaders/deferred/copy-depth.frag.glsl");
    deferred.shaders.copyDepth.link();

    deferred.shaders.pointLight.init();
    deferred.shaders.pointLight.vertex("./gamma/opengl/shaders/deferred/instanced-light.vert.glsl");
    deferred.shaders.pointLight.fragment("./gamma/opengl/shaders/deferred/point-light-without-shadow.frag.glsl");
    deferred.shaders.pointLight.link();

    deferred.shaders.directionalLight.init();
    deferred.shaders.directionalLight.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    deferred.shaders.directionalLight.fragment("./gamma/opengl/shaders/deferred/directional-light-without-shadow.frag.glsl");
    deferred.shaders.directionalLight.link();

    deferred.shaders.directionalShadowcaster.init();
    deferred.shaders.directionalShadowcaster.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    deferred.shaders.directionalShadowcaster.fragment("./gamma/opengl/shaders/deferred/directional-light-with-shadow.frag.glsl");
    deferred.shaders.directionalShadowcaster.link();

    deferred.shaders.directionalShadowcasterView.init();
    deferred.shaders.directionalShadowcasterView.vertex("./gamma/opengl/shaders/directional-light-view.vert.glsl");
    deferred.shaders.directionalShadowcasterView.fragment("./gamma/opengl/shaders/directional-light-view.frag.glsl");
    deferred.shaders.directionalShadowcasterView.link();

    // @todo define remaining shadowcaster shaders

    deferred.shaders.skybox.init();
    deferred.shaders.skybox.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    deferred.shaders.skybox.fragment("./gamma/opengl/shaders/deferred/skybox.frag.glsl");
    deferred.shaders.skybox.link();

    deferred.shaders.copyFrame.init();
    deferred.shaders.copyFrame.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    deferred.shaders.copyFrame.fragment("./gamma/opengl/shaders/deferred/copy-frame.frag.glsl");
    deferred.shaders.copyFrame.link();

    // @todo define different SSR quality levels
    deferred.shaders.reflections.init();
    deferred.shaders.reflections.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    deferred.shaders.reflections.fragment("./gamma/opengl/shaders/deferred/reflections.frag.glsl");
    deferred.shaders.reflections.link();

    deferred.shaders.reflectionsDenoise.init();
    deferred.shaders.reflectionsDenoise.vertex("./gamma/opengl/shaders/quad.vert.glsl");
    deferred.shaders.reflectionsDenoise.fragment("./gamma/opengl/shaders/deferred/reflections-denoise.frag.glsl");
    deferred.shaders.reflectionsDenoise.link();

    deferred.shaders.refractiveGeometry.init();
    deferred.shaders.refractiveGeometry.vertex("./gamma/opengl/shaders/deferred/geometry.vert.glsl");
    deferred.shaders.refractiveGeometry.fragment("./gamma/opengl/shaders/deferred/refractive-geometry.frag.glsl");
    deferred.shaders.refractiveGeometry.link();

    deferred.shaders.refractivePrepass.init();
    deferred.shaders.refractivePrepass.vertex("./gamma/opengl/shaders/deferred/geometry.vert.glsl");
    deferred.shaders.refractivePrepass.fragment("./gamma/opengl/shaders/deferred/refractive-prepass.frag.glsl");
    deferred.shaders.refractivePrepass.link();

    #if GAMMA_DEVELOPER_MODE
      deferred.shaders.gBufferDebug.init();
      deferred.shaders.gBufferDebug.vertex("./gamma/opengl/shaders/quad.vert.glsl");
      deferred.shaders.gBufferDebug.fragment("./gamma/opengl/shaders/debug/g-buffer.frag.glsl");
      deferred.shaders.gBufferDebug.link();

      deferred.shaders.directionalShadowMapDebug.init();
      deferred.shaders.directionalShadowMapDebug.vertex("./gamma/opengl/shaders/quad.vert.glsl");
      deferred.shaders.directionalShadowMapDebug.fragment("./gamma/opengl/shaders/debug/directional-shadow-map.frag.glsl");
      deferred.shaders.directionalShadowMapDebug.link();
    #endif

    // Initialize remaining components
    deferred.lightDisc.init();
  }

  void Gm_DestroyDeferredRenderPath(DeferredPath& deferred) {
    deferred.buffers.gBuffer.destroy();
    deferred.buffers.reflections.destroy();
    deferred.buffers.post.destroy();

    deferred.shaders.geometry.destroy();
    // deferred.shaders.emissives.destroy();
    deferred.shaders.copyDepth.destroy();
    deferred.shaders.pointLight.destroy();
    // deferred.shaders.pointShadowcaster.destroy();
    // deferred.shaders.pointShadowcasterView.destroy();
    deferred.shaders.directionalLight.destroy();
    deferred.shaders.directionalShadowcaster.destroy();
    deferred.shaders.directionalShadowcasterView.destroy();
    deferred.shaders.skybox.destroy();
    deferred.shaders.copyFrame.destroy();
    deferred.shaders.reflections.destroy();
    deferred.shaders.reflectionsDenoise.destroy();
    deferred.shaders.refractiveGeometry.destroy();
    deferred.shaders.refractivePrepass.destroy();

    #if GAMMA_DEVELOPER_MODE
      deferred.shaders.gBufferDebug.destroy();
      deferred.shaders.directionalShadowMapDebug.destroy();
    #endif

    deferred.lightDisc.destroy();
  }
}