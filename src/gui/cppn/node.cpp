#include <QTextOption>
#include <QPainter>
#include <QAbstractTextDocumentLayout>
#include <QGraphicsScene>

#include "node.h"
#include "../../phenotype/cppn.h"

#include <QDebug>

namespace gui::cppn {

using NodeFunc = genotype::ES_HyperNEAT::CPPN::Node::FuncID;

namespace autoplot {

static constexpr float X_MAX = 5, Y_MAX = 1, Y_SCALE = 7*X_MAX/10,
                       Y_MAX_SCALED = Y_MAX * Y_SCALE;
static const QPainterPath arrowHead = [] {
  static constexpr float dx = X_MAX*.15, dy = Y_MAX, a = .25*dy;
  QPainterPath p;
  QPointF h (0.f, -Y_MAX_SCALED*1.25f-.5*dy);
  p.moveTo(h+QPointF(+dx, dy));
  p.lineTo(h);
  p.lineTo(h+QPointF(-dx, dy));
  p.lineTo(h+QPointF(0, dy-a));
  p.closeSubpath();
  return p;
}();

void basePainter (QPainter *p) {
  p->drawLine(QPointF(-X_MAX, 0), QPointF(X_MAX, 0));
  p->drawLine(QPointF(0, -Y_MAX_SCALED*1.25), QPointF(0, Y_MAX_SCALED*1.25));
  p->fillPath(arrowHead, p->pen().color());
}

static constexpr uint PLOT_STEPS = 50;
const auto autoplotter = [] (phenotype::CPPN::Function f) {
  QVector<QPointF> points;
  points.reserve(PLOT_STEPS);
  for (uint i=0; i<PLOT_STEPS; i++) {
    float x = X_MAX*(2*(float(i) / (PLOT_STEPS-1))-1);
    points.push_back({x,-f(x)*Y_SCALE});
  }
  QPainterPath p;
  p.addPolygon(QPolygonF(points));
  return p;
};

const auto mapBuilder = [] {
  std::map<NodeFunc, QPainterPath> m;
  for (const auto &p: phenotype::CPPN::functions)
    m[p.first] = autoplotter(p.second);
  return m;
};

QPainterPath defaultPath;
const QPainterPath* functionPaths (const NodeFunc &f) {
  static const auto m = mapBuilder();
  return &m.at(f);
}

} // end of namespace autoplot

const QTextOption TextAspect = QTextOption(Qt::AlignCenter);

const QFont BaseFont { "Courrier", 10 };
const QFont NodeFont { BaseFont.family(), BaseFont.pointSize()};
const QFont EdgeFont { BaseFont.family(), int(BaseFont.pointSize() * .8)};

static constexpr int MARGIN = 1;

Node::Node (Agnode_t *node, qreal scale) {
  _name = QString(agnameof(node));

  auto b = ND_bb(node);
  _bounds = toQt(b, scale);
  setPos(_bounds.center());
  _bounds.translate(-_bounds.center());
  _shape = _bounds;
  _shape.adjust(+MARGIN, +MARGIN, -MARGIN, -MARGIN);

  _border = (gvc::get(node, "shape", std::string()) != "plain");

  QString label (ND_label(node)->text);
  if (!label.isEmpty()) {
    if (label.indexOf('_') != -1) {
      label.replace('_', "<sub>");
      label.append("</sub>");
    }
    _label.setHtml(label);
    _label.setDefaultTextOption(QTextOption(Qt::AlignCenter));

    QFont font = NodeFont;
    font.setPointSize(gvc::get(node, "fontsize", 10));
    _label.setDefaultFont(font);
  }

  auto fID = NodeFunc (gvc::get(node, "kgdfunc", std::string()));
  _fPath = (fID.empty() ? nullptr : autoplot::functionPaths(fID));

  if (_fPath && !label.isEmpty()) {
    // Node has both a label and a function (i.e. should be an output)
    // -> Make some space below for the text

    QFontMetrics fm (NodeFont);
    _bounds.adjust(0, 0, 0, fm.height()*1.25);
  }
}


void Node::paint (QPainter *painter, const QStyleOptionGraphicsItem*,
                      QWidget*) {

  if (_fPath) {
    painter->save();
      QPainterPath cpath;
      cpath.addEllipse(_shape);
      painter->setClipPath(cpath);

      float m = .05;
      float scale = (.5 - m) * _shape.width() / autoplot::X_MAX;
      QPen p = painter->pen();      
      auto pwidth = p.widthF() / scale;
      p.setColor(Qt::black);
      p.setWidthF(pwidth*1.5);
      painter->setPen(p);
      painter->scale(scale, scale);

      if (_fPath->boundingRect().height() <= autoplot::Y_MAX_SCALED)
        painter->translate(0, .5*autoplot::Y_MAX_SCALED);

      autoplot::basePainter(painter);

      p.setColor(Qt::red);
      p.setWidthF(pwidth*2);
      painter->setPen(p);
      painter->drawPath(*_fPath);

    painter->restore();
  }

  if (_border)  painter->drawEllipse(_shape);

  if (!_label.isEmpty())  drawRichText(painter);
}

void Node::drawRichText(QPainter *painter) {
  static const int flags = Qt::AlignCenter;

  QRectF rect = _bounds;
  if (fabs(_bounds.height() - _shape.height()) > MARGIN*2) {
    rect.adjust(0, _shape.height(), 0, 0);
//    painter->drawRect(rect);
  }

  _label.setPageSize(QSize(rect.width(), 1000));

  QAbstractTextDocumentLayout* layout = _label.documentLayout();

  QSizeF trect = layout->documentSize();
  const int width = qRound(trect.width());
  int x = rect.x();
  if (flags & Qt::AlignRight)
    x += (rect.width() - width);
  else if (flags & Qt::AlignHCenter)
    x += (rect.width() - width)/2;

  const int height = qRound(trect.height());
  int y = rect.y();
  if (flags & Qt::AlignBottom)
    y += (rect.height() - height);
  else if (flags & Qt::AlignVCenter)
    y += (rect.height() - height)/2;

  QAbstractTextDocumentLayout::PaintContext context;
  context.palette.setColor(QPalette::Text, painter->pen().color());

  painter->save();

  int dx = 0, dy = 0;
  painter->translate(x + dx, y + dy);
  layout->draw(painter, context);

  painter->restore();
}

} // end of namespace gui::cppn
