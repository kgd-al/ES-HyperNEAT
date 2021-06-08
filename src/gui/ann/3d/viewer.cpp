#include <QKeyEvent>

#include <Qt3DCore/QTransform>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QMultiSampleAntiAliasing>

#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/QDiffuseSpecularMaterial>

#include "viewer.h"
#include "node.h"
#include "edge.h"

#include <set>
//#include "kgd/random/dice.hpp"

namespace kgd::es_hyperneat::gui::ann3d {

Viewer::Viewer(void) {
  // create scene
  _scene = new Entity;

  Qt3DRender::QCamera *camera = this->camera();
  camera->lens()->setPerspectiveProjection(45.0f, 16.f/9.f, .1f, 1000.f);
  camera->setPosition(QVector3D(0, 0, 5.f));
  camera->setViewCenter(QVector3D(0, 0, 0));

  _manipulator = new Qt3DExtras::QOrbitCameraController (_scene);
  _manipulator->setLinearSpeed(50.f);
  _manipulator->setLookSpeed(180.f);
  _manipulator->setZoomInLimit(2);
  _manipulator->setCamera(camera);
  connect(camera, &Qt3DRender::QCamera::viewCenterChanged,
          this, &Viewer::updateCamera);

  auto lightEntity = new Qt3DCore::QEntity(camera);
  auto light = new Qt3DRender::QPointLight(lightEntity);
  light->setColor(Qt::white);
  light->setIntensity(1);
  lightEntity->addComponent(light);

  auto *renderStateSet = new Qt3DRender::QRenderStateSet(_scene);

  auto *msaa = new Qt3DRender::QMultiSampleAntiAliasing(_scene);
  renderStateSet->addRenderState(msaa);

  setRootEntity(_scene);

  // ====

  {
    Entity *bounds = new Entity(_scene);

    Qt3DExtras::QPlaneMesh *mesh = new Qt3DExtras::QPlaneMesh;
    mesh->setWidth(2);
    mesh->setHeight(2);

    auto *material = new Qt3DExtras::QDiffuseSpecularMaterial(_scene);
    material->setDiffuse(QColor::fromRgbF(.25, .25, .25, .1));
    material->setShininess(0);
    material->setAlphaBlendingEnabled(true);

    for (int i: {0,2}) {
      Entity *side = new Entity(bounds);
      Qt3DCore::QTransform *rotation = new Qt3DCore::QTransform;
      rotation->setRotation(Qt3DCore::QTransform::fromEulerAngles(90+i*90, 0, 0));

      side->addComponent(rotation);
      side->addComponent(mesh);
      side->addComponent(material);
    }
  }

  _axis = buildAxis(_scene);
  _selectionHighlighter = buildSelectionHighlighter(_scene);
}

Viewer::~Viewer (void) {
  delete _manipulator;
  delete _scene;
}

Viewer::Entity* Viewer::buildAxis(Entity *parent) {
  Entity *axis = new Entity (parent),
         *arrow = new Entity (axis), *shaft = new Entity (axis);

  auto *material = new Qt3DExtras::QDiffuseSpecularMaterial(parent);
  material->setDiffuse(QColor::fromRgbF(.5, .5, .5));
  material->setShininess(0);

  static constexpr float sr = .0625;
  static constexpr float ar = 1.5*sr;
  static constexpr float al = .25;

  Qt3DExtras::QConeMesh *amesh = new Qt3DExtras::QConeMesh;
  amesh->setBottomRadius(ar);
  amesh->setLength(al);
  amesh->setSlices(10);
  amesh->setRings(10);

  Qt3DCore::QTransform *atransform = new Qt3DCore::QTransform;
  atransform->setTranslation({1.5f, 1.f-.5f*al, 0});
  arrow->addComponent(amesh);
  arrow->addComponent(atransform);
  arrow->addComponent(material);

  Qt3DExtras::QCylinderMesh *smesh = new Qt3DExtras::QCylinderMesh;
  smesh->setRadius(sr);
  smesh->setLength(2-al);
  smesh->setSlices(10);
  smesh->setRings(10);

  Qt3DCore::QTransform *stransform = new Qt3DCore::QTransform;
  stransform->setTranslation({1.5f, -.5f*al, 0});

  shaft->addComponent(smesh);
  shaft->addComponent(stransform);
  shaft->addComponent(material);

  return axis;
}

Viewer::Entity* Viewer::buildSelectionHighlighter(Entity *parent) {
  Entity *sh = new Entity (parent);
  sh->setEnabled(false);

  auto *material = new Qt3DExtras::QDiffuseSpecularMaterial(sh);
  material->setDiffuse(QColor::fromRgbF(.25, .25, .25));
  material->setShininess(0);

  static constexpr float r = .125;
  Qt3DExtras::QTorusMesh *mesh = new Qt3DExtras::QTorusMesh;
  mesh->setRadius((1+r) * Node::RADIUS);
  mesh->setMinorRadius(r * Node::RADIUS);

  Qt3DCore::QTransform *translation = new Qt3DCore::QTransform;
  translation->setTranslation({0, 0, 0});
  sh->addComponent(translation);

  for (int i: {-1,1}) {
    Entity *torus = new Entity (sh);

    Qt3DCore::QTransform *rotation = new Qt3DCore::QTransform;
    rotation->setRotation(Qt3DCore::QTransform::fromEulerAngles(i*30, 90, 0));

    torus->addComponent(mesh);
    torus->addComponent(material);
    torus->addComponent(rotation);
  }

  return sh;
}

#define T(F) if (lhs.F() != rhs.F()) return lhs.F() , rhs.F();
struct QV3DCMP {
  bool operator() (const QVector3D &lhs, const QVector3D &rhs) const {
    T(x)
    T(y)
    T(z)
    return false;
  }
};
struct FakeLink {
  QVector3D src, dst;

