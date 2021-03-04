/*
***************************************************************************
*
* Author: Thomas Berger
* Based on the C-Version by Teunis van Beelen
*
* Copyright (C) 2009 - 2021 Teunis van Beelen
* Copyright (C) 2021        Thomas Berger
*
* Email: loki@lokis-chaos.de, teuniz@protonmail.com
*
***************************************************************************
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program.  If not, see <http://www.gnu.org/licenses/>.
*
***************************************************************************
*/

#include <QStringList>
#include <QDir>

#include "utils.h"

QString suffixed_metric_value(double value, int decimals)
{
  auto isNegative = value < 0.0;
  auto d = value;

  if (isNegative)
    d *= -1;

  // if our value is >= 1, we calculate the "upper case suffix", else the "lower case suffix"
  int suffix = 0;
  if (d >= 1) {
    for (auto i = 0; i < 4; ++i) {
      auto t = d / 1000;
      if (t < 1.0)
        break;
      d = t;
      // 0 = "", 1 = "K", 2 = "M", 3 = "G", 4 = "T"
      suffix = i + 1;
    }
  }
  else {
    for (auto i = 0; i < 4; ++i) {
      // we know for sure, that the number we got is smaller then 1.0, so we can simplify this loop by
      // checking at the end if the number is now larger then one
      d *= 1000;
      // -1 = "m", -2 = "µ", -3 = "n", -4 = "p"
      suffix = i + 1;
      if (d > 1.0)
        break;
    }
  }
  if (isNegative)
    d *= -1;

  QString suffixStr;
  switch (suffix) {
    case -4:
      suffixStr = "p";
      break;
    case -3:
      suffixStr = "n";
      break;
    case -2:
      suffixStr = "µ";
      break;
    case -1:
      suffixStr = "m";
      break;
    case 1:
      suffixStr = "K";
      break;
    case 2:
      suffixStr = "M";
      break;
    case 3:
      suffixStr = "G";
      break;
    case 4:
      suffixStr = "T";
      break;
  }

  return QString("%1%2")
      .arg(d, 0, 'f', decimals)
      .arg(suffixStr);
}

QString directory_from_path(const QString &path)
{
  auto parts = path.split(QDir::separator());
  // remove the file element
  parts.removeLast();
  return parts.join(QDir::separator());
}