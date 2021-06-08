#include <QPainter>

#include <Qt3DCore/QTransform>
#include <Qt3DRender/QPickEvent>
#include <Qt3DRender/QPaintedTextureImage>
#include <Qt3DExtras/QSphereMesh>

#include "node.h"

#include "../../../phenotype/ann.h"

#include <QDebug>

namespace kgd::es_hyperneat::gui::ann3d {

static constexpr auto defaultColor = Qt::gray;
static constexpr auto highlightColor = Qt::blue;

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
  phenotype::Point p;
  auto spos_str = gvc::get(n, "spos", std::string());
  std::istringstream (spos_str) >> p;
  _pos = _spos = maybePromoteTo3D(p);

  Qt3DExtras::QSphereMesh *mesh = new Qt3DExtras::QSphereMesh;
  mesh->setRadius(RADIUS);
  mesh->setRings(10);
  mesh->setSlices(10);

  Qt3DCore::QTransform *transform = new Qt3DCore::QTransform;
  transform->setTranslation(_pos);
  //  transform->setScale3D(QVector3D(1.5, 1, 0.5));
  //  transform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 45.f ));

  auto *canvas = new TextureImage;

  _material = new Qt3DExtras::QDiffuseSpecularMaterial(this);
  _material->setDiffuse(QColor(defaultColor));
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
  _material->setDiffuse(QColor(highlightColor));
}

void Node::hoverExited(void) {
  _material->setDiffuse(QColor(defaultColor));
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
