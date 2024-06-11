#include "types.hpp"

class PostProcess {
private:
  val::Engine &engine;
  val::GraphicsPipeline pipeline;

public:
  PostProcess(val::Engine &engine);

  void renderPostProcess(RenderState &rs, val::Texture *finalImage);
};