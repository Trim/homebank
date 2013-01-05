/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2013 Maxime DOYEN
 *
 *  This file is part of HomeBank.
 *
 *  HomeBank is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  HomeBank is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include "homebank.h"

#include "dsp_mainwindow.h"

#include "list_account.h"
#include "list_upcoming.h"
#include "list_topspending.h"

#include "dsp_account.h"
#include "import.h"
#include "imp_qif.h"
#include "ui-assist-start.h"
#include "ui-account.h"
#include "ui-payee.h"
#include "ui-category.h"
#include "ui-archive.h"
#include "ui-assign.h"
#include "ui-budget.h"
#include "ui-pref.h"
#include "ui-hbfile.h"
#include "ui-transaction.h"

#include "rep_balance.h"
#include "rep_budget.h"
#include "rep_stats.h"
#include "rep_time.h"
#include "rep_vehicle.h"

#include "gtk-chart.h"

//#define HOMEBANK_URL_HELP           "http://homebank.free.fr/help/"
#define HOMEBANK_URL_HELP           "index.html"
#define HOMEBANK_URL_HELP_ONLINE    "https://launchpad.net/homebank/+addquestion"
#define HOMEBANK_URL_HELP_TRANSLATE "https://launchpad.net/homebank/+translations"
#define HOMEBANK_URL_HELP_PROBLEM   "https://launchpad.net/homebank/+filebug"


/****************************************************************************/
/* Debug macros                                                             */
/****************************************************************************/
#define MYDEBUG 0
#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;
extern gchar *homebank_pixmaps_dir;


/* our functions prototype */
static void ui_mainwindow_action_new(void);
static void ui_mainwindow_action_open(void);
static void ui_mainwindow_action_save(void);
static void ui_mainwindow_action_saveas(void);
static void ui_mainwindow_action_revert(void);
static void ui_mainwindow_action_properties(void);
static void ui_mainwindow_action_close(void);
static void ui_mainwindow_action_quit(void);

static void ui_mainwindow_action_defaccount(void);
static void ui_mainwindow_action_defpayee(void);
static void ui_mainwindow_action_defcategory(void);
static void ui_mainwindow_action_defarchive(void);
static void ui_mainwindow_action_defbudget(void);
static void ui_mainwindow_action_defassign(void);
static void ui_mainwindow_action_preferences(void);

static void ui_mainwindow_action_toggle_toolbar(GtkToggleAction *action);
static void ui_mainwindow_action_toggle_upcoming(GtkToggleAction *action);
static void ui_mainwindow_action_toggle_topspending(GtkToggleAction *action);
static void ui_mainwindow_action_toggle_minor(GtkToggleAction *action);

static void ui_mainwindow_action_showtransactions(void);
static void ui_mainwindow_action_addtransactions(void);
static void ui_mainwindow_action_checkscheduled(void);

static void ui_mainwindow_action_statistic(void);
static void ui_mainwindow_action_trendtime(void);
static void ui_mainwindow_action_budget(void);
static void ui_mainwindow_action_balance(void);
static void ui_mainwindow_action_vehiclecost(void);

static void ui_mainwindow_action_import(void);
static void ui_mainwindow_action_export(void);
static void ui_mainwindow_action_anonymize(void);

static void ui_mainwindow_action_help(void);
void ui_mainwindow_action_help_welcome(void);
static void ui_mainwindow_action_help_online(void);
static void ui_mainwindow_action_help_translate(void);
static void ui_mainwindow_action_help_problem(void);
static void ui_mainwindow_action_about(void);


static GtkWidget *ui_mainwindow_create_recent_chooser_menu (GtkRecentManager *manager);
static void ui_mainwindow_populate_accounts(GtkWidget *widget, gpointer user_data);
static void ui_mainwindow_populate_upcoming(GtkWidget *widget, gpointer user_data);

void ui_mainwindow_open(GtkWidget *widget, gpointer user_data);

