#include <memory>
#undef VK_NULL_HANDLE
#define VK_NULL_HANDLE nullptr
#include <imgui_impl_sdl3.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <algorithm>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/matrix.hpp>

#include "PostProcess.hpp"
#include "SkyboxRenderer.hpp"
#include "WaterRenderer.hpp"

class Window : public val::PresentationProvider
{
private:
  SDL_Window *window{};

public:
  Window(Size size, const char *name)
  {
    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS);
    window = SDL_CreateWindow(name, size.w, size.h, SDL_WINDOW_VULKAN);
  }
  ~Window() { SDL_DestroyWindow(window); }

  operator SDL_Window *() { return window; }

  VkSurfaceKHR getSurface(VkInstance instance) override
  {
    VkSurfaceKHR sur;
    SDL_Vulkan_CreateSurface(window, instance, nullptr, &sur);
    return sur;
  }

  Size getSize() override
  {
    int w, h;
    SDL_GetWindowSize(window, &w, &h);
    return {(uint32_t)w, (uint32_t)h};
  }

  void initImgui() override { ImGui_ImplSDL3_InitForVulkan(window); }
};

struct Camera
{
  glm::vec3 position = {0, 0, 0}, dir = {0, 0, 1};
  float fov = 90, w = 1, h = 1;

  void rotateX(float degrees)
  {
    auto angle = glm::radians(degrees / 2.f);
    glm::quat rotation(glm::cos(angle), glm::vec3(0, 1, 0) * glm::sin(angle));
    glm::quat rotationC = glm::conjugate(rotation);

    dir = rotation * dir * rotationC;
  }

  void rotateY(float degrees)
  {
    auto angle = glm::radians(degrees / 2.f);
    glm::quat rotation(glm::cos(angle),
                       glm::normalize(glm::cross(dir, glm::vec3(0, 1, 0))) *
                           glm::sin(angle));
    glm::quat rotationC = glm::conjugate(rotation);

    dir = rotation * dir * rotationC;
  }

  glm::mat4 getView()
  {
    return glm::lookAt(position, dir + position, glm::vec3(0, 1, 0));
  }

  glm::mat4 getProjection()
  {
    auto ret = glm::perspective(glm::radians(fov), w / h, 0.1f, 500.f);
    ret[1][1] *= -1;
    return ret;
  }
};

class InputManager
{
private:
  bool captureMouse = false;
  const Uint8 *state;
  glm::vec2 mouseDelta;
  Window *window;

public:
  InputManager(Window *window) : window(window)
  {
    state = SDL_GetKeyboardState(NULL);
  }

  void handleEvent(SDL_Event &ev)
  {
    if (ev.type == SDL_EVENT_KEY_DOWN)
    {
      if (ev.key.keysym.scancode == SDL_SCANCODE_ESCAPE)
        captureMouse = !captureMouse;
    }
  }

  void update()
  {
    auto winSize = window->getSize();
    glm::vec2 center(winSize.w / 2.f, winSize.h / 2.f);
    if (captureMouse)
    {
      SDL_SetRelativeMouseMode(true);
      glm::vec2 mousePos;
      SDL_GetMouseState(&mousePos.x, &mousePos.y);

      mouseDelta =
          (center - mousePos) / glm::vec2((float)winSize.w, (float)winSize.h);

      SDL_WarpMouseInWindow(*window, center.x, center.y);
    }
    else
    {
      SDL_SetRelativeMouseMode(false);
      mouseDelta = {};
    }
  }

  glm::vec3 getMoveVector()
  {
    glm::vec3 input(0);
    if (state[SDL_SCANCODE_A])
      input.x += -1;
    if (state[SDL_SCANCODE_D])
      input.x += 1;

    if (state[SDL_SCANCODE_W])
      input.z += 1;
    if (state[SDL_SCANCODE_S])
      input.z += -1;

    return input;
  }

  glm::vec2 getMouseDelta() { return mouseDelta; }
};

