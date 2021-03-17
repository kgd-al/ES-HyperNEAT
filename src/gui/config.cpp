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

DEFINE_CONTAINER_PARAMETER(CFILE::ANNCustomColors, annColorMapping, {
  { 1<<8,  QColor("#00FF00") },
  { 1<<9,  QColor("#0000FF") },
  { 1<<10, QColor("#FF0000") },
  { 1<<11, QColor("#FF0000") },
  { 1<<12, QColor("#0000FF") },
})

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
