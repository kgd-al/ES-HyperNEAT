#include <QPainterPath>

#include <QVector3D>

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QBuffer>

#include <Qt3DExtras/QDiffuseSpecularMaterial>

#include "edge.h"
#include "node.h"

namespace kgd::es_hyperneat::gui::ann3d {

Edge::Edge(Agedge_t *edge, Node *i, Node *o, Entity *parent)
  : Entity(parent), in(nullptr), out(nullptr) {

  const splines* spl = ED_spl(edge);
  QVector3D src = i->pos(), dst = o->pos();

  const QPointF offset = kgd::gui::toQt(spl->bb).center();
  auto convert = [offset] (pointf p) {
    return (kgd::gui::toQt(p) / gvc::Graph::scale) - offset;
  };

  QPainterPath edge_p;
  if ((spl->list != 0) && (spl->list->size%3 == 1)) {
    bezier bez = spl->list[0];
    // If there is a starting point,
    //  draw a line from it to the first curve point
    if (bez.sflag) {
      edge_p.moveTo(convert(bez.sp));
      edge_p.lineTo(convert(bez.list[0]));

    } else
      edge_p.moveTo(convert(bez.list[0]));

    // Loop over the curve points
    for (int i=1; i<bez.size; i+=3)
      edge_p.cubicTo(convert(bez.list[i]), convert(bez.list[i+1]),
                   convert(bez.list[i+2]));

    // If there is an ending point, draw a line to it
    if (bez.eflag)  edge_p.lineTo(convert(bez.ep));

//    QPointF end = edge_p.currentPosition();
//    QPointF axis = end - edge_p.pointAtPercent(.99);
//    float length = sqrt(axis.x()*axis.x()+axis.y()*axis.y());
//    axis = arrowLength * axis / length;
//    end -= .05 * axis;

//    QPointF lastPoint = edge_p.elementAt(edge_p.elementCount()-1);
//    lastPoint -= (1-arrowFolding)*axis;
//    edge_p.setElementPositionAt(edge_p.elementCount()-1,
//                              lastPoint.x(), lastPoint.y());

//    float a = 0, b = .25;
//    QPointF cross (-axis.y()/2, axis.x()/2);
//    arrow.moveTo(end-a*axis-cross);
//    arrow.lineTo(end-b*axis);
//    arrow.lineTo(end-a*axis+cross);
////    arrow.lineTo(end);
//    arrow.lineTo(end-axis);
//    arrow.closeSubpath();

//    qDebug() << "edge " << src << " -> " << dst << ":\n\t" << edge_p;
  }
  _currentColor = _color =
    QColor(QString::fromStdString(gvc::get(edge, "color",
                                                 std::string("green"))));

  auto *geometry = new Qt3DRender::QGeometry(this);

  QVector3D fwd = (dst - src).normalized();
  QVector3D ort = QVector3D(-fwd.y(), fwd.x(), 0).normalized();

  static constexpr auto R = Node::RADIUS, T = R/4, L = R/2;
  QVector3D start = src + R * fwd;
  QVector3D end = dst - (R + L * 1.25) * fwd;
  QVector3D lsyn = end - T * ort + L * fwd,
            rsyn = end + T * ort + L * fwd;

  // position vertices (start and end)
  uint vertices = 4;
  QByteArray bufferBytes;
  // start.x, start.y, start.end + end.x, end.y, end.z
  bufferBytes.resize(vertices * 3 * sizeof(float));
  float *positions = reinterpret_cast<float*>(bufferBytes.data());
  for (const QVector3D &v: {start, end, lsyn, rsyn}) {
    *positions++ = v.x();
    *positions++ = v.y();
    *positions++ = v.z();
  }

  auto *buf = new Qt3DRender::QBuffer(geometry);
  buf->setData(bufferBytes);

  auto *positionAttribute = new Qt3DRender::QAttribute(geometry);
  positionAttribute->setName(Qt3DRender::QAttribute::defaultPositionAttributeName());
  positionAttribute->setVertexBaseType(Qt3DRender::QAttribute::Float);
  positionAttribute->setVertexSize(3);
  positionAttribute->setAttributeType(Qt3DRender::QAttribute::VertexAttribute);
  positionAttribute->setBuffer(buf);
  positionAttribute->setByteStride(3 * sizeof(float));
  positionAttribute->setCount(vertices);
  geometry->addAttribute(positionAttribute); // We add the vertices in the geometry

  // connectivity between vertices
  uint segments = 3;
  QByteArray indexBytes;
  indexBytes.resize(segments * 2 * sizeof(unsigned int)); // start to end
  unsigned int *indices = reinterpret_cast<unsigned int*>(indexBytes.data());
  *indices++ = 0;
  *indices++ = 1;
  *indices++ = 1;
  *indices++ = 2;
  *indices++ = 1;
  *indices++ = 3;

  auto *indexBuffer = new Qt3DRender::QBuffer(geometry);
  indexBuffer->setData(indexBytes);

  auto *indexAttribute = new Qt3DRender::QAttribute(geometry);
  indexAttribute->setVertexBaseType(Qt3DRender::QAttribute::UnsignedInt);
  indexAttribute->setAttributeType(Qt3DRender::QAttribute::IndexAttribute);
  indexAttribute->setBuffer(indexBuffer);
  indexAttribute->setCount(segments * 2);
  geometry->addAttribute(indexAttribute); // We add the indices linking the points in the geometry

  // mesh
  auto *line = new Qt3DRender::QGeometryRenderer(this);
  line->setGeometry(geometry);
  line->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);

  addComponent(line);

  static auto material = new Qt3DExtras::QDiffuseSpecularMaterial(parentEntity());
  addComponent(material);

  setEnabled(false);
}

void Edge::setHovered (bool h) {
  setEnabled(h);
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
