#ifndef KGD_GVC_WRAPPER_HPP
#define KGD_GVC_WRAPPER_HPP

#ifdef WITH_GVC

#include <sstream>
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif
#include <graphviz/gvc.h>

#undef TRUE
#undef FALSE

#ifdef __cplusplus
}
#endif

namespace gvc {

GVC_t* context (void);

template <typename ...ARGS>
auto toCString (ARGS... args) {
  std::ostringstream oss;
  ((oss << std::forward<ARGS>(args)), ...);
  auto str = oss.str();
  auto raw = std::unique_ptr<char[]>(new char [str.length()+1]);
  strcpy(raw.get(), str.c_str());
  return raw;
}

template <typename... ARGS>
auto add_node(Agraph_t *g, ARGS... args) {
  auto str = toCString(args...);
  return agnode(g, str.get(), 1);
}

template <typename ...ARGS>
auto add_edge(Agraph_t *g, Agnode_t *src, Agnode_t *dst,
                       ARGS... args) {
  auto str = toCString(args...);
  return agedge(g, src, dst, str.get(), 1);
}

template <typename T>
auto add_graph(Agraph_t *g, const T &name) {
  auto str = toCString(name);
  return agsubg(g, str.get(), 1);
}

template <typename O, typename K>
auto get (O *o, const K &key) {
  auto str = toCString(key);
  auto res = agget(o, str.get());
  if (!res) return std::string();
  return std::string (res);
}

template <typename O, typename K, typename R>
R get (O *o, const K &key, const R &def) {
  R value;
  if (!(std::istringstream(get(o, key)) >> value))  return def;
  return value;
}

template <typename O, typename K, typename ...ARGS>
auto set(O *o, const K &key, ARGS... args) {
  auto k_str = toCString(key), v_str = toCString(args...),
       v_def = toCString("");
  agsafeset(o, k_str.get(), v_str.get(), v_def.get());
}

template <typename O, typename K, typename ...ARGS>
auto set_html(Agraph_t *g, O *o, const K &key, ARGS... args) {
  auto s = gvc::toCString(args...);
  char *l = agstrdup_html(g, s.get());
  auto k = gvc::toCString(key);
  auto d = gvc::toCString("");
  agsafeset(o, k.get(), l, d.get());
  agstrfree(g, l);
}

struct GraphWrapper {
  Agraph_t *graph;
  bool layedOut;

  GraphWrapper(const char *name);
  ~GraphWrapper(void);

  void layout (const char *engine);
  void freeLayout (void);

  template <typename... ARGS>
  auto add_node (ARGS... args) {  return gvc::add_node(graph, args...); }

  template <typename... ARGS>
  auto add_edge (Agnode_t *src, Agnode_t *dst, ARGS... args) {
    return gvc::add_edge(graph, src, dst, args...);
  }

  template <typename T>
  auto add_graph (const T &name) {  return gvc::add_graph(graph, name);  }
};

struct Graph {
  static constexpr float scale = 100;

  virtual gvc::GraphWrapper build_gvc_graph (const char *ext = "png") const = 0;
  virtual void render_gvc_graph(const std::string &path) const = 0;
};

} // end of namespace gvc
#endif

#endif // KGD_GVC_WRAPPER_HPP
