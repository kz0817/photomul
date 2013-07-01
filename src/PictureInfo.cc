/* photomul
   Copyright (C) 2013 Kazuhiro Yamato
 
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "PictureInfo.h"
#include "Utils.h"

using namespace Utils;

PictureInfo::PictureInfo(void)
: gfile(NULL),
  orientation(ORIENTATION_UNKNOWN),
  pixbuf(NULL),
  exposure_numerator(-1),
  exposure_denominator(-1),
  fnumber_numerator(-1),
  fnumber_denominator(-1),
  iso_speed(-1),
  exposure_bias_numerator(-1),
  exposure_bias_denominator(-1),
  focal_length_numerator(-1),
  focal_length_denominator(-1)
{
}

PictureInfo::~PictureInfo()
{
	if (gfile)
		g_object_unref(gfile);
	if (pixbuf)
		g_object_unref(pixbuf);
}

string PictureInfo::get_exposure_string(void)
{
	if (exposure_numerator < 0 || exposure_denominator <= 0)
		return "N/A";

	double exposure = (double)exposure_numerator / exposure_denominator;
	if (exposure >= 1)
		return format("%d", (int)exposure);
	return format("1/%d", (int)(1.0/exposure));
}

string PictureInfo::get_fnumber_string(void)
{
	if (fnumber_numerator < 0 || fnumber_denominator <= 0)
		return "N/A";

	double fnumber = (double)fnumber_numerator / fnumber_denominator;
	return format("%.1f", fnumber);
}

