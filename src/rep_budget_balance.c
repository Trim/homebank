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
#include "hb-category.h"
#include "rep_budget_balance.h"


/****************************************************************************/
/* Debug macros */
/****************************************************************************/
#define MYDEBUG 1

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;

/* The different views available */
gchar *VIEW_MODE[] = {
	N_("Summary"),
	N_("Incomes"),
	N_("Expenses"),
	NULL
};

/* These values has to correspond to VIEW_MODE */
enum {
	BUDGBAL_VIEW_SUMMARY = 0,
	BUDGBAL_VIEW_INCOME,
	BUDGBAL_VIEW_EXPENSE
};

/* These values corresponds to the GF_INCOME flag from hb-category */
enum {
	BUDGBAL_CAT_TYPE_EXPENSE = 0,
	BUDGBAL_CAT_TYPE_INCOME
};

/* enum for the Budget Tree Store model */
enum {
	BUDGBAL_CATEGORY_KEY = 0,
	BUDGBAL_CATEGORY_NAME,
	BUDGBAL_CATEGORY_TYPE,
	BUDGBAL_ISTITLE, // To retrieve the 3 main categories which are only titles
	BUDGBAL_ISSAMEAMOUNT,
	BUDGBAL_ISTOTAL,
	BUDGBAL_SAMEAMOUNT,
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
	N_("Jan"), N_("Feb"), N_("Mar"),
	N_("Apr"), N_("May"), N_("Jun"),
	N_("Jul"), N_("Aug"), N_("Sept"),
	N_("Oct"), N_("Nov"), N_("Dec"),
	NULL};

struct KeyIterator {
	guint32 key;
	gboolean is_row_found;
	GtkTreeIter iter;
};

/* action functions -------------------- */


/* ======================== */

// look for parent category
static gboolean get_parent_keyiterator (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, struct KeyIterator *data)
{
guint32 row_category_key;
gboolean is_title, is_total;
guint32 key = data->key;

	data->is_row_found = FALSE;

	gtk_tree_model_get (model, iter,
											BUDGBAL_CATEGORY_KEY, &row_category_key,
											BUDGBAL_ISTITLE, &is_title,
											BUDGBAL_ISTOTAL, &is_total,
											-1);

	if ( row_category_key == key
			&& !(is_total)
			&& !(is_title))
	{
		DB(g_print("\tFound row with key %d !\n", key));
		data->iter = *iter;
		data->is_row_found = TRUE;

		return TRUE;
	}

	return FALSE;
}

/* Recursive function which had a new row in the budget model with all its ancestors */
static void insert_category_with_ancestors(GtkTreeStore *budget, GtkTreeIter *balanceIter, guint32 *key_category)
{
GtkTreeIter child, parent;
Category *bdg_category;
gboolean cat_is_sameamount;

	bdg_category = da_cat_get(*key_category);

	if (bdg_category == NULL)
	{
		return;
	}

	cat_is_sameamount = (! (bdg_category->flags & GF_CUSTOM));

	/* Check if parent category already exists */
	struct KeyIterator* parent_keyiter;

		parent_keyiter = g_malloc0(sizeof(struct KeyIterator));
		parent_keyiter->key = bdg_category->parent;

		gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
													 (GtkTreeModelForeachFunc) get_parent_keyiterator,
													 parent_keyiter);

	if (bdg_category->parent == 0)
	{
		// If we are one of the oldest parent, stop recursion
		gtk_tree_store_insert (
			budget,
			&child,
			balanceIter,
			-1);
	}
	else
	{
		if (parent_keyiter->is_row_found)
		{
			DB(g_print("\tRecursion optimisation: parent key %d already exists\n", parent_keyiter->key));
			// If parent already exists, stop recursion
			parent = parent_keyiter->iter;
		}
		else
		{
			// Parent has not been found, ask to create it first
			insert_category_with_ancestors(budget, balanceIter, &(bdg_category->parent));

			// Now, we are sure parent exists, look for it again
			gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
														 (GtkTreeModelForeachFunc) get_parent_keyiterator,
														 parent_keyiter);

			parent = parent_keyiter->iter;
		}

		gtk_tree_store_insert (
			budget,
			&child,
			&parent,
			-1);
	}

	DB(g_print("insert new category %d\n", bdg_category->key));

	gtk_tree_store_set(
		budget,
		&child,
		BUDGBAL_CATEGORY_KEY, bdg_category->key,
		BUDGBAL_CATEGORY_NAME, bdg_category->name,
		BUDGBAL_CATEGORY_TYPE, category_type_get ((bdg_category)),
		BUDGBAL_ISTITLE, FALSE,
		BUDGBAL_ISSAMEAMOUNT, cat_is_sameamount,
		BUDGBAL_ISTOTAL, FALSE,
		BUDGBAL_SAMEAMOUNT, bdg_category->budget[0],
		BUDGBAL_JANUARY, bdg_category->budget[1],
		BUDGBAL_FEBRUARY, bdg_category->budget[2],
		BUDGBAL_MARCH, bdg_category->budget[3],
		BUDGBAL_APRIL, bdg_category->budget[4],
		BUDGBAL_MAY, bdg_category->budget[5],
		BUDGBAL_JUNE, bdg_category->budget[6],
		BUDGBAL_JULY, bdg_category->budget[7],
		BUDGBAL_AUGUST, bdg_category->budget[8],
		BUDGBAL_SEPTEMBER, bdg_category->budget[9],
		BUDGBAL_OCTOBER, bdg_category->budget[10],
		BUDGBAL_NOVEMBER, bdg_category->budget[11],
		BUDGBAL_DECEMBER, bdg_category->budget[12],
		-1);

	g_free(parent_keyiter);

	return;
}

