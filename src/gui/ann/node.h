#ifndef KGD_ANN_GRAPHIC_NODE_H
#define KGD_ANN_GRAPHIC_NODE_H

#include <QGraphicsObject>
#include <QTextDocument>

#include "../gvcqtinterface.h"
#include "../../phenotype/ann.h"

namespace kgd::es_hyperneat::gui::ann {

struct Edge;
class Node : public QGraphicsObject {
  Q_OBJECT
public:
  using Point = phenotype::ANN::Point;
  using Neuron = phenotype::ANN::Neuron;
  using CustomColors = QVector<QColor>;

  Node (Agnode_t *node, const Neuron &neuron, qreal scale);

  QRectF boundingRect(void) const override {    return _bounds; }

#ifndef NDEBUG
  const QString& name (void) const {  return _name; }

  friend std::ostream& operator<< (std::ostream &os, const Node &n);
#endif

  const Neuron& neuron (void) const { return _neuron; }
  const Point& substratePosition (void) const { return _neuron.pos; }

  void updateAnimation (bool running);

  void setCustomColors (const CustomColors &c);
  void clearCustomColors (void);
  const auto& customColors (void) const {
    return _customColors;
  }

  void paint (QPainter *painter,
              const QStyleOptionGraphicsItem*, QWidget*) override;

  void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;

  std::vector<Edge*> in, out;

signals:
  void hovered (const Neuron &n);

private:
#ifndef NDEBUG
  QString _name;
#endif

  const Neuron &_neuron;

  QRectF _bounds, _shape;
  bool _hovered;

  QColor _fill;
  QTextDocument _label;

  bool _srecurrent;

  QColor _currentColor;
  CustomColors _customColors;

  void drawRichText(QPainter *painter);
};

} // end of namespace kgd::es_hyperneat::gui::ann

#endif // KGD_ANN_GRAPHIC_NODE_H
