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
  m_surface(NULL),
  m_area_width(0),
  m_area_height(0),
  m_surf_width(0),
  m_surf_height(0),
  m_area_aspect_ratio(1.0),
  m_surf_aspect_ratio(1.0)
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

void ImageView::set_cairo_surface(cairo_surface_t *surface)
{
	m_surface = surface;
	m_surf_width = cairo_image_surface_get_width(surface);
	m_surf_height = cairo_image_surface_get_height(surface);
	m_surf_aspect_ratio = (float)m_surf_width / m_surf_height;
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
	ImageView *obj = static_cast<ImageView *>(user_data);
	return obj->draw(widget, cr);
}

gboolean ImageView::draw(GtkWidget *widget, cairo_t *cr)
{
	g_debug("DRAW");
	if (!m_surface)
		g_debug("surface: NULL");

	// draw block for all region
	cairo_set_source_rgb(cr, 0, 0, 0);
	cairo_rectangle(cr, 0, 0, m_area_width, m_area_width);
	cairo_fill(cr);
	
	// draw the surface at center
	float x_scale = 1.0;
	float y_scale = 1.0;
	float x_offset = 0;
	float y_offset = 0;
	if (m_area_aspect_ratio >= m_surf_aspect_ratio) {
		y_scale = (float)m_area_height / m_surf_height;
		x_scale = y_scale;
		x_offset = (m_area_width - m_surf_width * x_scale) / 2;
	} else {
		x_scale = (float)m_area_width / m_surf_width;
		y_scale = x_scale;
		y_offset = (m_area_height - m_surf_height  * y_scale) / 2;
	}
	cairo_scale(cr, x_scale, y_scale);
	cairo_set_source_surface(cr, m_surface,
	                         x_offset / x_scale, y_offset / y_scale);
	cairo_paint(cr);

	return TRUE;
}

gboolean ImageView::_configure_event(GtkWidget *widget, GdkEvent *event,
                                     gpointer user_data)
{
	ImageView *obj = static_cast<ImageView *>(user_data);
	GdkEventConfigure *configureEvt = (GdkEventConfigure *)event;
	obj->m_area_width = configureEvt->width;
	obj->m_area_height = configureEvt->height;
	obj->m_area_aspect_ratio =
	  (float)obj->m_area_width / obj->m_area_height;
	g_debug("Configure Event: %zdx%zd",
	        obj->m_area_width, obj->m_area_height);
	return TRUE;
}
