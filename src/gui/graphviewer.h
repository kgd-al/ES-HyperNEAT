#ifndef KGD_GRAPHVIEWER_H
#define KGD_GRAPHVIEWER_H

#include <QGraphicsView>

#include "../misc/gvc_wrapper.h"

namespace kgd::gui {

class GraphViewer : public QGraphicsView {
public:
  GraphViewer(QWidget *parent, const QString &type);

  void setGraph (const gvc::Graph &graph);
  void clearGraph (void);

  void ensureFit (void);
  void resizeEvent(QResizeEvent *e);

  void render (const QString &filename);

  template <typename Viewer>
  static void render (const gvc::Graph &graph, const std::string &filename) {
      render<Viewer>(graph, QString::fromStdString(filename));
  }

  template <typename Viewer>
  static void render (const gvc::Graph &graph, const QString &filename) {
    Viewer v;
    v.setGraph(graph);
    v.GraphViewer::render(filename);
  }

private:
  const QString _type;

  virtual const char* gvc_layout (void) const = 0;
  virtual void processGraph (const gvc::Graph &g,
                             const gvc::GraphWrapper &gw) = 0;
};

} // end of namespace kgd::gui

#endif // KGD_GRAPHVIEWER_H
