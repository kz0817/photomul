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

	// This variable is not null during the making of file list.
	GCancellable          *m_file_list_cancellable;

	int get_integer(ExifEntry *exif_entry);
	void parse_exif(const string &path, PictureInfo *picture_info);
	void rotate_picture_if_needed(PictureInfo *picture_info);
	void set_current_directory(const string &path);
	void set_current_directory(GFile *dir);
	void connect_signals(void);
	void request_file_enum_next(GFileEnumerator *file_enum);
	void cleanup_file_enum(void);
	void show_next(void);
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
