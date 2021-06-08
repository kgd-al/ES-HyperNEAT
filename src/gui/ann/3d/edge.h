#ifndef KGD_ANN_3D_GRAPHIC_EDGE_H
#define KGD_ANN_3D_GRAPHIC_EDGE_H

#include <Qt3DCore/QEntity>

namespace kgd::es_hyperneat::gui::ann3d {
using Entity = Qt3DCore::QEntity;

struct Node;
class Edge : public Entity {
public:
  Edge(const QVector3D &src, const QVector3D &dst, Entity *parent);
  virtual ~Edge (void) = default;

  void updateIO (Node *i, Node *o);

private:
  Node *in, *out;
};

} // end of namespace kgd::es_hyperneat::gui::ann3d

#endif // KGD_ANN_3D_GRAPHIC_EDGE_H