void ui_mainwindow_save(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_revert(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_action(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_toggle_minor(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_clear(GtkWidget *widget, gpointer user_data);

void ui_mainwindow_busy(GtkWidget *widget, gboolean state);

gboolean ui_dialog_msg_savechanges(GtkWidget *widget, gpointer user_data);

void ui_mainwindow_update(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_check_scheduled(GtkWidget *widget, gpointer user_data);
void ui_mainwindow_addtransactions(GtkWidget *widget, gpointer user_data);

void ui_mainwindow_recent_add (struct hbfile_data *data, const gchar *path);

extern gchar *CYA_ACC_TYPE[];

static GtkActionEntry entries[] = {

  /* name, stock id, label */

  { "FileMenu"     , NULL, N_("_File") },
  { "EditMenu"     , NULL, N_("_Edit") },
  { "ViewMenu"     , NULL, N_("_View") },
  { "ManageMenu"   , NULL, N_("_Manage") },
  { "TransactionMenu", NULL, N_("_Transactions") },
  { "ReportMenu"   , NULL, N_("_Reports")  },
  { "HelpMenu"     , NULL, N_("_Help") },

//  { "Import"       , NULL, N_("Import") },
//  { "Export"       , NULL, N_("Export to") },
	/* name, stock id, label, accelerator, tooltip */

  /* FileMenu */
  { "New"        , GTK_STOCK_NEW            , N_("_New")          , NULL, N_("Create a new file"),    G_CALLBACK (ui_mainwindow_action_new) },
  { "Open"       , GTK_STOCK_OPEN           , N_("_Open...")      , NULL, N_("Open a file"),    G_CALLBACK (ui_mainwindow_action_open) },
  { "Save"       , GTK_STOCK_SAVE           , N_("_Save")         , NULL, N_("Save the current file"),    G_CALLBACK (ui_mainwindow_action_save) },
  { "SaveAs"     , GTK_STOCK_SAVE_AS        , N_("Save As...")    , "<shift><control>S", N_("Save the current file with a different name"),    G_CALLBACK (ui_mainwindow_action_saveas) },
  { "Revert"     , GTK_STOCK_REVERT_TO_SAVED, N_("Revert")        , NULL, N_("Revert to a saved version of this file"),    G_CALLBACK (ui_mainwindow_action_revert) },

  { "Properties" , GTK_STOCK_PROPERTIES     , N_("_Properties..."), NULL, N_("Configure the file"),    G_CALLBACK (ui_mainwindow_action_properties) },
  { "Close"      , GTK_STOCK_CLOSE          , N_("_Close")        , NULL, N_("Close the current file"),    G_CALLBACK (ui_mainwindow_action_close) },
  { "Quit"       , GTK_STOCK_QUIT           , N_("_Quit")         , NULL, N_("Quit homebank"),    G_CALLBACK (ui_mainwindow_action_quit) },

  /* Exchange */
  { "FileImport" , "hb-file-import"         , N_("Import...")     , NULL, N_("Open the import assistant"),    G_CALLBACK (ui_mainwindow_action_import) },
  { "ExportQIF"  , "hb-file-export"         , N_("Export...")     , NULL, N_("Open the export to QIF assistant"),    G_CALLBACK (ui_mainwindow_action_export) },
  { "Anonymize"  , NULL                     , N_("Anonymize...")  , NULL, NULL,    G_CALLBACK (ui_mainwindow_action_anonymize) },

  /* EditMenu */
  { "Preferences", GTK_STOCK_PREFERENCES, N_("Preferences..."), NULL,    N_("Configure homebank"),    G_CALLBACK (ui_mainwindow_action_preferences) },

  /* ManageMenu */
//  { "Currency"   , "hb-currency"  , N_("Currencies...") , NULL,    N_("Configure the currencies"), G_CALLBACK (ui_mainwindow_action_defcurrency) },
  { "Account"    , "hb-account"   , N_("Acc_ounts...")  , NULL,    N_("Configure the accounts"), G_CALLBACK (ui_mainwindow_action_defaccount) },
  { "Payee"      , "hb-payee"     , N_("_Payees...")    , NULL,    N_("Configure the payees"),    G_CALLBACK (ui_mainwindow_action_defpayee) },
  { "Category"   , "hb-category"  , N_("Categories...") , NULL,    N_("Configure the categories"),    G_CALLBACK (ui_mainwindow_action_defcategory) },
  { "Archive"    , "hb-archive"   , N_("Scheduled/Template...")  , NULL,    N_("Configure the scheduled/template transactions"),    G_CALLBACK (ui_mainwindow_action_defarchive) },
  { "Budget"     , "hb-budget"    , N_("Budget...")     , NULL,    N_("Configure the budget"),    G_CALLBACK (ui_mainwindow_action_defbudget) },
  { "Assign"     , "hb-assign"    , N_("Assignments..."), NULL,    N_("Configure the automatic assignments"),    G_CALLBACK (ui_mainwindow_action_defassign) },

  /* TransactionMenu */
  { "ShowOpe"    , HB_STOCK_OPE_SHOW, N_("Show...")             , NULL, N_("Shows selected account transactions"),    G_CALLBACK (ui_mainwindow_action_showtransactions) },
  { "AddOpe"     , HB_STOCK_OPE_ADD , N_("Add...")              , NULL, N_("Add transaction"),    G_CALLBACK (ui_mainwindow_action_addtransactions) },
  { "Scheduler"  , NULL             , N_("Set scheduler...")    , NULL, N_("Configure the transaction scheduler"),    G_CALLBACK (ui_mainwindow_action_properties) },
  { "AddScheduled"  , NULL             , N_("Process scheduled..."), NULL, N_("Insert pending scheduled transactions"),    G_CALLBACK (ui_mainwindow_action_checkscheduled) },

  /* ReportMenu */
  { "RStatistics" , HB_STOCK_REP_STATS , N_("_Statistics...") , NULL,    N_("Open the Statistics report"),    G_CALLBACK (ui_mainwindow_action_statistic) },
  { "RTrendTime"   , HB_STOCK_REP_TIME , N_("_Trend Time...") , NULL,    N_("Open the Trend Time report"),    G_CALLBACK (ui_mainwindow_action_trendtime) },
  { "RBudget"    , HB_STOCK_REP_BUDGET, N_("B_udget...")     , NULL,    N_("Open the Budget report"),    G_CALLBACK (ui_mainwindow_action_budget) },
  { "RBalance"  , HB_STOCK_REP_BALANCE, N_("Balance...")  , NULL,    N_("Open the Balance report"),    G_CALLBACK (ui_mainwindow_action_balance) },
  { "RVehiculeCost"    , HB_STOCK_REP_CAR   , N_("_Vehicle cost...")   , NULL,    N_("Open the Vehicle cost report"),    G_CALLBACK (ui_mainwindow_action_vehiclecost) },

  /* HelpMenu */
  { "Contents"   , GTK_STOCK_HELP    , N_("_Contents")                    , "F1", N_("Documentation about HomeBank"), G_CALLBACK (ui_mainwindow_action_help) },
  { "Welcome"    , NULL              , N_("Show welcome dialog...")       , NULL, NULL                              , G_CALLBACK (ui_mainwindow_action_help_welcome) },
  { "Online"     , "lpi-help"        , N_("Get Help Online...")           , NULL, N_("Connect to the LaunchPad website for online help"), G_CALLBACK (ui_mainwindow_action_help_online) },
  { "Translate"  , "lpi-translate"   , N_("Translate this Application..."), NULL, N_("Connect to the LaunchPad website to help translate this application"), G_CALLBACK (ui_mainwindow_action_help_translate) },
  { "Problem"    , "lpi-bug"         , N_("Report a Problem...")          , NULL, N_("Connect to the LaunchPad website to help fix problems"), G_CALLBACK (ui_mainwindow_action_help_problem) },

  { "About"      , GTK_STOCK_ABOUT      , N_("_About")     , NULL, N_("About HomeBank")      ,G_CALLBACK (ui_mainwindow_action_about) },

};
static guint n_entries = G_N_ELEMENTS (entries);


static GtkToggleActionEntry toggle_entries[] = {
/*  name         , stockid, label, accelerator, tooltip, callback, is_active */
  { "Toolbar"    , NULL                 , N_("_Toolbar")  , NULL,    NULL,    G_CALLBACK (ui_mainwindow_action_toggle_toolbar), TRUE },
  { "Spending"   , NULL                 , N_("_Top spending") , NULL,    NULL,    G_CALLBACK (ui_mainwindow_action_toggle_topspending), TRUE },
  { "Upcoming"   , NULL                 , N_("_Scheduled list") , NULL,    NULL,    G_CALLBACK (ui_mainwindow_action_toggle_upcoming), TRUE },
  { "AsMinor"    , NULL                 , N_("Minor currency"), "<control>M",    NULL,    G_CALLBACK (ui_mainwindow_action_toggle_minor), FALSE },
};

static guint n_toggle_entries = G_N_ELEMENTS (toggle_entries);


static const gchar *ui_info =
"<ui>"

"  <menubar name='MenuBar'>"
"    <menu action='FileMenu'>"
"      <menuitem action='New'/>"
"      <menuitem action='Open'/>"
"        <separator/>"
"      <menuitem action='Save'/>"
"      <menuitem action='SaveAs'/>"
"      <menuitem action='Revert'/>"
//"        <separator/>"
// print to come here
"        <separator/>"
"      <menuitem action='Properties'/>"
"        <separator/>"
"      <menuitem action='FileImport'/>"
"      <menuitem action='ExportQIF'/>"
"      <menuitem action='Anonymize'/>"
"        <separator/>"
"      <menuitem action='Close'/>"
"      <menuitem action='Quit'/>"
"    </menu>"
"    <menu action='EditMenu'>"
"      <menuitem action='Preferences'/>"
"    </menu>"
"    <menu action='ViewMenu'>"
"      <menuitem action='Toolbar'/>"
"        <separator/>"
"      <menuitem action='Spending'/>"
"        <separator/>"
"      <menuitem action='Upcoming'/>"
"        <separator/>"
"      <menuitem action='AsMinor'/>"
"    </menu>"
"    <menu action='ManageMenu'>"
//"      <menuitem action='Currency'/>"
"      <menuitem action='Account'/>"
"      <menuitem action='Payee'/>"
"      <menuitem action='Category'/>"
"      <menuitem action='Assign'/>"
"      <menuitem action='Archive'/>"
"      <menuitem action='Budget'/>"
"    </menu>"
"    <menu action='TransactionMenu'>"
"      <menuitem action='ShowOpe'/>"
"      <menuitem action='AddOpe'/>"
"        <separator/>"
"      <menuitem action='Scheduler'/>"
"      <menuitem action='AddScheduled'/>"
"    </menu>"
"    <menu action='ReportMenu'>"
"      <menuitem action='RStatistics'/>"
"      <menuitem action='RTrendTime'/>"
"      <menuitem action='RBalance'/>"
"      <menuitem action='RBudget'/>"
"      <menuitem action='RVehiculeCost'/>"
"    </menu>"
"    <menu action='HelpMenu'>"
"      <menuitem action='Contents'/>"
"        <separator/>"
"      <menuitem action='Welcome'/>"
"        <separator/>"
"      <menuitem action='Online'/>"
"      <menuitem action='Translate'/>"
"      <menuitem action='Problem'/>"
"        <separator/>"
"      <menuitem action='About'/>"
"    </menu>"
"  </menubar>"

"  <toolbar  name='ToolBar'>"
"    <toolitem action='New'/>"
"    <toolitem action='Save'/>"
"      <separator/>"
//"    <toolitem action='Currency'/>"
"    <toolitem action='Account'/>"
"    <toolitem action='Payee'/>"
"    <toolitem action='Category'/>"
"    <toolitem action='Assign'/>"
"    <toolitem action='Archive'/>"
"    <toolitem action='Budget'/>"
"      <separator/>"
"    <toolitem action='ShowOpe'/>"
"    <toolitem action='AddOpe'/>"
"      <separator/>"
"    <toolitem action='RStatistics'/>"
"    <toolitem action='RTrendTime'/>"
"    <toolitem action='RBalance'/>"
"    <toolitem action='RBudget'/>"
"    <toolitem action='RVehiculeCost'/>"
"  </toolbar>"

"</ui>";


/* TODO: a bouger */


/*
**
*/
void ui_mainwindow_revert(GtkWidget *widget, gpointer user_data)
{
//struct hbfile_data *data;
GtkWidget *dialog;
gchar *basename;
gint result;

	DB( g_printf("\n[ui-mainwindow] revert\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

		basename = g_path_get_basename(GLOBALS->xhb_filepath);
		dialog = gtk_message_dialog_new
		(
			GTK_WINDOW(GLOBALS->mainwindow),
			GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			GTK_MESSAGE_QUESTION,
			GTK_BUTTONS_NONE,
			_("Revert to the previously saved file of '%s'?"),
			basename
		);
		g_free(basename);

		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("- Changes made to the file will be permanently lost\n"
			"- File will be restored to the last save (.xhb~)")
			);

		gtk_dialog_add_buttons (GTK_DIALOG(dialog),
		    GTK_STOCK_CANCEL, 0,
			GTK_STOCK_REVERT_TO_SAVED, 1,
			NULL);

	  result = gtk_dialog_run( GTK_DIALOG( dialog ) );
	  gtk_widget_destroy( dialog );

	if( result == 1)
	{
		DB( g_printf(" - should revert\n") );
		hbfile_change_filepath(homebank_filepath_with_extention(GLOBALS->xhb_filepath, "xhb~"));

		ui_mainwindow_open_internal(widget, NULL);

		//todo: error here after, we must set the filename bak to .xhb

	}

}

static void
activate_url (GtkAboutDialog *about,
	      const gchar    *link,
	      gpointer        data)
{
	DB( g_print("activate url %s\n", link) );
	
	homebank_util_url_show (link);
}

static void hbfile_about(void)
{
GtkWidget *about;
gchar *pathfilename;
GdkPixbuf *pixbuf;


  static const gchar *artists[] = {
    "Maxime DOYEN",
    NULL
  };

  static const gchar *authors[] = {
    "Lead developer:\n" \
    "Maxime DOYEN",
    "\nContributor:\n" \
    "Ga\xc3\xabtan LORIDANT (Maths formulas for charts)\n",
    NULL
  };

/*
  const gchar *documenters[] = {
    "Maxime DOYEN",
    NULL
  };
*/

	static const gchar license[] =
		"This program is free software; you can redistribute it and/or modify\n"
		  "it under the terms of the GNU General Public License as\n"
		  "published by the Free Software Foundation; either version 2 of the\n"
		  "License, or (at your option) any later version.\n\n"
		  "This program is distributed in the hope that it will be useful,\n"
		  "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		  "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		  "GNU General Public License for more details.\n\n"
		  "You should have received a copy of the GNU General Public License\n"
		  "along with this program; if not, write to the Free Software\n"
		  "Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, "
		  "MA 02110-1301, USA.";

	static const gchar *copyright = "Copyright \xc2\xa9 1995-2013 - Maxime DOYEN";


	pathfilename = g_build_filename(homebank_app_get_images_dir(), "splash.png", NULL);
	pixbuf = gdk_pixbuf_new_from_file(pathfilename, NULL);
	g_free(pathfilename);

	about  = gtk_about_dialog_new();
	gtk_about_dialog_set_program_name (GTK_ABOUT_DIALOG(about), g_get_application_name ());
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about), PACKAGE_VERSION);
	gtk_about_dialog_set_copyright(GTK_ABOUT_DIALOG(about), copyright);
	gtk_about_dialog_set_comments(GTK_ABOUT_DIALOG(about), _("Free, easy, personal accounting for everyone."));
	gtk_about_dialog_set_license(GTK_ABOUT_DIALOG(about), license);
	//gtk_about_dialog_set_wrap_license(GTK_ABOUT_DIALOG(about), );
	gtk_about_dialog_set_website(GTK_ABOUT_DIALOG(about), "http://homebank.free.fr");
	gtk_about_dialog_set_website_label(GTK_ABOUT_DIALOG(about), "Visit the HomeBank website");
	gtk_about_dialog_set_authors(GTK_ABOUT_DIALOG(about), authors);
	gtk_about_dialog_set_artists(GTK_ABOUT_DIALOG(about), artists);
	//gtk_about_dialog_set_documenters(GTK_ABOUT_DIALOG(about), );
	//gtk_about_dialog_set_translator_credits(GTK_ABOUT_DIALOG(about), );
	gtk_about_dialog_set_logo(GTK_ABOUT_DIALOG(about), pixbuf);
	gtk_about_dialog_set_logo_icon_name(GTK_ABOUT_DIALOG(about), "homebank");
	
	/*
 	gtk_show_about_dialog(GTK_WINDOW(GLOBALS->mainwindow),
		"name", g_get_application_name (),
		"logo-icon-name", "homebank",
		"logo"      , pixbuf,
		"artists"	, artists,
		"authors"	, authors,
	//	"translator-credits"	, "trans",
		"comments"	, _("Free, easy, personal accounting for everyone."),
		"license"	, license,
		"copyright"	, copyright,
		"version"	, PACKAGE_VERSION,
		"website"	, "http://homebank.free.fr",
		"website-label", "Visit the HomeBank website",
        NULL);
	*/
	g_signal_connect (about, "activate-link", G_CALLBACK (activate_url), NULL);

	gtk_dialog_run (GTK_DIALOG (about));

	gtk_widget_destroy (about);
}






/* hbfile action functions -------------------- */
static void ui_mainwindow_action_new(void)
{
GtkWidget *widget = GLOBALS->mainwindow;

	if( ui_dialog_msg_savechanges(widget,NULL) == TRUE )
	{
		//clear all, and init GLOBALS->xhb_filepath to default
		ui_mainwindow_clear(widget, GINT_TO_POINTER(TRUE)); // GPOINTER_TO_INT(
		ui_mainwindow_update(widget, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
	}
}

static void ui_mainwindow_action_open(void)
{
	ui_mainwindow_open(GLOBALS->mainwindow, NULL);
}

static void ui_mainwindow_action_save(void)
{
	ui_mainwindow_save(GLOBALS->mainwindow, GINT_TO_POINTER(FALSE));
}

static void ui_mainwindow_action_saveas(void)
{
	ui_mainwindow_save(GLOBALS->mainwindow, GINT_TO_POINTER(TRUE));
}

static void ui_mainwindow_action_revert(void)
{
	ui_mainwindow_revert(GLOBALS->mainwindow, NULL);
}

static void ui_mainwindow_action_close(void)
{
GtkWidget *widget = GLOBALS->mainwindow;

	if( ui_dialog_msg_savechanges(widget,NULL) == TRUE )
	{
		//clear all, and init GLOBALS->xhb_filepath to default
		ui_mainwindow_clear(widget, GINT_TO_POINTER(TRUE));
		ui_mainwindow_update(widget, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_BALANCE+UF_REFRESHALL));
	}

}


static void ui_mainwindow_action_quit(void)
{
gboolean result;

	//gtk_widget_destroy(GLOBALS->mainwindow);

	g_signal_emit_by_name(GLOBALS->mainwindow, "delete-event", NULL, &result);

	//gtk_main_quit();
}




static void ui_mainwindow_action_properties(void)
{
	create_defhbfile_window();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}

static void ui_mainwindow_action_anonymize(void)
{
gint result;

	result = ui_dialog_msg_question(
		GTK_WINDOW(GLOBALS->mainwindow),
		_("Anonymize the file ?"),
		_("Proceeding will changes name/memo to anonymous datas,\n"
		"please confirm."),
		NULL
		);

	if( result == GTK_RESPONSE_NO )
		return;	
	
	hbfile_anonymize();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_REFRESHALL));
}

/*
static void ui_mainwindow_action_defcurrency(void)
{
	//ui_cur_manage_dialog();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}
*/

static void ui_mainwindow_action_defaccount(void)
{
	ui_acc_manage_dialog();

	//our global list has changed, so update the treeview
	//todo: optimize this, should not call compute balance here
	account_compute_balances ();
	ui_mainwindow_populate_accounts(GLOBALS->mainwindow, NULL);

	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_BALANCE));
}

