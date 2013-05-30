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
#include "ImageView.h"

using namespace std;

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

	void connect_signals(void);
	static gboolean _key_press_event(GtkWidget *widget, GdkEvent *event,
	                                 gpointer user_data);
};

#endif // Controller_h
