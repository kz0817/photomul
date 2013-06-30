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
#include "Utils.h"

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

	// box for the main picture
	GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
	gtk_box_pack_start(GTK_BOX(vbox), m_image_view.get_widget(),
	                   TRUE, TRUE, 0);
	
	// overlay for the base
	m_widget = gtk_overlay_new();
	gtk_widget_set_can_focus(m_widget, TRUE);
	connect_signals();
	gtk_container_add(GTK_CONTAINER(m_widget), vbox);
}

Controller::~Controller()
{
	PictureInfoMapIterator it = m_picture_info_map.begin();
	for (; it != m_picture_info_map.end(); ++it)
		delete it->second;
}

GtkWidget *Controller::get_widget(void)
{
	return m_widget;
}

void Controller::set_path(const string &path)
{
	PictureInfo *picture_info = find_picture_info(path);
	if (!picture_info) {
		picture_info = make_picture_info(path);
		if (!picture_info)
			return;
		// insert the instance to the set
		m_picture_info_map[path] = picture_info;
	}

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
	return Utils::get_path(m_curr_dir);
}

bool Controller::get_integer(ExifEntry *exif_entry, int &data)
{
	if (!exif_entry)
		return false;
	data = 0;
	for (size_t i = 0; i < exif_entry->size; i++) {
		data <<= 8;
		data += exif_entry->data[i];
	}
	return true;
}

bool Controller::get_rational(ExifEntry *exif_entry,
                              int &numerator, int &denominator)
{
	static const size_t EXPECTED_SIZE = 8;
	if (!exif_entry)
		return false;
	if (exif_entry->size != EXPECTED_SIZE) {
		g_warning("exif_entry->size != %zd: %d",
		          EXPECTED_SIZE, exif_entry->size);
		return false;
	}

	numerator = 0;
	for (size_t i = 0; i < 4; i++) {
		numerator <<= 8;
		numerator += exif_entry->data[i];
	}

	denominator = 0;
	for (size_t i = 5; i < 8; i++) {
		denominator <<= 8;
		denominator += exif_entry->data[i];
	}

	return true;
}

bool Controller::get_string(ExifEntry *exif_entry, string &str)
{
	if (!exif_entry)
		return false;
	str = string((const char *)exif_entry->data, exif_entry->size);
	return true;
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

PictureInfo *Controller::make_picture_info(const string &path)
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
		delete picture_info;
		return NULL;
	}

	// rotation if needed
	rotate_picture_if_needed(picture_info);

	return picture_info;
}

