/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 2018 Adrien Dorsaz <adorsaz.ch>
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
#include "rep_budget_balance.h"


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

/* enum for the Budget Tree Store model */
enum {
  BUDGBAL_CATEGORY = 0,
  BUDGBAL_ISSAMEEACHMONTH,
  BUDGBAL_JANUARY,
  BUDGBAL_FEBRUARY,
  BUDGBAL_MARCH,
  BUDGBAL_APRIL,
  BUDGBAL_MAY,
  BUDGBAL_JUNE,
  BUDGBAL_JULY,
  BUDGBAL_AUGUST,
  BUDGBAL_SEPTEMBER,
  BUDGBAL_OCTOBER,
  BUDGBAL_NOVEMBER,
  BUDGBAL_DECEMBER,
  BUDGBAL_YEAR,
  BUDGBAL_NUMCOLS
};

gchar *BUDGBAL_MONTHS[] = {N_("January"), N_("February"), N_("March"),
                            N_("April"), N_("May"), N_("June"),
                            N_("July"), N_("August"), N_("September"),
                            N_("October"), N_("November"), N_("December"), NULL};

/* action functions -------------------- */


/* ======================== */

// the budget model creation
static GtkTreeModel *
budget_model_new (void) {
  GtkTreeStore *budget;
  GtkTreeIter toplevel, child;
  gchar* buffer;
  GtkTreePath *path;

  // Create Tree Store
  budget = gtk_tree_store_new ( BUDGBAL_NUMCOLS,
                                    G_TYPE_STRING, // Category
                                    G_TYPE_BOOLEAN, // Is same each month
                                    G_TYPE_INT, // January
                                    G_TYPE_INT, // February
                                    G_TYPE_INT, // March
                                    G_TYPE_INT, // April
                                    G_TYPE_INT, // Mai
                                    G_TYPE_INT, // June
                                    G_TYPE_INT, // July
                                    G_TYPE_INT, // August
                                    G_TYPE_INT, // September
                                    G_TYPE_INT, // October
                                    G_TYPE_INT, // November
                                    G_TYPE_INT, // December
                                    G_TYPE_INT  // Sums
                                    );

  // Populate the store

  // Add a root
  gtk_tree_store_insert_with_values (budget,
                                     &toplevel,
                                     NULL,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Income"),
                                     BUDGBAL_ISSAMEEACHMONTH, TRUE,
                                     BUDGBAL_JANUARY, NULL,
                                     BUDGBAL_FEBRUARY, NULL,
                                     BUDGBAL_MARCH, NULL,
                                     BUDGBAL_APRIL, NULL,
                                     BUDGBAL_MAY, NULL,
                                     BUDGBAL_JUNE, NULL,
                                     BUDGBAL_JULY, NULL,
                                     BUDGBAL_AUGUST, NULL,
                                     BUDGBAL_SEPTEMBER, NULL,
                                     BUDGBAL_OCTOBER, NULL,
                                     BUDGBAL_NOVEMBER, NULL,
                                     BUDGBAL_DECEMBER, NULL,
                                     BUDGBAL_YEAR, NULL,
                                     -1);

  // Add a child to the root created above
  gtk_tree_store_insert_with_values (budget,
                                     &child,
                                     &toplevel,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Salary"),
                                     BUDGBAL_ISSAMEEACHMONTH, FALSE,
                                     BUDGBAL_JANUARY, 100,
                                     BUDGBAL_FEBRUARY, 200,
                                     BUDGBAL_MARCH, 300,
                                     BUDGBAL_APRIL, 4,
                                     BUDGBAL_MAY, 5,
                                     BUDGBAL_JUNE, 6,
                                     BUDGBAL_JULY, 7,
                                     BUDGBAL_AUGUST, 8,
                                     BUDGBAL_SEPTEMBER, 9,
                                     BUDGBAL_OCTOBER, 10,
                                     BUDGBAL_NOVEMBER, 11,
                                     BUDGBAL_DECEMBER, -12,
                                     BUDGBAL_YEAR, 2000,
                                     -1);

  // Add a root
  gtk_tree_store_insert_with_values (budget,
                                     &toplevel,
                                     NULL,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Outcome"),
                                     BUDGBAL_ISSAMEEACHMONTH, TRUE,
                                     BUDGBAL_JANUARY, NULL,
                                     BUDGBAL_FEBRUARY, NULL,
                                     BUDGBAL_MARCH, NULL,
                                     BUDGBAL_APRIL, NULL,
                                     BUDGBAL_MAY, NULL,
                                     BUDGBAL_JUNE, NULL,
                                     BUDGBAL_JULY, NULL,
                                     BUDGBAL_AUGUST, NULL,
                                     BUDGBAL_SEPTEMBER, NULL,
                                     BUDGBAL_OCTOBER, NULL,
                                     BUDGBAL_NOVEMBER, NULL,
                                     BUDGBAL_DECEMBER, NULL,
                                     BUDGBAL_YEAR, NULL,
                                     -1);

  // Add a child to the root created above
  gtk_tree_store_insert_with_values (budget,
                                     &child,
                                     &toplevel,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Bills"),
                                     BUDGBAL_ISSAMEEACHMONTH, FALSE,
                                     BUDGBAL_JANUARY, 100,
                                     BUDGBAL_FEBRUARY, 200,
                                     BUDGBAL_MARCH, 300,
                                     BUDGBAL_APRIL, 4,
                                     BUDGBAL_MAY, 5,
                                     BUDGBAL_JUNE, 6,
                                     BUDGBAL_JULY, 7,
                                     BUDGBAL_AUGUST, 8,
                                     BUDGBAL_SEPTEMBER, 9,
                                     BUDGBAL_OCTOBER, 10,
                                     BUDGBAL_NOVEMBER, 11,
                                     BUDGBAL_DECEMBER, -12,
                                     BUDGBAL_YEAR, 2000,
                                     -1);

  // Add a root
  gtk_tree_store_insert_with_values (budget,
                                     &toplevel,
                                     NULL,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Totals"),
                                     BUDGBAL_ISSAMEEACHMONTH, FALSE,
                                     BUDGBAL_JANUARY, NULL,
                                     BUDGBAL_FEBRUARY, NULL,
                                     BUDGBAL_MARCH, NULL,
                                     BUDGBAL_APRIL, NULL,
                                     BUDGBAL_MAY, NULL,
                                     BUDGBAL_JUNE, NULL,
                                     BUDGBAL_JULY, NULL,
                                     BUDGBAL_AUGUST, NULL,
                                     BUDGBAL_SEPTEMBER, NULL,
                                     BUDGBAL_OCTOBER, NULL,
                                     BUDGBAL_NOVEMBER, NULL,
                                     BUDGBAL_DECEMBER, NULL,
                                     BUDGBAL_YEAR, NULL,
                                     -1);

  // Add a child to the root created above
  gtk_tree_store_insert_with_values (budget,
                                     &child,
                                     &toplevel,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Incomes"),
                                     BUDGBAL_ISSAMEEACHMONTH, FALSE,
                                     BUDGBAL_JANUARY, 100,
                                     BUDGBAL_FEBRUARY, 200,
                                     BUDGBAL_MARCH, 300,
                                     BUDGBAL_APRIL, 4,
                                     BUDGBAL_MAY, 5,
                                     BUDGBAL_JUNE, 6,
                                     BUDGBAL_JULY, 7,
                                     BUDGBAL_AUGUST, 8,
                                     BUDGBAL_SEPTEMBER, 9,
                                     BUDGBAL_OCTOBER, 10,
                                     BUDGBAL_NOVEMBER, 11,
                                     BUDGBAL_DECEMBER, -12,
                                     BUDGBAL_YEAR, 2000,
                                     -1);

  // Add a child to the root created above
  gtk_tree_store_insert_with_values (budget,
                                     &child,
                                     &toplevel,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Outcomes"),
                                     BUDGBAL_ISSAMEEACHMONTH, FALSE,
                                     BUDGBAL_JANUARY, 100,
                                     BUDGBAL_FEBRUARY, 200,
                                     BUDGBAL_MARCH, 300,
                                     BUDGBAL_APRIL, 4,
                                     BUDGBAL_MAY, 5,
                                     BUDGBAL_JUNE, 6,
                                     BUDGBAL_JULY, 7,
                                     BUDGBAL_AUGUST, 8,
                                     BUDGBAL_SEPTEMBER, 9,
                                     BUDGBAL_OCTOBER, 10,
                                     BUDGBAL_NOVEMBER, 11,
                                     BUDGBAL_DECEMBER, -12,
                                     BUDGBAL_YEAR, 2000,
                                     -1);

  // Add a child to the root created above
  gtk_tree_store_insert_with_values (budget,
                                     &child,
                                     &toplevel,
                                     -1,
                                     BUDGBAL_CATEGORY, N_("Differences"),
                                     BUDGBAL_ISSAMEEACHMONTH, FALSE,
                                     BUDGBAL_JANUARY, 100,
                                     BUDGBAL_FEBRUARY, 200,
                                     BUDGBAL_MARCH, 300,
                                     BUDGBAL_APRIL, 4,
                                     BUDGBAL_MAY, 5,
                                     BUDGBAL_JUNE, 6,
                                     BUDGBAL_JULY, 7,
                                     BUDGBAL_AUGUST, 8,
                                     BUDGBAL_SEPTEMBER, 9,
                                     BUDGBAL_OCTOBER, 10,
                                     BUDGBAL_NOVEMBER, 11,
                                     BUDGBAL_DECEMBER, -12,
                                     BUDGBAL_YEAR, 2000,
                                     -1);

  return GTK_TREE_MODEL(budget);
}

