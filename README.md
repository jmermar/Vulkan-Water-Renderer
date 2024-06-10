# Vulkan Water Renderer

![](screenshots/screenshot.gif)

WIP water rendering using periodic functions and Fractional Brownian Motion.

I'm basing this project in [chapter 1 of GPU Gems](https://developer.nvidia.com/gpugems/gpugems/contributors) where they explain a common real time water rendering approach.

For wave generation there are two functions, one being the wave height calculation, called per vertex and other being the normal generation, called per fragment in order to increase image details in water without adding a lot of vertices.

For vertex processing tessellation is also used, a compute shader will generate water patches plane surrounding the player FOV and the tessellation shaders will increase resolution the closer the patches are to camera.

It currently uses environment maps for reflections and bling-phong model for illumination.
