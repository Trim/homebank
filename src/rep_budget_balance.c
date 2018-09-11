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
#include "hb-misc.h"
#include "dsp_mainwindow.h"
#include "rep_budget_balance.h"


/****************************************************************************/
/* Debug macros */
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
	BUDGBAL_ISBALANCECATEGORY,
	BUDGBAL_ISFIXEDAMOUNT,
	BUDGBAL_ISTOTAL,
	BUDGBAL_FIXEDAMOUNT,
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
	BUDGBAL_NUMCOLS
};

static gchar *BUDGBAL_MONTHS[] = {
	N_("January"), N_("February"), N_("March"),
	N_("April"), N_("May"), N_("June"),
	N_("July"), N_("August"), N_("September"),
	N_("October"), N_("November"), N_("December"),
	NULL};

/* action functions -------------------- */


/* ======================== */

// the budget model creation
static GtkTreeModel * budget_model_new (void) {
GtkTreeStore *budget;
GtkTreeIter toplevel, child;

	// Create Tree Store
	budget = gtk_tree_store_new ( BUDGBAL_NUMCOLS,
		G_TYPE_STRING, // Category
		G_TYPE_BOOLEAN, // BUDGBAL_ISBALANCECATEGORY
		G_TYPE_BOOLEAN, // BUDGBAL_ISFIXEDAMOUNT
		G_TYPE_BOOLEAN, // BUDGBAL_ISTOTAL
		G_TYPE_DOUBLE, // BUDGBAL_FIXEDAMOUNT
		G_TYPE_DOUBLE, // BUDGBAL_JANUARY
		G_TYPE_DOUBLE, // BUDGBAL_FEBRUARY
		G_TYPE_DOUBLE, // BUDGBAL_MARCH
		G_TYPE_DOUBLE, // BUDGBAL_APRIL
		G_TYPE_DOUBLE, // BUDGBAL_MAY
		G_TYPE_DOUBLE, // BUDGBAL_JUNE
		G_TYPE_DOUBLE, // BUDGBAL_JULY
		G_TYPE_DOUBLE, // BUDGBAL_AUGUST
		G_TYPE_DOUBLE, // BUDGBAL_SEPTEMBER
		G_TYPE_DOUBLE, // BUDGBAL_OCTOBER
		G_TYPE_DOUBLE, // BUDGBAL_NOVEMBER
		G_TYPE_DOUBLE  // BUDGBAL_DECEMBER
	);

	// Populate the store

	// Add a root
	gtk_tree_store_insert_with_values (budget,
	 &toplevel,
		NULL,
		-1,
		BUDGBAL_CATEGORY, _(N_("Income")),
		BUDGBAL_ISBALANCECATEGORY, TRUE,
		-1);

	// Add a child to the root created above
	gtk_tree_store_insert_with_values (
		budget,
		&child,
		&toplevel,
		-1,
		BUDGBAL_CATEGORY, _(N_("Salary")),
		BUDGBAL_ISFIXEDAMOUNT, FALSE,
		BUDGBAL_JANUARY, 100.0,
		BUDGBAL_FEBRUARY, 200.0,
		BUDGBAL_MARCH, 300.0,
		BUDGBAL_APRIL, 4.0,
		BUDGBAL_MAY, 5.0,
		BUDGBAL_JUNE, 6.0,
		BUDGBAL_JULY, 7.0,
		BUDGBAL_AUGUST, 8.0,
		BUDGBAL_SEPTEMBER, 9.0,
		BUDGBAL_OCTOBER, 10.0,
		BUDGBAL_NOVEMBER, 11.0,
		BUDGBAL_DECEMBER, -12.0,
		-1);

	// Add a root
	gtk_tree_store_insert_with_values (
		budget,
		&toplevel,
		NULL,
		-1,
		BUDGBAL_CATEGORY, _(N_("Expenses")),
		BUDGBAL_ISBALANCECATEGORY, TRUE,
		-1);

	// Add a child to the root created above
	gtk_tree_store_insert_with_values (
		budget,
		&child,
		&toplevel,
		-1,
		BUDGBAL_CATEGORY, N_("Bills"),
		BUDGBAL_ISFIXEDAMOUNT, FALSE,
		BUDGBAL_JANUARY, 100.0,
		BUDGBAL_FEBRUARY, 200.0,
		BUDGBAL_MARCH, 300.0,
		BUDGBAL_APRIL, 4.0,
		BUDGBAL_MAY, 5.0,
		BUDGBAL_JUNE, 6.0,
		BUDGBAL_JULY, 7.0,
		BUDGBAL_AUGUST, 8.0,
		BUDGBAL_SEPTEMBER, 9.0,
		BUDGBAL_OCTOBER, 10.0,
		BUDGBAL_NOVEMBER, 11.0,
		BUDGBAL_DECEMBER, -12.5,
		-1);

	gtk_tree_store_insert_with_values (
		budget,
		&child,
		&toplevel,
		-1,
		BUDGBAL_CATEGORY, N_("Subscriptions"),
		BUDGBAL_ISFIXEDAMOUNT, TRUE,
		BUDGBAL_FIXEDAMOUNT, 100.0,
		-1);

	// Add a root
	gtk_tree_store_insert_with_values (
		budget,
		&toplevel,
		NULL,
		-1,
		BUDGBAL_CATEGORY, _(N_("Totals")),
		BUDGBAL_ISBALANCECATEGORY, TRUE,
		-1);

	// Add a child to the root created above
	gtk_tree_store_insert_with_values (
		budget,
		&child,
		&toplevel,
		-1,
		BUDGBAL_CATEGORY, _(N_("Incomes")),
		BUDGBAL_ISTOTAL, TRUE,
		-1);

	// Add a child to the root created above
	gtk_tree_store_insert_with_values (
		budget,
		&child,
		&toplevel,
		-1,
		BUDGBAL_CATEGORY, _(N_("Expenses")),
		BUDGBAL_ISTOTAL, TRUE,
		-1);

	// Add a child to the root created above
	gtk_tree_store_insert_with_values (
		budget,
		&child,
		&toplevel,
		-1,
		BUDGBAL_CATEGORY, _(N_("Differences")),
		BUDGBAL_ISTOTAL, TRUE,
		-1);

	return GTK_TREE_MODEL(budget);
}