  friend bool operator< (const FakeLink &lhs, const FakeLink &rhs) {
    T(src.x)
    T(src.y)
    T(src.z)
    T(dst.x)
    T(dst.y)
    T(dst.z)
    return false;
  }
};
#undef T

void Viewer::setGraph(const gvc::Graph &graph) {
  auto gw = graph.build_gvc_graph();
  gw.layout("nop");
  gvRender(gvc::context(), gw.graph, "dot", NULL);

  // process graph
  /// TODO Lots of duplicated code with 2d/viewer

//  std::function<NeuralData*(const phenotype::Point&)> neuralData;
//  if (auto *ann = dynamic_cast<const phenotype::ANN*>(&g))
//    neuralData = [ann] (auto p) {
//      return new NeuronData (*ann->neurons().at(p));
//    };

//  else if (auto *mann = dynamic_cast<const phenotype::ModularANN*>(&g))
//    neuralData = [mann] (auto p) {
//      return new ModuleData (*mann->module(p));
//    };

//  else
//    throw std::invalid_argument(
//      "Provided graph is of wrong derived type. How did you do that?");

  auto gvc = gw.graph;
  qreal s = gvc::get(gvc, "dpi", 96.0) / 72.;

  for (auto *n = agfstnode(gvc); n != NULL; n = agnxtnode(gvc, n)) {
    phenotype::Point p;
    auto spos_str = gvc::get(n, "spos", std::string());
    std::istringstream (spos_str) >> p;

    auto qn = new Node(n, /*neuralData(p), */_scene);
//    scene->addItem(qn);
//    _nodes.append(qn);
//    connect(qn, &Node::hovered, this, &Viewer::neuronHovered);
  }


//  rng::FastDice dice (0);
//  std::set<QVector3D, QV3DCMP> coords;
//  std::set<FakeLink> links;

//  const auto val = [&dice] (void) -> float {
//    return std::round(dice(-9, 9)) / 10.f;
//  };

//  while (coords.size() < 10)  coords.insert({ val(), val(), val() });
//  while (coords.size() < 12)  coords.insert({ val(),    -1, val() });
//  while (coords.size() < 14)  coords.insert({ val(),    +1, val() });

//  for (const QVector3D &p: coords) {
//    auto n = new Node(p, _scene);
//    connect(n, &Node::clicked, this, &Viewer::selectionChanged);
//  }

//  while (links.size() < 2/**coords.size()*/) {
//    auto src = *dice(coords), dst = *dice(coords);
//    if (src.y() == 1) continue;
//    if (dst.y() == -1)  continue;

//    links.insert({src, dst});
//    new Edge (src, dst, _scene);
//  }
}

void Viewer::updateCamera(void) {
  QVector3D c (0,0,0);
  if (_selection) c = _selection->substratePos();
  camera()->setViewCenter(c);
//  camera()->setUpVector({0,0,0});
}

void Viewer::selectionChanged(Node *n) {
  _selection = n;
  _selectionHighlighter->setEnabled(_selection);
  if (_selection)
    _selectionHighlighter->componentsOfType<Qt3DCore::QTransform>()
        .front()->setTranslation(_selection->pos());
  updateCamera();
}

void Viewer::mouseDoubleClickEvent(QMouseEvent */*ev*/) {
  if (_selection && !_selection->isHovered()) selectionChanged(nullptr);
}

void Viewer::keyReleaseEvent(QKeyEvent *e) {
  if (e->modifiers() != Qt::KeypadModifier) return;

  static constexpr int I = 4;
  QVector3D p, u;
  switch (e->key()) {
  case Qt::Key_5: p = { 0,  0, I}; u = {0,1,0}; break;
  case Qt::Key_4: p = {-I,  0, 0}; u = {0,0,1}; break;
  case Qt::Key_6: p = { I,  0, 0}; u = {0,0,1}; break;
  case Qt::Key_8: p = { 0,  I, 0}; u = {0,0,1}; break;
  case Qt::Key_2: p = { 0, -I, 0}; u = {0,0,1}; break;
  }

  if (!p.isNull())  camera()->setPosition(p);
  if (!u.isNull())  camera()->setUpVector(u);
  camera()->viewAll();
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