static void ui_mainwindow_action_defpayee(void)
{
	ui_pay_manage_dialog();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}

static void ui_mainwindow_action_defcategory(void)
{
	ui_cat_manage_dialog();
	//todo:why refresh upcoming here??
	//ui_mainwindow_populate_upcoming(GLOBALS->mainwindow, NULL);
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}

static void ui_mainwindow_action_defarchive(void)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
GtkTreeModel *model;

	data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	// upcoming list have direct pointer to the arc (which may have chnaged)
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc));
	gtk_list_store_clear (GTK_LIST_STORE(model));

	ui_arc_manage_dialog();

	ui_mainwindow_populate_upcoming(GLOBALS->mainwindow, NULL);

	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}


static void ui_mainwindow_action_defbudget(void)
{
	create_defbudget_window();
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}


static void ui_mainwindow_action_defassign(void)
{

	ui_asg_manage_dialog();

	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
}


static void ui_mainwindow_action_preferences(void)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	defpref_dialog_new();
	homebank_pref_createformat();
	if(!PREFS->euro_active)
	{
	GtkToggleAction *action = (GtkToggleAction *)gtk_ui_manager_get_action(data->manager, "/MenuBar/ViewMenu/AsMinor");

		gtk_toggle_action_set_active(action, FALSE);
		ui_mainwindow_action_toggle_minor(action);
	}
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_VISUAL+UF_REFRESHALL));
}

/* display action */

static void ui_mainwindow_action_toggle_toolbar(GtkToggleAction *action)
{
//struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	PREFS->wal_toolbar = gtk_toggle_action_get_active(action);
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_VISUAL));
}

static void ui_mainwindow_action_toggle_upcoming(GtkToggleAction *action)
{
//struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	PREFS->wal_upcoming = gtk_toggle_action_get_active(action);
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_VISUAL));
}

static void ui_mainwindow_action_toggle_topspending(GtkToggleAction *action)
{
//struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	PREFS->wal_spending = gtk_toggle_action_get_active(action);
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_VISUAL));
}

static void ui_mainwindow_action_toggle_minor(GtkToggleAction *action)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");

	GLOBALS->minor = gtk_toggle_action_get_active(action);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_acc));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_upc));

	// top spending
	gtk_chart_show_minor(GTK_CHART(data->RE_pie), GLOBALS->minor);
	hb_label_set_amount(GTK_LABEL(data->TX_topamount), data->toptotal, GLOBALS->minor);

}

static void ui_mainwindow_action_showtransactions(void)
{
struct hbfile_data *data = g_object_get_data(G_OBJECT(GLOBALS->mainwindow), "inst_data");
GtkWidget *window;

	//todo:change this
	if( data->acc )
	{
		if( data->acc->window == NULL )
		{
			window = create_account_window(data->acc->key, data->acc);
			account_init_window(window, NULL);
		}
		else
		{
			if(GTK_IS_WINDOW(data->acc->window))
				gtk_window_present(data->acc->window);

		}
	}
}


static void ui_mainwindow_action_addtransactions(void)
{
	ui_mainwindow_addtransactions(GLOBALS->mainwindow, NULL);
}

static void ui_mainwindow_action_checkscheduled(void)
{
	ui_mainwindow_check_scheduled(GLOBALS->mainwindow, GINT_TO_POINTER(TRUE));
}

static void ui_mainwindow_action_statistic(void)
{
	create_statistic_window();
}

static void ui_mainwindow_action_trendtime(void)
{
	create_trendtime_window();
}

static void ui_mainwindow_action_budget(void)
{
	repbudget_window_new();
}

static void ui_mainwindow_action_balance(void)
{
	repbalance_window_new();
}

static void ui_mainwindow_action_vehiclecost(void)
{
	repcost_window_new();
}

static void ui_mainwindow_action_import(void)
{
	ui_import_window_new();


}


static void ui_mainwindow_action_about(void)
{
	hbfile_about();


}

static void ui_mainwindow_action_export(void)
{
	test_qif_export();


	//create_import_window();
}

static void ui_mainwindow_action_help(void)
{
gchar *link;

    link = g_build_filename("file:///", homebank_app_get_help_dir(), HOMEBANK_URL_HELP, NULL );
	homebank_util_url_show (link);

    g_free(link);
}


//todo: move this to a ui-assist-welcome.c

static void ui_mainwindow_action_help_welcome1 (GtkButton *button, gpointer user_data)
{
	gtk_dialog_response (GTK_DIALOG(user_data), 1);
}

static void ui_mainwindow_action_help_welcome2 (GtkButton *button, gpointer user_data)
{
	gtk_dialog_response (GTK_DIALOG(user_data), 2);
}

static void ui_mainwindow_action_help_welcome3 (GtkButton *button, gpointer user_data)
{
	gtk_dialog_response (GTK_DIALOG(user_data), 3);
}

static void ui_mainwindow_action_help_welcome4 (GtkButton *button, gpointer user_data)
{
	gtk_dialog_response (GTK_DIALOG(user_data), 4);
}

static void ui_mainwindow_action_help_welcome5 (GtkButton *button, gpointer user_data)
{
	gtk_dialog_response (GTK_DIALOG(user_data), 5);
}

void ui_mainwindow_action_help_welcome(void)
{
GtkWidget *dialog;
GtkWidget *mainvbox, *widget, *label;

	dialog = gtk_dialog_new_with_buttons (_("Welcome to HomeBank"),
			GTK_WINDOW(GLOBALS->mainwindow),
			0,
			GTK_STOCK_CLOSE,
			GTK_RESPONSE_ACCEPT,
			NULL);

	mainvbox = gtk_vbox_new (FALSE, 0);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), mainvbox, FALSE, FALSE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(mainvbox), HB_MAINBOX_SPACING);

	label = make_label (_("HomeBank"), 0, 0);
	gimp_label_set_attributes(GTK_LABEL(label), PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, -1);
	gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

	label = make_label (_("Free, easy, personal accounting for everyone."), 0, 0);
	gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

		widget = gtk_hseparator_new();
		gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), widget, FALSE, FALSE, 0);

	mainvbox = gtk_vbox_new (FALSE, HB_MAINBOX_SPACING);
	gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), mainvbox, TRUE, TRUE, 0);
	gtk_container_set_border_width (GTK_CONTAINER(mainvbox), HB_MAINBOX_SPACING);

	label = make_label (_("What do you want to do:"), 0, 0);
	gimp_label_set_attributes(GTK_LABEL(label), PANGO_ATTR_WEIGHT, PANGO_WEIGHT_BOLD, -1);
	gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

	widget = gtk_button_new_with_mnemonic(_("Read HomeBank _Manual"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome1), dialog);
	
	widget = gtk_button_new_with_mnemonic(_("Configure _Preferences"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome2), dialog);
	
	widget = gtk_button_new_with_mnemonic(_("Create a _new file"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome3), dialog);

	widget = gtk_button_new_with_mnemonic(_("_Open an existing file"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome4), dialog);

	widget = gtk_button_new_with_mnemonic(_("Open the _example file"));
	gtk_box_pack_start (GTK_BOX (mainvbox), widget, FALSE, FALSE, 0);
	g_signal_connect (widget, "clicked", G_CALLBACK (ui_mainwindow_action_help_welcome5), dialog);

	//connect all our signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);

	gtk_widget_show_all (dialog);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	// cleanup and destroy
	gtk_widget_destroy (dialog);

	// do appropriate action
	switch(result)
	{
		case 1:
			ui_mainwindow_action_help();
			break;
		case 2:
			ui_mainwindow_action_preferences();
			break;
		case 3:
			ui_mainwindow_action_new();
			break;
		case 4:
			ui_mainwindow_action_open();
			break;
		case 5:
			hbfile_change_filepath(g_build_filename(homebank_app_get_datas_dir(), "example.xhb", NULL));
			ui_mainwindow_open_internal(GLOBALS->mainwindow, NULL);
			break;
	}

}



