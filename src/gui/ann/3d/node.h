#ifndef KGD_ANN_3D_GRAPHIC_NODE_H
#define KGD_ANN_2D_GRAPHIC_NODE_H

#include <QVector3D>

#include <Qt3DCore/QEntity>

#include <Qt3DRender/QObjectPicker>

#include <Qt3DExtras/QDiffuseSpecularMaterial>

#include "../../gvcqtinterface.h"

namespace kgd::es_hyperneat::gui::ann3d {
using Entity = Qt3DCore::QEntity;

struct Edge;
class Node : public Entity {
  Q_OBJECT
public:
  static constexpr float RADIUS = .1;

  Node(Agnode_t *node, Entity *parent);
  virtual ~Node (void) = default;

  const QVector3D& pos (void) const {
    return _pos;
  }

  const QVector3D& substratePos (void) const {
    return _spos;
  }

  bool isHovered (void) const {
    return _picker->containsMouse();
  }

  std::vector<Edge*> in, out;

signals:
  void clicked (Node *me);

private:
  Qt3DExtras::QDiffuseSpecularMaterial *_material;
  Qt3DRender::QObjectPicker *_picker;
  QVector3D _pos, _spos;

  void hoverEntered (void);
  void hoverExited (void);
};

} // end of namespace kgd::es_hyperneat::gui::ann3d

#endif // KGD_ANN_2D_GRAPHIC_NODE_H
