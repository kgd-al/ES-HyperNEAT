#ifndef VERTICALPANEL_HPP
#define VERTICALPANEL_HPP

namespace kgd::gui {

struct VLabel : public QLabel {
  VLabel (const QString &text = "", QWidget *parent = nullptr)
    : QLabel(text, parent) {}

  QSize minimumSizeHint(void) const override {
    return QLabel::minimumSizeHint().transposed();
  }

  QSize sizeHint(void) const override {
    return QLabel::sizeHint().transposed();
  }

  void paintEvent(QPaintEvent*) {
    QPainter painter(this);
    painter.translate(rect().center());
    painter.rotate(-90);
    painter.translate(-rect().transposed().center());
    painter.drawText(rect().transposed(), alignment(), text());
  }
};

} // end of namespace kgd::gui

#endif // VERTICALPANEL_HPP
