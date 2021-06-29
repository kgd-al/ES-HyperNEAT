#ifndef KGD_ANN_3D_GRAPHIC_NODE_H
#define KGD_ANN_2D_GRAPHIC_NODE_H

#include <QVector3D>

#include <Qt3DCore/QEntity>

#include <Qt3DRender/QObjectPicker>

#include <Qt3DExtras/QDiffuseSpecularMaterial>

#include "../../../phenotype/ann.h"

#if ESHN_SUBSTRATE_DIMENSION == 3
namespace kgd::es_hyperneat::gui::ann3d {
using Entity = Qt3DCore::QEntity;

QVector3D toQt3D (const phenotype::Point &p);

struct Edge;
class Node : public Entity {
  Q_OBJECT
public:
  static constexpr float RADIUS = .05;

  Node(const phenotype::ANN::Neuron &n, Entity *parent);
  virtual ~Node (void) = default;

  const QVector3D& pos (void) const {
    return _pos;
  }

  bool isHovered (void) const {
    return _picker->containsMouse();
  }

  void setSelected (bool s);
  bool isSelected (void) const {
    return _selected;
  }

  std::vector<Edge*> in, out;

signals:
  void clicked (Node *me);
  void hovered (const phenotype::ANN::Neuron *n);

private:
  const phenotype::ANN::Neuron &_neuron;

  Qt3DExtras::QDiffuseSpecularMaterial *_material;
  Qt3DRender::QObjectPicker *_picker;
  QVector3D _pos;
  QColor _color;
  bool _selected, _hovered, _highlighted;

  void hoverEntered (void);
  void hoverExited (void);

  void setHighlighted (void);
};

} // end of namespace kgd::es_hyperneat::gui::ann3d
#endif

#endif // KGD_ANN_2D_GRAPHIC_NODE_H
