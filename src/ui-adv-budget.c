/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 2018-2019 Adrien Dorsaz <adrien@adorsaz.ch>
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
#include "ui-adv-budget.h"

/****************************************************************************/
/* Implementation notes */
/****************************************************************************/
/*
 * This dialog allows user to manage its budget with a GtkTreeView.
 *
 * The view rows are separated in three main tree roots:
 *   - Income: contains all Homebank categories of income type (see GF_INCOME)
 *   - Expense: contains all Homebank categories of expense type
 *   - Total: contains 3 sub-rows:
 *      - Income: sum all amounts of the Income root
 *      - Expense: sum all amounts of the Expense root
 *      - Balance: difference between the two above sub-rows
 *
 * The view columns contain:
 *   - Category name: Homebank categories organised in hierarchy
 *     See the main tree roots above.
 *   - Force Display: toggle the GF_FORCED flag of Homebank categories
 *     Is this category forced to be displayed, even if no budget has been set?
 *   - Same: toggle the GF_CUSTOM flag of Homebank categories
 *     Does this category have same amount planned for every month ?
 *   - Monthly: set the monthly amount when the Same flag is active
 *   - 12 columns for each month of the year containing their specific amount
 *   - Total: sum all amounts of the year for the category
 *
 * The dialog show 3 radio buttons to choose between 3 edition modes:
 *   - Balance: show Homebank categories with budget set or set with GF_FORCED
 *     This mode hide the "Force Display" column
 *   - Income: show all available Homebank categories of income type
 *   - Expense: show all available Homebank categories of expense type
 *
 */

/****************************************************************************/
/* Debug macros */
/****************************************************************************/
#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* Global data */
extern struct HomeBank *GLOBALS;
extern struct Preferences *PREFS;

static gchar *ADVBUD_MONTHS[] = {
	N_("Jan"), N_("Feb"), N_("Mar"),
	N_("Apr"), N_("May"), N_("Jun"),
	N_("Jul"), N_("Aug"), N_("Sept"),
	N_("Oct"), N_("Nov"), N_("Dec"),
	NULL};

/* The different view mode available */
static gchar *ADVBUD_VIEW_MODE[] = {
	N_("Balance"),
	N_("Income"),
	N_("Expense"),
	NULL
};

/* These values has to correspond to ADVBUD_VIEW_MODE[] */
enum advbud_view_mode {
	ADVBUD_VIEW_BALANCE = 0,
	ADVBUD_VIEW_INCOME,
	ADVBUD_VIEW_EXPENSE
};
typedef enum advbud_view_mode advbud_view_mode_t;

/* These values corresponds to the return of category_type_get from hb-category */
enum advbud_cat_type {
	ADVBUD_CAT_TYPE_EXPENSE = -1,
	ADVBUD_CAT_TYPE_NONE = 0, // Not real category type: used to retrieve tree roots
	ADVBUD_CAT_TYPE_INCOME = 1
};
typedef enum advbud_cat_type advbud_cat_type_t;

/* enum for the Budget Tree Store model */
enum advbud_store {
	ADVBUD_CATEGORY_KEY = 0,
	ADVBUD_CATEGORY_NAME,
	ADVBUD_CATEGORY_TYPE,
	ADVBUD_ISROOT, // To retrieve easier the 3 main tree roots
	ADVBUD_ISTOTAL, // To retrieve rows inside the Total root
	ADVBUD_ISSEPARATOR, // Row to just display a separator in Tree View
	ADVBUD_ISDISPLAYFORCED,
	ADVBUD_ISSAMEAMOUNT,
	ADVBUD_SAMEAMOUNT,
	ADVBUD_JANUARY,
	ADVBUD_FEBRUARY,
	ADVBUD_MARCH,
	ADVBUD_APRIL,
	ADVBUD_MAY,
	ADVBUD_JUNE,
	ADVBUD_JULY,
	ADVBUD_AUGUST,
	ADVBUD_SEPTEMBER,
	ADVBUD_OCTOBER,
	ADVBUD_NOVEMBER,
	ADVBUD_DECEMBER,
	ADVBUD_NUMCOLS
};
typedef enum advbud_store advbud_store_t;

// A small structure to retrieve a category with its iterator
struct advbud_category_iterator {
	guint32 key; // key defining the category
	GtkTreeIter *iterator; // NULL if iterator has not been found
};
typedef struct advbud_category_iterator advbud_category_iterator_t;

// Retrieve a budget iterator according
struct advbud_budget_iterator {
	advbud_cat_type_t category_type; // Type of the category
	gboolean category_isroot;
	gboolean category_istotal;
	GtkTreeIter *iterator;
};
typedef struct advbud_budget_iterator advbud_budget_iterator_t;

/*
 * Local headers
 **/

// GtkTreeStore model
static gboolean ui_adv_bud_model_get_category_iterator (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, advbud_category_iterator_t *data);
static void ui_adv_bud_model_add_category_with_lineage(GtkTreeStore *budget, GtkTreeIter *balanceIter, guint32 *key_category);
static gboolean ui_adv_bud_model_get_budget_iterator (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, advbud_budget_iterator_t * budget_iter);
static void ui_adv_bud_model_collapse (GtkTreeView *view);
static void ui_adv_bud_model_insert_roots(GtkTreeStore* budget);
static void ui_adv_bud_model_update_monthlytotal(GtkTreeStore* budget);
static gboolean ui_adv_bud_model_row_filter (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static gboolean ui_adv_bud_model_row_filter_parents (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static gint ui_adv_bud_model_row_sort (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer user_data);
static GtkTreeModel * ui_adv_bud_model_new ();

// GtkTreeView widget
static void ui_adv_bud_view_display_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_display_issameamount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_display_isdisplayforced (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_display_annualtotal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_toggle (gpointer user_data, advbud_view_mode_t view_mode);
static gboolean ui_adv_view_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer data);
static GtkWidget *ui_adv_bud_view_new (gpointer user_data);

// UI actions
static void ui_adv_bud_cell_update_category(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data);
static void ui_adv_bud_cell_update_amount(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data);
static void ui_adv_bud_cell_update_issameamount(GtkCellRendererText *renderer, gchar *filter_path, gpointer user_data);
static void ui_adv_bud_cell_update_isdisplayforced(GtkCellRendererText *renderer, gchar *filter_path, gpointer user_data);
static void ui_adv_bud_view_update_mode (GtkToggleButton *button, gpointer user_data);
static void ui_adv_bud_view_expand (GtkButton *button, gpointer user_data);
static void ui_adv_bud_view_collapse (GtkButton *button, gpointer user_data);
static void ui_adv_bud_category_add (GtkButton *button, gpointer user_data);
static void ui_adv_bud_dialog_close(adv_bud_data_t *data, gint response);

/**
 * GtkTreeStore model
 **/

// look for parent category
static gboolean ui_adv_bud_model_get_category_iterator (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, advbud_category_iterator_t *data)
{
guint32 row_category_key;
gboolean is_root, is_total;
guint32 key = data->key;

	data->iterator = NULL;

	gtk_tree_model_get (model, iter,
		ADVBUD_CATEGORY_KEY, &row_category_key,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISTOTAL, &is_total,
		-1);

	if ( row_category_key == key
			&& !(is_total)
			&& !(is_root)
	)
	{
		DB(g_print("\tFound row with key %d\n", key));

		data->iterator = g_malloc0(sizeof(GtkTreeIter));
		*data->iterator = *iter;

		return TRUE;
	}

	return FALSE;
}

/* Recursive function which add a new row in the budget model with all its ancestors */
static void ui_adv_bud_model_add_category_with_lineage(GtkTreeStore *budget, GtkTreeIter *balanceIter, guint32 *key_category)
{
GtkTreeIter child;
GtkTreeIter *parent;
Category *bdg_category;
gboolean cat_is_sameamount;

	bdg_category = da_cat_get(*key_category);

	if (bdg_category == NULL)
	{
		return;
	}

	cat_is_sameamount = (! (bdg_category->flags & GF_CUSTOM));

	/* Check if parent category already exists */
	advbud_category_iterator_t *parent_category_iterator;

	parent_category_iterator = g_malloc0(sizeof(advbud_category_iterator_t));
	parent_category_iterator->key = bdg_category->parent;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_category_iterator,
		parent_category_iterator);

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
		if (parent_category_iterator->iterator)
		{
			DB(g_print("\tRecursion optimisation: parent key %d already exists\n", parent_category_iterator->key));
			// If parent already exists, stop recursion
			parent = parent_category_iterator->iterator;
		}
		else
		{
			// Parent has not been found, ask to create it first
			ui_adv_bud_model_add_category_with_lineage(budget, balanceIter, &(bdg_category->parent));

			// Now, we are sure parent exists, look for it again
			gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
				(GtkTreeModelForeachFunc) ui_adv_bud_model_get_category_iterator,
				parent_category_iterator);

			parent = parent_category_iterator->iterator;
		}

		gtk_tree_store_insert (
			budget,
			&child,
			parent,
			-1);
	}

	DB(g_print("insert new category %s (key: %d, type: %d)\n",
		bdg_category->name, bdg_category->key, category_type_get (bdg_category)));

	gtk_tree_store_set(
		budget,
		&child,
		ADVBUD_CATEGORY_KEY, bdg_category->key,
		ADVBUD_CATEGORY_NAME, bdg_category->name,
		ADVBUD_CATEGORY_TYPE, category_type_get (bdg_category),
		ADVBUD_ISDISPLAYFORCED, (bdg_category->flags & GF_FORCED),
		ADVBUD_ISROOT, FALSE,
		ADVBUD_ISSAMEAMOUNT, cat_is_sameamount,
		ADVBUD_ISTOTAL, FALSE,
		ADVBUD_SAMEAMOUNT, bdg_category->budget[0],
		ADVBUD_JANUARY, bdg_category->budget[1],
		ADVBUD_FEBRUARY, bdg_category->budget[2],
		ADVBUD_MARCH, bdg_category->budget[3],
		ADVBUD_APRIL, bdg_category->budget[4],
		ADVBUD_MAY, bdg_category->budget[5],
		ADVBUD_JUNE, bdg_category->budget[6],
		ADVBUD_JULY, bdg_category->budget[7],
		ADVBUD_AUGUST, bdg_category->budget[8],
		ADVBUD_SEPTEMBER, bdg_category->budget[9],
		ADVBUD_OCTOBER, bdg_category->budget[10],
		ADVBUD_NOVEMBER, bdg_category->budget[11],
		ADVBUD_DECEMBER, bdg_category->budget[12],
		-1);

	g_free(parent_category_iterator->iterator);
	g_free(parent_category_iterator);

	return;
}

