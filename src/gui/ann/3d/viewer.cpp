#include <QKeyEvent>

#include <Qt3DCore/QTransform>

#include <Qt3DRender/QCamera>
#include <Qt3DRender/QPointLight>
#include <Qt3DRender/QRenderStateSet>
#include <Qt3DRender/QMultiSampleAntiAliasing>
#include <Qt3DRender/QSortPolicy>

#include <Qt3DExtras/QPlaneMesh>
#include <Qt3DExtras/QConeMesh>
#include <Qt3DExtras/QCylinderMesh>
#include <Qt3DExtras/QTorusMesh>
#include <Qt3DExtras/QDiffuseSpecularMaterial>

#include "viewer.h"
#include "node.h"
#include "edge.h"

#include <set>

#if ESHN_SUBSTRATE_DIMENSION == 3
namespace kgd::es_hyperneat::gui::ann3d {

struct CameraController : public Qt3DExtras::QAbstractCameraController {
//#define DEBUG

  CameraController (Qt3DCore::QNode *parent)
    : Qt3DExtras::QAbstractCameraController (parent) {}

  void setViewCenter (const QVector3D &p) {
    auto c = camera();
    QVector3D p0 = c->viewCenter();

#ifdef DEBUG
    auto q = qDebug().nospace();
    q << __PRETTY_FUNCTION__ << ":\n";
    q << "\t        pos: " << c->position() << "\n";
    q << "\t       view: " << c->viewCenter() << "\n";
    q << "\ttranslation: " << (p - p0) << "\n";
#endif

    c->translateWorld(p - p0, c->DontTranslateViewCenter);
    c->setViewCenter(p);

#ifdef DEBUG
    q << "\t    new pos: " << c->position() << "\n";
    q << "\t   new view: " << c->viewCenter() << "\n";
#endif
  }

