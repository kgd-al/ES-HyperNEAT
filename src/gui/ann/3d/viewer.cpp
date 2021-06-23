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

void Viewer::setGraph(const gvc::Graph &graph) {
  auto gw = graph.build_gvc_graph();
  gw.layout("nop");
  gvRender(gvc::context(), gw.graph, "dot", NULL);

  // process graph
  /// TODO Lots of duplicated code with 2d/viewer
  struct EdgeBuildData {
    Agedge_t *e;
    Node *i = nullptr, *o = nullptr;
  };

  std::map<std::string, EdgeBuildData> edges;
  const auto getOrNew = [this, &edges] (Agedge_t *e, Node *i, Node *o) {
    auto ename = std::string(agnameof(e));
    auto it = edges.find(ename);

    if (it != edges.end()) {
      auto &d = it->second;
      if (i) d.i = i;
      if (o) d.o = o;

    } else
      edges[ename] = EdgeBuildData{e,i,o};
  };

  struct NodeCMP {
    using is_transparent = void;

    bool operator() (const Node *lhs, const Node *rhs) const {
//      return lhs->pos() < rhs->pos();
      return lhs < rhs; /// TODO Fix
    }
  };
  std::set<Node*, NodeCMP> nodes;

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

  for (auto *n = agfstnode(gvc); n != NULL; n = agnxtnode(gvc, n)) {
//    auto p = gvc::get(n, "spos", phenotype::Point{NAN,NAN});

    auto qn = new Node(n, /*neuralData(p), */_scene);
    nodes.insert(qn);
    connect(qn, &Node::clicked, this, &Viewer::selectionChanged);

    for (auto *e = agfstout(gvc, n); e != NULL; e = agnxtout(gvc, e))
      getOrNew(e, qn, nullptr);

    for (auto *e = agfstin(gvc, n); e != NULL; e = agnxtin(gvc, e))
      getOrNew(e, nullptr, qn);
  }

  for (const auto &it: edges) {
    const EdgeBuildData &d = it.second;
    auto qe = new Edge(d.e, d.i, d.o, _scene);
    d.i->out.push_back(qe);
    d.o->in.push_back(qe);
  }

  qDebug() << __PRETTY_FUNCTION__ << " rendering" << nodes.size() << "nodes &"
           << edges.size() << "edges";
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

  qDebug() << "Selection:" << _selection;
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

} // end of namespace kgd::es_hyperneat::gui::ann3d
