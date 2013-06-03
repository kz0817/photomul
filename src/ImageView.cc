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

#include <stdint.h>
#include "ImageView.h"

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
ImageView::ImageView(void)
: m_widget(NULL),
  m_surface(NULL),
  m_picture_info(NULL),
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
	if (m_surface)
		cairo_surface_destroy(m_surface);
}

GtkWidget *ImageView::get_widget(void)
{
	if (!m_widget) {
		m_widget = gtk_drawing_area_new();
		connect_signals();
	}
	return m_widget;
}

void ImageView::set_picture_info(PictureInfo *picture_info)
{
	m_picture_info = picture_info;
	m_surf_width = 0;
	m_surf_height = 0;
	m_surf_aspect_ratio = 1.0;
	if (m_surface) {
		cairo_surface_destroy(m_surface);
		m_surface = NULL;
	}
	if (m_widget)
		gtk_widget_queue_draw(m_widget);
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

bool ImageView::prepare_surface(void)
{
	// return immediately if we already have the surface
	if (m_surface)
		return true;

	// check if the required variables have been prepared.
	if (!m_picture_info) {
		g_error("m_picture_info: NULL");
		return false;
	}

	if (!m_picture_info->pixbuf) {
		g_error("m_picture_info->pixbuf: NULL");
		return false;
	}

	// make cairo surface
	size_t n_channels = gdk_pixbuf_get_n_channels(m_picture_info->pixbuf);
	if (n_channels != 3) {
		g_warning(
		  "color channel is %zd: %s", n_channels,
		  g_file_get_path(m_picture_info->gfile));
		return false;
	}
	size_t src_width  = gdk_pixbuf_get_width(m_picture_info->pixbuf);
	size_t src_height = gdk_pixbuf_get_height(m_picture_info->pixbuf);
	cairo_surface_t *surf = cairo_image_surface_create(
	                          CAIRO_FORMAT_RGB24,
	                          src_width, src_height);
	cairo_status_t stat = cairo_surface_status(surf);
	if (stat != CAIRO_STATUS_SUCCESS) {
		g_warning(
		  "Failed to call cairo_image_surface_create(): "
		  "%s: %d\n",
		  g_file_get_path(m_picture_info->gfile), stat);
		return false;
	}

	// convert data
	uint8_t *src = gdk_pixbuf_get_pixels(m_picture_info->pixbuf);
	uint8_t *buf = cairo_image_surface_get_data(surf);
	for (size_t y = 0; y < src_height;  y++) {
		for (size_t x = 0; x < src_width; x++) {
			buf[0] = src[2];
			buf[1] = src[1];
			buf[2] = src[0];
			src += n_channels;
			buf += 4; // cairo's pix is always 32bit.
		}
	}

	// calculate surface info
	m_surf_width = cairo_image_surface_get_width(surf);
	m_surf_height = cairo_image_surface_get_height(surf);
	m_surf_aspect_ratio = (float)m_surf_width / m_surf_height;
	m_surface = surf;
	return true;
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
	if (!prepare_surface())
		return FALSE;

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
