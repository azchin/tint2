/**************************************************************************
*
* Tint2conf
*
* Copyright (C) 2009 Thierry lorthiois (lorthiois@bbsoft.fr)
*
* This program is free software; you can redistribute it and/or
* modify it under the terms of the GNU General Public License version 2
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
**************************************************************************/

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <locale.h>
#include <gtk/gtk.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <glib/gi18n.h>

#include "common.h"


// TODO
// ** add, saveas
// - liste de fichiers tint2rc*
// - menu contextuel dans liste
// - double clic dans liste
// - données globales
// - delete
// - rename
// - apply
// - sauvegarde et lecture taille de fenetre
// - activation des menus sur sélection dans la liste
// - dialogue propriétés ...


#define LONG_VERSION_STRING "0.7"

enum { LIST_ITEM = 0, N_COLUMNS };

// default config file and directory
char *pathConfig = 0;
char *pathDir = 0;

GtkWidget *g_theme_view;
GtkListStore *g_store;

static GtkUIManager *globalUIManager = NULL;

static void menuAddWidget (GtkUIManager *, GtkWidget *, GtkContainer *);

// action on menus
static void menuAdd (GtkWindow * parent);
//static void menuSaveAs (GtkWindow *parent);
static void menuDelete (void);
static void menuProperties (void);
static void menuRename (void);
static void menuQuit (void);
static void menuRefresh (void);
static void menuRefreshAll (void);
static void menuApply (void);
static void menuAbout(GtkWindow * parent);

static gboolean view_onPopupMenu (GtkWidget *treeview, gpointer userdata);
static gboolean view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata);
static void viewRowActivated( GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data);

// TreeView
static void loadTheme();
static void init_list(GtkWidget *list);
static void add_to_list(GtkWidget *list, const gchar *str);
void on_changed(GtkWidget *widget, gpointer label);

void read_config(char **defaultTheme);
void write_config(char *defaultTheme);
void check_theme();


// define menubar, toolbar and popup
static const char *global_ui =
	"<ui>"
	"  <menubar name='MenuBar'>"
	"    <menu action='ThemeMenu'>"
	"      <menuitem action='ThemeAdd'/>"
//	"      <menuitem action='ThemeSaveAs'/>"
	"      <separator/>"
	"      <menuitem action='ThemeDelete'/>"
	"      <separator/>"
	"      <menuitem action='ThemeProperties'/>"
	"      <menuitem action='ThemeRename'/>"
	"      <separator/>"
	"      <menuitem action='ThemeQuit'/>"
	"    </menu>"
	"    <menu action='ViewMenu'>"
	"      <menuitem action='ViewRefresh'/>"
	"      <menuitem action='ViewRefreshAll'/>"
	"    </menu>"
	"    <menu action='HelpMenu'>"
	"      <menuitem action='HelpAbout'/>"
	"    </menu>"
	"  </menubar>"
	"  <toolbar  name='ToolBar'>"
	"    <toolitem action='ViewRefreshAll'/>"
	"    <separator/>"
	"    <toolitem action='ThemeProperties'/>"
	"    <toolitem action='ViewApply'/>"
	"  </toolbar>"
	"  <popup  name='ThemePopup'>"
	"    <menuitem action='ThemeProperties'/>"
	"    <menuitem action='ThemeRename'/>"
	"    <separator/>"
	"    <menuitem action='ThemeDelete'/>"
	"  </popup>"
	"</ui>";


// define menubar and toolbar action
static GtkActionEntry entries[] = {
	{"ThemeMenu", NULL, "Theme", NULL, NULL, NULL},
	{"ThemeAdd", GTK_STOCK_ADD, "_Add...", "<Control>N", "Add theme", G_CALLBACK (menuAdd)},
//	{"ThemeSaveAs", GTK_STOCK_SAVE_AS, "_Save as...", NULL, "Save theme as", G_CALLBACK (menuSaveAs)},
	{"ThemeDelete", GTK_STOCK_DELETE, "_Delete", NULL, "Delete theme", G_CALLBACK (menuDelete)},
	{"ThemeProperties", GTK_STOCK_PROPERTIES, "_Properties...", NULL, "Show properties", G_CALLBACK (menuProperties)},
	{"ThemeRename", NULL, "_Rename...", NULL, "Rename theme", G_CALLBACK (menuRename)},
	{"ThemeQuit", GTK_STOCK_QUIT, "_Quit", "<control>Q", "Quit", G_CALLBACK (menuQuit)},
	{"ViewMenu", NULL, "View", NULL, NULL, NULL},
	{"ViewRefresh", GTK_STOCK_REFRESH, "Refresh", NULL, "Refresh", G_CALLBACK (menuRefresh)},
	{"ViewRefreshAll", GTK_STOCK_REFRESH, "Refresh all", NULL, "Refresh all", G_CALLBACK (menuRefreshAll)},
	{"ViewApply", GTK_STOCK_APPLY, "Apply", NULL, "Apply theme", G_CALLBACK (menuApply)},
	{"HelpMenu", NULL, "Help", NULL, NULL, NULL},
	{"HelpAbout", GTK_STOCK_ABOUT, "_About", "<Control>A", "About", G_CALLBACK (menuAbout)}
};


