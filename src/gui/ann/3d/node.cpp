#include <QPainter>

#include <Qt3DCore/QTransform>
#include <Qt3DRender/QPickEvent>
#include <Qt3DRender/QPaintedTextureImage>
#include <Qt3DExtras/QSphereMesh>

#include <Qt3DExtras/QText2DEntity>

#include "node.h"
#include "edge.h"

#include "../../../phenotype/ann.h"

#include <QDebug>

#if ESHN_SUBSTRATE_DIMENSION == 3
namespace kgd::es_hyperneat::gui::ann3d {

static const QColor defaultColor = Qt::black,
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

QVector3D toQt3D (const phenotype::Point &p) {
  return QVector3D(p.x(), p.y(), p.z());
}

using Neuron = phenotype::ANN::Neuron;
Node::Node (const Neuron &n, Entity *parent)
  : Entity(parent), _neuron(n) {

  _pos = toQt3D(n.pos); // from pos attribute

  _baseColor = (n.type == Neuron::H ? hiddensColor : defaultColor);

  static Qt3DExtras::QSphereMesh *mesh = [] (Entity *parent) {
    auto m = new Qt3DExtras::QSphereMesh (parent);
    m->setRadius(RADIUS);
    m->setRings(10);
    m->setSlices(10);
    return m;
  }(parent);

  Qt3DCore::QTransform *transform = new Qt3DCore::QTransform;
  transform->setTranslation(_pos);
  //  transform->setScale3D(QVector3D(1.5, 1, 0.5));
  //  transform->setRotation(QQuaternion::fromAxisAndAngle(QVector3D(1,0,0), 45.f ));

//  auto *canvas = new TextureImage;

  _material = new Qt3DExtras::QDiffuseSpecularMaterial(this);
  _material->setDiffuse(_baseColor);
  _material->setAlphaBlendingEnabled(true);
  _material->setShininess(0);
//  _material->diffuse()->addTextureImage(canvas);

  addComponent(mesh);
  addComponent(_material);
  addComponent(transform);

  _picker = new Qt3DRender::QObjectPicker(this);
  _picker->setHoverEnabled(true);

  addComponent(_picker);
  connect(_picker, &Qt3DRender::QObjectPicker::entered,
          this, &Node::hoverEntered);
  connect(_picker, &Qt3DRender::QObjectPicker::exited,
          this, &Node::hoverExited);
  connect(_picker, &Qt3DRender::QObjectPicker::clicked,
          [this] { emit clicked(this); });

  _selected = _hovered = _highlighted = false;
}

void Node::setHighlighted (void) {
  bool h = _selected | _hovered;
  updateColor(h ? highlightColor : _currentColor);
  for (auto v: {in,out}) for (auto e: v) e->setVisible(h);
}

void Node::setSelected(bool s) {
  _selected = s;
  setHighlighted();
}

void Node::hoverEntered(void) {
  _hovered = true;
  setHighlighted();
  emit hovered(&_neuron);
}

void Node::hoverExited(void) {
  _hovered = false;
  setHighlighted();
}

void Node::updateAnimation(bool running) {
  _currentColor = _baseColor;

  if (running)  // No updating edges
    _currentColor.setAlphaF(std::fabs(_neuron.value));

//  std::cerr << __PRETTY_FUNCTION__ << " " << _neuron.pos << " "
//            << " " << _neuron.value << " " << _currentColor.alphaF() << "\n";

  updateColor(_currentColor);
}

void Node::depthDebugDraw(bool active, uint maxDepth) {
  if (active)
    _currentColor = QColor::fromHsvF(0, 0, _neuron.depth / float(maxDepth));
  else
    _currentColor = _baseColor;
  updateColor(_currentColor);
}

void Node::updateColor (const QColor &c) {
  _material->setDiffuse(c);
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
#endif