// Look for category by deterministic characteristics
// Only categories with specific characteristics can be easily found
// like roots and total rows
static gboolean ui_adv_bud_model_get_budget_iterator (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, advbud_budget_iterator_t * budget_iter)
{
advbud_cat_type_t category_type;
gboolean result = FALSE, is_root, is_total, is_separator;

	gtk_tree_model_get (model, iter,
		ADVBUD_CATEGORY_TYPE, &category_type,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISTOTAL, &is_total,
		ADVBUD_ISSEPARATOR, &is_separator,
		-1);

	budget_iter->iterator = NULL;

	if (is_root == budget_iter->category_isroot
		&& is_total == budget_iter->category_istotal
		&& category_type == budget_iter->category_type
		&& !is_separator
	)
	{
		budget_iter->iterator = g_malloc0(sizeof(GtkTreeIter));
		*budget_iter->iterator = *iter;

		result = TRUE;
	}

	return result;
}

// Collapse all categories except root
static void ui_adv_bud_model_collapse (GtkTreeView *view) {
GtkTreeModel *budget;
GtkTreePath *path;
advbud_budget_iterator_t *budget_iter;

	budget_iter = g_malloc0(sizeof(advbud_budget_iterator_t));

	budget = gtk_tree_view_get_model (view);

	gtk_tree_view_collapse_all(view);

	// Keep root categories expanded

	// Retrieve income root
	budget_iter->category_isroot = TRUE;
	budget_iter->category_istotal = FALSE;
	budget_iter->category_type = ADVBUD_CAT_TYPE_INCOME;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);

	if (budget_iter->iterator != NULL) {
		path = gtk_tree_model_get_path(budget, budget_iter->iterator);
		gtk_tree_view_expand_row(view, path, FALSE);
	}

	// Retrieve expense root
	budget_iter->category_isroot = TRUE;
	budget_iter->category_istotal = FALSE;
	budget_iter->category_type = ADVBUD_CAT_TYPE_EXPENSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);

	if (budget_iter->iterator != NULL) {
		path = gtk_tree_model_get_path(budget, budget_iter->iterator);
		gtk_tree_view_expand_row(view, path, FALSE);
	}

	// Retrieve total root
	budget_iter->category_isroot = TRUE;
	budget_iter->category_istotal = FALSE;
	budget_iter->category_type = ADVBUD_CAT_TYPE_NONE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);

	if (budget_iter->iterator != NULL) {
		path = gtk_tree_model_get_path(budget, budget_iter->iterator);
		gtk_tree_view_expand_row(view, path, FALSE);
	}

	g_free(budget_iter->iterator);
	g_free(budget_iter);

	return;
}

// Create tree roots for the store
static void ui_adv_bud_model_insert_roots(GtkTreeStore* budget)
{
GtkTreeIter iter, root;

	gtk_tree_store_insert_with_values (
		budget,
		&root,
		NULL,
		-1,
		ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_INCOME]),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_INCOME,
		ADVBUD_ISROOT, TRUE,
		ADVBUD_ISTOTAL, FALSE,
		-1);

	// For add category dialog: copy of the root to be able to select it
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_INCOME]),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_INCOME,
		ADVBUD_CATEGORY_KEY, 0,
		ADVBUD_ISROOT, FALSE,
		ADVBUD_ISTOTAL, FALSE,
		-1);

	// For add category dialog: add a separator to distinguish root with children
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		ADVBUD_ISSEPARATOR, TRUE,
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_INCOME,
		-1);

	gtk_tree_store_insert_with_values (
		budget,
		&root,
		NULL,
		-1,
		ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_EXPENSE]),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_EXPENSE,
		ADVBUD_ISROOT, TRUE,
		ADVBUD_ISTOTAL, FALSE,
		-1);

	// For add category dialog: copy of the root to be able to select it
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_EXPENSE]),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_EXPENSE,
		ADVBUD_CATEGORY_KEY, 0,
		ADVBUD_ISROOT, FALSE,
		ADVBUD_ISTOTAL, FALSE,
		-1);

	// For add category dialog: add a separator to distinguish root with children
	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		&root,
		-1,
		ADVBUD_ISSEPARATOR, TRUE,
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_EXPENSE,
		-1);

	gtk_tree_store_insert_with_values (
		budget,
		&root,
		NULL,
		-1,
		ADVBUD_CATEGORY_NAME, _("Totals"),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_NONE,
		ADVBUD_ISROOT, TRUE,
		ADVBUD_ISTOTAL, FALSE,
		-1);

	return;
}