int main (int argc, char ** argv)
{
	GtkWidget *window;
	GtkWidget *vBox = NULL;
	GtkActionGroup *actionGroup;
	GtkTreeSelection *sel;

	gtk_init (&argc, &argv);
	g_thread_init( NULL );
	check_theme();

	// define main layout : container, menubar, toolbar
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), _("Panel theming"));
	gtk_window_set_default_size(GTK_WINDOW(window), 600, 350);
	g_signal_connect (G_OBJECT (window), "destroy", G_CALLBACK (menuQuit), NULL);
	vBox = gtk_vbox_new (FALSE, 0);
   gtk_container_add (GTK_CONTAINER(window), vBox);

	actionGroup = gtk_action_group_new ("menuActionGroup");
   gtk_action_group_add_actions (actionGroup, entries, G_N_ELEMENTS (entries), NULL);
	globalUIManager = gtk_ui_manager_new();
   gtk_ui_manager_insert_action_group (globalUIManager, actionGroup, 0);
	gtk_ui_manager_add_ui_from_string (globalUIManager, global_ui, -1, NULL );
	g_signal_connect(globalUIManager, "add_widget", G_CALLBACK (menuAddWidget), vBox);
	gtk_ui_manager_ensure_update(globalUIManager);

	// define tree view
	g_theme_view = gtk_tree_view_new();
	gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(g_theme_view), FALSE);
	//gtk_tree_view_set_fixed_height_mode(GTK_TREE_VIEW(g_theme_view), TRUE);
	//col = GTK_TREE_VIEW_COLUMN (g_object_new (GTK_TYPE_TREE_VIEW_COLUMN, "title", _("Theme"), "resizable", TRUE, "sizing", GTK_TREE_VIEW_COLUMN_FIXED, NULL));
	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	gtk_tree_selection_set_mode(GTK_TREE_SELECTION(sel), GTK_SELECTION_SINGLE);
   gtk_box_pack_start(GTK_BOX(vBox), g_theme_view, TRUE, TRUE, 0);
   gtk_widget_show(g_theme_view);
	g_signal_connect(g_theme_view, "button-press-event", (GCallback)view_onButtonPressed, NULL);
	g_signal_connect(g_theme_view, "popup-menu", (GCallback)view_onPopupMenu, NULL);
	g_signal_connect(g_theme_view, "row-activated", G_CALLBACK(viewRowActivated), NULL);
	g_signal_connect(sel, "changed",	G_CALLBACK(on_changed), NULL);

	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	renderer = gtk_cell_renderer_text_new();
	column = gtk_tree_view_column_new_with_attributes("List Items", renderer, "text", LIST_ITEM, NULL);
	gtk_tree_view_append_column(GTK_TREE_VIEW(g_theme_view), column);
	g_store = gtk_list_store_new(N_COLUMNS, G_TYPE_STRING);
	gtk_tree_view_set_model(GTK_TREE_VIEW(g_theme_view), GTK_TREE_MODEL(g_store));

   // load themes
	loadTheme(g_theme_view);

	// rig up idle/thread routines
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::load_cache_images), true);
	//Glib::Thread::create(sigc::mem_fun(window.view, &Thumbview::create_cache_images), true);

	gtk_widget_show_all(window);
	gtk_main ();
	return 0;
}


static void menuAddWidget (GtkUIManager * p_uiManager, GtkWidget * p_widget, GtkContainer * p_box)
{
   gtk_box_pack_start(GTK_BOX(p_box), p_widget, FALSE, FALSE, 0);
   gtk_widget_show(p_widget);
}


