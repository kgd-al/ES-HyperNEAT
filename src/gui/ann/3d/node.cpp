#include <QPainter>

#include <Qt3DCore/QTransform>
#include <Qt3DRender/QPickEvent>
#include <Qt3DRender/QPaintedTextureImage>
#include <Qt3DExtras/QSphereMesh>

#include "node.h"
#include "edge.h"

#include "../../../phenotype/ann.h"

#include <QDebug>

namespace kgd::es_hyperneat::gui::ann3d {

static const QColor inputsColor = Qt::black,
                    outputsColor = Qt::black,
                    hiddensColor = Qt::gray,
                    highlightColor = Qt::blue;

struct TextureImage : public Qt3DRender::QPaintedTextureImage {
public:
  TextureImage (void) {
    setSize({100,100});
  }

  void paint (QPainter *painter) override {
    int w = width(), h = height();

    painter->fillRect(0, 0, w, h/3, Qt::red);
    painter->fillRect(0, h/3, w, h/3, Qt::green);
    painter->fillRect(0, 2*h/3, w, h/3, Qt::blue);
  }
};

QVector3D get3dCoord (Agnode_t *n) {
  QVector3D p;
  auto gvc_p = ND_pos(n);
  for (uint i=0; i<phenotype::ANN::DIMENSIONS; i++)
    p[i] = gvc_p[i] / gvc::Graph::scale;
  return p;
}

template <typename P, uint D = P::DIMENSIONS>
std::enable_if_t<D == 2, QVector3D>
maybePromoteTo3D (const P &p) {
  return QVector3D(p.x(), p.y(), 0);
}

template <typename P, uint D = P::DIMENSIONS>
std::enable_if_t<D == 3, QVector3D>
maybePromoteTo3D (const P &p) {
  return QVector3D(p.x(), p.y(), p.z());
}

Node::Node(Agnode_t *n, Entity *parent) : Entity(parent) {
  QVector3D pos1 = get3dCoord(n); // from pos attribute

  auto sp = gvc::get(n, "spos", phenotype::Point{NAN,NAN});
  QVector3D pos2 = maybePromoteTo3D(sp); // from provided substrate position
  qDebug() << pos1 << " =?= " << pos2 << " =?= "
           << kgd::gui::toQt(ND_coord(n));

  _pos = _spos = pos2;
  _color = QColor(QString::fromStdString(gvc::get(n, "fillcolor",
                                                  std::string("green"))));

  Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh;
  mesh->setRadius(RADIUS);
  mesh->setRings(10);
  mesh->setSlices(10);

  Qt3DCore::QTransform *transform = new Qt3DCore::QTransform;
  transform->setTranslation(_pos);
  //  transform->setScale3D(QVector3D(1.5, 1, 0.5));
  //  transform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 45.f ));

//  auto *canvas = new TextureImage;

  _material = new Qt3DExtras::QDiffuseSpecularMaterial(this);
  _material->setDiffuse(_color);
  _material->setShininess(0);
//  _material->diffuse()->addTextureImage(canvas);

  addComponent(mesh);
  addComponent(transform);
  addComponent(_material);

  _picker = new Qt3DRender::QObjectPicker(this);
  _picker->setHoverEnabled(true);

  addComponent(_picker);
  connect(_picker, &Qt3DRender::QObjectPicker::entered,
          this, &Node::hoverEntered);
  connect(_picker, &Qt3DRender::QObjectPicker::exited,
          this, &Node::hoverExited);
  connect(_picker, &Qt3DRender::QObjectPicker::clicked,
          [this] { emit clicked(this); });
}

void Node::hoverEntered(void) {
  _material->setDiffuse(highlightColor);
  for (auto v: {in,out}) for (auto e: v) e->setHovered(true);
}

void Node::hoverExited(void) {
  _material->setDiffuse(_color);
  for (auto v: {in,out}) for (auto e: v) e->setHovered(false);
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
