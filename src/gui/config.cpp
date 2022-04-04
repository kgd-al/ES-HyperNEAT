#include "config.h"

void to_json (nlohmann::json &j, const QColor &c) {
  j = c.name();
}

void from_json (const nlohmann::json &j, QColor &c) {
  c = QColor(QString::fromStdString(j.get<std::string>()));
}

std::istream& operator>> (std::istream &is, QColor &c) {
  std::string name;
  is >> name;
  c = QColor(QString::fromStdString(name));
  if (!c.isValid()) is.setstate(std::ios_base::failbit);
  return is;
}

std::ostream& operator<< (std::ostream &os, const QColor &c) {
  return os << c.name();
}

namespace config {
#define CFILE ESHNGui

auto colors (void) {
//  ESHNGui::ANNCustomColors colors {
//    { 1<<1, QColor("#FF0000") },
//    { 1<<2, QColor("#00FF00") },
//    { 1<<3, QColor("#0000FF") },
//  };

  ESHNGui::ANNCustomColors colors;
  float count = 3;
  for (uint i=0; i<count; i++)
    colors[1<<i] = QColor::fromHsvF(i / count, 1, 1);

  return colors;
}

DEFINE_CONTAINER_PARAMETER(CFILE::ANNCustomColors, annColorMapping, colors())

#undef CFILE

ESHNGui::CustomColors ESHNGui::colorsForFlag(NFlag flag) {
  std::set<QRgb> set;
  for (const auto &p: annColorMapping())
    if (p.first & flag)
      set.insert(p.second.rgba());
  CustomColors colors;
  for (QRgb rgb: set)
    colors.append(QColor(rgb));
  return colors;
}

} // end of namespace config
