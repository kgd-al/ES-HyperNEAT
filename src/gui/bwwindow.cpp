#include <QGridLayout>

#include "bwwindow.h"

#include <QDebug>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QToolButton>
#include "soundgenerator.h"
#include <QMetaEnum>

namespace gui {

BWWindow::BWWindow(QWidget *parent) : QMainWindow(parent) {
  _sounds = new SoundGenerator(this);

  auto *layout = new QGridLayout;

  layout->addWidget(_details = new ES_HyperNEATPanel, 0, 1);

  setLayout(layout);

//  c=0;
//  auto sgen = new SoundGenerator(this);
//  auto t_layout = new QHBoxLayout;
//  auto icb = new QComboBox;
//  auto ncb = new QComboBox;
//  auto ssb = new QDoubleSpinBox;
//  auto b = new QToolButton;
//  t_layout->addWidget(icb);
//  t_layout->addWidget(ncb);
//  t_layout->addWidget(ssb);
//  t_layout->addWidget(b);

//  auto ime = QMetaEnum::fromType<SoundGenerator::Instrument>();
//  for (const auto &p: SoundGenerator::instruments)
//    icb->addItem(ime.valueToKey(p.first), p.first);
//  icb->setCurrentText("SAW");

//  int A3_index = -1, i = 0;
//  std::map<float, QString> notesInvMap;
//  for (const auto &p: SoundGenerator::notes)  notesInvMap[p.second] = p.first;
//  for (auto &p: notesInvMap) {
//    QString text;
//    QTextStream qts (&text);
//    qts.setRealNumberNotation(QTextStream::FixedNotation);
//    qts.setRealNumberPrecision(0);
//    qts << p.second << " (" << p.first << "kHz)";
//    ncb->addItem(text, p.first);
//    if (A3_index < 0) {
//      if (p.second == "A3") A3_index = i;
//      i++;
//    }
//  }
//  ncb->setCurrentIndex(A3_index);

//  ssb->setRange(0.05, 1);
//  ssb->setSingleStep(.05);
//  ssb->setValue(1);

//  b->setShortcut(QKeySequence("space"));
//  layout->addLayout(t_layout, r++, c, 1, 3, Qt::AlignCenter);
//  connect(b, &QToolButton::clicked, [sgen, icb, ncb, ssb] {
////    SoundGenerator::make_sound(220, .1);

//    auto instrument = SoundGenerator::Instrument(icb->currentData().toInt());
//    sgen->make_sound(instrument, ncb->currentData().toFloat(), ssb->value());
//  });
}

} // end of namespace gui