static void ui_mainwindow_action_help_online(void)
{
const gchar *link = HOMEBANK_URL_HELP_ONLINE;

	homebank_util_url_show (link);

}

static void ui_mainwindow_action_help_translate(void)
{
const gchar *link = HOMEBANK_URL_HELP_TRANSLATE;

	homebank_util_url_show (link);

}

static void ui_mainwindow_action_help_problem(void)
{
const gchar *link = HOMEBANK_URL_HELP_PROBLEM;

	homebank_util_url_show (link);

}




/* hbfile functions -------------------- */




/*
**
*/
static void ui_mainwindow_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	ui_mainwindow_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(UF_SENSITIVE));
}


static void ui_mainwindow_close_openbooks(void)
{
GList *list;

	DB( g_printf("\n[ui-mainwindow] close openbooks\n") );

	list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *item = list->data;

		if(item->window)
		{
			gtk_widget_destroy(GTK_WIDGET(item->window));
			item->window = NULL;
		}

		list = g_list_next(list);
	}
	g_list_free(list);

}



/*
**
*/
void ui_mainwindow_clear(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gboolean file_clear = GPOINTER_TO_INT(user_data);

	DB( g_printf("\n[ui-mainwindow] clear\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	// Close opened account window
	// Clear TreeView
	ui_mainwindow_close_openbooks();
	gtk_tree_store_clear(GTK_TREE_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc))));
	gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc))));

	hbfile_cleanup(file_clear);

	hbfile_setup(file_clear);

	if(file_clear == TRUE)
	{
		ui_start_assistant();
	}

}


/*
** add some transactions directly
*/
void ui_mainwindow_addtransactions(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkWidget *window;
gint result = 1;
guint32 date;
gint account = 1, count;

	DB( g_printf("\n[ui-mainwindow] add transactions\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	/* init the transaction */
	date = GLOBALS->today;
	if(data->acc != NULL)
		account = data->acc->key;

	window = create_deftransaction_window(GTK_WINDOW(data->window), TRANSACTION_EDIT_ADD);
	count = 0;
	while(result == GTK_RESPONSE_ADD)
	{
	Transaction *ope;

		/* fill in the transaction */
		ope = da_transaction_malloc();
		ope->date    = date;
		ope->kacc = account;

		deftransaction_set_transaction(window, ope);

		result = gtk_dialog_run (GTK_DIALOG (window));

		DB( g_printf(" -> dialog result is %d\n", result) );

		if(result == GTK_RESPONSE_ADD || result == GTK_RESPONSE_ACCEPT)
		{
			deftransaction_get(window, NULL);
			transaction_add(ope, NULL, ope->kacc);

			DB( g_printf(" -> added 1 transaction to %d\n", ope->kacc) );

			count++;
		}

		da_transaction_free(ope);
		ope = NULL;

	}


	deftransaction_dispose(window, NULL);
	gtk_widget_destroy (window);

	/* todo optimize this */
	if(count > 0)
	{
		GLOBALS->changes_count += count;
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_BALANCE));
	}
}

struct tmptop
{
	guint32		key;
	gdouble		value;
};

static gint tmptop_compare_func(struct tmptop *tt1, struct tmptop *tt2)
{
	return tt1->value > tt2->value ? 1 : -1;
}


static void ui_mainwindow_populate_topspending(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
gint range;
gint n_result, i;
GArray *garray;
gdouble total;
//Account *acc;

	DB( g_printf("\n[ui-mainwindow] populate_topspending\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_range));

	if(range == FLT_RANGE_OTHER)
		return;
	
	filter_preset_daterange_set(data->filter, range);
	
	
	n_result = da_cat_get_max_key() + 1;
	total = 0.0;

	DB( g_printf(" - max key is %d\n", n_result) );

	/* allocate some memory */
	garray = g_array_sized_new(FALSE, FALSE, sizeof(struct tmptop), n_result);

	if(garray)
	{
	struct tmptop zero = { .key=0, .value=0.0 };
	struct tmptop *initzero = &zero;
	
		for(i=0 ; i<n_result ; i++)
		{
			g_array_insert_val(garray, i, initzero);
		}

		/* compute the results */
		list = g_list_first(GLOBALS->ope_list);
		while (list != NULL)
		{
		Transaction *ope = list->data;
		Account *acc;
			//debug
			//DB( g_print("** testing '%s', cat=%d==> %d\n", ope->wording, ope->category, filter_test(data->filter, ope)) );
			acc = da_acc_get(ope->kacc);
			if(acc == NULL) goto next1;
			if((acc->flags & (AF_CLOSED|AF_NOREPORT))) goto next1;
			if(ope->paymode == PAYMODE_INTXFER) goto next1;

			if( !(ope->flags & OF_REMIND) )
			{
				if( (ope->date >= data->filter->mindate) && (ope->date <= data->filter->maxdate) )
				{
				guint32 pos = 0;
				gdouble trn_amount;

					//trn_amount = to_base_amount(ope->amount, acc->kcur);
					trn_amount = ope->amount;

					if(  ope->flags & OF_SPLIT )
					{
					guint nbsplit = da_transaction_splits_count(ope);
					Split *split;
					
						for(i=0;i<nbsplit;i++)
						{
							split = ope->splits[i];
							Category *catentry = da_cat_get(split->kcat);
								if(catentry)
									pos = (catentry->flags & GF_SUB) ? catentry->parent : catentry->key;

							//trn_amount = to_base_amount(split->amount, acc->kcur);
							trn_amount = split->amount;
							if( trn_amount < 0 )
							{
							struct tmptop *item;

								item = &g_array_index (garray, struct tmptop, pos);
								item->key = pos;
								item->value += trn_amount;
							}
						}
					}
					else
					{
						Category *catentry = da_cat_get(ope->kcat);
							if(catentry)
								pos = (catentry->flags & GF_SUB) ? catentry->parent : catentry->key;
			
						if( trn_amount < 0 )
						{
						struct tmptop *item;
						
							item = &g_array_index (garray, struct tmptop, pos);
							item->key = pos;
							item->value += trn_amount;
						}
					}

				}
			}
next1:
			list = g_list_next(list);
		}

		//todo: we need to sort this and limit before
		g_array_sort(garray, (GCompareFunc)tmptop_compare_func);

		for(i=0 ; i<MIN(garray->len,5) ; i++)
		{
		struct tmptop *item;
		
			item = &g_array_index (garray, struct tmptop, i);
			total += item->value;

			DB( g_printf("%4d k='%d' v='%.2f'\n", i, item->key, item->value) );
		}

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_top));
		gtk_list_store_clear (GTK_LIST_STORE(model));
		g_object_ref(model); /* Make sure the model stays with us after the tree view unrefs it */
		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_top), NULL); /* Detach model from view */

		/* insert into the treeview */
		for(i=0 ; i<MIN(garray->len,5) ; i++)
		{
		gchar *name;
		Category *entry;
		struct tmptop *item;
		gdouble value;
		
			item = &g_array_index (garray, struct tmptop, i);

			if(!item->value) continue;

			value = arrondi(item->value, 2);
			entry = da_cat_get(item->key);
			if(entry == NULL) continue;

			name = entry->key == 0 ? _("(no category)") : entry->name;

			// append test
			gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				  LST_TOPSPEND_ID, i,
				  LST_TOPSPEND_KEY, 0,
				  LST_TOPSPEND_NAME, name,
				  LST_TOPSPEND_AMOUNT, value,
				  LST_TOPSPEND_RATE, (gint)(((ABS(value)*100)/ABS(total)) + 0.5),
				  -1);

		}

		/* Re-attach model to view */
  		gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_top), model);
		g_object_unref(model);
		
		data->toptotal = total;
		hb_label_set_amount(GTK_LABEL(data->TX_topamount), total, GLOBALS->minor);

		gtk_chart_set_color_scheme(GTK_CHART(data->RE_pie), PREFS->report_color_scheme);
		gtk_chart_set_datas(GTK_CHART(data->RE_pie), model, LST_TOPSPEND_AMOUNT);
		//gtk_chart_show_legend(GTK_CHART(data->RE_pie), FALSE);


	  /* update info range text */
		{
		gchar *daterange;
		
			daterange = filter_daterange_text_get(data->filter);
			gtk_widget_set_tooltip_markup(GTK_WIDGET(data->CY_range), daterange);
			g_free(daterange);
		}
	}
	
	/* free our memory */
	g_array_free (garray, TRUE);

}


