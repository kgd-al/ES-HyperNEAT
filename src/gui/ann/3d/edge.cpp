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

Qt3DRender::QGeometryRenderer* arrowMesh (Qt3DCore::QNode *parent,
                                          const QVector3D &src,
                                          const QVector3D &dst,
                                          float width, float length) {

  QVector3D fwd = (dst - src).normalized();
  QVector3D ort = QVector3D(-fwd.y(), fwd.x(), 0).normalized();

  QVector3D start = src;
  QVector3D end = dst;

  QVector3D lsyn = dst - width * ort + length * fwd,
            rsyn = dst + width * ort + length * fwd;

  auto *geometry = new Qt3DRender::QGeometry(parent);

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
  auto *line = new Qt3DRender::QGeometryRenderer(parent);
  line->setGeometry(geometry);
  line->setPrimitiveType(Qt3DRender::QGeometryRenderer::Lines);

  return line;
}

Edge::Edge(Agedge_t *edge, Node *i, Node *o, Entity *parent)
  : Entity(parent), in(nullptr), out(nullptr) {

  QVector3D src = i->pos(), dst = o->pos();
  QVector3D fwd = (dst - src).normalized();

  _currentColor = _color =
    QColor(QString::fromStdString(gvc::get(edge, "color",
                                                 std::string("green"))));

  static constexpr auto R = Node::RADIUS, T = R/4, L = R/2;
  addComponent(arrowMesh(this,
                         i->pos() + R * fwd,
                         o->pos() - (R + L * 1.25) * fwd,
                         T, L));

  setVisible(false);
}

void Edge::setVisible (bool v) {
  static auto material =
    new Qt3DExtras::QDiffuseSpecularMaterial(parentEntity());
  if (v)
    addComponent(material);
  else
    removeComponent(material);
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
