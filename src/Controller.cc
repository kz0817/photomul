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
#include <libexif/exif-data.h>
#include "Controller.h"

// ----------------------------------------------------------------------------
// PictureInfo
// ----------------------------------------------------------------------------
PictureInfo::PictureInfo(void)
: orientation(ORIENTATION_UNKNOWN),
  pixbuf(NULL),
  surface(NULL)
{
}

PictureInfo::~PictureInfo()
{
	if (pixbuf)
		g_object_unref(pixbuf);
	if (surface)
		cairo_surface_destroy(surface);
}

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
Controller::Controller(void)
: m_widget(NULL),
  m_curr_dir(NULL),
  m_in_making_file_list(false)
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
	PictureInfo *picture_info = new PictureInfo();
	picture_info->path = path;

	// exif
	parse_exif(path, picture_info);
	g_message("%s: orientation: %d", path.c_str(),
	          picture_info->orientation);

	// make pixbuf
	GError *error = NULL;
	picture_info->pixbuf = gdk_pixbuf_new_from_file(path.c_str(), &error);
	if (!picture_info->pixbuf) {
		g_warning("Failed to open file: %s: %s",
		          path.c_str(), error->message);
		g_error_free(error);
		return;
	}

	// rotation if needed
	rotate_picture_if_needed(picture_info);

	// make cairo surface
	size_t n_channels = gdk_pixbuf_get_n_channels(picture_info->pixbuf);
	if (n_channels != 3) {
		g_warning(
		  "color channel is %zd: %s", n_channels, path.c_str());
		return;
	}
	size_t src_width = gdk_pixbuf_get_width(picture_info->pixbuf);
	size_t src_height = gdk_pixbuf_get_height(picture_info->pixbuf);
	cairo_surface_t *surf = cairo_image_surface_create(
	                          CAIRO_FORMAT_RGB24,
	                          src_width, src_height);
	cairo_status_t stat = cairo_surface_status(surf);
	if (stat != CAIRO_STATUS_SUCCESS) {
		g_warning(
		  "Failed to call cairo_image_surface_create(): "
		  "%s: %d\n", path.c_str(), stat);
		return;
	}

	// convert data
	uint8_t *src = gdk_pixbuf_get_pixels(picture_info->pixbuf);
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

	picture_info->surface = surf;
	m_image_view.set_cairo_surface(surf);

	// set current directory
	set_current_directory(path);
}

// ----------------------------------------------------------------------------
// Private methods
// ----------------------------------------------------------------------------
int Controller::get_integer(ExifEntry *exif_entry)
{
	int data = 0;
	for (size_t i = 0; i < exif_entry->size; i++) {
		data <<= 8;
		data += exif_entry->data[i];
	}
	return data;
}

void Controller::rotate_picture_if_needed(PictureInfo *picture_info)
{
	GdkPixbuf *pixbuf = NULL;
	GdkPixbuf *src = picture_info->pixbuf;
	switch (picture_info->orientation) {
	case ORIENTATION_UNKNOWN:
	case ORIENTATION_NORMAL:
		return;
	case ORIENTATION_MIRROR_H:
	case ORIENTATION_ROT_180:
	case ORIENTATION_MIRROR_V:
	case ORIENTATION_MIRROR_H_ROT_270:
		g_warning("Not implemented: rotation %d",
		          picture_info->orientation);
		return;
	case ORIENTATION_ROT_90:
		pixbuf = gdk_pixbuf_rotate_simple
		           (src, GDK_PIXBUF_ROTATE_CLOCKWISE);
		break;
	case ORIENTATION_MIRROR_H_ROT_90:
		g_warning("Not implemented: rotation %d",
		          picture_info->orientation);
		return;
	case ORIENTATION_ROT_270:
		pixbuf = gdk_pixbuf_rotate_simple
		           (src, GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
		break;
	default:
		g_warning("Unknown rotation %d",
		          picture_info->orientation);
		return;
	}

	if (!pixbuf) {
		g_warning("Failed to rotate");
		return;
	}
	g_object_unref(picture_info->pixbuf);
	picture_info->pixbuf = pixbuf;
}

void Controller::set_current_directory(const string &path)
{
	// check the path type
	GFile *gfile = g_file_new_for_path(path.c_str());
	GCancellable *cancellable = NULL;
	GFileType file_type =
	  g_file_query_file_type(gfile, G_FILE_QUERY_INFO_NONE, cancellable);
	if (file_type == G_FILE_TYPE_UNKNOWN) {
		// Probably the path doesn't exist.
		g_object_unref(gfile);
		return;
	}

	if (file_type == G_FILE_TYPE_DIRECTORY) {
		set_current_directory(gfile);
	} else if (file_type == G_FILE_TYPE_REGULAR) {
		GFile *dir = g_file_get_parent(gfile);
		set_current_directory(dir);
		// If 'dir' is used, the used counter of it is incremented in
		// the above set_current_directory(). So we have to call
		// g_object_unref() here.
		g_object_unref(dir);
	} else {
		g_warning("Unexpected file type: %d, %s",
		          file_type, path.c_str());
	}
	g_object_unref(gfile);
}

void Controller::set_current_directory(GFile *dir)
{
	// We do nothing if 'dir' is the same directory as the current one.
	if (m_curr_dir &&
	    strcmp(g_file_get_path(m_curr_dir), g_file_get_path(dir)) == 0) {
		return;
	}

	if (m_curr_dir)
		g_object_unref(m_curr_dir);
	m_curr_dir = dir;
	g_object_ref(m_curr_dir);
	g_debug("New current path: %s", g_file_get_path(m_curr_dir));
	m_in_making_file_list = true;
}

void Controller::parse_exif(const string &path, PictureInfo *picture_info)
{
	ExifEntry *exif_entry;

	// open
	ExifData *exif_data = exif_data_new_from_file(path.c_str());
	if (!exif_data) {
		g_warning("Failed to parse exif: %s", path.c_str());
		return;
	}

	// rotation
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_ORIENTATION);
	if (exif_entry) {
		if (exif_entry->format != EXIF_FORMAT_SHORT) {
			g_warning("Format of ORIENTATION: unexpected: %d",
			          exif_entry->format);
		} else {
			picture_info->orientation =
			  static_cast<Orientation>(get_integer(exif_entry));
		}
	} else {
		g_warning("Failed to parse ORIENTATION");
	}
	exif_data_unref(exif_data);
}

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