static void ui_mainwindow_populate_upcoming(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
gdouble total = 0;
gint count = 0;
//Account *acc;

	DB( g_printf("\n[ui-mainwindow] refresh upcoming\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_printf(" should insert in %d days\n", archive_add_get_nbdays()) );


	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_upc));
	gtk_list_store_clear (GTK_LIST_STORE(model));

	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *arc = list->data;
	guint decay;

		if((arc->flags & OF_AUTO) && arc->kacc > 0)
		{
			count++;
			
			decay = arc->nextdate - GLOBALS->today;
			DB( g_printf(" populate: %s : %d\n", arc->wording, decay) );

			gtk_list_store_append (GTK_LIST_STORE(model), &iter);
			gtk_list_store_set (GTK_LIST_STORE(model), &iter,
				  LST_DSPUPC_DATAS, arc,
			      LST_DSPUPC_WORDING, arc->wording,
			      LST_DSPUPC_AMOUNT, arc->amount,
				  LST_DSPUPC_REMAINING, decay,
				  -1);

			//acc = da_acc_get(arc->kacc);
			//total += to_base_amount(arc->amount, acc->kcur);
			total += arc->amount;
			
			DB( g_printf(" total: %f\n", total) );
		}
		list = g_list_next(list);
	}

	// insert total
	if(count > 0 )
	{
		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			  LST_DSPUPC_DATAS, NULL,
			  LST_DSPUPC_WORDING, _("Total"),
			  LST_DSPUPC_AMOUNT, total,
			  LST_DSPUPC_REMAINING, G_MAXINT32,
		  -1);
	}

}

/*
** called after load, importamiga, on demand
*/
void ui_mainwindow_check_scheduled(GtkWidget *widget, gpointer user_data)
{
//struct hbfile_data *data;
gint count;
gint usermode = GPOINTER_TO_INT(user_data);

	DB( g_printf("\n[ui-mainwindow] check scheduled\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	ui_mainwindow_busy(widget, TRUE);

	count = hbfile_insert_scheduled_transactions();

	ui_mainwindow_busy(widget, FALSE);

	//inform the user
	if(usermode == TRUE)
	{
	gchar *txt;

		if(count == 0)
			txt = _("No transaction to add");
		else
			txt = _("transaction added: %d");

		ui_dialog_msg_infoerror(GTK_WINDOW(GLOBALS->mainwindow), GTK_MESSAGE_INFO,
			_("Check scheduled transactions result"),
			txt,
			count);
	}

}

/*
**
*/
void ui_mainwindow_open(GtkWidget *widget, gpointer user_data)
{
//struct hbfile_data *data;
gchar *filename = NULL;

	DB( g_printf("\n[ui-mainwindow] open\n") );

	//data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( ui_dialog_msg_savechanges(widget,NULL) == TRUE )
	{
		if(ui_file_chooser_xhb(GTK_FILE_CHOOSER_ACTION_OPEN, &filename) == TRUE)
		{
			hbfile_change_filepath(filename);

			ui_mainwindow_open_internal(widget, NULL);


		}
	}
}

/*
 *	open the file stored in GLOBALS->xhb_filepath
 */
void ui_mainwindow_open_internal(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gint r;

	DB( g_printf("\n[ui-mainwindow] open internal\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_printf(" -> filename: '%s'\n", GLOBALS->xhb_filepath) );

	if( GLOBALS->xhb_filepath != NULL )
	{
		ui_mainwindow_clear(GLOBALS->mainwindow, GINT_TO_POINTER(FALSE));
		GLOBALS->hbfile_is_new = FALSE;

		r = homebank_load_xml(GLOBALS->xhb_filepath);
		if( r == XML_OK )
		{
			DB( g_printf(" -> file loaded ok : rcode=%d\n", r) );
			
			hbfile_file_hasbackup(GLOBALS->xhb_filepath);
			
			if(PREFS->appendscheduled)
				hbfile_insert_scheduled_transactions();

			homebank_lastopenedfiles_save();

			//todo: remove this after computing done at xml read
			account_compute_balances();

			ui_mainwindow_recent_add(data, GLOBALS->xhb_filepath);
			ui_mainwindow_populate_accounts(GLOBALS->mainwindow, NULL);
			ui_mainwindow_populate_upcoming(GLOBALS->mainwindow, NULL);
			ui_mainwindow_populate_topspending(GLOBALS->mainwindow, NULL);
		}
		else
		{
		gchar *msg = _("Unknow error");

			switch(r)
			{
				case XML_IO_ERROR:
					msg = _("I/O error for file '%s'.");
					break;
				case XML_FILE_ERROR:
					msg = _("The file '%s' is not a valid HomeBank file.");
					break;
				case XML_VERSION_ERROR:	
					msg = _("The file '%s' was saved with a higher version of HomeBank\nand cannot be loaded by the current version.");
					break;
			}

			ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_ERROR,
				_("File error"),
				msg,
				GLOBALS->xhb_filepath
				);

			ui_mainwindow_clear(GLOBALS->mainwindow, GINT_TO_POINTER(TRUE));

		}

		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_VISUAL));
	}


}

/*
**
*/
void ui_mainwindow_save(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gboolean saveas = GPOINTER_TO_INT(user_data);
gchar *filename = NULL;
gint r = XML_UNSET;

	DB( g_printf("\n[ui-mainwindow] save\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( GLOBALS->hbfile_is_new == TRUE )
		saveas = 1;

	if(saveas == 1)
	{
		if(ui_file_chooser_xhb(GTK_FILE_CHOOSER_ACTION_SAVE, &filename) == TRUE)
		{
			DB( g_printf(" + should save as %s\n", GLOBALS->xhb_filepath) );
			hbfile_change_filepath(filename);
			homebank_backup_current_file(GLOBALS->xhb_filepath);
			homebank_file_ensure_xhb();
			r = homebank_save_xml(GLOBALS->xhb_filepath);
			GLOBALS->hbfile_is_new = FALSE;

		}
		else
			return;
	}
	else
	{
		DB( g_printf(" + should quick save %s\n", GLOBALS->xhb_filepath) );
		homebank_backup_current_file(GLOBALS->xhb_filepath);
		homebank_file_ensure_xhb();
		r = homebank_save_xml(GLOBALS->xhb_filepath);
	}


	if(r == XML_OK)
	{
		GLOBALS->changes_count = 0;
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_VISUAL));
	}
	else
	{
	gchar *msg = _("I/O error for file %s.");

		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_ERROR,
			_("File error"),
			msg,
			GLOBALS->xhb_filepath
			);

	}


}


static void ui_mainwindow_populate_accounts(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
GtkTreeModel *model;
GtkTreeIter  iter1, child_iter;
GList *list;
Account *acc;
gint i, j, nbtype;
gdouble gtbank, gttoday, gtfuture;

	DB( g_printf("\n[ui-mainwindow] populate accounts\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	/* here we create a count and a list of every account pointer by type */

	GPtrArray *typeacc[ACC_TYPE_MAXVALUE] = {0};
	list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
		acc = list->data;
		if( !(acc->flags & (AF_CLOSED|AF_NOREPORT|AF_NOSUMMARY)) )
		{
			DB( g_printf(" -> insert %d:%s\n", acc->key, acc->name) );

			if(typeacc[acc->type] == NULL)
				typeacc[acc->type] = g_ptr_array_sized_new(da_acc_length ());

			g_ptr_array_add(typeacc[acc->type], (gpointer)acc);
		}
		list = g_list_next(list);
	}
	g_list_free(list);

	gtbank = gttoday = gtfuture = 0;

	DB( g_printf(" -> populate listview\n") );


	/* then populate the listview */
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_acc));
	gtk_tree_store_clear (GTK_TREE_STORE(model));

	nbtype = 0;
	for(i=0;i<ACC_TYPE_MAXVALUE;i++)
	{
	GPtrArray *gpa = typeacc[i];
	gdouble tbank, ttoday, tfuture;

		if(gpa != NULL)
		{
			nbtype++;
			//1: Header: Bank, Cash, ...
			DB( g_printf(" -> append '%s'\n", CYA_ACC_TYPE[i]) );

			gtk_tree_store_append (GTK_TREE_STORE(model), &iter1, NULL);
			gtk_tree_store_set (GTK_TREE_STORE(model), &iter1,
					  LST_DSPACC_DATATYPE, DSPACC_TYPE_HEADER,
					  LST_DSPACC_NAME, CYA_ACC_TYPE[i],
					  -1);

			tbank = ttoday = tfuture = 0;

			//2: Accounts for real
			for(j=0;j<gpa->len;j++)
			{
				acc = g_ptr_array_index(gpa, j);
				DB( g_printf(" -> append '%s'\n", acc->name) );

				//if(acc->kcur == GLOBALS->kcur)
				//{
					tbank += acc->bal_bank;
					ttoday += acc->bal_today;
					tfuture += acc->bal_future;
				/*}
				else
				{
					tbank += to_base_amount(acc->bal_bank, acc->kcur);
					ttoday += to_base_amount(acc->bal_today, acc->kcur);
					tfuture += to_base_amount(acc->bal_future, acc->kcur);
				}*/

				DB( g_printf("%s :: %.2f %.2f %.2f\n", acc->name, acc->bal_bank, acc->bal_today, acc->bal_future) );

				gtk_tree_store_append (GTK_TREE_STORE(model), &child_iter, &iter1);
				gtk_tree_store_set (GTK_TREE_STORE(model), &child_iter,
						LST_DSPACC_DATAS, acc,
						LST_DSPACC_DATATYPE, DSPACC_TYPE_NORMAL,
						LST_DSPACC_BANK, acc->bal_bank,
						LST_DSPACC_TODAY, acc->bal_today,
						LST_DSPACC_FUTURE, acc->bal_future,
					  -1);
			}

			if(gpa->len > 1)
			{
				DB( g_printf("Total :: %.2f %.2f %.2f\n", tbank, ttoday, tfuture) );

				// insert the total line
				gtk_tree_store_append (GTK_TREE_STORE(model), &child_iter, &iter1);
				gtk_tree_store_set (GTK_TREE_STORE(model), &child_iter,
						LST_DSPACC_DATATYPE, DSPACC_TYPE_SUBTOTAL,
						LST_DSPACC_NAME, _("Total"),
						LST_DSPACC_BANK, tbank,
						LST_DSPACC_TODAY, ttoday,
						LST_DSPACC_FUTURE, tfuture,
						  -1);
			}

			/* set balance to header to display when collasped */
			gtk_tree_store_set (GTK_TREE_STORE(model), &iter1,
					LST_DSPACC_BANK, tbank,
					LST_DSPACC_TODAY, ttoday,
					LST_DSPACC_FUTURE, tfuture,
					  -1);

			/* add to grand total */
			gtbank += tbank;
			gttoday += ttoday;
			gtfuture += tfuture;

		}

	}

	DB( g_printf(" -> grand total\n") );

	// Grand total
	if( nbtype > 1 )
	{
		gtk_tree_store_append (GTK_TREE_STORE(model), &iter1, NULL);
		gtk_tree_store_set (GTK_TREE_STORE(model), &iter1,
					LST_DSPACC_DATATYPE, DSPACC_TYPE_SUBTOTAL,
					LST_DSPACC_NAME, _("Grand total"),
					LST_DSPACC_BANK, gtbank,
					LST_DSPACC_TODAY, gttoday,
					LST_DSPACC_FUTURE, gtfuture,
				  -1);
	}


	gtk_tree_view_expand_all(GTK_TREE_VIEW(data->LV_acc));

	DB( g_printf(" -> free ressources\n") );


	/* free all temp stuff */
	for(i=0;i<ACC_TYPE_MAXVALUE;i++)
	{
	GPtrArray *gpa = typeacc[i];

		if(gpa != NULL)
			g_ptr_array_free (gpa, TRUE);
	}


}