// Update (or insert) total rows for a budget according to the view mode
// This function will is used to initiate model and to refresh it after change by user
static void ui_adv_bud_model_update_monthlytotal(GtkTreeStore* budget)
{
advbud_budget_iterator_t *budget_iter;
GtkTreeIter total_root, child;
double total_income[12] = {0}, total_expense[12] = {0};
gboolean cat_is_sameamount;
int n_category;

	budget_iter = g_malloc0(sizeof(advbud_budget_iterator_t));

	// Go through all categories to compute totals
	n_category = da_cat_get_max_key();

	for(guint32 i=1; i<=n_category; ++i)
	{
	Category *bdg_category;
	gboolean cat_is_income;

		bdg_category = da_cat_get(i);

		if (bdg_category == NULL)
		{
			continue;
		}

		cat_is_income = (category_type_get (bdg_category) == 1);
		cat_is_sameamount = (! (bdg_category->flags & GF_CUSTOM));

		for (gint j=0; j<=11; ++j)
		{
			if (cat_is_income)
			{
				if (cat_is_sameamount)
				{
					total_income[j] += bdg_category->budget[0];
				}
				else
				{
					total_income[j] += bdg_category->budget[j+1];
				}
			}
			else
			{
				if (cat_is_sameamount)
				{
					total_expense[j] += bdg_category->budget[0];
				}
				else
				{
					total_expense[j] += bdg_category->budget[j+1];
				}
			}
		}
	}

	// Retrieve total root and insert required total rows
	budget_iter->category_isroot = TRUE;
	budget_iter->category_istotal = FALSE;
	budget_iter->category_type = ADVBUD_CAT_TYPE_NONE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);

	if (!budget_iter->iterator) {
		return;
	}

	total_root = *budget_iter->iterator;

	// Retrieve and set total income
	budget_iter->category_isroot = FALSE;
	budget_iter->category_istotal = TRUE;

	budget_iter->category_type = ADVBUD_CAT_TYPE_INCOME;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);

	if (budget_iter->iterator) {
		child = *budget_iter->iterator;
	}
	else {
		gtk_tree_store_insert(budget, &child, &total_root, -1);
	}

	gtk_tree_store_set (
		budget,
		&child,
		ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_INCOME]),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_INCOME,
		ADVBUD_ISTOTAL, TRUE,
		ADVBUD_JANUARY, total_income[0],
		ADVBUD_FEBRUARY, total_income[1],
		ADVBUD_MARCH, total_income[2],
		ADVBUD_APRIL, total_income[3],
		ADVBUD_MAY, total_income[4],
		ADVBUD_JUNE, total_income[5],
		ADVBUD_JULY, total_income[6],
		ADVBUD_AUGUST, total_income[7],
		ADVBUD_SEPTEMBER, total_income[8],
		ADVBUD_OCTOBER, total_income[9],
		ADVBUD_NOVEMBER, total_income[10],
		ADVBUD_DECEMBER, total_income[11],
		-1);

	budget_iter->category_type = ADVBUD_CAT_TYPE_EXPENSE;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);

	if (budget_iter->iterator) {
		child = *budget_iter->iterator;
	}
	else {
		gtk_tree_store_insert(budget, &child, &total_root, -1);
	}

	gtk_tree_store_set (
		budget,
		&child,
		ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_EXPENSE]),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_EXPENSE,
		ADVBUD_ISTOTAL, TRUE,
		ADVBUD_JANUARY, total_expense[0],
		ADVBUD_FEBRUARY, total_expense[1],
		ADVBUD_MARCH, total_expense[2],
		ADVBUD_APRIL, total_expense[3],
		ADVBUD_MAY, total_expense[4],
		ADVBUD_JUNE, total_expense[5],
		ADVBUD_JULY, total_expense[6],
		ADVBUD_AUGUST, total_expense[7],
		ADVBUD_SEPTEMBER, total_expense[8],
		ADVBUD_OCTOBER, total_expense[9],
		ADVBUD_NOVEMBER, total_expense[10],
		ADVBUD_DECEMBER, total_expense[11],
		-1);

	budget_iter->category_type = ADVBUD_CAT_TYPE_NONE;

	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);

	if (budget_iter->iterator) {
		child = *budget_iter->iterator;
	}
	else {
		gtk_tree_store_insert(budget, &child, &total_root, -1);
	}

	gtk_tree_store_set (
		budget,
		&child,
		ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_BALANCE]),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_NONE,
		ADVBUD_ISTOTAL, TRUE,
		ADVBUD_JANUARY, total_income[0] + total_expense[0],
		ADVBUD_FEBRUARY, total_income[1] + total_expense[1],
		ADVBUD_MARCH, total_income[2] + total_expense[2],
		ADVBUD_APRIL, total_income[3] + total_expense[3],
		ADVBUD_MAY, total_income[4] + total_expense[4],
		ADVBUD_JUNE, total_income[5] + total_expense[5],
		ADVBUD_JULY, total_income[6] + total_expense[6],
		ADVBUD_AUGUST, total_income[7] + total_expense[7],
		ADVBUD_SEPTEMBER, total_income[8] + total_expense[8],
		ADVBUD_OCTOBER, total_income[9] + total_expense[9],
		ADVBUD_NOVEMBER, total_income[10] + total_expense[10],
		ADVBUD_DECEMBER, total_income[11] + total_expense[11],
		-1);

	g_free(budget_iter->iterator);
	g_free(budget_iter);

	return;
}

// Filter shown rows according to VIEW mode
static gboolean ui_adv_bud_model_row_filter (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_visible, is_root, is_total, is_separator;
adv_bud_data_t* data;
advbud_view_mode_t view_mode;
guint32 category_key;
advbud_cat_type_t category_type;
Category *bdg_category;

	is_visible = TRUE;
	data = user_data;
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));

	gtk_tree_model_get(model, iter,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISTOTAL, &is_total,
		ADVBUD_ISSEPARATOR, &is_separator,
		ADVBUD_CATEGORY_KEY, &category_key,
		ADVBUD_CATEGORY_TYPE, &category_type,
		-1);

	// On specific mode, hidde categories of opposite type
	if (!is_total
		&& category_type == ADVBUD_CAT_TYPE_INCOME
		&& view_mode == ADVBUD_VIEW_EXPENSE)
	{
		is_visible = FALSE;
	}

	if (!is_total
		&& category_type == ADVBUD_CAT_TYPE_EXPENSE
		&& view_mode == ADVBUD_VIEW_INCOME)
	{
		is_visible = FALSE;
	}

	// Hidde fake first child root used for add dialog
	if (!is_total
		&& !is_root
		&& category_key == 0)
	{
		is_visible = FALSE;
	}

	// On balance mode, hidde not forced empty categories
	if (!is_total
		&& !is_root
		&& category_key != 0
		&& view_mode == ADVBUD_VIEW_BALANCE)
	{
		bdg_category = da_cat_get(category_key);

		if (bdg_category != NULL)
		{
			// Either the category has some budget, or its display is forced
			is_visible = (bdg_category->flags & (GF_BUDGET|GF_FORCED));

			// Force display if one of its children should be displayed
			if (!is_visible)
			{
			GtkTreeIter child;
			Category *subcat;
			guint32 subcat_key;
			gint child_id=0;

				while (gtk_tree_model_iter_nth_child(model,
					&child,
					iter,
					child_id))
				{
					gtk_tree_model_get(model, &child,
						ADVBUD_CATEGORY_KEY, &subcat_key,
						-1);

					if (subcat_key != 0)
					{
						subcat = da_cat_get (subcat_key);

						if (subcat != NULL)
						{
							is_visible = (subcat->flags & (GF_BUDGET|GF_FORCED));
						}
					}

					// Stop loop on first visible children
					if (is_visible)
					{
						break;
					}

					++child_id;
				}
			}
		}
	}

	return is_visible;
}


