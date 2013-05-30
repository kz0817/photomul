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

#include "ImageView.h"

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
ImageView::ImageView(void)
: m_widget(NULL),
  m_area_width(0),
  m_area_height(0)
{
}

ImageView::~ImageView(void)
{
}

GtkWidget *ImageView::get_widget(void)
{
	if (!m_widget) {
		m_widget = gtk_drawing_area_new();
		connect_signals();
	}
	return m_widget;
}

void ImageView::set_file(const string &path)
{
}

// ----------------------------------------------------------------------------
// Private methods
// ----------------------------------------------------------------------------
void ImageView::connect_signals(void)
{
	g_signal_connect(m_widget, "draw",
	                 G_CALLBACK(_draw), this);
	g_signal_connect(m_widget, "configure-event",
	                 G_CALLBACK(_configure_event), this);
}

gboolean ImageView::_draw(GtkWidget *widget, cairo_t *cr,
                          gpointer user_data)
{
	cairo_move_to(cr, 0, 0);
	cairo_line_to(cr, 100, 100);
	cairo_stroke(cr);
	g_debug("DRAW");
	return TRUE;
}

gboolean ImageView::_configure_event(GtkWidget *widget, GdkEvent *event,
                                     gpointer user_data)
{
	ImageView *obj = static_cast<ImageView *>(user_data);
	GdkEventConfigure *configureEvt = (GdkEventConfigure *)event;
	obj->m_area_width = configureEvt->width;
	obj->m_area_height = configureEvt->height;
	g_debug("Configure Event: %zdx%zd",
	        obj->m_area_width, obj->m_area_height);
	return TRUE;
}
