#ifndef KGD_ANN_3D_GRAPHIC_EDGE_H
#define KGD_ANN_3D_GRAPHIC_EDGE_H

#include <Qt3DCore/QEntity>

#include <Qt3DExtras/QDiffuseSpecularMaterial>

#if ESHN_SUBSTRATE_DIMENSION == 3
namespace kgd::es_hyperneat::gui::ann3d {
using Entity = Qt3DCore::QEntity;

Qt3DRender::QGeometryRenderer* arrowMesh (Qt3DCore::QNode *parent,
                                          const QVector3D &src,
                                          const QVector3D &dst,
                                          float width, float length);

struct Node;
class Edge : public Entity {
public:
  Edge(Node *i, Node *o, Entity *parent);
  virtual ~Edge (void) = default;

  void setVisible (bool v);

private:
  Node *in, *out;

  Qt3DExtras::QDiffuseSpecularMaterial *_material;
};

} // end of namespace kgd::es_hyperneat::gui::ann3d
#endif

#endif // KGD_ANN_3D_GRAPHIC_EDGE_H
