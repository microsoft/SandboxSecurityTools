set(ESTTProcessHeaderFiles sandbox.h renderer/renderer.h)
set(ESTTProcessSourceFiles sandbox.cc renderer/renderer.cc)
add_library(estt_core STATIC ${ESTTProcessHeaderFiles} ${ESTTProcessSourceFiles})