// Filter shown rows according to VIEW mode
static gboolean ui_adv_bud_model_row_filter_parents (GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_visible, is_root, is_total;
Category *bdg_category;
guint32 category_key;
advbud_cat_type_t category_type;
adv_bud_data_t* data;
advbud_view_mode_t view_mode;

	is_visible = TRUE;

	data = user_data;
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));;

	gtk_tree_model_get(model, iter,
		ADVBUD_CATEGORY_KEY, &category_key,
		ADVBUD_CATEGORY_TYPE, &category_type,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISTOTAL, &is_total,
		-1);

	// Hidde Total root
	if (is_root
			&& category_type == ADVBUD_CAT_TYPE_NONE )
	{
		is_visible = FALSE;
	}

	// Hidde Total rows
	if (is_total) {
		is_visible = FALSE;
	}

	// Hidde rows without the good category type according to the view mode
	if (category_type == ADVBUD_CAT_TYPE_INCOME
		&& view_mode == ADVBUD_VIEW_EXPENSE)
	{
		is_visible = FALSE;
	}
	else if(category_type == ADVBUD_CAT_TYPE_EXPENSE
					&& view_mode == ADVBUD_VIEW_INCOME)
	{
		is_visible = FALSE;
	}
	else if(category_key > 0) {
		// Show categories without parents
		bdg_category = da_cat_get(category_key);

		if (bdg_category != NULL)
		{
			if (bdg_category->parent > 0)
			{
				is_visible = FALSE;
			}
		}
	}

	return is_visible;
}

static gint ui_adv_bud_model_row_sort (GtkTreeModel *model, GtkTreeIter *cat_a, GtkTreeIter *cat_b, gpointer user_data)
{
const gchar* cat_a_name;
const gchar* cat_b_name;
advbud_cat_type_t cat_a_type, cat_b_type;
guint32 cat_a_key, cat_b_key;
gboolean cat_a_is_separator, cat_b_is_separator;
gint order = 0;

	gtk_tree_model_get(model, cat_a,
		ADVBUD_CATEGORY_NAME, &cat_a_name,
		ADVBUD_CATEGORY_TYPE, &cat_a_type,
		ADVBUD_CATEGORY_KEY, &cat_a_key,
		ADVBUD_ISSEPARATOR, &cat_a_is_separator,
		-1);

	gtk_tree_model_get(model, cat_b,
		ADVBUD_CATEGORY_NAME, &cat_b_name,
		ADVBUD_CATEGORY_TYPE, &cat_b_type,
		ADVBUD_CATEGORY_KEY, &cat_b_key,
		ADVBUD_ISSEPARATOR, &cat_b_is_separator,
		-1);

	// Sort first by category type
	if (cat_a_type != cat_b_type)
	{
		switch (cat_a_type)
		{
			case ADVBUD_CAT_TYPE_INCOME:
				order = -1;
				break;
			case ADVBUD_CAT_TYPE_EXPENSE:
				order = 0;
				break;
			case ADVBUD_CAT_TYPE_NONE:
				order = 1;
				break;
		}
	}
	else
	{
		// On standard categories, just order by name
		if (cat_a_key != 0 && cat_b_key != 0)
		{
			order = g_strcmp0(g_utf8_casefold(cat_a_name, -1),
				g_utf8_casefold(cat_b_name, -1)
				);
		}
		// Otherwise, fake categories have to be first (header and separator)
		else if (cat_a_key == 0)
		{
			if (cat_b_key != 0)
			{
				order = -1;
			}
			// When both are fake, header has to be first
			else
			{
				order = (cat_a_is_separator ? 1 : -1);
			}
		}
		else
		{
			// Same idea for fake categories when cat_b is fake, but
			// with reversed result, because sort function return
			// result according to cat_a

			if (cat_a_key != 0)
			{
				order = 1;
			}
			else
			{
				order = (cat_b_is_separator ? -1 : 1);
			}
		}
	}

	return order;
}

// the budget model creation
static GtkTreeModel * ui_adv_bud_model_new ()
{
GtkTreeStore *budget;
GtkTreeIter *iter_income, *iter_expense;
guint32 n_category;
advbud_budget_iterator_t *budget_iter;

	// Create Tree Store
	budget = gtk_tree_store_new ( ADVBUD_NUMCOLS,
		G_TYPE_UINT, // ADVBUD_CATEGORY_KEY
		G_TYPE_STRING, // ADVBUD_CATEGORY_NAME
		G_TYPE_INT, // ADVBUD_CATEGORY_TYPE
		G_TYPE_BOOLEAN, // ADVBUD_ISROOT
		G_TYPE_BOOLEAN, // ADVBUD_ISTOTAL
		G_TYPE_BOOLEAN, // ADVBUD_ISSEPARATOR
		G_TYPE_BOOLEAN, // ADVBUD_ISDISPLAYFORCED
		G_TYPE_BOOLEAN, // ADVBUD_ISSAMEAMOUNT
		G_TYPE_DOUBLE, // ADVBUD_SAMEAMOUNT
		G_TYPE_DOUBLE, // ADVBUD_JANUARY
		G_TYPE_DOUBLE, // ADVBUD_FEBRUARY
		G_TYPE_DOUBLE, // ADVBUD_MARCH
		G_TYPE_DOUBLE, // ADVBUD_APRIL
		G_TYPE_DOUBLE, // ADVBUD_MAY
		G_TYPE_DOUBLE, // ADVBUD_JUNE
		G_TYPE_DOUBLE, // ADVBUD_JULY
		G_TYPE_DOUBLE, // ADVBUD_AUGUST
		G_TYPE_DOUBLE, // ADVBUD_SEPTEMBER
		G_TYPE_DOUBLE, // ADVBUD_OCTOBER
		G_TYPE_DOUBLE, // ADVBUD_NOVEMBER
		G_TYPE_DOUBLE  // ADVBUD_DECEMBER
	);

	// Populate the store

	/* Create tree roots */
	ui_adv_bud_model_insert_roots (budget);

	// Retrieve required root
	budget_iter = g_malloc0(sizeof(advbud_budget_iterator_t));
	budget_iter->iterator = g_malloc0(sizeof(GtkTreeIter));
	budget_iter->category_isroot = TRUE;
	budget_iter->category_istotal = FALSE;

	budget_iter->category_type = ADVBUD_CAT_TYPE_INCOME;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);
	iter_income = budget_iter->iterator;

	budget_iter->category_type = ADVBUD_CAT_TYPE_EXPENSE;
	gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
		(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
		budget_iter);
	iter_expense = budget_iter->iterator;

	/* Create rows for real categories */
	n_category = da_cat_get_max_key();

	for(guint32 i=1; i<=n_category; ++i)
	{
	Category *bdg_category;
	gboolean cat_is_income;

		bdg_category = da_cat_get(i);

		if (bdg_category == NULL)
		{
			continue;
		}

		cat_is_income = (category_type_get (bdg_category) == 1);

		DB(g_print(" category %d:'%s' isincome=%d, issub=%d hasbudget=%d parent=%d\n",
			bdg_category->key, bdg_category->name,
			cat_is_income, (bdg_category->flags & GF_SUB),
			(bdg_category->flags & GF_BUDGET), bdg_category->parent));

		// Compute totals and initiate category in right tree root
		if (cat_is_income)
		{
			ui_adv_bud_model_add_category_with_lineage(budget, iter_income, &(bdg_category->key));
		}
		else if (!cat_is_income)
		{
			ui_adv_bud_model_add_category_with_lineage(budget, iter_expense, &(bdg_category->key));
		}
	}

	/* Create rows for total root */
	ui_adv_bud_model_update_monthlytotal(GTK_TREE_STORE(budget));

	g_free(budget_iter->iterator);
	g_free(budget_iter);

	return GTK_TREE_MODEL(budget);
}

/**
 * GtkTreeView functions
 **/