// the budget model creation
static GtkTreeModel * repbudgetbalance_model_new (gint view_mode)
{
GtkTreeStore *budget;
GtkTreeIter iter_income, iter_expense, iter_total, child;
guint32 n_category;
gdouble total_income[12] = {0}, total_expense[12] = {0};

	// Create Tree Store
	budget = gtk_tree_store_new ( BUDGBAL_NUMCOLS,
		G_TYPE_UINT, // BUDGBAL_CATEGORY_KEY
		G_TYPE_STRING, // BUDGBAL_CATEGORY_NAME
		G_TYPE_INT, // BUDGBAL_CATEGORY_TYPE
		G_TYPE_BOOLEAN, // BUDGBAL_ISTITLE
		G_TYPE_BOOLEAN, // BUDGBAL_ISSAMEAMOUNT
		G_TYPE_BOOLEAN, // BUDGBAL_ISTOTAL
		G_TYPE_DOUBLE, // BUDGBAL_SAMEAMOUNT
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

	/* Create title rows */

	if (view_mode == BUDGBAL_VIEW_SUMMARY || view_mode == BUDGBAL_VIEW_INCOME)
	{
		gtk_tree_store_insert_with_values (budget,
		 &iter_income,
			NULL,
			-1,
			BUDGBAL_CATEGORY_NAME, _(N_("Income")),
			BUDGBAL_CATEGORY_TYPE, BUDGBAL_CAT_TYPE_INCOME,
			BUDGBAL_ISTITLE, TRUE,
			-1);
	}

	if (view_mode == BUDGBAL_VIEW_SUMMARY || view_mode == BUDGBAL_VIEW_EXPENSE)
	{
		gtk_tree_store_insert_with_values (
			budget,
			&iter_expense,
			NULL,
			-1,
			BUDGBAL_CATEGORY_NAME, _(N_("Expense")),
			BUDGBAL_CATEGORY_TYPE, BUDGBAL_CAT_TYPE_EXPENSE,
			BUDGBAL_ISTITLE, TRUE,
			-1);
	}

	gtk_tree_store_insert_with_values (
		budget,
		&iter_total,
		NULL,
		-1,
		BUDGBAL_CATEGORY_NAME, _(N_("Totals")),
		BUDGBAL_ISTITLE, TRUE,
		-1);

	/* Create rows for real categories */

	n_category = da_cat_get_max_key();

	for(guint32 i=1; i<=n_category; ++i)
	{
	Category *bdg_category;
	gboolean cat_is_sameamount;
	gboolean cat_is_income;

		bdg_category = da_cat_get(i);

		if (bdg_category == NULL)
		{
			continue;
		}

		/* Display category only if forced or if a budget has been defined. */
		if ( view_mode == BUDGBAL_VIEW_SUMMARY
			&& !(bdg_category->flags & (GF_BUDGET|GF_FORCED)))
		{
			continue;
		}

		cat_is_income = (category_type_get (bdg_category) == 1);
		cat_is_sameamount = (! (bdg_category->flags & GF_CUSTOM));

		DB(g_print(" category %d:'%s' isincome=%d, issub=%d hasbudget=%d issameamount=%d parent=%d\n",
			bdg_category->key, bdg_category->name,
			cat_is_income, (bdg_category->flags & GF_SUB),
			(bdg_category->flags & GF_BUDGET), cat_is_sameamount, bdg_category->parent));

		// Compute totals and init category in right balance category
		if (cat_is_income
				&& (view_mode == BUDGBAL_VIEW_SUMMARY || view_mode == BUDGBAL_VIEW_INCOME)
		)
		{
			for (int month = 1; month <= 12; ++month)
			{
				if (cat_is_sameamount)
				{
					total_income[month-1] += bdg_category->budget[0];
				}
				else {
					total_income[month-1] += bdg_category->budget[month];
				}
			}

			insert_category_with_ancestors(budget, &iter_income, &(bdg_category->key));
		}
		else if (!cat_is_income
						 && (view_mode == BUDGBAL_VIEW_SUMMARY || view_mode == BUDGBAL_VIEW_EXPENSE)
		)
		{
			for (int month = 1; month <= 12; ++month)
			{
				if (cat_is_sameamount)
				{
					total_expense[month-1] += bdg_category->budget[0];
				}
				else {
					total_expense[month-1] += bdg_category->budget[month];
				}
			}

			insert_category_with_ancestors(budget, &iter_expense, &(bdg_category->key));
		}
	}


	/* Create sub-categories for total balance */

	if (view_mode == BUDGBAL_VIEW_SUMMARY || view_mode == BUDGBAL_VIEW_INCOME)
	{
		gtk_tree_store_insert_with_values (
			budget,
			&child,
			&iter_total,
			-1,
			BUDGBAL_CATEGORY_NAME, _(N_("Incomes")),
			BUDGBAL_CATEGORY_TYPE, BUDGBAL_CAT_TYPE_INCOME,
			BUDGBAL_ISTOTAL, TRUE,
			BUDGBAL_JANUARY, total_income[0],
			BUDGBAL_FEBRUARY, total_income[1],
			BUDGBAL_MARCH, total_income[2],
			BUDGBAL_APRIL, total_income[3],
			BUDGBAL_MAY, total_income[4],
			BUDGBAL_JUNE, total_income[5],
			BUDGBAL_JULY, total_income[6],
			BUDGBAL_AUGUST, total_income[7],
			BUDGBAL_SEPTEMBER, total_income[8],
			BUDGBAL_OCTOBER, total_income[9],
			BUDGBAL_NOVEMBER, total_income[10],
			BUDGBAL_DECEMBER, total_income[11],
			-1);
	}

	if (view_mode == BUDGBAL_VIEW_SUMMARY || view_mode == BUDGBAL_VIEW_EXPENSE)
	{
		gtk_tree_store_insert_with_values (
			budget,
			&child,
			&iter_total,
			-1,
			BUDGBAL_CATEGORY_NAME, _(N_("Expenses")),
			BUDGBAL_CATEGORY_TYPE, BUDGBAL_CAT_TYPE_EXPENSE,
			BUDGBAL_ISTOTAL, TRUE,
			BUDGBAL_JANUARY, total_expense[0],
			BUDGBAL_FEBRUARY, total_expense[1],
			BUDGBAL_MARCH, total_expense[2],
			BUDGBAL_APRIL, total_expense[3],
			BUDGBAL_MAY, total_expense[4],
			BUDGBAL_JUNE, total_expense[5],
			BUDGBAL_JULY, total_expense[6],
			BUDGBAL_AUGUST, total_expense[7],
			BUDGBAL_SEPTEMBER, total_expense[8],
			BUDGBAL_OCTOBER, total_expense[9],
			BUDGBAL_NOVEMBER, total_expense[10],
			BUDGBAL_DECEMBER, total_expense[11],
			-1);
	}

	if (view_mode == BUDGBAL_VIEW_SUMMARY)
	{
		gtk_tree_store_insert_with_values (
			budget,
			&child,
			&iter_total,
			-1,
			BUDGBAL_CATEGORY_NAME, _(N_("Differences")),
			BUDGBAL_ISTOTAL, TRUE,
			BUDGBAL_JANUARY, total_income[0] + total_expense[0],
			BUDGBAL_FEBRUARY, total_income[1] + total_expense[1],
			BUDGBAL_MARCH, total_income[2] + total_expense[2],
			BUDGBAL_APRIL, total_income[3] + total_expense[3],
			BUDGBAL_MAY, total_income[4] + total_expense[4],
			BUDGBAL_JUNE, total_income[5] + total_expense[5],
			BUDGBAL_JULY, total_income[6] + total_expense[6],
			BUDGBAL_AUGUST, total_income[7] + total_expense[7],
			BUDGBAL_SEPTEMBER, total_income[8] + total_expense[8],
			BUDGBAL_OCTOBER, total_income[9] + total_expense[9],
			BUDGBAL_NOVEMBER, total_income[10] + total_expense[10],
			BUDGBAL_DECEMBER, total_income[11] + total_expense[11],
			-1);
	}

	return GTK_TREE_MODEL(budget);
}

// to enable or not edition on month columns
static void repbudgetbalance_view_display_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_sameamount, is_title, is_total, is_visible, is_sensitive, is_editable;
gboolean row_category_type;
gdouble amount = 0.0;
gchar *text;
gchar *fgcolor;
const gint column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter,
		BUDGBAL_CATEGORY_TYPE, &row_category_type,
		BUDGBAL_ISTITLE, &is_title,
		BUDGBAL_ISSAMEAMOUNT, &is_sameamount,
		BUDGBAL_ISTOTAL, &is_total,
		-1);

	// Text to display
	if (is_sameamount)
	{
		gtk_tree_model_get(model, iter, BUDGBAL_SAMEAMOUNT, &amount, -1);
	}
	else if (column_id >= BUDGBAL_JANUARY && column_id <= BUDGBAL_DECEMBER)
	{
		gtk_tree_model_get(model, iter, column_id, &amount, -1);
	}

	text = g_strdup_printf("%.2f", amount);
	fgcolor = get_minimum_color_amount(amount, 0.0);

	// Default styling values
	is_visible = TRUE;
	is_editable = FALSE;
	is_sensitive = TRUE;

	if (is_title)
	{
		is_visible = FALSE;
		is_editable = FALSE;
		is_sensitive = FALSE;
	}
	else if (is_total)
	{
		is_visible = TRUE;
		is_editable = FALSE;
		is_sensitive = FALSE;

		if (column_id == BUDGBAL_SAMEAMOUNT)
		{
			is_visible = FALSE;
		}
	}
	else if (is_sameamount)
	{
		is_visible = TRUE;
		is_editable = FALSE;
		is_sensitive = FALSE;

		if (column_id == BUDGBAL_SAMEAMOUNT)
		{
			is_sensitive = TRUE;
			is_editable = TRUE;
		}
	}
	else if (! is_sameamount)
	{
		is_visible = TRUE;
		is_editable = TRUE;
		is_sensitive = TRUE;

		if (column_id == BUDGBAL_SAMEAMOUNT)
		{
			is_sensitive = FALSE;
			is_editable = FALSE;
		}
	}


	g_object_set(renderer,
		"text", text,
		"visible", is_visible,
		"editable", is_editable,
		"sensitive", is_sensitive,
		"foreground", fgcolor,
		"xalign", 1.0,
		NULL);

	g_free(text);
}