  void moveCamera(const InputState &state, float dt) override {
    Qt3DRender::QCamera *c = camera();

    if (state.leftMouseButtonActive) {
      float step = lookSpeed() * dt;

#ifdef DEBUG
      auto q = qDebug().nospace();
      q << __PRETTY_FUNCTION__ << ":\n";
      q << "\t        pos: " << c->position() << "\n";
      q << "\t       view: " << c->viewCenter() << "\n";
#endif

      c->panAboutViewCenter(state.rxAxisValue * step, {0,0,1});
      c->panAboutViewCenter(state.ryAxisValue * step,
                            QVector3D::crossProduct(c->viewVector(),
                                                    c->upVector()));

#ifdef DEBUG
      q << "\t    new pos: " << c->position() << "\n";
      q << "\t   new view: " << c->viewCenter() << "\n";
#endif

    } else if (state.tzAxisValue != 0) {
      static constexpr auto MIN_DIST = 1.f;
      float dz = state.tzAxisValue * c->viewVector().length() * .1
               * lookSpeed() * dt;
      QVector3D newPos = c->position() + c->viewVector().normalized() * dz;
      float newDist = (newPos - c->viewCenter()).length();

#ifdef DEBUG
      auto q = qDebug().nospace();
      q << __PRETTY_FUNCTION__ << ":\n";
      q << "\t    dz = " << dz << "\n";
      q << "\t newPos: " << newPos << "\n";
      q << "\tnewDist: " << newDist << "\n";
#endif

      if (dz > 0 && newDist <= MIN_DIST) dz = 0;

#ifdef DEBUG
      q << "\t    dz = " << dz << "\n";
#endif

      c->translate({0,0, dz }, c->DontTranslateViewCenter);
    }
  }

#ifdef DEBUG
#undef DEBUG
#endif
};

Viewer::Viewer(void) {
  // create scene
  _scene = new Entity;

  Qt3DRender::QCamera *camera = this->camera();
  camera->lens()->setPerspectiveProjection(45.0f, 16.f/9.f, .1f, 1000.f);
  camera->setPosition(QVector3D(0, 0, 5.f));
  camera->setViewCenter(QVector3D(0, 0, 0));

//  _manipulator = new Qt3DExtras::QOrbitCameraController (_scene);
  _manipulator = new CameraController (_scene);
  _manipulator->setLinearSpeed(50.f);
  _manipulator->setLookSpeed(180.f);
//  manipulator->setZoomInLimit(2);
  _manipulator->setCamera(camera);

  auto lightEntity = new Qt3DCore::QEntity(camera);
  auto light = new Qt3DRender::QPointLight(lightEntity);
  light->setColor(Qt::white);
  light->setIntensity(1);
  lightEntity->addComponent(light);

  auto *renderStateSet = new Qt3DRender::QRenderStateSet(_scene);

  auto *msaa = new Qt3DRender::QMultiSampleAntiAliasing(_scene);
  renderStateSet->addRenderState(msaa);

  setRootEntity(_scene);

  auto framegraph = activeFrameGraph();
  auto sort = new Qt3DRender::QSortPolicy (_scene);
  framegraph->setParent(sort);
  QVector<Qt3DRender::QSortPolicy::SortType> sortTypes {sort->BackToFront};
  sort->setSortTypes(sortTypes);
  setActiveFrameGraph(framegraph);

  _axis = buildAxis(_scene);
  _selectionHighlighter = buildSelectionHighlighter(_scene);

  _ann = nullptr;
  _animating = false;
}

Viewer::~Viewer (void) {
//  delete _manipulator;
  delete _scene;
}

Viewer::Entity* Viewer::buildAxis(Entity *parent) {
  using T = Qt3DCore::QTransform;
  struct AxisData {
    QQuaternion r;
    Qt::GlobalColor c;
  };
  static const QVector<AxisData> data {
    { T::fromAxisAndAngle(0, 0, 1, -90), Qt::red    },
    { T::fromAxisAndAngle(0, 0, 0,   0), Qt::green  },
    { T::fromAxisAndAngle(1, 0, 0, +90), Qt::blue   },
  };

  static constexpr float sr = .01;  // Shaft radius
  static constexpr float ar = 1.5*sr; // Arrowhead radius
  static constexpr float al = .25;    // Arrowhead length

  Qt3DExtras::QConeMesh *amesh = new Qt3DExtras::QConeMesh;
  amesh->setBottomRadius(ar);
  amesh->setLength(al);
  amesh->setSlices(10);
  amesh->setRings(10);

  Qt3DExtras::QCylinderMesh *smesh = new Qt3DExtras::QCylinderMesh;
  smesh->setRadius(sr);
  smesh->setLength(2-al);
  smesh->setSlices(10);
  smesh->setRings(10);

  Entity *axes (parent);
  for (const AxisData &d: data) {
    Entity *axis = new Entity (axes),
           *arrow = new Entity (axis), *shaft = new Entity (axis);

    auto *material = new Qt3DExtras::QDiffuseSpecularMaterial(parent);
    material->setDiffuse(QColor(d.c));
    material->setShininess(0);


    Qt3DCore::QTransform *atransform = new Qt3DCore::QTransform;
    atransform->setTranslation({0, 1.f-.5f*al, 0});

    arrow->addComponent(amesh);
    arrow->addComponent(atransform);
    arrow->addComponent(material);

    Qt3DCore::QTransform *stransform = new Qt3DCore::QTransform;
    stransform->setTranslation({0, -.5f*al, 0});

    shaft->addComponent(smesh);
    shaft->addComponent(stransform);
    shaft->addComponent(material);

    T *rotation = new T;
    rotation->setRotation(d.r);
    axis->addComponent(rotation);
  }

  return axes;
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

void Viewer::setANN(const phenotype::ANN &ann) {
  struct EdgeBuildData {
    const phenotype::ANN::Neuron *i = nullptr, *o = nullptr;
  };
  std::vector<EdgeBuildData> edges;
  std::map<phenotype::Point, Node*> nodes;

  if (!_nodes.empty() || _edges.empty())  clearANN();

  for (const auto &n: ann.neurons()) {
    auto qn = new Node(*n, _scene);
    nodes[n->pos] = qn;
    _nodes.push_back(qn);
    connect(qn, &Node::clicked, this, &Viewer::selectionChanged);
    connect(qn, &Node::hovered, this, &Viewer::neuronHovered);

    for (const phenotype::ANN::Neuron::Link &l: n->links())
      edges.push_back({ l.in.lock().get(), n.get() });
  }

  for (const EdgeBuildData &d: edges) {
    Node *i = nodes.at(d.i->pos), *o = nodes.at(d.o->pos);
    auto qe = new Edge(i, o, _scene);
    _edges.push_back(qe);
    i->out.push_back(qe);
    o->in.push_back(qe);
  }

  _ann = &ann;

//  qDebug() << "Set up" << this << _nodes.size() << "nodes &"
//           << _edges.size() << "edges";
}

void Viewer::clearANN(void) {
//  qDebug() << "Clearing" << this << _nodes.size() << "nodes &"
//           << _edges.size() << "edges";
  for (Node *n: _nodes)  delete n;
  _nodes.clear();
  for (Edge *e: _edges)  delete e;
  _edges.clear();

  selectionChanged(nullptr);
  _ann = nullptr;
}

void Viewer::selectionChanged(Node *n) {
  if (_selection && _selection == n) {
    _selection->setSelected(false);
    _selection = nullptr;

  } else {
    _selection = n;
  }

  _selectionHighlighter->setEnabled(_selection);
  if (_selection) {
    _selectionHighlighter->componentsOfType<Qt3DCore::QTransform>()
        .front()->setTranslation(_selection->pos());
    _manipulator->setViewCenter(_selection->pos());
    _selection->setSelected(true);
  } else
    _manipulator->setViewCenter({0,0,0});

//  qDebug() << "Selection:" << _selection;
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

  auto q = qDebug().nospace();
//  q << __PRETTY_FUNCTION__ << ":\n";
//  q << "\t" << p << u << "\n";

  if (!p.isNull())  camera()->setPosition(p);
  if (!u.isNull())  camera()->setUpVector(u);
  if (!p.isNull() || !u.isNull()) {
//    q << "\t" << camera()->viewCenter() << camera()->position() << "\n";
    camera()->viewSphere(camera()->viewCenter(), 1);
//    q << "\t>>\n";
//    q << "\t" << camera()->viewCenter() << camera()->position() << "\n";
  }
}

void Viewer::depthDebugDraw (bool active) {
  for (Node *n: _nodes) n->depthDebugDraw(active, _ann->stats().depth);
}

void Viewer::startAnimation(void) {
  _animating = true;
  for (Node *n: _nodes) n->updateAnimation(true);
}

void Viewer::updateAnimation(void) {
  for (Node *n: _nodes) n->updateAnimation(true);
}

void Viewer::stopAnimation(void) {
  _animating = false;
  for (Node *n: _nodes) n->updateAnimation(false);
}

} // end of namespace kgd::es_hyperneat::gui::ann3d
#endif
