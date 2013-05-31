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

#ifndef Controller_h
#define Controller_h

#include <string>
#include <gtk/gtk.h>
#include <libexif/exif-data.h>
#include "ImageView.h"

using namespace std;

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
	string      path;
	Orientation orientation;
	GdkPixbuf  *pixbuf;

	PictureInfo(void)
	: orientation(ORIENTATION_UNKNOWN),
	  pixbuf(NULL)
	{
	}
};

class Controller
{
public:
	Controller(void);
	virtual ~Controller();
	GtkWidget *get_widget(void);
	void set_path(const string &path);

private:
	GtkWidget *m_widget;
	ImageView  m_image_view;

	int get_integer(ExifEntry *exif_entry);
	void parse_exif(const string &path, PictureInfo *picture_info);
	void rotate_picture_if_needed(PictureInfo *picture_info);
	void connect_signals(void);
	static gboolean _key_press_event(GtkWidget *widget, GdkEvent *event,
	                                 gpointer user_data);
};

#endif // Controller_h