// to enable or not edition on month columns
static void repbudgetbalance_view_display_issameamount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_sameamount, is_title, is_total, is_visible, is_sensitive;

	gtk_tree_model_get(model, iter,
		BUDGBAL_ISTITLE, &is_title,
		BUDGBAL_ISSAMEAMOUNT, &is_sameamount,
		BUDGBAL_ISTOTAL, &is_total,
		-1);

	// Default values
	is_visible = TRUE;
	is_sensitive = TRUE;

	if (is_title || is_total)
	{
		is_visible = FALSE;
		is_sensitive = FALSE;
	}

	g_object_set(renderer,
		"active", is_sameamount,
		"visible", is_visible,
		"sensitive", is_sensitive,
		NULL);
}

// Compute dynamically the yearly total
static void repbudgetbalance_view_display_annualtotal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_sameamount = FALSE, is_total = FALSE, is_title = FALSE;
gdouble amount = 0.0;
gdouble total = 0.0;
gchar *text;
gchar *fgcolor;
gboolean is_visible = TRUE;

	gtk_tree_model_get(model, iter,
		BUDGBAL_ISTITLE, &is_title,
		BUDGBAL_ISSAMEAMOUNT, &is_sameamount,
		BUDGBAL_SAMEAMOUNT, &amount,
		BUDGBAL_ISTOTAL, &is_total,
		-1);

	if (is_sameamount)
	{
		total = 12.0 * amount;
	}
	else
	{
		for (int i = BUDGBAL_JANUARY ; i <= BUDGBAL_DECEMBER ; ++i)
		{
			gtk_tree_model_get(model, iter, i, &amount, -1);
			total += amount;
		}
	}

	text = g_strdup_printf("%.2f", total);
	fgcolor = get_minimum_color_amount(total, 0.0);

	if (is_title)
	{
		is_visible = FALSE;
	}

	g_object_set(renderer,
		"text", text,
		"foreground", fgcolor,
		"visible", is_visible,
		"sensitive", FALSE,
		"xalign", 1.0,
		NULL);

	g_free(text);
}

