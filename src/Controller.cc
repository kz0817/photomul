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

#include <cstring>
#include <stdint.h>
#include "Controller.h"

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
Controller::Controller(void)
: m_widget(NULL)
{
	m_widget = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_can_focus(m_widget, TRUE); 
	connect_signals();

	gtk_box_pack_start(GTK_BOX(m_widget), m_image_view.get_widget(),
	                   TRUE, TRUE, 0);
}

Controller::~Controller()
{
}

GtkWidget *Controller::get_widget(void)
{
	return m_widget;
}

void Controller::set_path(const string &path)
{
	GError *error = NULL;
	GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
	if (!pixbuf) {
		g_warning("Failed to open file: %s: %s\n",
		          path.c_str(), error->message);
		g_error_free(error);
		return;
	}
	size_t n_channels = gdk_pixbuf_get_n_channels(pixbuf);
	if (n_channels != 3) {
		g_warning(
		  "color channel is %zd: %s", n_channels, path.c_str());
		return;
	}
	size_t src_width = gdk_pixbuf_get_width(pixbuf);
	size_t src_height = gdk_pixbuf_get_height(pixbuf);
	cairo_surface_t *surf = cairo_image_surface_create(
	                          CAIRO_FORMAT_RGB24,
	                          src_width, src_height);
	cairo_status_t stat = cairo_surface_status(surf);
	if (stat != CAIRO_STATUS_SUCCESS) {
		g_warning(
		  "Failed to call cairo_image_surface_create(): "
		  "%s: %d\n", path.c_str(), stat);
	}

	// convert data
	uint8_t *src = gdk_pixbuf_get_pixels(pixbuf);
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

	m_image_view.set_cairo_surface(surf);
}

// ----------------------------------------------------------------------------
// Private methods
// ----------------------------------------------------------------------------
void Controller::connect_signals(void)
{
	g_signal_connect(m_widget, "key-press-event",
	                 G_CALLBACK(_key_press_event), this);
}

gboolean Controller::_key_press_event(GtkWidget *widget, GdkEvent *event,
                                      gpointer user_data)
{
	guint keyval;
	gboolean succeeded = gdk_event_get_keyval(event, &keyval);
	if (!succeeded) {
		g_warning("Failed to get key value");
		return FALSE;
	}
	g_debug("Key press event: %u", keyval);
	return TRUE;
}
