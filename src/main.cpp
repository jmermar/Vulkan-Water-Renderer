#include <memory>

#include <val/vulkan_abstraction.hpp>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

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
};

int main() {
  Size winsize = {640, 480};
  auto win = std::make_unique<Window>(winsize, "My vulkan app!!");

  val::EngineInitConfig init;
  init.presentation = val::PresentationFormat::Fifo;

  auto engine = std::make_unique<val::Engine>(init, win.get());
  val::BufferWriter writer(*engine);

  auto image = file::loadImage("tex.jpg");

  auto texture = engine->createTexture(image.size, val::TextureFormat::RGBA8);
  writer.enqueueTextureWrite(texture, image.data.data());

  bool isOpen = true;

  while (isOpen) {
    SDL_Event ev;

    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_EVENT_QUIT:
        isOpen = false;
        break;
      }
    }

    engine->update();

    auto cmd = engine->initFrame();

    if (cmd.isValid()) {
      writer.updateWrites(cmd);

      engine->submitFrame(texture);
    }
  }

  engine->waitFinishAllCommands();

  return 0;
}