// When view mode is toggled:
// - compute again the model to add required rows
// - update the view columns to show only the required ones
static void *repbudgetbalance_view_toggle (gpointer user_data, gint view_mode)
{
struct repbudgetbalance_data *data = user_data;
GtkWidget *budget;
GtkTreeModel *model;

	budget = data->TV_budget;

	// Replace model with the new one
	model = repbudgetbalance_model_new(view_mode);
	gtk_tree_view_set_model(GTK_TREE_VIEW(budget), model);
	gtk_tree_view_expand_all(GTK_TREE_VIEW(budget));

	/* to automatically destroy then model with view */
	g_object_unref(model);

	DB(g_print("[repbudgetbalance] : button state changed to: %d\n", view_mode));

	switch(view_mode) {
		case BUDGBAL_VIEW_SUMMARY:
			gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(budget)),
				GTK_SELECTION_NONE);
			gtk_tree_view_column_set_visible(data->TVC_issame, FALSE);
			gtk_tree_view_column_set_visible(data->TVC_monthly, FALSE);
			break;
		case BUDGBAL_VIEW_INCOME:
			gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(budget)),
				GTK_SELECTION_SINGLE);
			gtk_tree_view_column_set_visible(data->TVC_issame, TRUE);
			gtk_tree_view_column_set_visible(data->TVC_monthly, TRUE);
			break;
		case BUDGBAL_VIEW_EXPENSE:
			gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(budget)),
				GTK_SELECTION_SINGLE);
			gtk_tree_view_column_set_visible(data->TVC_issame, TRUE);
			gtk_tree_view_column_set_visible(data->TVC_monthly, TRUE);
			break;
	}
}