// to enable or not edition on month columns
static void display_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_fixedamount, is_balance_category, is_total, is_visible, is_sensitive, is_editable;
gdouble amount = 0.0;
gchar *text;
gchar *fgcolor;
const gint column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter,
		BUDGBAL_ISBALANCECATEGORY, &is_balance_category,
		BUDGBAL_ISFIXEDAMOUNT, &is_fixedamount,
		BUDGBAL_ISTOTAL, &is_total,
		-1);

	// Text to display
	if (is_fixedamount) {
		gtk_tree_model_get(model, iter, BUDGBAL_FIXEDAMOUNT, &amount, -1);
	}
	else if (column_id >= BUDGBAL_JANUARY && column_id <= BUDGBAL_DECEMBER) {
		if (is_total) {
			amount = 0.0;
		}
		else {
			gtk_tree_model_get(model, iter, column_id, &amount, -1);
		}
	}

	text = g_strdup_printf("%.2f", amount);
	fgcolor = get_minimum_color_amount(amount, 0.0);

	// Default styling values
	is_visible = TRUE;
	is_editable = FALSE;
	is_sensitive = TRUE;

	if (is_balance_category) {
		is_visible = FALSE;
		is_editable = FALSE;
		is_sensitive = FALSE;
	}
	else if (is_total) {
		is_visible = TRUE;
		is_editable = FALSE;
		is_sensitive = FALSE;

		if (column_id == BUDGBAL_FIXEDAMOUNT) {
			is_visible = FALSE;
		}

		g_object_set(renderer,
			"foreground", fgcolor,
			NULL);
	}
	else if (is_fixedamount) {
		is_visible = TRUE;
		is_editable = FALSE;
		is_sensitive = FALSE;

		if (column_id == BUDGBAL_FIXEDAMOUNT) {
			is_sensitive = TRUE;
		}
	}
	else if (! is_fixedamount) {
		is_visible = TRUE;
		is_editable = FALSE;
		is_sensitive = TRUE;

		if (column_id == BUDGBAL_FIXEDAMOUNT) {
			is_sensitive = FALSE;
		}
	}


	g_object_set(renderer,
		"text", text,
		"visible", is_visible,
		"editable", is_editable,
		"sensitive", is_sensitive,
		NULL);

	g_free(text);
}

