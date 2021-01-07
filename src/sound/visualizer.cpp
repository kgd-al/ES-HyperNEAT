#include <QPainter>

#include "visualizer.h"

#include <iostream>
#include "kgd/utils/utils.h"

namespace sound {

Visualizer::Visualizer(QWidget *parent) : QWidget(parent) {
//  setSizePolicy(QSizePolicy::Fixed,QSizePolicy::Fixed);
  QSizePolicy sizePolicy (QSizePolicy::Expanding, QSizePolicy::Preferred );
  sizePolicy.setHeightForWidth(true);
  setSizePolicy(sizePolicy);
  setAttribute(Qt::WA_Hover, true);

  _highlight = false;

  _data = QImage(NOTES, CHANNELS, QImage::Format_RGB32);
}

Visualizer::~Visualizer(void) {}

void Visualizer::setNoteSheet(const NoteSheet &notes) {
  Generator::setNoteSheet(notes);

  using utils::operator<<;
  std::cerr << "Provided note sheet: " << notes << "\n";

  for (uint c=0; c<CHANNELS; c++) {
    auto b = (QRgb*) _data.scanLine(c);
    for (uint n=0; n<NOTES; n++)
      b[n] = QColor::fromHsv(0, 0,
                             255*std::max(0.f, notes[n*CHANNELS+c])).rgb();
  }

  update();
}

void Visualizer::setHighlighted(bool h) {
  _highlight = h;
  update();
}

void Visualizer::paintEvent(QPaintEvent *e) {
  QWidget::paintEvent(e);

  QPainter p (this);
  if (_highlight)  p.setPen(palette().color(QPalette::Highlight));
  p.drawRect(rect().adjusted(0, 0, -1, -1));
  p.drawImage(rect().adjusted(1, 1, -1, -1), _data);
}

} // end of namespace sound