void ui_mainwindow_update(GtkWidget *widget, gpointer user_data)
{
struct hbfile_data *data;
gint flags;

	DB( g_printf("\n[ui-mainwindow] refresh_display\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//data = INST_DATA(widget);

	flags = GPOINTER_TO_INT(user_data);

	/* set window title */
	if(flags & UF_TITLE)
	{
	gchar *basename;
	gchar *changed;

		DB( printf(" +  1: wintitle %x\n", (gint)data->wintitle) );

		basename = g_path_get_basename(GLOBALS->xhb_filepath);

		DB( printf(" global changes: %d\n", GLOBALS->changes_count) );

		g_free(data->wintitle);

		changed = (GLOBALS->changes_count > 0) ? "*" : "";

		data->wintitle = g_strdup_printf("%s%s - %s - " PROGNAME, changed, basename, GLOBALS->owner);

	    gtk_window_set_title (GTK_WINDOW (gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), data->wintitle);

		g_free(basename);
	}

	/* update disabled things */
	if(flags & UF_SENSITIVE)
	{
	GtkTreeSelection *selection;
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GtkTreePath		*path;
	gboolean	active,sensitive;

		DB( printf(" +  2: disabled, opelist count\n") );

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc));

		active = gtk_tree_selection_get_selected(selection, &model, &iter);
		if(active)
		{
		Account *acc;
		gint depth;

			path = gtk_tree_model_get_path(model, &iter);
			depth =	gtk_tree_path_get_depth(path);

			if( depth > 1 )
			{
				DB( printf(" depth is %d\n", depth) );

				gtk_tree_model_get(GTK_TREE_MODEL(model), &iter, LST_DSPACC_DATAS, &acc, -1);
				data->acc = acc;

			}
			else
				active = FALSE;
		}
		else
		{
			//ensure data->acc will not be null
			data->acc = da_acc_get(1);
		}


		// no change: disable save
		DB( printf(" changes %d - new %d\n", GLOBALS->changes_count, GLOBALS->hbfile_is_new) );


		sensitive = (GLOBALS->changes_count != 0 ) ? TRUE : FALSE;
		//gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/FileMenu/SaveAs"), sensitive);
		//if(sensitive == TRUE && GLOBALS->hbfile_is_new == TRUE) sensitive = FALSE;
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/FileMenu/Save"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/FileMenu/Revert"), GLOBALS->xhb_hasbak);


	// define off ?
		sensitive = GLOBALS->define_off == 0 ? TRUE : FALSE;

		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ManageMenu/Account"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ManageMenu/Payee"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ManageMenu/Category"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ManageMenu/Budget"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/EditMenu/Preferences"), sensitive);

	// empty account list: disable Import, Archives, Edit, Filter, Add, Statistics, Overdrawn, Car Cost
		sensitive = da_acc_length() > 0 ? TRUE : FALSE;

		//gtk_action_set_sensitive(gtk_ui_manager_get_action(data-data->manager, "/MenuBar/FileMenu/Import"), sensitive);

		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/FileMenu/Close"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ManageMenu/Archive"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/TransactionMenu/AddOpe"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/TransactionMenu/ShowOpe"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ReportMenu/RStatistics"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ReportMenu/RTrendTime"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ReportMenu/RBudget"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ReportMenu/RBalance"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ReportMenu/RVehiculeCost"), sensitive);

	// empty category list: disable Budget & Budget report
		sensitive = da_cat_length() > 1 ? TRUE : FALSE;

		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/ManageMenu/Budget"), sensitive);

	// empty archive list: disable scheduled check
		sensitive = g_list_length(GLOBALS->arc_list) > 0 ? TRUE : FALSE;

		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/TransactionMenu/AddScheduled"), sensitive);

	// no active account: disable Edit, Over
		sensitive = (active == TRUE ) ? TRUE : FALSE;
		if(data->acc && data->acc->window != NULL)
			sensitive = FALSE;

		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->manager, "/MenuBar/TransactionMenu/ShowOpe"), sensitive);

	}

	/* update toolbar, list */
	if(flags & UF_VISUAL)
	{
		DB( printf(" +  8: visual\n") );

		if(PREFS->toolbar_style == 0)
			gtk_toolbar_unset_style(GTK_TOOLBAR(data->toolbar));
		else
			gtk_toolbar_set_style(GTK_TOOLBAR(data->toolbar), PREFS->toolbar_style-1);

		gtk_tree_view_set_rules_hint (GTK_TREE_VIEW (data->LV_acc), PREFS->rules_hint);
		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_acc));

		//homebank_pref_createformat();
		homebank_pref_init_measurement_units();

		DB( printf(" toolbar %d\n", PREFS->wal_toolbar) );
		if(PREFS->wal_toolbar)
			gtk_widget_show(GTK_WIDGET(data->toolbar));
		else
			gtk_widget_hide(GTK_WIDGET(data->toolbar));

		DB( printf(" top spending %d\n", PREFS->wal_spending) );
		if(PREFS->wal_spending)
			gtk_widget_show(GTK_WIDGET(data->GR_top));
		else
			gtk_widget_hide(GTK_WIDGET(data->GR_top));


		DB( printf(" upcoming %d\n", PREFS->wal_upcoming) );
		if(PREFS->wal_upcoming)
			gtk_widget_show(GTK_WIDGET(data->GR_upc));
		else
			gtk_widget_hide(GTK_WIDGET(data->GR_upc));

		DB( printf(" minor %d\n", PREFS->euro_active) );
		gtk_action_set_visible(gtk_ui_manager_get_action(data->manager, "/MenuBar/ViewMenu/AsMinor"), PREFS->euro_active);
	}

	/* update balances */
	if(flags & UF_BALANCE)
	{

		DB( printf(" +  4: balances\n") );

		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_acc));

		//minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

		/*
		hb-label_set_colvalue(GTK_LABEL(data->TX_balance[0]), data->bank, minor);
		hb-label_set_colvalue(GTK_LABEL(data->TX_balance[1]), data->today, minor);
		hb-label_set_colvalue(GTK_LABEL(data->TX_balance[2]), data->future, minor);
		*/
	}

	if(flags & UF_REFRESHALL)
	{
		ui_mainwindow_populate_accounts(GLOBALS->mainwindow, NULL);
		ui_mainwindow_populate_topspending(GLOBALS->mainwindow, NULL);

	
	}


}

void ui_mainwindow_busy(GtkWidget *widget, gboolean state)
{
struct hbfile_data *data;
GtkWidget *window;
GdkCursor *cursor;

	DB( g_printf("\n[ui-mainwindow] busy: %d\n", state) );

	window = gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW);
	data = g_object_get_data(G_OBJECT(window), "inst_data");

	// should busy ?
	if(state == TRUE)
	{
		cursor = gdk_cursor_new(GDK_WATCH);
		gdk_window_set_cursor(GTK_WIDGET(window)->window, cursor);
		gdk_cursor_unref(cursor);

		//gtk_grab_add(data->busy_popup);

		gtk_widget_set_sensitive(window, FALSE);
		gtk_action_group_set_sensitive(data->actions, FALSE);

		  /* make sure chnages is up */
		  while (gtk_events_pending ())
		    gtk_main_iteration ();
	}
	// unbusy
	else
	{
		gtk_widget_set_sensitive(window, TRUE);
		gtk_action_group_set_sensitive(data->actions, TRUE);

		gdk_window_set_cursor(GTK_WIDGET(window)->window, NULL);
		//gtk_grab_remove(data->busy_popup);
	}
}


static void
  ui_mainwindow_onRowActivated (GtkTreeView        *treeview,
                       GtkTreePath        *path,
                       GtkTreeViewColumn  *col,
                       gpointer            userdata)
  {
    GtkTreeModel *model;
    GtkTreeIter   iter;

    DB( g_print ("\n[ui-mainwindow] A row has been double-clicked!\n") );

    model = gtk_tree_view_get_model(treeview);

    if (gtk_tree_model_get_iter(model, &iter, path))
    {
	Account *acc;

       gtk_tree_model_get(model, &iter, LST_DSPACC_DATAS, &acc, -1);

		if( acc != NULL )
		{

       DB( g_print ("Double-clicked row contains name %s\n", acc->name) );

		ui_mainwindow_action_showtransactions();

       //g_free(name);
    	}
    }
  }

/*
**
*/
static gboolean ui_mainwindow_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct hbfile_data *data = user_data;
struct WinGeometry *wg;
gboolean retval = FALSE;

	DB( g_printf("\n[ui-mainwindow] dispose\n") );

	//store position and size
	wg = &PREFS->wal_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);

	DB( g_printf(" window: l=%d, t=%d, w=%d, h=%d\n", wg->l, wg->t, wg->w, wg->h) );

 	PREFS->wal_vpaned = gtk_paned_get_position(GTK_PANED(data->vpaned));
 	PREFS->wal_hpaned = gtk_paned_get_position(GTK_PANED(data->hpaned));

	DB( g_print(" -> paned position is %d\n", PREFS->wal_vpaned) );

	//todo
	if(ui_dialog_msg_savechanges(widget, NULL) == FALSE)
	{
		retval = TRUE;
	}
	else
	{
		DB( g_printf(" free wintitle %x\n", (gint)data->wintitle) );

		gtk_widget_destroy(data->LV_top);

		g_free(data->wintitle);
		da_filter_free(data->filter);
		g_free(user_data);
		gtk_main_quit();
	}



	//delete-event TRUE abort/FALSE destroy
	return retval;
}