// to enable or not edition on month columns
void
is_amount_editable (GtkTreeViewColumn *col,
                       GtkCellRenderer   *renderer,
                       GtkTreeModel      *model,
                       GtkTreeIter       *iter,
                       gpointer           user_data)
{
  gboolean is_sameeachmonth;

  gtk_tree_model_get(model, iter, BUDGBAL_ISSAMEEACHMONTH, &is_sameeachmonth, -1);

  if (is_sameeachmonth) {
   g_object_set(renderer, "editable", FALSE, NULL);
  }
  else {
    g_object_set(renderer, "editable", TRUE, NULL);
  }
}

// the budget view creation which run the model creation tool

static GtkWidget *
budget_view_new (void)
{
  GtkTreeViewColumn   *col;
  GtkCellRenderer     *renderer;
  GtkWidget           *view;
  GtkTreeModel        *model;

  view = gtk_tree_view_new();

  /* --- Category --- */
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, N_("Category"));

  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  renderer = gtk_cell_renderer_text_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);

  gtk_tree_view_column_add_attribute(col, renderer, "text", BUDGBAL_CATEGORY);

  /* --- Is same amount each month ? --- */
  col = gtk_tree_view_column_new();
  gtk_tree_view_column_set_title(col, N_("Same Each Month"));

  gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
  renderer = gtk_cell_renderer_toggle_new();
  gtk_tree_view_column_pack_start(col, renderer, TRUE);
  gtk_tree_view_column_add_attribute(col, renderer, "active", BUDGBAL_ISSAMEEACHMONTH);

  /* --- Each month and the year sum --- */
  for (int i = BUDGBAL_JANUARY ; i < BUDGBAL_NUMCOLS ; ++i) {
    int month = i - BUDGBAL_JANUARY ;
    col = gtk_tree_view_column_new();

    if (i <= BUDGBAL_DECEMBER) {
      gtk_tree_view_column_set_title(col, _(BUDGBAL_MONTHS[month]));
    }
    else if (i == BUDGBAL_YEAR) {
      gtk_tree_view_column_set_title(col, N_("Total"));
    }

    gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
    renderer = gtk_cell_renderer_text_new();

    gtk_tree_view_column_pack_start(col, renderer, TRUE);
    gtk_tree_view_column_add_attribute(col, renderer, "text", i);

    if (i == BUDGBAL_JANUARY) {
      g_object_set(renderer, "editable", TRUE, NULL);
    }
   else if (i <= BUDGBAL_DECEMBER) {
     gtk_tree_view_column_set_cell_data_func(col, renderer, is_amount_editable, NULL, NULL);
   }

  }

  /* set 'cell-background' property of the cell renderer */
  g_object_set(renderer,
               "cell-background", "Orange",
               "cell-background-set", TRUE,
               NULL);

  model = budget_model_new();

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
  gtk_tree_view_expand_all(view);

  /* to automatically destroy then model with view */
  g_object_unref(model);

  return view;
}

