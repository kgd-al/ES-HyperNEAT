#ifndef KGD_ANN_3D_GRAPHIC_EDGE_H
#define KGD_ANN_3D_GRAPHIC_EDGE_H

#include <Qt3DCore/QEntity>

#include <Qt3DExtras/QDiffuseSpecularMaterial>

#include "../../gvcqtinterface.h"

namespace kgd::es_hyperneat::gui::ann3d {
using Entity = Qt3DCore::QEntity;

struct Node;
class Edge : public Entity {
public:
  Edge(Agedge_t *edge, Node *i, Node *o, Entity *parent);
  virtual ~Edge (void) = default;

  void setHovered (bool h);

private:
  Node *in, *out;
  QColor _currentColor, _color;

  Qt3DExtras::QDiffuseSpecularMaterial *_material;
};

} // end of namespace kgd::es_hyperneat::gui::ann3d

#endif // KGD_ANN_3D_GRAPHIC_EDGE_H