// to enable or not edition on month columns
static void ui_adv_bud_view_display_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GtkAdjustment *adjustment;
gboolean is_sameamount, is_root, is_total, is_visible, is_sensitive, is_editable;
advbud_cat_type_t row_category_type;
gdouble amount = 0.0;
gchar *text;
gchar *fgcolor;
const advbud_store_t column_id = GPOINTER_TO_INT(user_data);

	gtk_tree_model_get(model, iter,
		ADVBUD_CATEGORY_TYPE, &row_category_type,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISSAMEAMOUNT, &is_sameamount,
		ADVBUD_ISTOTAL, &is_total,
		-1);

	// Text to display
	if (is_sameamount)
	{
		gtk_tree_model_get(model, iter, ADVBUD_SAMEAMOUNT, &amount, -1);
	}
	else if (column_id >= ADVBUD_JANUARY && column_id <= ADVBUD_DECEMBER)
	{
		gtk_tree_model_get(model, iter, column_id, &amount, -1);
	}

	text = g_strdup_printf("%.2f", amount);
	fgcolor = get_minimum_color_amount(amount, 0.0);

	// Default styling values
	is_visible = TRUE;
	is_editable = FALSE;
	is_sensitive = TRUE;

	if (is_root)
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

		if (column_id == ADVBUD_SAMEAMOUNT)
		{
			is_visible = FALSE;
		}
	}
	else if (is_sameamount)
	{
		is_visible = TRUE;
		is_editable = FALSE;
		is_sensitive = FALSE;

		if (column_id == ADVBUD_SAMEAMOUNT)
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

		if (column_id == ADVBUD_SAMEAMOUNT)
		{
			is_sensitive = FALSE;
			is_editable = FALSE;
		}
	}

	adjustment = gtk_adjustment_new(
		0.0, // initial-value
		-G_MAXDOUBLE, // minmal-value
		G_MAXDOUBLE, // maximal-value
		0.5, // step increment
		10, // page increment
		0); // page size (0 because irrelevant for GtkSpinButton)

	g_object_set(renderer,
		"text", text,
		"visible", is_visible,
		"editable", is_editable,
		"sensitive", is_sensitive,
		"foreground", fgcolor,
		"xalign", 1.0,
		"adjustment", adjustment,
		"digits", 2,
		NULL);

	g_free(text);
}

// to enable or not edition on month columns
static void ui_adv_bud_view_display_issameamount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_sameamount, is_root, is_total, is_visible, is_sensitive;

	gtk_tree_model_get(model, iter,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISSAMEAMOUNT, &is_sameamount,
		ADVBUD_ISTOTAL, &is_total,
		-1);

	// Default values
	is_visible = TRUE;
	is_sensitive = TRUE;

	if (is_root || is_total)
	{
		is_visible = FALSE;
		is_sensitive = FALSE;
	}

	g_object_set(renderer,
		"activatable", TRUE,
		"active", is_sameamount,
		"visible", is_visible,
		"sensitive", is_sensitive,
		NULL);
}

// Toggle force display
static void ui_adv_bud_view_display_isdisplayforced (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_displayforced, is_root, is_total, is_visible, is_sensitive;

	gtk_tree_model_get(model, iter,
		ADVBUD_ISDISPLAYFORCED, &is_displayforced,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISTOTAL, &is_total,
		-1);

	// Default values
	is_visible = TRUE;
	is_sensitive = TRUE;

	if (is_root || is_total)
	{
		is_visible = FALSE;
		is_sensitive = FALSE;
	}

	g_object_set(renderer,
		"activatable", TRUE,
		"active", is_displayforced,
		"visible", is_visible,
		"sensitive", is_sensitive,
		NULL);
}

// Compute dynamically the yearly total
static void ui_adv_bud_view_display_annualtotal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
gboolean is_sameamount = FALSE, is_total = FALSE, is_root = FALSE;
gdouble amount = 0.0;
gdouble total = 0.0;
gchar *text;
gchar *fgcolor;
gboolean is_visible = TRUE;

	gtk_tree_model_get(model, iter,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISSAMEAMOUNT, &is_sameamount,
		ADVBUD_SAMEAMOUNT, &amount,
		ADVBUD_ISTOTAL, &is_total,
		-1);

	if (is_sameamount)
	{
		total = 12.0 * amount;
	}
	else
	{
		for (int i = ADVBUD_JANUARY ; i <= ADVBUD_DECEMBER ; ++i)
		{
			gtk_tree_model_get(model, iter, i, &amount, -1);
			total += amount;
		}
	}

	text = g_strdup_printf("%.2f", total);
	fgcolor = get_minimum_color_amount(total, 0.0);

	if (is_root)
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
// - recreate the view to update columns rendering
static void ui_adv_bud_view_toggle (gpointer user_data, advbud_view_mode_t view_mode)
{
adv_bud_data_t *data = user_data;
GtkTreeModel *budget;
GtkWidget *view;

	view = data->TV_budget;

	budget = gtk_tree_view_get_model(GTK_TREE_VIEW(view));

	if (view_mode == ADVBUD_VIEW_BALANCE)
	{
		gtk_tree_view_column_set_visible(data->TVC_category, TRUE);
		gtk_tree_view_column_set_visible(data->TVC_category_with_force, FALSE);
	}
	else
	{
		gtk_tree_view_column_set_visible(data->TVC_category, FALSE);
		gtk_tree_view_column_set_visible(data->TVC_category_with_force, TRUE);
	}

	gtk_tree_model_filter_refilter(GTK_TREE_MODEL_FILTER(budget));

	if (data->TV_isexpanded) {
		gtk_tree_view_expand_all(GTK_TREE_VIEW(view));
	}
	else {
		ui_adv_bud_model_collapse(GTK_TREE_VIEW(view));
	}

	DB(g_print("[ui_adv_bud] : button state changed to: %d\n", view_mode));

	return;
}

static gboolean ui_adv_view_separator (GtkTreeModel *model, GtkTreeIter *iter, gpointer data)
{
gboolean is_separator;

	gtk_tree_model_get(model, iter,
		ADVBUD_ISSEPARATOR, &is_separator,
		-1);

	return is_separator;
}

// the budget view creation which run the model creation tool

