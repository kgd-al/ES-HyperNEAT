#include <QVector3D>

#include <Qt3DRender/QGeometry>
#include <Qt3DRender/QAttribute>
#include <Qt3DRender/QGeometryRenderer>
#include <Qt3DRender/QBuffer>

#include <Qt3DExtras/QDiffuseSpecularMaterial>

#include "edge.h"

namespace kgd::es_hyperneat::gui::ann3d {

Edge::Edge(const QVector3D &src, const QVector3D &dst, Entity *parent)
  : Entity(parent) {
  auto *geometry = new Qt3DRender::QGeometry(this);

  QVector3D fwd = (dst - src).normalized();
  QVector3D ort = QVector3D(-fwd.y(), fwd.x(), 0).normalized();
  QVector3D end = dst - .15 * fwd;
  QVector3D lsyn = end - .025 * ort + .05 * fwd,
            rsyn = end + .025 * ort + .05 * fwd;

  // position vertices (start and end)
  uint vertices = 4;
  QByteArray bufferBytes;
  // start.x, start.y, start.end + end.x, end.y, end.z
  bufferBytes.resize(vertices * 3 * sizeof(float));
  float *positions = reinterpret_cast<float*>(bufferBytes.data());
  for (const QVector3D &v: {src, end, lsyn, rsyn}) {
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

  auto material = new Qt3DExtras::QDiffuseSpecularMaterial(this);
  material->setAmbient(Qt::black);

  addComponent(line);
  addComponent(material);
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
