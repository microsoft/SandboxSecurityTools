set(RendererProcessHeaderFiles child.h renderer/renderer_process.h)
set(RendererProcessSourceFiles renderer/renderer_process.cc)
add_library(renderer_process STATIC ${RendererProcessHeaderFiles} ${RendererProcessSourceFiles})