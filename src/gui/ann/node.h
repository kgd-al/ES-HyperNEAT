#ifndef KGD_ANN_GRAPHIC_NODE_H
#define KGD_ANN_GRAPHIC_NODE_H

#include <QGraphicsObject>
#include <QTextDocument>

#include "../gvcqtinterface.h"
#include "../../phenotype/ann.h"
#include "../config.h"

namespace kgd::es_hyperneat::gui::ann {

struct NeuralData {
  using Point = phenotype::ANN::Point;
  using Neuron = phenotype::ANN::Neuron;

  virtual ~NeuralData (void) {}

  virtual const Neuron* neuron (void) const = 0;
  virtual const Point& pos (void) const = 0;
  virtual Neuron::Type type (void) const = 0;
  virtual Neuron::Flags_t flags (void) const = 0;
  virtual float value (void) const = 0;
};

struct Edge;
class Node : public QGraphicsObject {
  Q_OBJECT
public:
  using Point = NeuralData::Point;
  using Neuron = NeuralData::Neuron;
  using Data_ptr = std::unique_ptr<NeuralData>;

  Node (Agnode_t *node, NeuralData *ndata, qreal scale);

  QRectF boundingRect(void) const override {    return _bounds; }

#ifndef NDEBUG
  const QString& name (void) const {  return _name; }

  friend std::ostream& operator<< (std::ostream &os, const Node &n);
#endif

  const Neuron* neuron (void) const { return _ndata->neuron(); }
  const Point& substratePosition (void) const { return _ndata->pos(); }
  auto ntype (void) const { return _ndata->type(); }
  auto flags (void) const { return _ndata->flags(); }

  void updateAnimation (bool running);

  void updateCustomColors (void);

  void paint (QPainter *painter,
              const QStyleOptionGraphicsItem*, QWidget*) override;

  void hoverEnterEvent(QGraphicsSceneHoverEvent *e) override;
  void hoverLeaveEvent(QGraphicsSceneHoverEvent *e) override;

  std::vector<Edge*> in, out;

signals:
  void hovered (const Neuron *n);

private:
#ifndef NDEBUG
  QString _name;
#endif

//  const Neuron &_neuron;
  Data_ptr _ndata;

  QRectF _bounds, _shape;
  bool _hovered;

  QColor _fill;
  QTextDocument _label;

  bool _srecurrent;

  QColor _currentColor;
  config::ESHNGui::CustomColors _customColors;

  void drawRichText(QPainter *painter);
};

} // end of namespace kgd::es_hyperneat::gui::ann

#endif // KGD_ANN_GRAPHIC_NODE_H