static GtkWidget *ui_adv_bud_view_new (gpointer user_data)
{
GtkTreeViewColumn *col;
GtkCellRenderer *renderer, *cat_name_renderer;
GtkWidget *view;
adv_bud_data_t *data = user_data;

	view = gtk_tree_view_new();

	/* --- Category column --- */
	col = gtk_tree_view_column_new();
	data->TVC_category = col;

	gtk_tree_view_column_set_title(col, _("Category"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	// Category Name
	cat_name_renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start (col, cat_name_renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, cat_name_renderer, "text", ADVBUD_CATEGORY_NAME);

	g_object_set(cat_name_renderer, "editable", TRUE, NULL);
	g_signal_connect(cat_name_renderer, "edited", G_CALLBACK(ui_adv_bud_cell_update_category), (gpointer) data);

	/* --- Category column with is forced display check --- */
	col = gtk_tree_view_column_new();
	data->TVC_category_with_force = col;

	gtk_tree_view_column_set_title(col, _("Category (check to force display)"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	// Is display forced ?
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start (col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_isdisplayforced, NULL, NULL);

	g_signal_connect (renderer, "toggled", G_CALLBACK(ui_adv_bud_cell_update_isdisplayforced), (gpointer) data);

	// Category Name
	gtk_tree_view_column_pack_start (col, cat_name_renderer, TRUE);
	gtk_tree_view_column_add_attribute(col, cat_name_renderer, "text", ADVBUD_CATEGORY_NAME);

	/* --- Monthly column --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Monthly"));
	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);

	// Monthly toggler
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_issameamount, NULL, NULL);

	g_signal_connect (renderer, "toggled", G_CALLBACK(ui_adv_bud_cell_update_issameamount), (gpointer) data);

	// Monthly amount
	renderer = gtk_cell_renderer_spin_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_amount, GINT_TO_POINTER(ADVBUD_SAMEAMOUNT), NULL);

	g_object_set_data(G_OBJECT(renderer), "ui_adv_bud_column_id", GINT_TO_POINTER(ADVBUD_SAMEAMOUNT));
	g_signal_connect(renderer, "edited", G_CALLBACK(ui_adv_bud_cell_update_amount), (gpointer) data);

	/* --- Each month amount --- */
	for (int i = ADVBUD_JANUARY ; i <= ADVBUD_DECEMBER ; ++i)
	{
		int month = i - ADVBUD_JANUARY ;
		col = gtk_tree_view_column_new();

		gtk_tree_view_column_set_title(col, _(ADVBUD_MONTHS[month]));
		gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
		renderer = gtk_cell_renderer_spin_new();

		gtk_tree_view_column_pack_start(col, renderer, TRUE);
		gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_amount, GINT_TO_POINTER(i), NULL);

		g_object_set_data(G_OBJECT(renderer), "ui_adv_bud_column_id", GINT_TO_POINTER(i));
		g_signal_connect(renderer, "edited", G_CALLBACK(ui_adv_bud_cell_update_amount), (gpointer) data);
	}

	/* --- Year Total -- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _("Total"));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_annualtotal, NULL, NULL);

	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_SINGLE);

	g_object_set(view,
		"enable-grid-lines", PREFS->grid_lines,
		"enable-tree-lines", TRUE,
		NULL);

	return view;
}

/*
 * UI actions
 **/

// Update homebank category on user change
static void ui_adv_bud_cell_update_category(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter filter_iter, iter;
GtkTreeModel *filter, *budget;
Category* category;
guint32 category_key;

	DB(g_print("\n[ui_adv_bud] category name updated with new name '%s'\n", new_text));

	view = data->TV_budget;

	// Read filter data
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter_from_string (filter, &filter_iter, filter_path);

	// Convert data to budget model
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		&filter_iter);

	gtk_tree_model_get (budget, &iter,
		ADVBUD_CATEGORY_KEY, &category_key,
		-1);

	category = da_cat_get (category_key);

	if (! category)
	{
		return;
	}

	// Update category name
	category_rename(category, new_text);

	// Notify of changes
	data->change++;

	// Update budget model

	// Current row
	gtk_tree_store_set(
		GTK_TREE_STORE(budget),
		&iter,
		ADVBUD_CATEGORY_NAME, new_text,
		-1);

	return;
}

// Update amount in budget model and homebank category on user change
static void ui_adv_bud_cell_update_amount(GtkCellRendererText *renderer, gchar *filter_path, gchar *new_text, gpointer user_data)
{
const advbud_store_t column_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(renderer), "ui_adv_bud_column_id"));
adv_bud_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter filter_iter, iter;
GtkTreeModel *filter, *budget;
Category* category;
gdouble amount;
guint32 category_key;

	DB(g_print("\n[ui_adv_bud] amount updated:\n"));

	view = data->TV_budget;

	// Read filter data
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter_from_string (filter, &filter_iter, filter_path);

	// Convert data to budget model
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		&filter_iter);

	gtk_tree_model_get (budget, &iter,
		ADVBUD_CATEGORY_KEY, &category_key,
		-1);

	category = da_cat_get (category_key);

	if (! category)
	{
		return;
	}

	amount = g_strtod(new_text, NULL);

	DB(g_print("\tcolumn: %d (month: %d), category key: %d, amount %.2f\n", column_id, column_id - ADVBUD_JANUARY + 1, category_key, amount));

	// Update Category
	category->budget[column_id - ADVBUD_JANUARY + 1] = amount;

	// Reset Budget Flag
	category->flags &= ~(GF_BUDGET);
	for(gint budget_id = 0; budget_id <=12; ++budget_id)
	{
		if( category->budget[budget_id] != 0.0)
		{
			category->flags |= GF_BUDGET;
			break;
		}
	}

	// Notify of changes
	data->change++;

	// Update budget model

	// Current row
	gtk_tree_store_set(
		GTK_TREE_STORE(budget),
		&iter,
		column_id, amount,
		-1);

	// Refresh total rows
	ui_adv_bud_model_update_monthlytotal (GTK_TREE_STORE(budget));

	return;
}

// Update the row to (dis/enable) same amount for this category
static void ui_adv_bud_cell_update_issameamount(GtkCellRendererText *renderer, gchar *filter_path, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter filter_iter, iter;
GtkTreeModel *filter, *budget;
Category* category;
gboolean issame;
guint32 category_key;

	DB(g_print("\n[ui_adv_bud] Is same amount updated:\n"));

	view = data->TV_budget;

	// Read filter data
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter_from_string (filter, &filter_iter, filter_path);

	// Convert data to budget model
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		&filter_iter);

	gtk_tree_model_get (budget, &iter,
		ADVBUD_CATEGORY_KEY, &category_key,
		ADVBUD_ISSAMEAMOUNT, &issame,
		-1);

	category = da_cat_get (category_key);

	if (! category)
	{
		return;
	}

	// Value has been toggled !
	issame = !(issame);

	DB(g_print("\tcategory key: %d, issame: %d (before: %d)\n", category_key, issame, !(issame)));

	// Update Category

	// Reset Forced Flag
	category->flags &= ~(GF_CUSTOM);

	if (issame == FALSE)
	{
		category->flags |= (GF_CUSTOM);
	}

	// Notify of changes
	data->change++;

	// Update budget model

	// Current row
	gtk_tree_store_set(
		GTK_TREE_STORE(budget),
		&iter,
		ADVBUD_ISSAMEAMOUNT, issame,
		-1);

	// Refresh total rows
	ui_adv_bud_model_update_monthlytotal (GTK_TREE_STORE(budget));

	return;
}

// Update the row to (dis/enable) forced display for this category
static void ui_adv_bud_cell_update_isdisplayforced(GtkCellRendererText *renderer, gchar *filter_path, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter filter_iter, iter;
GtkTreeModel *filter, *budget;
Category* category;
gboolean isdisplayforced;
guint32 category_key;

	DB(g_print("\n[ui_adv_bud] Is display forced updated:\n"));

	view = data->TV_budget;

	// Read filter data
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	gtk_tree_model_get_iter_from_string (filter, &filter_iter, filter_path);

	// Convert data to budget model
	budget = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&iter,
		&filter_iter);

	gtk_tree_model_get (budget, &iter,
		ADVBUD_CATEGORY_KEY, &category_key,
		ADVBUD_ISDISPLAYFORCED, &isdisplayforced,
		-1);

	category = da_cat_get (category_key);

	if (! category)
	{
		return;
	}

	// Value has been toggled !
	isdisplayforced = !(isdisplayforced);

	DB(g_print("\tcategory key: %d, isdisplayforced: %d (before: %d)\n", category_key, isdisplayforced, !(isdisplayforced)));

	// Update Category

	// Reset Forced Flag
	category->flags &= ~(GF_FORCED);

	if (isdisplayforced == TRUE)
	{
		category->flags |= (GF_FORCED);
	}

	// Notify of changes
	data->change++;

	// Update budget model

	// Current row
	gtk_tree_store_set(
		GTK_TREE_STORE(budget),
		&iter,
		ADVBUD_ISDISPLAYFORCED, isdisplayforced,
		-1);

	// Refresh total rows
	ui_adv_bud_model_update_monthlytotal (GTK_TREE_STORE(budget));

	return;
}

