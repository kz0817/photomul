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
#include <algorithm>
#include <libexif/exif-data.h>
#include "Controller.h"

// ----------------------------------------------------------------------------
// Public methods
// ----------------------------------------------------------------------------
Controller::Controller(void)
: m_widget(NULL),
  m_curr_dir(NULL),
  m_curr_picture_info(NULL),
  m_file_list_cancellable(NULL)
{
	m_supported_extensions.insert("jpg");
	m_supported_extensions.insert("jpeg");

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
	picture_info->gfile = g_file_new_for_path(path.c_str());

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

	// set the picture_info to ImageView (the picture will be shown)
	m_image_view.set_picture_info(picture_info);

	// set current directory
	m_curr_picture_info = picture_info;
	set_current_directory(path);
}

// ----------------------------------------------------------------------------
// Private methods
// ----------------------------------------------------------------------------
string Controller::get_current_dir_name(void)
{
	if (!m_curr_dir) {
		g_warning("m_curr_dir: NULL at %s", __PRETTY_FUNCTION__);
		return "";
	}
	char *dir_name = g_file_get_path(m_curr_dir);
	string dir = g_file_get_path(m_curr_dir);
	g_free(dir_name);
	return dir;
}

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

	// clear file list information
	m_file_list.clear();
	m_file_list_itr = m_file_list.end();

	// list up files in the directory.
	const char *attributes = "";
	m_file_list_cancellable = g_cancellable_new();
	g_file_enumerate_children_async(m_curr_dir, attributes,
	                                G_FILE_QUERY_INFO_NONE,
	                                G_PRIORITY_DEFAULT,
	                                m_file_list_cancellable,
	                                file_enum_ready_cb, this);
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
	Controller *obj = static_cast<Controller *>(user_data);
	guint keyval;
	gboolean succeeded = gdk_event_get_keyval(event, &keyval);
	if (!succeeded) {
		g_warning("Failed to get key value");
		return FALSE;
	}
	g_debug("Key press event: %u", keyval);
	if (keyval == ' ')
		obj->show_next();
	return TRUE;
}

void Controller::request_file_enum_next(GFileEnumerator *file_enum)
{
	int num_request_files = 1;
	g_file_enumerator_next_files_async(file_enum, num_request_files,
	                                   G_PRIORITY_DEFAULT,
	                                   m_file_list_cancellable,
	                                   file_enum_next_cb, this);
}

void Controller::cleanup_file_enum(void)
{
	// free cancellable object
	g_object_unref(m_file_list_cancellable);
	m_file_list_cancellable = NULL;
}

void Controller::show_next(void)
{
	if (m_file_list_itr == m_file_list.end()) {
		g_warning("File list iterator is not set.");
		return;
	}
	if (m_file_list.empty()) {
		g_warning("File list is empty.");
		return;
	}

	m_file_list_itr++;
	if (m_file_list_itr == m_file_list.end())
		m_file_list_itr = m_file_list.begin();
	g_message("next file: %s", m_file_list_itr->c_str());
	set_path(*m_file_list_itr);
}

bool Controller::is_supported_picture(const string &file_name)
{
	// get the position of the last '.'
	size_t pos = file_name.find_last_of('.');
	if (pos == string::npos)
		return false;

	// get the extension in small letters
	string ext(file_name, pos + 1);
	transform(ext.begin(), ext.end(), ext.begin(), ::tolower);

	// check if the extension is the supported 
	set<string>::iterator it = m_supported_extensions.find(ext);
	return it != m_supported_extensions.end();
}

void Controller::add_picture_of_curr_dir(GFileInfo *file_info)
{
	const char *file_name = g_file_info_get_name(file_info);
	g_debug("added to the file list: %s", file_name);
	string path = get_current_dir_name();
	path += G_DIR_SEPARATOR;
	path += file_name;
	m_file_list.push_back(path);

	// set iterator for current position if the file is the pic. shown now
	if (m_curr_picture_info) {
		const char *curr_shown_file_name =
		  g_file_get_basename(m_curr_picture_info->gfile);
		if (strcmp(curr_shown_file_name, file_name) == 0) {
			g_debug(
			  "Found the currently shown picture for iterator.");
			m_file_list_itr = m_file_list.end();
			m_file_list_itr--;
		}
	}
}

void Controller::file_enum_ready_cb(GObject *source_object,
                                    GAsyncResult *res, gpointer user_data)
{
	Controller *obj = static_cast<Controller *>(user_data);
	GError *error = NULL;
	GFileEnumerator *file_enum =
	  g_file_enumerate_children_finish(G_FILE(source_object), res, &error);
	if (!file_enum) {
		g_warning("Failed to g_file_enumerate_children_finish: %s: %s",
		          g_file_get_path(obj->m_curr_dir), error->message);
		g_error_free(error);
		obj->cleanup_file_enum();
		return;
	}
	obj->request_file_enum_next(file_enum);
}

void Controller::file_enum_next_cb(GObject *source_object,
                                   GAsyncResult *res, gpointer user_data)
{
	Controller *obj = static_cast<Controller *>(user_data);
	GError *error = NULL;
	GFileEnumerator *file_enum = G_FILE_ENUMERATOR(source_object);
	GList *list =
	  g_file_enumerator_next_files_finish(file_enum, res, &error);
	if (error) {
		g_warning("Failed to g_file_enumerate_children_finish: %s: %s",
		          g_file_get_path(obj->m_curr_dir), error->message);
		g_error_free(error);
		obj->cleanup_file_enum();
		return;
	}

	size_t count = 0;
	for (; list; list = g_list_next(list), count++) {
		GFileInfo *file_info = G_FILE_INFO(list->data);
		GFileType type = g_file_info_get_file_type(file_info);
		if (type != G_FILE_TYPE_REGULAR)
			continue;
		const char *file_name = g_file_info_get_name(file_info);
		if (!obj->is_supported_picture(file_name))
			continue;
		obj->add_picture_of_curr_dir(file_info);
		g_object_unref(file_info);
	}
	g_list_free(list);

	if (count != 0) {
		// request the next file
		obj->request_file_enum_next(file_enum);
		return;
	}
	g_debug("Successfully got file list.");

	obj->cleanup_file_enum();
}