// the budget view creation which run the model creation tool

static GtkWidget *repbudgetbalance_view_new (gpointer user_data)
{
GtkTreeViewColumn *col;
GtkCellRenderer *renderer;
GtkWidget *view;
struct repbudgetbalance_data *data = user_data;

	view = gtk_tree_view_new();

	/* --- Category --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Category")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_add_attribute(col, renderer, "text", BUDGBAL_CATEGORY_NAME);

	/* --- Is same amount each month ? --- */
	col = gtk_tree_view_column_new();
	data->TVC_issame = col;
	gtk_tree_view_column_set_title(col, _(N_("Same ?")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, repbudgetbalance_view_display_issameamount, NULL, NULL);

	/* --- Monthly --- */
	col = gtk_tree_view_column_new();
	data->TVC_monthly = col;
	gtk_tree_view_column_set_title(col, _(N_("Monthly")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, repbudgetbalance_view_display_amount, GINT_TO_POINTER(BUDGBAL_SAMEAMOUNT), NULL);

	/* --- Each month --- */
	for (int i = BUDGBAL_JANUARY ; i <= BUDGBAL_DECEMBER ; ++i)
	{
		int month = i - BUDGBAL_JANUARY ;
		col = gtk_tree_view_column_new();

		gtk_tree_view_column_set_title(col, _(BUDGBAL_MONTHS[month]));
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		renderer = gtk_cell_renderer_text_new();

		gtk_tree_view_column_pack_start(col, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(col, renderer, repbudgetbalance_view_display_amount, GINT_TO_POINTER(i), NULL);
	}

	/* --- Year Total -- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Total")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_set_cell_data_func(col, renderer, repbudgetbalance_view_display_annualtotal, NULL, NULL);

	g_object_set(view,
		"enable-grid-lines", GTK_TREE_VIEW_GRID_LINES_BOTH,
		"enable-tree-lines", TRUE,
		NULL);

	return view;
}

// UI actions
static void repbudgetbalance_changed_view_mode (GtkToggleButton *button, gpointer user_data)
{
struct repbudgetbalance_data *data = user_data;
gint view_mode = BUDGBAL_VIEW_SUMMARY;

	// Only run once the view update, so only run on the activated button signal
	if(!gtk_toggle_button_get_active(button))
	{
		return;
	}

	// Mode is directly setted by radio button, because the VIE_MODE and enum
	// for view mode are constructed to correspond (manually)
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));

	DB(g_print("\n[repbudgetbalance] view mode toggled to: %d\n", view_mode));

	repbudgetbalance_view_toggle((gpointer) data, view_mode);

	return;
}

// the window close / deletion
static gboolean repbudgetbalance_window_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
struct repbudgetbalance_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[repbudgetbalance] start dispose\n") );

	g_free(data);

	// TODO Uncomment and adjust when we know name of the window and pref
	//store position and size
	//wg = &PREFS->bud_bal_wg;
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
GtkWidget *window, *grid;
GtkWidget *radiomode, *menu;
GtkWidget *widget, *image;
GtkWidget *scrolledwindow, *treeview;
gint gridrow;

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

	//set a nice dialog size
	gtk_window_set_default_size (GTK_WINDOW(window), 1280, 768);

	// design content
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	g_object_set(grid, "margin", SPACING_MEDIUM, NULL);
	gtk_container_add(GTK_CONTAINER(window), grid);

	// First row displays radio button to change mode (edition / view) and menu
	gridrow = 0;

	// edition mode radio buttons
	//
	radiomode = make_radio(VIEW_MODE, TRUE, GTK_ORIENTATION_HORIZONTAL);
	data->RA_mode = radiomode;
	gtk_widget_set_halign (radiomode, GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (grid), radiomode, 0, gridrow, 1, 1);

	// connect every radio button to the toggled signal to correctly update the view
	for (int i=0; i<**VIEW_MODE; i++){
		widget = radio_get_nth_widget (GTK_CONTAINER(radiomode), i);

		if (widget) {
			g_signal_connect (widget, "toggled", G_CALLBACK (repbudgetbalance_changed_view_mode), (gpointer)data);
		}
	}


	// Hamburger menu
	menu = gtk_menu_new ();
	gtk_widget_set_halign (menu, GTK_ALIGN_END);

	widget = gtk_menu_item_new_with_mnemonic (_("_Import CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), widget);
	//g_signal_connect (G_OBJECT (widget), "activate", G_CALLBACK (ui_bud_manage_load_csv), (gpointer)data);

	widget = gtk_menu_item_new_with_mnemonic (_("E_xport CSV"));
	gtk_menu_shell_append (GTK_MENU_SHELL (menu), widget);
	//g_signal_connect (G_OBJECT (widget), "activate", G_CALLBACK (ui_bud_manage_save_csv), (gpointer)data);

	gtk_widget_show_all (menu);

	widget = gtk_menu_button_new();
	image = gtk_image_new_from_icon_name (ICONNAME_HB_BUTTON_MENU, GTK_ICON_SIZE_MENU);

	g_object_set (widget, "image", image, "popup", GTK_MENU(menu),  NULL);

	gtk_widget_set_hexpand (widget, FALSE);
	gtk_widget_set_halign (widget, GTK_ALIGN_END);
	gtk_grid_attach (GTK_GRID (grid), widget, 0, gridrow, 1, 1);

	// Next row displays the budget tree
	gridrow++;

	// ScrolledWindow will permit to display budgets with a lot of active categories
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_hexpand (scrolledwindow, TRUE);
	gtk_widget_set_vexpand (scrolledwindow, TRUE);
	gtk_grid_attach (GTK_GRID (grid), scrolledwindow, 0, gridrow, 1, 1);

	treeview = repbudgetbalance_view_new ((gpointer) data);
	data->TV_budget = treeview;
	gtk_container_add(GTK_CONTAINER(scrolledwindow), treeview);

	// By default, show the reader mode
	repbudgetbalance_view_toggle((gpointer) data, BUDGBAL_VIEW_SUMMARY);

	/* signal connect */
	g_signal_connect (window, "delete-event", G_CALLBACK (repbudgetbalance_window_dispose), (gpointer)data);

	// TODO Uncomment and adjust when we know name of the window and pref
	//setup, init and show window
	//wg = &PREFS->bud_wg;
	//gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
	//gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);

	gtk_widget_show_all (window);

	return(window);
}

