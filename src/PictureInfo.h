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

#ifndef PictureInfo_h
#define PictureInfo_h

#include <gio/gio.h>
#include <gtk/gtk.h>

enum Orientation
{
	ORIENTATION_UNKNOWN           = 0,
	ORIENTATION_NORMAL            = 1,
	ORIENTATION_MIRROR_H          = 2,
	ORIENTATION_ROT_180           = 3,
	ORIENTATION_MIRROR_V          = 4,
	ORIENTATION_MIRROR_H_ROT_270  = 5,
	ORIENTATION_ROT_90            = 6,
	ORIENTATION_MIRROR_H_ROT_90   = 7,
	ORIENTATION_ROT_270           = 8,
};

struct PictureInfo {
	GFile       *gfile;
	Orientation orientation;
	GdkPixbuf  *pixbuf;

	// methods
	PictureInfo(void);
	virtual ~PictureInfo();
};

#endif // PictureInfo_h