// Update budget view and model according to the new view mode selected
static void ui_adv_bud_view_update_mode (GtkToggleButton *button, gpointer user_data)
{
adv_bud_data_t *data = user_data;
advbud_view_mode_t view_mode = ADVBUD_VIEW_BALANCE;

	// Only run once the view update, so only run on the activated button signal
	if(!gtk_toggle_button_get_active(button))
	{
		return;
	}

	// Mode is directly set by radio button, because the ADVBUD_VIEW_MODE and enum
	// for view mode are constructed to correspond (manually)
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));

	DB(g_print("\n[ui_adv_bud] view mode toggled to: %d\n", view_mode));

	ui_adv_bud_view_toggle((gpointer) data, view_mode);

	return;
}

// Expand all categories inside the current view
static void ui_adv_bud_view_expand (GtkButton *button, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;

	view = data->TV_budget;

	data->TV_isexpanded = TRUE;

	gtk_tree_view_expand_all(GTK_TREE_VIEW(view));

	return;
}

// Collapse all categories inside the current view
static void ui_adv_bud_view_collapse (GtkButton *button, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;

	view = data->TV_budget;

	data->TV_isexpanded = FALSE;

	ui_adv_bud_model_collapse (GTK_TREE_VIEW(view));

	return;
}

// Add a category according to the current selection
static void ui_adv_bud_category_add (GtkButton *button, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;
advbud_view_mode_t view_mode;
GtkTreeModel *filter, *sort, *budget, *categories;
GtkTreeSelection *selection;
GtkTreeIter filter_iter, sort_iter, iter, categories_iter;
GtkWidget *dialog, *content_area, *grid, *combobox, *textentry, *widget;
GtkCellRenderer *renderer;
gint gridrow, response;

	view = data->TV_budget;
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));

	// Read filter
	filter = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(view));
	gtk_tree_selection_get_selected(selection, &filter, &filter_iter);

	// Read sort
	sort = gtk_tree_model_filter_get_model(GTK_TREE_MODEL_FILTER(filter));
	gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(filter),
		&sort_iter,
		&filter_iter);

	// Convert data to budget model
	budget = gtk_tree_model_sort_get_model(GTK_TREE_MODEL_SORT(sort));
	gtk_tree_model_sort_convert_iter_to_child_iter(GTK_TREE_MODEL_SORT(sort),
		&iter,
		&sort_iter);

	// Selectable categories from original model
	categories = gtk_tree_model_filter_new(budget, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(categories),
		ui_adv_bud_model_row_filter_parents, data, NULL);

	gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(categories),
		&categories_iter,
		&iter);

	DB( g_print("[ui_adv_bud] open sub-dialog to add a category\n") );

	dialog = gtk_dialog_new_with_buttons (_("Add a category"),
		GTK_WINDOW(data->dialog),
		GTK_DIALOG_MODAL,
		_("_Cancel"),
		GTK_RESPONSE_CANCEL,
		_("_Apply"),
		GTK_RESPONSE_APPLY,
		NULL);

	//window contents
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	// design content
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	g_object_set(grid, "margin", SPACING_MEDIUM, NULL);
	gtk_container_add(GTK_CONTAINER(content_area), grid);

	// First row display parent selector
	gridrow = 0;

	widget = gtk_label_new(_("Parent category"));
	gtk_grid_attach (GTK_GRID (grid), widget, 0, gridrow, 1, 1);

	combobox = gtk_combo_box_new_with_model(categories);
	gtk_grid_attach (GTK_GRID (grid), combobox, 1, gridrow, 1, 1);

	gtk_combo_box_set_row_separator_func(
		GTK_COMBO_BOX(combobox),
		ui_adv_view_separator,
		data,
		NULL
	);

	gtk_combo_box_set_id_column(GTK_COMBO_BOX(combobox), ADVBUD_CATEGORY_KEY);
	// TODO First retrieve first ancestor of iter and then select it
	gtk_combo_box_set_active_iter(GTK_COMBO_BOX(combobox), &categories_iter);

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start (GTK_CELL_LAYOUT(combobox), renderer, TRUE);
	gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combobox), renderer, "text", ADVBUD_CATEGORY_NAME);

	// Next row displays the new category entry
	gridrow++;

	widget = gtk_label_new(_("Category name"));
	gtk_grid_attach (GTK_GRID (grid), widget, 0, gridrow, 1, 1);

	textentry = gtk_entry_new();
	gtk_grid_attach (GTK_GRID (grid), textentry, 1, gridrow, 1, 1);

	gtk_widget_show_all (dialog);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	if (response == GTK_RESPONSE_APPLY
			&& gtk_combo_box_get_active_iter(GTK_COMBO_BOX(combobox), &categories_iter)) {
	Category *new_item;
	const gchar *new_name;
	gchar *parent_name;
	guint32 parent_key;
	advbud_cat_type_t parent_type;
	gboolean parent_is_separator;
	advbud_budget_iterator_t *budget_iter;
	GtkTreeIter *root_iter;

		DB( g_print("[ui_adv_bud] applying creation of a new category\n") );

		// Retrieve info from dialog

		gtk_tree_model_filter_convert_iter_to_child_iter(GTK_TREE_MODEL_FILTER(categories),
			&iter,
			&categories_iter);

		gtk_tree_model_get (budget, &iter,
			ADVBUD_CATEGORY_NAME, &parent_name,
			ADVBUD_CATEGORY_KEY, &parent_key,
			ADVBUD_CATEGORY_TYPE, &parent_type,
			ADVBUD_ISSEPARATOR, &parent_is_separator,
			-1);

		DB( g_print(" -> from parent cat: %s (key: %d, type: %d)\n",
								parent_name, parent_key ,parent_type) );

		// Retrieve required root
		budget_iter = g_malloc0(sizeof(advbud_budget_iterator_t));
		budget_iter->iterator = g_malloc0(sizeof(GtkTreeIter));
		budget_iter->category_isroot = TRUE;
		budget_iter->category_istotal = FALSE;
		budget_iter->category_type = parent_type;
		gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
			(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
			budget_iter);

		if (budget_iter->iterator == NULL) {
			DB(g_print(" -> error: not found good tree root !\n"));
			return;
		}

		root_iter = budget_iter->iterator;

		new_name = gtk_entry_get_text(GTK_ENTRY(textentry));

		/* ignore if item is empty */
		if (new_name && *new_name)
		{
			data->change++;

			new_item = da_cat_malloc();

			new_item->name = g_strdup(new_name);
			g_strstrip(new_item->name);

			if (strlen(new_item->name) > 0)
			{
				new_item->parent = parent_key;

				if (parent_key)
				{
					new_item->flags |= GF_SUB;
				}

				if (parent_type == ADVBUD_CAT_TYPE_INCOME)
				{
					new_item->flags |= GF_INCOME;
				}

				// On balance mode, enable forced display too to render it to user
				if (view_mode == ADVBUD_VIEW_BALANCE)
				{
					new_item->flags |= GF_FORCED;
				}

				if(da_cat_append(new_item))
				{
				GtkTreePath *path;
				advbud_category_iterator_t *new_item_iter;

					DB( g_print(" => add cat: %p (%d), type=%d\n", new_item->name, new_item->key, category_type_get(new_item)) );

					// Finally add it to model
					ui_adv_bud_model_add_category_with_lineage (GTK_TREE_STORE(budget), root_iter, &(new_item->key));

					// Expand view up to the newly added
					new_item_iter = g_malloc0(sizeof(advbud_category_iterator_t));
					new_item_iter->key = new_item->key;

					if (new_item_iter->iterator != NULL)
					{
						gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
							(GtkTreeModelForeachFunc) ui_adv_bud_model_get_category_iterator,
							new_item_iter);

						gtk_tree_model_sort_convert_child_iter_to_iter(GTK_TREE_MODEL_SORT(sort),
							&sort_iter,
							&iter);

						if(gtk_tree_model_filter_convert_child_iter_to_iter(GTK_TREE_MODEL_FILTER(filter),
							&filter_iter,
							&sort_iter)
						)
						{
							path = gtk_tree_model_get_path(filter, &filter_iter);
							gtk_tree_view_expand_row(GTK_TREE_VIEW(view), path, TRUE);
						}
					}
					g_free(new_item_iter);
				}
			}
			else
			{
				da_cat_free(new_item);
				g_free(budget_iter->iterator);
				g_free(budget_iter);
			}
		}

	}

	gtk_widget_destroy(dialog);

	return;
}

