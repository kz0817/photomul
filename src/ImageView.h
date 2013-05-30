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

#ifndef ImageView_h
#define ImageView_h

#include <string>
#include <gtk/gtk.h>

using namespace std;

class ImageView {
public:
	ImageView(void);
	virtual ~ImageView();
	GtkWidget *get_widget(void);
	void set_file(const string &path);
private:
	GtkWidget *m_widget;
	size_t m_area_width, m_area_height;
	
	void connect_signals(void);

	static gboolean
	_draw(GtkWidget *widget, cairo_t *cr, gpointer user_data);
	static gboolean
	_configure_event(GtkWidget *widget, GdkEvent *event, gpointer user_data);
};

#endif // ImageView_h