static void ui_mainwindow_recent_chooser_item_activated_cb (GtkRecentChooser *chooser, struct hbfile_data *data)
{
	gchar *uri, *path;
	GError *error = NULL;

	uri = gtk_recent_chooser_get_current_uri (chooser);

	path = g_filename_from_uri (uri, NULL, NULL);
	if (error)
	{
		g_warning ("Could not convert uri \"%s\" to a local path: %s", uri, error->message);
		g_error_free (error);
		return;
	}

	if( ui_dialog_msg_savechanges(data->window, NULL) == TRUE )
	{

		//todo: FixMe
		/*
		if (! load)
		{
			gpw_recent_remove (gpw, path);
		}
		*/

		hbfile_change_filepath(path);
		ui_mainwindow_open_internal(data->window, NULL);
	}
	else
	{
		g_free (path);
	}
	g_free (uri);
}


static void ui_mainwindow_window_screen_changed_cb (GtkWidget *widget,
			      GdkScreen *old_screen,
			      struct hbfile_data *data)
{

	DB( g_printf("hbfile_window_screen_changed_cb\n") );


	data->recent_manager = gtk_recent_manager_get_default ();

	gtk_menu_detach (GTK_MENU (data->recent_menu));
	g_object_unref (G_OBJECT (data->recent_menu));

	data->recent_menu = ui_mainwindow_create_recent_chooser_menu (data->recent_manager);

	g_signal_connect (data->recent_menu,
			  "item-activated",
			  G_CALLBACK (ui_mainwindow_recent_chooser_item_activated_cb),
			  data);

	//menu_item = gtk_ui_manager_get_widget (data->manager, "/MenuBar/FileMenu/OpenRecent");
	//gtk_menu_item_set_submenu (GTK_MENU_ITEM (menu_item), data->recent_menu);
}


void ui_mainwindow_recent_add (struct hbfile_data *data, const gchar *path)
{
	GtkRecentData *recent_data;
	gchar *uri;
	GError *error = NULL;

	DB( g_printf("ui_mainwindow_recent_add\n") );

	DB( g_printf("suffix xhb %d", g_str_has_suffix (path, ".xhb") ) );

	if( g_str_has_suffix (path, ".xhb") == FALSE )	//ignore reverted file
		return;

	uri = g_filename_to_uri (path, NULL, &error);
	if (error)
	{
		g_warning ("Could not convert uri \"%s\" to a local path: %s", uri, error->message);
		g_error_free (error);
		return;
	}

	recent_data = g_slice_new (GtkRecentData);

	recent_data->display_name   = NULL;
	recent_data->description    = NULL;
	recent_data->mime_type      = "application/x-homebank";
	recent_data->app_name       = (gchar *) g_get_application_name ();
	recent_data->app_exec       = g_strjoin (" ", g_get_prgname (), "%u", NULL);
	recent_data->groups         = NULL;
	recent_data->is_private     = FALSE;

	if (!gtk_recent_manager_add_full (data->recent_manager,
				          uri,
				          recent_data))
	{
      		g_warning ("Unable to add '%s' to the list of recently used documents", uri);
	}

	g_free (uri);
	g_free (recent_data->app_exec);
	g_slice_free (GtkRecentData, recent_data);

}





enum
{
	TARGET_URI_LIST
};

static GtkTargetEntry drop_types[] =
{
	{"text/uri-list", 0, TARGET_URI_LIST}
};

static void ui_mainwindow_drag_data_received (GtkWidget *widget,
			GdkDragContext *context,
			gint x, gint y,
			GtkSelectionData *selection_data,
			guint info, guint time, GtkWindow *window)
{
	gchar **uris, **str;
	gchar *data;
	gint filetype;

	if (info != TARGET_URI_LIST)
		return;

	/* On MS-Windows, it looks like `selection_data->data' is not NULL terminated. */
	data = g_new (gchar, selection_data->length + 1);
	memcpy (data, selection_data->data, selection_data->length);
	data[selection_data->length] = 0;

	uris = g_uri_list_extract_uris (data);

	str = uris;
	//for (str = uris; *str; str++)
	if( *str )
	{
		GError *error = NULL;
		gchar *path = g_filename_from_uri (*str, NULL, &error);

		if (path)
		{
			filetype = homebank_alienfile_recognize(path);

			DB( g_print(" dragged %s, type is %d\n", path, filetype ) );

			if( filetype == FILETYPE_HOMEBANK)
			{
				hbfile_change_filepath(path);
				ui_mainwindow_open_internal(GTK_WIDGET(window), NULL);
			}
			else
			{
				//todo: future here to implement import for other filetype

				ui_dialog_msg_infoerror(GTK_WINDOW(window), GTK_MESSAGE_ERROR,
					_("File error"),
					_("The file %s is not a valid HomeBank file."),
					path
					);


			}

		}
		else
		{
			g_warning ("Could not convert uri to local path: %s", error->message);

			g_error_free (error);
		}
		g_free (path);
	}
	g_strfreev (uris);
}


static GtkWidget *ui_mainwindow_create_recent_chooser_menu (GtkRecentManager *manager)
{
GtkWidget *toolbar_recent_menu;
GtkRecentFilter *filter;

	toolbar_recent_menu = gtk_recent_chooser_menu_new_for_manager (manager);

	gtk_recent_chooser_set_local_only (GTK_RECENT_CHOOSER (toolbar_recent_menu),
					FALSE);
	gtk_recent_chooser_set_sort_type (GTK_RECENT_CHOOSER (toolbar_recent_menu),
					GTK_RECENT_SORT_MRU);
	//todo: add a user pref for this
	gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER (toolbar_recent_menu),
					5);


	//gtk_recent_chooser_set_show_icons (GTK_RECENT_CHOOSER (toolbar_recent_menu), FALSE);

	//gtk_recent_chooser_menu_set_show_numbers (GTK_RECENT_CHOOSER_MENU (toolbar_recent_menu), TRUE);

	filter = gtk_recent_filter_new ();
	//gtk_recent_filter_add_application (filter, g_get_application_name());
	gtk_recent_filter_add_pattern (filter, "*.[Xx][Hh][Bb]");
	gtk_recent_chooser_set_filter (GTK_RECENT_CHOOSER (toolbar_recent_menu), filter);

	return toolbar_recent_menu;
}


static void ui_mainwindow_create_menu_bar_and_toolbar(struct hbfile_data *data, GtkWidget *mainvbox)
{
GtkUIManager *manager;
GtkActionGroup *action_group;
GtkAction *action;
GError *error = NULL;

	manager = gtk_ui_manager_new ();
	data->manager = manager;

	gtk_window_add_accel_group (GTK_WINDOW (data->window),
				gtk_ui_manager_get_accel_group(manager));

	action_group = gtk_action_group_new ("MainWindow");
	gtk_action_group_set_translation_domain(action_group, GETTEXT_PACKAGE);

	gtk_action_group_add_actions (action_group,
			entries,
			n_entries,
			NULL);

	gtk_action_group_add_toggle_actions (action_group,
			toggle_entries,
			n_toggle_entries,
			NULL);

	gtk_ui_manager_insert_action_group (data->manager, action_group, 0);
	g_object_unref (action_group);
	data->actions = action_group;

	/* set short labels to use in the toolbar */
	action = gtk_action_group_get_action(action_group, "Open");
	g_object_set(action, "short_label", _("Open"), NULL);

	action = gtk_action_group_get_action(action_group, "Save");
	g_object_set(action, "is_important", TRUE, NULL);

	action = gtk_action_group_get_action(action_group, "Account");
	g_object_set(action, "short_label", _("Account"), NULL);

	action = gtk_action_group_get_action(action_group, "Payee");
	g_object_set(action, "short_label", _("Payee"), NULL);

	action = gtk_action_group_get_action(action_group, "Category");
	g_object_set(action, "short_label", _("Category"), NULL);

	action = gtk_action_group_get_action(action_group, "Archive");
	//TRANSLATORS: an archive is stored transaction buffers (kind of bookmark to prefill manual insertion)
	g_object_set(action, "short_label", _("Archive"), NULL);

	action = gtk_action_group_get_action(action_group, "Budget");
	g_object_set(action, "short_label", _("Budget"), NULL);

	action = gtk_action_group_get_action(action_group, "ShowOpe");
	g_object_set(action, "short_label", _("Show"), NULL);

	action = gtk_action_group_get_action(action_group, "AddOpe");
	g_object_set(action, "is_important", TRUE, "short_label", _("Add"), NULL);

	action = gtk_action_group_get_action(action_group, "RStatistics");
	g_object_set(action, "short_label", _("Statistics"), NULL);

	action = gtk_action_group_get_action(action_group, "RBudget");
	g_object_set(action, "short_label", _("Budget"), NULL);

	action = gtk_action_group_get_action(action_group, "RBalance");
	g_object_set(action, "short_label", _("Balance"), NULL);

	action = gtk_action_group_get_action(action_group, "RVehiculeCost");
	g_object_set(action, "short_label", _("Vehicle cost"), NULL);

	/* now load the UI definition */
	gtk_ui_manager_add_ui_from_string (data->manager, ui_info, -1, &error);
	if (error != NULL)
	{
		g_message ("Building menus failed: %s", error->message);
		g_error_free (error);
	}


	data->menubar = gtk_ui_manager_get_widget (manager, "/MenuBar");
	gtk_box_pack_start (GTK_BOX (mainvbox),
			    data->menubar,
			    FALSE,
			    FALSE,
			    0);

	data->toolbar = gtk_ui_manager_get_widget (manager, "/ToolBar");
	gtk_box_pack_start (GTK_BOX (mainvbox),
			    data->toolbar,
			    FALSE,
			    FALSE,
			    0);

	/* recent files menu */



	data->recent_manager = gtk_recent_manager_get_default ();

	data->recent_menu = ui_mainwindow_create_recent_chooser_menu (data->recent_manager);

	g_signal_connect (data->recent_menu,
			  "item-activated",
			  G_CALLBACK (ui_mainwindow_recent_chooser_item_activated_cb),
			  data);

/*
	widget = gtk_ui_manager_get_widget (data->manager, "/MenuBar/FileMenu/OpenRecent");
	gtk_menu_item_set_submenu (GTK_MENU_ITEM (widget), data->recent_menu);
*/

	/* testing */
		/* add the custom Open button to the toolbar */
	GtkToolItem *open_button = gtk_menu_tool_button_new_from_stock (GTK_STOCK_OPEN);
	gtk_menu_tool_button_set_menu (GTK_MENU_TOOL_BUTTON (open_button),
				       data->recent_menu);

	gtk_tool_item_set_tooltip_text (open_button, _("Open a file"));
	gtk_menu_tool_button_set_arrow_tooltip_text (GTK_MENU_TOOL_BUTTON (open_button),
						     _("Open a recently used file"));


	action = gtk_action_group_get_action (data->actions, "Open");
	g_object_set (action,
		      "short_label", _("Open"),
		      NULL);
	//gtk_action_connect_proxy (action, GTK_WIDGET (open_button));
	gtk_activatable_set_related_action (GTK_ACTIVATABLE (open_button), action);

	gtk_toolbar_insert (GTK_TOOLBAR (data->toolbar),
			    open_button,
			    1);
	/* end testing */

}