static void ui_adv_bud_dialog_close(adv_bud_data_t *data, gint response)
{
	DB( g_print("[ui_adv_bud] dialog close\n") );

	GLOBALS->changes_count += data->change;

	return;
}

// Open / create the main dialog, the budget view and the budget model
GtkWidget *ui_adv_bud_manage_dialog(void)
{
adv_bud_data_t *data;
GtkWidget *dialog, *content_area, *grid;
GtkWidget *radiomode;
GtkWidget *widget;
GtkWidget *vbox, *hbox;
GtkWidget *scrolledwindow, *treeview;
GtkWidget *toolbar;
GtkToolItem *toolitem;
GtkTreeModel *model, *sort, *filter;
gint response;
gint gridrow, w, h;

	data = g_malloc0(sizeof(adv_bud_data_t));
	data->change = 0;
	if(!data) return NULL;

	DB( g_print("\n[ui_adv_bud] open dialog\n") );

	// create window
	dialog = gtk_dialog_new_with_buttons (_("Advanced Budget Management"),
		GTK_WINDOW(GLOBALS->mainwindow),
		GTK_DIALOG_MODAL,
		_("_Close"),
		GTK_RESPONSE_ACCEPT,
		NULL);

	data->dialog = dialog;

	gtk_window_set_icon_name(GTK_WINDOW (dialog), ICONNAME_HB_BUDGET);

	//window contents
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog)); // return a vbox

	// store data inside dialog property to retrieve them easily in callbacks
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)&data);
	DB( g_print(" - new dialog=%p, inst_data=%p\n", dialog, data) );

	// set a nice default dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	gtk_window_resize (GTK_WINDOW(data->dialog), w * 0.9, h * 0.9);

	// design content
	grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	gtk_grid_set_column_spacing (GTK_GRID (grid), SPACING_MEDIUM);
	g_object_set(grid, "margin", SPACING_MEDIUM, NULL);
	gtk_container_add(GTK_CONTAINER(content_area), grid);

	// First row displays radio button to change mode (edition / view) and menu
	gridrow = 0;

	// edition mode radio buttons
	//
	radiomode = make_radio(ADVBUD_VIEW_MODE, TRUE, GTK_ORIENTATION_HORIZONTAL);
	data->RA_mode = radiomode;
	gtk_widget_set_halign (radiomode, GTK_ALIGN_CENTER);
	gtk_grid_attach (GTK_GRID (grid), radiomode, 0, gridrow, 1, 1);

	// Next row displays the budget tree with its toolbar
	gridrow++;

	// We use a Vertical Box to link tree with toolbar
	vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	gtk_widget_set_margin_right(vbox, SPACING_SMALL);
	gtk_grid_attach (GTK_GRID (grid), vbox, 0, gridrow, 1, 1);

	// Scrolled Window will permit to display budgets with a lot of active categories
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_hexpand (scrolledwindow, TRUE);
	gtk_widget_set_vexpand (scrolledwindow, TRUE);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_box_pack_start (GTK_BOX (vbox), scrolledwindow, TRUE, TRUE, 0);

	treeview = ui_adv_bud_view_new ((gpointer) data);
	data->TV_budget = treeview;
	gtk_container_add(GTK_CONTAINER(scrolledwindow), treeview);

	// Toolbar to add, remove categories, expand and collapse categorie
	toolbar = gtk_toolbar_new();
	gtk_toolbar_set_icon_size (GTK_TOOLBAR(toolbar), GTK_ICON_SIZE_MENU);
	gtk_toolbar_set_style(GTK_TOOLBAR(toolbar), GTK_TOOLBAR_ICONS);
	gtk_box_pack_start (GTK_BOX (vbox), toolbar, FALSE, FALSE, 0);

	gtk_style_context_add_class (gtk_widget_get_style_context (toolbar), GTK_STYLE_CLASS_INLINE_TOOLBAR);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

	toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), hbox);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem), -1);

	widget = make_image_button(ICONNAME_LIST_ADD, _("Add category"));
	data->BT_category_add = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	widget = make_image_button(ICONNAME_LIST_REMOVE, _("Remove category"));
	data->BT_category_delete = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	// Separator for add / remove category and collapse / expand buttons
	toolitem = gtk_separator_tool_item_new ();
	gtk_tool_item_set_expand (toolitem, TRUE);
	gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(toolitem), FALSE);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem), -1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);

	toolitem = gtk_tool_item_new();
	gtk_container_add (GTK_CONTAINER(toolitem), hbox);
	gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(toolitem), -1);

	widget = make_image_button(ICONNAME_HB_BUTTON_EXPAND, _("Expand all"));
	data->BT_expand = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	widget = make_image_button(ICONNAME_HB_BUTTON_COLLAPSE, _("Collapse all"));
	data->BT_collapse = widget;
	gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

	/* signal connect */

	// connect every radio button to the toggled signal to correctly update the view
	for (int i=0; ADVBUD_VIEW_MODE[i] != NULL; i++)
	{
		widget = radio_get_nth_widget (GTK_CONTAINER(radiomode), i);

		if (widget)
		{
			g_signal_connect (widget, "toggled", G_CALLBACK (ui_adv_bud_view_update_mode), (gpointer)data);
		}
	}

	// toolbar buttons
	g_signal_connect (data->BT_category_add, "clicked", G_CALLBACK(ui_adv_bud_category_add), (gpointer)data);
	g_signal_connect (data->BT_expand, "clicked", G_CALLBACK (ui_adv_bud_view_expand), (gpointer)data);
	g_signal_connect (data->BT_collapse, "clicked", G_CALLBACK (ui_adv_bud_view_collapse), (gpointer)data);

	// dialog
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);

	// tree model to map HomeBank categories to the tree view
	model = ui_adv_bud_model_new();

	sort = gtk_tree_model_sort_new_with_model(model);
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE (sort),
		ADVBUD_CATEGORY_NAME, ui_adv_bud_model_row_sort,
		data, NULL);
	gtk_tree_sortable_set_sort_column_id (GTK_TREE_SORTABLE (sort),
		ADVBUD_CATEGORY_NAME, GTK_SORT_ASCENDING);

	filter = gtk_tree_model_filter_new(sort, NULL);
	gtk_tree_model_filter_set_visible_func(GTK_TREE_MODEL_FILTER(filter), ui_adv_bud_model_row_filter, data, NULL);
	gtk_tree_view_set_model(GTK_TREE_VIEW(treeview), filter);

	g_object_unref(model); // Remove model with sort ?
	g_object_unref(sort); // Remove sort with filter ?
	g_object_unref(filter); // Remove filter with view

	// By default, show the balance mode with all categories expanded
	data->TV_isexpanded = TRUE;
	ui_adv_bud_view_toggle((gpointer) data, ADVBUD_VIEW_BALANCE);

	gtk_widget_show_all (dialog);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	ui_adv_bud_dialog_close(data, response);
	gtk_widget_destroy (dialog);

	g_free(data);

	return NULL;
}