static void menuAbout(GtkWindow * parent)
{
	const char *authors[] = { "Thierry Lorthiois", "Christian Ruppert (Build system)", NULL };

	gtk_show_about_dialog( parent, "name", g_get_application_name( ),
								"comments", _("Theming tool for tint2 panel"),
								"version", LONG_VERSION_STRING,
								"copyright", _("Copyright 2009 tint2 team\nLicense GNU GPL version 2"),
								"logo-icon-name", NULL, "authors", authors,
								/* Translators: translate "translator-credits" as
									your name to have it appear in the credits in the "About"
									dialog */
								"translator-credits", _("translator-credits"),
								NULL );
}


static void menuAdd (GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;
	GtkFileFilter *filter;

	dialog = gtk_file_chooser_dialog_new(_("Add a theme"), parent, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_ADD, GTK_RESPONSE_ACCEPT, NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_current_folder(chooser, g_get_home_dir());
	gtk_file_chooser_set_select_multiple(chooser, TRUE);

	filter = gtk_file_filter_new();
	gtk_file_filter_set_name(filter, _("Tint2 theme files"));
	gtk_file_filter_add_pattern(filter, "*.tint2rc");
	gtk_file_chooser_add_filter(chooser, filter);

	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		GSList *l, *list = gtk_file_chooser_get_filenames(chooser);

		gchar *file, *pt1, *name, *path;
		for (l = list; l ; l = l->next) {
			file = (char *)l->data;
			pt1 = strrchr (file, '/');
			if (pt1) {
				pt1++;
				if (*pt1) {
					name = strdup(pt1);
					path = g_build_filename (g_get_user_config_dir(), "tint2", name, NULL);
					copy_file(file, path);
					g_free(path);
					pt1 = strstr(name, ".tint2rc");
					if (pt1) {
						file = strndup(name, pt1-name);
						add_to_list(g_theme_view, file);
						g_free(file);
					}
					g_free(name);
				}
			}
		}

		g_slist_foreach(list, (GFunc)g_free, NULL);
		g_slist_free(list);
	}
	gtk_widget_destroy (dialog);
}

/*
static void menuSaveAs (GtkWindow *parent)
{
	GtkWidget *dialog;
	GtkFileChooser *chooser;

	dialog = gtk_file_chooser_dialog_new (_("Save theme as"), parent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
	chooser = GTK_FILE_CHOOSER(dialog);

	gtk_file_chooser_set_do_overwrite_confirmation (chooser, TRUE);
	gtk_file_chooser_set_current_folder (chooser, g_get_home_dir());
	gtk_file_chooser_set_current_name (chooser, _("Untitled document"));

	if (gtk_dialog_run (GTK_DIALOG(dialog)) == GTK_RESPONSE_ACCEPT) {
		char *filename = gtk_file_chooser_get_filename(chooser);
		printf("fichier %s\n", filename);
		//save_to_file (filename);
		g_free (filename);
	}
	gtk_widget_destroy (dialog);
}
*/

static void menuDelete (void)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *value, *name1, *name2;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
		name1 = g_build_filename (g_get_user_config_dir(), "tint2", value, NULL);
		name2 = g_strdup_printf("%s.tint2rc", name1);
		g_free(name1);

		printf("selected row %s\n", value);
		//g_remove(name2);
/*
		GtkListStore *store;
		GtkTreeIter iter;
		store = GTK_LIST_STORE(model);
		gtk_list_store_remove(store, &iter);
*/
		g_free(value);
		g_free(name2);
	}
}


static void menuProperties (void)
{
	system("python /home/thil/Desktop/tintwizard/tintwizard.py");
}


static void menuRename (void)
{
	printf("menuRename\n");
	// g_rename
}


static void menuQuit (void)
{
	g_object_unref(g_store);
   gtk_main_quit ();
}


static void menuRefresh (void)
{
	printf("menuRefresh\n");
}


static void menuRefreshAll (void)
{
	printf("menuRefreshAll\n");
}


static void menuApply (void)
{
	GtkTreeSelection *sel;
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *value, *name1, *name2;

	sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(g_theme_view));
	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(sel), &model, &iter)) {
		gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
		name1 = g_build_filename (g_get_user_config_dir(), "tint2", value, NULL);
		name2 = g_strdup_printf("%s.tint2rc", name1);
		g_free(name1);

		copy_file(name2, pathConfig);
		write_config(value);
		g_free(value);
		g_free(name2);

		// restart panel
		system("killall -SIGUSR1 tint2");
	}
}


static void view_popup_menu(GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	GtkWidget *w = gtk_ui_manager_get_widget(globalUIManager, "/ThemePopup");

	gtk_menu_popup(GTK_MENU(w), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0, gdk_event_get_time((GdkEvent*)event));
}