// the window close / deletion
static gboolean repbudgetbalance_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbudgetbalance_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[repbudgetbalance] start dispose\n") );

	g_free(data);

	//store position and size
	//wg = &PREFS->bud_wg;
	//gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	//gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);

	//DB( g_print(" window: l=%d, t=%d, w=%d, h=%d\n", wg->l, wg->t, wg->w, wg->h) );

	//enable define windows
	GLOBALS->define_off--;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	DB( g_print("\n[repbudgetbalance] end dispose\n") );

	return FALSE;
}


// the window creation
GtkWidget *repbudgetbalance_window_new(void)
{
struct repbudgetbalance_data *data;
struct WinGeometry *wg;
GtkWidget *window, *treeview;

	data = g_malloc0(sizeof(struct repbudgetbalance_data));
	if(!data) return NULL;

	DB( g_print("\n[repbudgetbalance] new\n") );

	//disable define windows
	GLOBALS->define_off++;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	// create window
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	data->window = window;

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" - new window=%p, inst_data=%p\n", window, data) );

	gtk_window_set_title (GTK_WINDOW (window), _("Budget balance report"));

	//set the window icon
	gtk_window_set_icon_name(GTK_WINDOW (window), ICONNAME_HB_REP_BUDGET);

	//window contents
	treeview = budget_view_new ();
	gtk_container_add(GTK_CONTAINER(window), treeview);


	//setup, init and show window
	wg = &PREFS->bud_wg;
	gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
	gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);

	gtk_widget_show_all (window);

	return(window);
}

