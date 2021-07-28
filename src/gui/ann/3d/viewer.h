#ifndef KGD_ANN_3D_VIEWER_H
#define KGD_ANN_3D_VIEWER_H

#include <Qt3DCore/QEntity>
#include <Qt3DExtras/Qt3DWindow>
#include <Qt3DExtras/QOrbitCameraController>

#include "../../../phenotype/ann.h"
#include "../../../misc/gvc_wrapper.h"

#if ESHN_SUBSTRATE_DIMENSION == 3
namespace kgd::es_hyperneat::gui::ann3d {

struct Node;
struct Edge;

struct CameraController;
class Viewer : public Qt3DExtras::Qt3DWindow {
  Q_OBJECT
public:
  Viewer(void);
  ~Viewer (void);

  void setANN (const phenotype::ANN &ann);
  void clearANN (void);

  void startAnimation (void);
  void updateAnimation (void);
  void stopAnimation (void);

  bool isAnimating (void) const {
    return _animating;
  }

  void depthDebugDraw (bool active);

signals:
  void neuronHovered (const phenotype::ANN::Neuron *n);

private:
  using Entity = Qt3DCore::QEntity;
  Entity *_scene = nullptr, *_axis = nullptr;

  QVector<Node*> _nodes;
  QVector<Edge*> _edges;

  CameraController *_manipulator = nullptr;

  Node *_selection = nullptr;
  Entity *_selectionHighlighter = nullptr;

  bool _animating;

  const phenotype::ANN *_ann;

  static Entity* buildAxis (Entity *parent);
  static Entity* buildSelectionHighlighter (Entity *parent);

  void processGraph (const gvc::Graph &g,
                     const gvc::GraphWrapper &gw) /*override*/;

  void selectionChanged (Node *n);

  void keyReleaseEvent(QKeyEvent *e) override;
  void mouseDoubleClickEvent(QMouseEvent *ev) override;
};

} // end of namespace kgd::es_hyperneat::gui::ann3d
#endif

#endif // KGD_ANN_3D_VIEWER_H
