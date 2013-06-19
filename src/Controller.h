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
#include <list>
#include <set>
#include <gtk/gtk.h>
#include <libexif/exif-data.h>
#include "ImageView.h"
#include "PictureInfo.h"

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
	GFile     *m_curr_dir;
	PictureInfo *m_curr_picture_info;
	set<string>            m_supported_extensions;
	list<string>           m_file_list;
	list<string>::iterator m_file_list_itr;
	PictureInfoMap         m_picture_info_map;

	// This variable is not null during the making of file list.
	GCancellable          *m_file_list_cancellable;

	string get_current_dir_name(void);
	int get_integer(ExifEntry *exif_entry);
	bool get_rational(ExifEntry *exif_entry,
	                  int &numerator, int &denominator);
	bool get_string(ExifEntry *exif_entry, string &str);
	void parse_exif(const string &path, PictureInfo *picture_info);
	void rotate_picture_if_needed(PictureInfo *picture_info);

	/**
	 * make a new instance of PictureInfo with loading a picture.
	 * @path
	 * A path of the target picture file
	 * @return
	 * A PictureInfo instance on success. Otherwise NULL.
	 */
	PictureInfo *make_picture_info(const string &path);

	/**
	 * find the the PictureInfo in m_picture_info_set with path.
	 * @path
	 * A path of the target picture file
	 * @return
	 * The found PictureInfo instance on success. Otherwise NULL.
	 */
	PictureInfo *find_picture_info(const string &path);

	void set_current_directory(const string &path);
	void set_current_directory(GFile *dir);
	void connect_signals(void);
	void request_file_enum_next(GFileEnumerator *file_enum);
	void cleanup_file_enum(void);
	bool show_check_common(void);
	void show_prev(void);
	void show_next(void);
	void show_info(void);
	bool is_supported_picture(const string &file_name);
	void add_picture_of_curr_dir(GFileInfo *file_info);
	static gboolean _key_press_event(GtkWidget *widget, GdkEvent *event,
	                                 gpointer user_data);
	static void file_enum_ready_cb(GObject *source_object,
	                               GAsyncResult *res, gpointer user_data);
	static void file_enum_next_cb(GObject *source_object,
	                              GAsyncResult *res, gpointer user_data);
};

#endif // Controller_h
