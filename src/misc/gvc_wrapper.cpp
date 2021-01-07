#include <memory>

#include "gvc_wrapper.h"

#include <iostream>

namespace gvc {

GVC_t* context(void) {
  static const auto c = std::unique_ptr<GVC_t, decltype(&gvFreeContext)>(
        gvContext(), &gvFreeContext);
  return c.get();
}

GraphWrapper::GraphWrapper (const char *name) {
  auto str = toCString(name);
  graph = agopen(str.get(), Agdirected, NULL);

  layedOut = false;
}

GraphWrapper::~GraphWrapper (void) {
  if (layedOut) freeLayout();
  agclose(graph);
}

void GraphWrapper::layout (const char *engine) {
  std::cerr << "Allocating gw layout for " << this << "\n";
  gvLayout(context(), graph, engine);
  layedOut = true;
}

void GraphWrapper::freeLayout(void) {
  std::cerr << ">>>> Freed gw layout for " << this << "\n";
  gvFreeLayout(context(), graph);
  layedOut = false;
}

} // end of namespace gvc