static GtkWidget *ui_mainwindow_create_youraccounts(struct hbfile_data *data)
{
GtkWidget *mainvbox, *align, *label, *widget, *sw;

	mainvbox = gtk_vbox_new (FALSE, 0);

	label = make_label(NULL, 0.0, 0.5);
	gtk_label_set_markup (GTK_LABEL(label), _("<b>Your accounts</b>"));
	gtk_misc_set_padding (GTK_MISC(label), HB_BOX_SPACING, HB_BOX_SPACING/2);
    gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

	align = gtk_alignment_new(0, 0, 1.0, 1.0);
	// top, bottom, left, right
	gtk_alignment_set_padding (GTK_ALIGNMENT(align), 0, HB_BOX_SPACING, 2*HB_BOX_SPACING, HB_BOX_SPACING);
	gtk_box_pack_start (GTK_BOX (mainvbox), align, TRUE, TRUE, 0);

	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add (GTK_CONTAINER (align), sw);
	
	widget = (GtkWidget *)create_list_account();
	data->LV_acc = widget;
	gtk_container_add (GTK_CONTAINER (sw), widget);

	return mainvbox;
}


static GtkWidget *ui_mainwindow_create_topspending(struct hbfile_data *data)
{
GtkWidget *mainvbox, *hbox, *vbox;
GtkWidget *label, *align, *widget;

		mainvbox = gtk_vbox_new (FALSE, 0);
		data->GR_top = mainvbox;
		
		label = make_label(NULL, 0.0, 0.5);
		gtk_label_set_markup (GTK_LABEL(label), _("<b>Where your money goes</b>"));
		gtk_misc_set_padding (GTK_MISC(label), HB_BOX_SPACING, HB_BOX_SPACING/2);
	    gtk_box_pack_start (GTK_BOX (mainvbox), label, FALSE, FALSE, 0);

		align = gtk_alignment_new(0, 0, 1.0, 1.0);
		// top, bottom, left, right
		gtk_alignment_set_padding (GTK_ALIGNMENT(align), 0, HB_BOX_SPACING, 2*HB_BOX_SPACING, HB_BOX_SPACING);
		gtk_box_pack_start (GTK_BOX (mainvbox), align, TRUE, TRUE, 0);

		vbox = gtk_vbox_new (FALSE, HB_BOX_SPACING);
		gtk_container_add (GTK_CONTAINER (align), vbox);
		
		/* total + date range */
		hbox = gtk_hbox_new (FALSE, HB_BOX_SPACING);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
		
		label = make_label(_("Top 5 spending"), 0.0, 0.5);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		
		label = make_label(NULL, 0.0, 0.5);
		data->TX_topamount = label;
		gtk_box_pack_start (GTK_BOX (hbox), label, TRUE, TRUE, 0);
	
		
		data->CY_range = make_daterange(label, FALSE);
		gtk_box_pack_end (GTK_BOX (hbox), data->CY_range, FALSE, FALSE, 0);

		/* pie + listview */
		hbox = gtk_hbox_new (FALSE, 0);
		gtk_box_pack_start (GTK_BOX (vbox), hbox, TRUE, TRUE, 0);

		widget = gtk_chart_new(CHART_PIE_TYPE);
		data->RE_pie = widget;
		gtk_chart_set_minor_prefs(GTK_CHART(widget), PREFS->euro_value, PREFS->minor_cur.suffix_symbol);
		gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

/*
		sw = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_box_pack_start (GTK_BOX (hbox), sw, FALSE, FALSE, 0);
		*/
		
		widget = (GtkWidget *)create_list_topspending();
		data->LV_top = widget;
		
//		gtk_container_add (GTK_CONTAINER (sw), widget);

	return mainvbox;
}


/*
** the window creation
*/
GtkWidget *create_hbfile_window(GtkWidget *do_widget)
{
struct hbfile_data *data;
GtkWidget *mainvbox, *vbox, *vpaned, *hpaned;
GtkWidget *label, *widget;
GtkWidget *treeview, *sw;
GtkWidget *window;
GtkWidget *align;
GtkAction *action;

	DB( g_print("\n[ui-mainwindow] create window\n") );

	data = g_malloc0(sizeof(struct hbfile_data));
	if(!data) return NULL;

    window = gtk_window_new (GTK_WINDOW_TOPLEVEL);

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_printf("\n[ui-mainwindow] new window=%x, inst_data=%0x\n", (gint)window, (gint)data) );

	// this is our mainwindow, so store it to GLOBALS data
	data->window = window;
	GLOBALS->mainwindow = window;


	mainvbox = gtk_vbox_new (FALSE, 0);
	gtk_container_add (GTK_CONTAINER (window), mainvbox);

	ui_mainwindow_create_menu_bar_and_toolbar (data, mainvbox);

	/* Add the main area */
	vbox = gtk_vbox_new (FALSE, 0);
    //gtk_container_set_border_width (GTK_CONTAINER(vbox), HB_MAINBOX_SPACING);
    gtk_box_pack_start (GTK_BOX (mainvbox), vbox, TRUE, TRUE, 0);

	vpaned = gtk_vpaned_new();
	data->vpaned = vpaned;
    gtk_box_pack_start (GTK_BOX (vbox), vpaned, TRUE, TRUE, 0);

		hpaned = gtk_hpaned_new();
		data->hpaned = hpaned;
		gtk_paned_pack1 (GTK_PANED(vpaned), hpaned, FALSE, FALSE);

		widget = ui_mainwindow_create_youraccounts(data);
		gtk_paned_pack1 (GTK_PANED(hpaned), widget, TRUE, FALSE);

		widget = ui_mainwindow_create_topspending(data);
		//gtk_widget_set_size_request (widget, 200, -1);
		gtk_paned_pack2 (GTK_PANED(hpaned), widget, FALSE, FALSE);

	/* bill book :: échéancier */
		vbox = gtk_vbox_new (FALSE, 0);
		data->GR_upc = vbox;
		gtk_paned_pack2 (GTK_PANED(vpaned), vbox, FALSE, FALSE);

		label = make_label(NULL, 0.0, 0.0);
		gtk_label_set_markup (GTK_LABEL(label), _("<b>Scheduled transactions (next occurence)</b>"));
		gtk_misc_set_padding (GTK_MISC(label), HB_BOX_SPACING, HB_BOX_SPACING/2);
	    gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

		align = gtk_alignment_new(0, 0, 1.0, 1.0);
		// top, bottom, left, right
		gtk_alignment_set_padding (GTK_ALIGNMENT(align), 0, HB_BOX_SPACING, 2*HB_BOX_SPACING, HB_BOX_SPACING);
		gtk_box_pack_start (GTK_BOX (vbox), align, TRUE, TRUE, 0);

		sw = gtk_scrolled_window_new (NULL, NULL);
		gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
		gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
		gtk_container_add (GTK_CONTAINER (align), sw);

		treeview = (GtkWidget *)create_list_upcoming();
		data->LV_upc = treeview;
		gtk_container_add (GTK_CONTAINER (sw), treeview);


	//todo: move this elsewhere
	DB( g_printf("init actions\n") );

	data->filter = da_filter_malloc();
	filter_default_all_set(data->filter);
	
	


	if(PREFS->wal_vpaned > 0)
		gtk_paned_set_position(GTK_PANED(data->vpaned), PREFS->wal_vpaned);
	if(PREFS->wal_hpaned > 0)
		gtk_paned_set_position(GTK_PANED(data->hpaned), PREFS->wal_hpaned);

	action = gtk_ui_manager_get_action(data->manager, "/MenuBar/ViewMenu/Toolbar");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), PREFS->wal_toolbar);
	action = gtk_ui_manager_get_action(data->manager, "/MenuBar/ViewMenu/Spending");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), PREFS->wal_spending);
	action = gtk_ui_manager_get_action(data->manager, "/MenuBar/ViewMenu/Upcoming");
	gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), PREFS->wal_upcoming);

	/* Drag and drop support, set targets to NULL because we add the
	   default uri_targets below */

	/* support for opening a file by dragging onto the project window */
	gtk_drag_dest_set (GTK_WIDGET (window),
			   GTK_DEST_DEFAULT_ALL,
			   drop_types,
	           G_N_ELEMENTS (drop_types),
			   GDK_ACTION_COPY);

	g_signal_connect (G_OBJECT (window), "drag-data-received",
			  G_CALLBACK (ui_mainwindow_drag_data_received), window);

	// setup some widgets
	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_range), PREFS->date_range_wal);


	//connect all our signals
	DB( g_printf("connect signals\n") );


	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_acc)), "changed", G_CALLBACK (ui_mainwindow_selection), NULL);

	g_signal_connect (data->CY_range, "changed", G_CALLBACK (ui_mainwindow_populate_topspending), NULL);

	g_signal_connect (GTK_TREE_VIEW(data->LV_acc), "row-activated", G_CALLBACK (ui_mainwindow_onRowActivated), GINT_TO_POINTER(2));

	/* GtkWindow events */
    g_signal_connect (window, "delete-event", G_CALLBACK (ui_mainwindow_dispose), (gpointer)data);


	g_signal_connect (window, "screen-changed",
			  G_CALLBACK (ui_mainwindow_window_screen_changed_cb),
			  data);


	//gtk_action_group_set_sensitive(data->actions, FALSE);


	
  return window;
}