PictureInfo *Controller::find_picture_info(const string &path)
{
	PictureInfoMapIterator it = m_picture_info_map.find(path);
	if (it == m_picture_info_map.end())
		return NULL;
	return it->second;
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
	    Utils::get_path(m_curr_dir) == Utils::get_path(dir)) {
		return;
	}

	if (m_curr_dir)
		g_object_unref(m_curr_dir);
	m_curr_dir = dir;
	g_object_ref(m_curr_dir);
	g_debug("New current path: %s", Utils::get_path(m_curr_dir).c_str());

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
	bool succeeded;
	ExifEntry *exif_entry;

	// open
	ExifData *exif_data = exif_data_new_from_file(path.c_str());
	if (!exif_data) {
		g_warning("Failed to parse exif: %s", path.c_str());
		return;
	}

	// rotation
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_ORIENTATION);
	int orientation = ORIENTATION_UNKNOWN;
	succeeded = get_integer(exif_entry, orientation);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_MAKE");
	picture_info->orientation = static_cast<Orientation>(orientation);
	g_debug("Orientation: %d", picture_info->orientation);

	// shutter speed
	exif_entry = exif_data_get_entry(exif_data,
	                                 EXIF_TAG_SHUTTER_SPEED_VALUE);
	if (exif_entry) {
		printf("format: %d\n", exif_entry->format);
	} else {
		g_warning("Failed to parse EXIF_TAG_SHUTTER_SPEED_VALUE");
	}

	// aperture
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_APERTURE_VALUE);
	if (exif_entry) {
		printf("format: %d\n", exif_entry->format);
	} else {
		g_warning("Failed to parse EXIF_TAG_APERTURE_VALUE");
	}

	// exposure time
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_EXPOSURE_TIME);
	succeeded = get_rational(exif_entry,
	                         picture_info->exposure_numerator,
	                         picture_info->exposure_denominator);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_EXPOSURE_TIME");
	g_debug("Exposure time: %d/%d",
	        picture_info->exposure_numerator,
	        picture_info->exposure_denominator);

	// F number
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_FNUMBER);
	succeeded = get_rational(exif_entry,
	                         picture_info->fnumber_numerator,
	                         picture_info->fnumber_denominator);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_FNUMBER");
	g_debug("F number: %d/%d",
	        picture_info->fnumber_numerator,
	        picture_info->fnumber_denominator);

	// maker, model
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_MAKE);
	succeeded = get_string(exif_entry, picture_info->maker);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_MAKE");

	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_MODEL);
	succeeded = get_string(exif_entry, picture_info->model);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_MODEL");
	g_debug("Maker: Model: %s: %s",
	        picture_info->maker.c_str(), picture_info->model.c_str());

	// ISO
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_ISO_SPEED_RATINGS);
	succeeded = get_integer(exif_entry, picture_info->iso_speed);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_ISO_SPEED_RATINGS");
	g_debug("ISO: %d", picture_info->iso_speed);

	// adjustment
	exif_entry = exif_data_get_entry(exif_data,
	                                 EXIF_TAG_EXPOSURE_BIAS_VALUE);
	succeeded = get_rational(exif_entry,
	                         picture_info->exposure_bias_numerator,
	                         picture_info->exposure_bias_denominator);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_EXPOSURE_BIAS_VALUE");
	g_debug("Expsure bias: %d/%d",
	        picture_info->exposure_bias_numerator,
	        picture_info->exposure_bias_denominator);

	// focal length
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_FOCAL_LENGTH);
	succeeded = get_rational(exif_entry,
	                         picture_info->focal_length_numerator,
	                         picture_info->focal_length_denominator);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_FOCAL_LENGTH");
	g_debug("Focal length: %d/%d",
	        picture_info->focal_length_numerator,
	        picture_info->focal_length_denominator);

	// date and time
	exif_entry = exif_data_get_entry(exif_data, EXIF_TAG_DATE_TIME);
	succeeded = get_string(exif_entry, picture_info->date_time);
	if (!succeeded)
		g_warning("Failed to parse EXIF_TAG_DATE_TIME");
	g_debug("DATE: %s", picture_info->date_time.c_str());

	// unref data
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
	// Key code definition can be found in gdk/gdkkeysms.h
	Controller *obj = static_cast<Controller *>(user_data);
	guint keyval;
	gboolean succeeded = gdk_event_get_keyval(event, &keyval);
	if (!succeeded) {
		g_warning("Failed to get key value");
		return FALSE;
	}
	g_debug("Key press event: %u", keyval);
	if (keyval == GDK_KEY_space || keyval == GDK_KEY_j)
		obj->show_next();
	else if (keyval == GDK_KEY_BackSpace || keyval == GDK_KEY_k)
		obj->show_prev();
	else if (keyval == GDK_KEY_i)
		obj->show_info();
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

bool Controller::show_check_common(void)
{
	if (m_file_list_itr == m_file_list.end()) {
		g_warning("File list iterator is not set.");
		return false;
	}
	if (m_file_list.empty()) {
		g_warning("File list is empty.");
		return false;
	}
	return true;
}

void Controller::show_prev(void)
{
	if (!show_check_common())
		return;

	if (m_file_list_itr == m_file_list.begin())
		m_file_list_itr = m_file_list.end();
	m_file_list_itr--;
	g_message("prev file: %s", m_file_list_itr->c_str());
	set_path(*m_file_list_itr);
}

void Controller::show_next(void)
{
	if (!show_check_common())
		return;

	m_file_list_itr++;
	if (m_file_list_itr == m_file_list.end())
		m_file_list_itr = m_file_list.begin();
	g_message("next file: %s", m_file_list_itr->c_str());
	set_path(*m_file_list_itr);
}

void Controller::show_info(void)
{
	g_message("SHOW_INFO ***********\n");
	GtkWidget *label = gtk_label_new ("LABBBBBEL\nFOOOOO\nAAABB***");
	gtk_widget_set_halign(label, GTK_ALIGN_START);
	gtk_widget_set_valign(label, GTK_ALIGN_START);
	gtk_overlay_add_overlay(GTK_OVERLAY(m_widget), label);
	gtk_widget_show_all(label);
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
		          Utils::get_path(obj->m_curr_dir).c_str(),
		          error->message);
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
		          Utils::get_path(obj->m_curr_dir).c_str(),
		          error->message);
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
