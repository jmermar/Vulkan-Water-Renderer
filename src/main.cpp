#include <memory>

#include "SkyboxRenderer.hpp"
#include <imgui_impl_sdl3.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/matrix.hpp>

class Window : public val::PresentationProvider {
private:
  SDL_Window *window{};

public:
  Window(Size size, const char *name) {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    window = SDL_CreateWindow(name, size.w, size.h, SDL_WINDOW_VULKAN);
  }
  ~Window() { SDL_DestroyWindow(window); }

  operator SDL_Window *() { return window; }

  VkSurfaceKHR getSurface(VkInstance instance) override {
    VkSurfaceKHR sur;
    SDL_Vulkan_CreateSurface(window, instance, nullptr, &sur);
    return sur;
  }

  Size getSize() override {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return {(uint32_t)w, (uint32_t)h};
  }

  void initImgui() override { ImGui_ImplSDL3_InitForVulkan(window); }
};

struct Camera {
  glm::vec3 position = {0, 0, 0}, dir = {0, 0, 1};
  float fov = 90, w = 1, h = 1;

  void rotateX(float degrees) {
    auto angle = glm::radians(degrees / 2.f);
    glm::quat rotation(glm::cos(angle), glm::vec3(0, 1, 0) * glm::sin(angle));
    glm::quat rotationC = glm::conjugate(rotation);

    dir = rotation * dir * rotationC;
  }

  void rotateY(float degrees) {
    auto angle = glm::radians(degrees / 2.f);
    glm::quat rotation(glm::cos(angle),
                       glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0))) *
                           glm::sin(angle));
    glm::quat rotationC = glm::conjugate(rotation);

    dir = rotation * dir * rotationC;
  }

  glm::mat4 getView() {
    return glm::lookAt(position, dir + position, glm::vec3(0, 1, 0));
  }

  glm::mat4 getProjection() {
    auto ret = glm::perspective(glm::radians(fov), w / h, 0.01f, 3000.f);
    ret[1][1] *= -1;
    return ret;
  }
};

int main() {
  Size winsize = {1280, 720};
  auto win = std::make_unique<Window>(winsize, "My vulkan app!!");

  val::EngineInitConfig init;
  init.presentation = val::PresentationFormat::Fifo;
  init.useImGUI = true;

  auto engine = std::make_unique<val::Engine>(init, win.get());
  val::BufferWriter writer(*engine);

  auto framebuffer = engine->createTexture(winsize, val::TextureFormat::RGBA16);
  bool isOpen = true;

  bool isTrue = true;

  SkyboxRenderer skyboxRenderer(*engine, writer);

  Camera camera;

  camera.dir = glm::normalize(glm::vec3(0, -0.5, 1));

  while (isOpen) {
    SDL_Event ev;

    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_EVENT_QUIT:
        isOpen = false;
        break;
      }
      ImGui_ImplSDL3_ProcessEvent(&ev);
    }

    engine->update();

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("vkRaster", &isTrue);

    if (ImGui::Button("Exit")) {
      isOpen = false;
    }

    ImGui::End();

    ImGui::Render();

    auto cmd = engine->initFrame();

    if (cmd.isValid()) {

      writer.updateWrites(cmd);

      RenderState rs;
      rs.cmd = &cmd;
      rs.colorBuffer = framebuffer;
      rs.depthBuffer = nullptr;
      rs.projectionMatrix = camera.getProjection();
      rs.viewMatrix = camera.getView();

      cmd.transitionTexture(framebuffer, vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal);

      skyboxRenderer.renderSkybox(rs);

      engine->submitFrame(framebuffer);
    }
  }

  engine->waitFinishAllCommands();

  return 0;
}
