#ifndef KGD_ANN_3D_VIEWER_H
#define KGD_ANN_3D_VIEWER_H

#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QOrbitCameraController>

#include "../../../phenotype/ann.h"
#include "../../../misc/gvc_wrapper.h"

namespace kgd::es_hyperneat::gui::ann3d {

struct Node;
struct CameraController;
class Viewer : public Qt3DExtras::Qt3DWindow {
public:
  Viewer(void);
  ~Viewer (void);

//  void processGraph (void);
  void setGraph (const gvc::Graph &graph);

private:
  using Entity = Qt3DCore::QEntity;
  Entity *_scene = nullptr, *_axis = nullptr;

  CameraController *_manipulator = nullptr;

  Node *_selection = nullptr;
  Entity *_selectionHighlighter = nullptr;

  static Entity* buildAxis (Entity *parent);
  static Entity* buildSelectionHighlighter (Entity *parent);

  void processGraph (const gvc::Graph &g,
                     const gvc::GraphWrapper &gw) /*override*/;

  void selectionChanged (Node *n);

  void keyReleaseEvent(QKeyEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *ev) override;
};

} // end of namespace kgd::es_hyperneat::gui::ann3d

#endif // KGD_ANN_3D_VIEWER_H