int main()
{
  Size winsize = {1920, 1080};
  auto win = std::make_unique<Window>(winsize, "My vulkan app!!");
  InputManager input(win.get());

  val::EngineInitConfig init;
  init.features10.tessellationShader = true;
  init.presentation = val::PresentationFormat::Mailbox;
  init.useImGUI = true;

  auto engine = std::make_unique<val::Engine>(init, win.get());
  val::BufferWriter writer(*engine);

  auto framebuffer = engine->createTexture(winsize, val::TextureFormat::RGBA16);
  auto depthbuffer = engine->createTexture(winsize, val::TextureFormat::DEPTH32,
                                           val::TextureSampler::NEAREST, 1);

  auto outputImage = engine->createTexture(winsize, val::TextureFormat::RGBA16);
  bool isOpen = true;

  bool isTrue = true;

  SkyboxRenderer skyboxRenderer(*engine, writer);
  WaterRenderer waterRenderer(*engine, writer);
  PostProcess postProcess(*engine);

  Camera camera;

  camera.dir = glm::normalize(glm::vec3(0, 0, 1));

  camera.position.y = 2;
  camera.position.z = -20;

  auto ticks = SDL_GetTicks();

  WaterMaterial material{};

  material.numFreqs = 65;
  material.baseA = 0.6;
  material.baseW = 0.2;
  material.aMult = 0.8;
  material.wMult = 1.2;
  material.diffuseColor = glm::vec4(0.f, 18.f / 255, 55.f / 255, 0.f);
  material.baseReflectivity = 0.015;
  material.roughness = 0.152;
  material.speed = 2;

  float time = 0;
  while (isOpen)
  {
    auto elapsed = SDL_GetTicks() - ticks;
    ticks = SDL_GetTicks();

    float delta = elapsed / 1000.f;
    time += delta;
    SDL_Event ev;

    while (SDL_PollEvent(&ev))
    {
      switch (ev.type)
      {
      case SDL_EVENT_QUIT:
        isOpen = false;
        break;
      }
      ImGui_ImplSDL3_ProcessEvent(&ev);
      input.handleEvent(ev);
    }

    engine->update();
    input.update();

    const float speed = 15;

    auto forward = camera.dir;
    auto right = glm::normalize(glm::cross(camera.dir, glm::vec3(0, 1, 0)));

    auto inputMove = input.getMoveVector();

    camera.position += forward * inputMove.z * speed * delta +
                       right * inputMove.x * speed * delta;

    camera.rotateX(input.getMouseDelta().x * 30);
    camera.rotateY(input.getMouseDelta().y * 30);

    ImGui_ImplVulkan_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("vkRaster", &isTrue);

    ImGui::Text("FPS: %d", (uint32_t)(1 / std::max(delta, 0.0001f)));

    if (ImGui::Button("Open sea")) {
      material.numFreqs = 65;
      material.baseA = 0.6;
      material.baseW = 0.2;
      material.aMult = 0.8;
      material.wMult = 1.2;
      material.diffuseColor = glm::vec4(0.f, 18.f / 255, 55.f / 255, 0.f);
      material.baseReflectivity = 0.015;
      material.roughness = 0.152;
      material.speed = 2;
    }

    if (ImGui::Button("Calm lake")) {
      material.numFreqs = 12;
      material.baseA = 0.06;
      material.baseW = 0.5;
      material.aMult = 0.8;
      material.wMult = 1.2;
      material.diffuseColor = glm::vec4(0.f, 54.f / 255, 89.f / 255, 0.f);
      material.baseReflectivity = 0.02;
      material.roughness = 0.06;
      material.speed = 0.9;
    }

    // Draw material params

    ImGui::SliderInt("Number of waves", (int *)&material.numFreqs, 1, 128);

    ImGui::SliderFloat("A", &material.baseA, 0, 1);
    ImGui::InputFloat("W", &material.baseW);

    ImGui::InputFloat("A Mult", &material.aMult);
    ImGui::InputFloat("W Mult", &material.wMult);

    ImGui::ColorPicker3("Diffuse color", glm::value_ptr(material.diffuseColor));

    ImGui::InputFloat("F0", &material.baseReflectivity);

    ImGui::SliderFloat("Roughness", &material.roughness, 0, 1);

    ImGui::InputFloat("Speed", &material.speed);

    ImGui::End();

    ImGui::Render();

    waterRenderer.updateMaterial(material);

    auto cmd = engine->initFrame();

    if (cmd.isValid())
    {

      writer.updateWrites(cmd);

      RenderState rs;
      rs.cmd = &cmd;
      rs.colorBuffer = framebuffer;
      rs.depthBuffer = depthbuffer;
      rs.projectionMatrix = camera.getProjection();
      rs.viewMatrix = camera.getView();
      rs.camPos = camera.position;
      rs.camDir = camera.dir;
      rs.time = time;
      rs.ambientMap = skyboxRenderer.getSkybox();

      cmd.transitionTexture(framebuffer, vk::ImageLayout::eUndefined,
                            vk::ImageLayout::eColorAttachmentOptimal);

      waterRenderer.generatePatches(rs);
      skyboxRenderer.renderSkybox(rs);

      cmd.memoryBarrier(vk::PipelineStageFlagBits2::eComputeShader,
                        vk::AccessFlagBits2::eMemoryWrite,
                        vk::PipelineStageFlagBits2::eVertexAttributeInput,
                        vk::AccessFlagBits2::eMemoryRead |
                            vk::AccessFlagBits2::eMemoryWrite);

      cmd.memoryBarrier(vk::PipelineStageFlagBits2::eLateFragmentTests,
                        vk::AccessFlagBits2::eMemoryWrite,
                        vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                        vk::AccessFlagBits2::eMemoryWrite);
      waterRenderer.renderWater(rs);

      cmd.memoryBarrier(vk::PipelineStageFlagBits2::eLateFragmentTests,
                        vk::AccessFlagBits2::eMemoryWrite,
                        vk::PipelineStageFlagBits2::eEarlyFragmentTests,
                        vk::AccessFlagBits2::eMemoryRead);

      postProcess.renderPostProcess(rs, outputImage);

      engine->submitFrame(outputImage);
    }
  }

  engine->waitFinishAllCommands();

  return 0;
}