// to enable or not edition on month columns
static void display_fixedamount_toggle (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_fixedamount, is_balance_category, is_total, is_visible, is_sensitive;

	gtk_tree_model_get(model, iter,
		BUDGBAL_ISBALANCECATEGORY, &is_balance_category,
		BUDGBAL_ISFIXEDAMOUNT, &is_fixedamount,
		BUDGBAL_ISTOTAL, &is_total,
		-1);

	// Default values
	is_visible = TRUE;
	is_sensitive = TRUE;

	if (is_balance_category || is_total) {
		is_visible = FALSE;
		is_sensitive = FALSE;
	}

	g_object_set(renderer,
		"active", is_fixedamount,
		"visible", is_visible,
		"sensitive", is_sensitive,
		NULL);
}

// Compute dynamically the yearly total
static void yearly_total (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_fixedamount = FALSE, is_total = FALSE, is_balance_category = FALSE;
gdouble amount = 0.0;
gdouble total = 0.0;
gchar *text;
gchar *fgcolor;

	gtk_tree_model_get(model, iter,
		BUDGBAL_ISBALANCECATEGORY, &is_balance_category,
		BUDGBAL_ISFIXEDAMOUNT, &is_fixedamount,
		BUDGBAL_FIXEDAMOUNT, &amount,
		BUDGBAL_ISTOTAL, &is_total,
		-1);

	if (is_total || is_balance_category) {
		g_object_set(renderer, "visible", FALSE, NULL);
	}
	else {
		if (is_fixedamount) {
			total = 12.0 * amount;
		}
		else {
			for (int i = BUDGBAL_JANUARY ; i <= BUDGBAL_DECEMBER ; ++i) {
				gtk_tree_model_get(model, iter, i, &amount, -1);
				total += amount;
			}
		}

		text = g_strdup_printf("%.2f", total);
		fgcolor = get_minimum_color_amount(total, 0.0);

		g_object_set(renderer,
			"text", text,
			"foreground", fgcolor,
			"visible", TRUE,
			"sensitive", FALSE,
			NULL);

		g_free(text);
	}
}

// the budget view creation which run the model creation tool

static GtkWidget *budget_view_new (void)
{
GtkTreeViewColumn *col;
GtkCellRenderer *renderer;
GtkWidget *view;
GtkTreeModel *model;

	view = gtk_tree_view_new();

	/* --- Category --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Category")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_add_attribute(col, renderer, "text", BUDGBAL_CATEGORY);

	/* --- Is same amount fixed for each month ? --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Fixed ?")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, display_fixedamount_toggle, NULL, NULL);

	/* --- Fixed amount --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Fixed amount")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, display_amount, GINT_TO_POINTER(BUDGBAL_FIXEDAMOUNT), NULL);

	/* --- Each month --- */
	for (int i = BUDGBAL_JANUARY ; i <= BUDGBAL_DECEMBER ; ++i) {
		int month = i - BUDGBAL_JANUARY ;
		col = gtk_tree_view_column_new();

		gtk_tree_view_column_set_title(col, _(BUDGBAL_MONTHS[month]));
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		renderer = gtk_cell_renderer_text_new();

		gtk_tree_view_column_pack_start(col, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(col, renderer, display_amount, GINT_TO_POINTER(i), NULL);
	}

	/* --- Year Total -- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Total")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_set_cell_data_func(col, renderer, yearly_total, NULL, NULL);

	/* --- Link with datas of the budget store -- */
	model = budget_model_new();

	gtk_tree_view_set_model(GTK_TREE_VIEW(view), model);
	gtk_tree_view_expand_all(GTK_TREE_VIEW(view));

	/* to automatically destroy then model with view */
	g_object_unref(model);

	return view;
}

// the window close / deletion
static gboolean repbudgetbalance_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbudgetbalance_data *data = user_data;
//struct WinGeometry *wg;

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