static gboolean view_onButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	/* single click with the right mouse button? */
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3) {
		/* optional: select row if no row is selected or only
		 *  one other row is selected (will only do something
		 *  if you set a tree selection mode as described later
		 *  in the tutorial) */
		GtkTreeSelection *selection;

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));

		/* Note: gtk_tree_selection_count_selected_rows() does not
		*   exist in gtk+-2.0, only in gtk+ >= v2.2 ! */
		if (gtk_tree_selection_count_selected_rows(selection)  <= 1) {
			GtkTreePath *path;

			/* Get tree path for row that was clicked */
			if (gtk_tree_view_get_path_at_pos(GTK_TREE_VIEW(treeview), (gint) event->x, (gint) event->y, &path, NULL, NULL, NULL)) {
				gtk_tree_selection_unselect_all(selection);
				gtk_tree_selection_select_path(selection, path);
				gtk_tree_path_free(path);
			}
		}

		view_popup_menu(treeview, event, userdata);
		return TRUE;
	}
	return FALSE;
}


static gboolean view_onPopupMenu (GtkWidget *treeview, gpointer userdata)
{
	view_popup_menu(treeview, NULL, userdata);
	return TRUE;
}


static void viewRowActivated(GtkTreeView *tree_view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	menuApply();
}


static void loadTheme(GtkWidget *list)
{
	GDir *dir;
	gchar *file, *pt1, *name;

	dir = g_dir_open(pathDir, 0, NULL);
	while ((file = g_dir_read_name(dir))) {
		pt1 = strstr(file, ".tint2rc");
		if (pt1) {
			name = strndup(file, pt1-file);
			add_to_list(list, name);
			g_free(name);
		}
	}
	g_dir_close(dir);

	// search default theme
	GtkTreeIter iter;
	GtkTreeModel *model;
	name = NULL;
	read_config(&name);
	if (name) {
		printf("defaultTheme %s\n", name);
		//gtk_tree_selection_select_iter(GtkTreeSelection *selection, GtkTreeIter *iter);
	}
}


// theme file management
void read_config(char **defaultTheme)
{
	char *path;

	path = g_build_filename (g_get_user_config_dir(), "tint2", "tint2confrc", NULL);
	if (g_file_test (path, G_FILE_TEST_EXISTS)) {
		FILE *fp;
		char line[80];
		char *key, *value;
		if ((fp = fopen(path, "r")) != NULL) {
			while (fgets(line, sizeof(line), fp) != NULL) {
				if (parse_line(line, &key, &value)) {
					if (strcmp (key, "default_theme") == 0) {
						*defaultTheme = strdup (value);
					}
					free (key);
					free (value);
				}
			}
			fclose (fp);
		}
	}
	g_free(path);
}


void write_config(char *defaultTheme)
{
	char *path;
	FILE *fp;

	path = g_build_filename (g_get_user_config_dir(), "tint2", "tint2confrc", NULL);
	fp = fopen(path, "w");
	if (fp != NULL) {
		fputs("#---------------------------------------------\n", fp);
		fputs("# TINT2CONF CONFIG FILE\n", fp);
		fprintf(fp, "default_theme = %s\n\n", defaultTheme);
		fclose (fp);
	}
	g_free(path);
}


void check_theme()
{
	pathDir = g_build_filename (g_get_user_config_dir(), "tint2", NULL);
	if (!g_file_test (pathDir, G_FILE_TEST_IS_DIR))
		g_mkdir(pathDir, 0777);

	pathConfig = g_build_filename (g_get_user_config_dir(), "tint2", "tint2rc", NULL);

}


static void add_to_list(GtkWidget *list, const gchar *str)
{
	GtkListStore *store;
	GtkTreeIter iter;

	store = GTK_LIST_STORE(gtk_tree_view_get_model (GTK_TREE_VIEW(list)));

	gtk_list_store_append(store, &iter);
	gtk_list_store_set(store, &iter, LIST_ITEM, str, -1);
}


void on_changed(GtkWidget *widget, gpointer label)
{
	GtkTreeIter iter;
	GtkTreeModel *model;
	char *value;

	if (gtk_tree_selection_get_selected(GTK_TREE_SELECTION(widget), &model, &iter)) {
		gtk_tree_model_get(model, &iter, LIST_ITEM, &value,  -1);
		//gtk_label_set_text(GTK_LABEL(label), value);
		g_free(value);
	}

}


