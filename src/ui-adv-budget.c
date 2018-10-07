/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 2018 Adrien Dorsaz <adrien@adorsaz.ch>
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

/* These values has to correspond to ADVBUD_VIEW_MODE */
enum advbud_view_mode {
	ADVBUD_VIEW_BALANCE = 0,
	ADVBUD_VIEW_INCOME,
	ADVBUD_VIEW_EXPENSE
};
typedef enum advbud_view_mode advbud_view_mode_t;

/* These values corresponds to the GF_INCOME flag from hb-category */
enum advbud_cat_type {
	// Not real category type: used to retrieve tree roots
	ADVBUD_CAT_TYPE_NONE = -1,
	// Currently defined types of Category
	ADVBUD_CAT_TYPE_EXPENSE = 0,
	ADVBUD_CAT_TYPE_INCOME = GF_INCOME
};
typedef enum advbud_cat_type advbud_cat_type_t;

/* enum for the Budget Tree Store model */
enum advbud_store {
	ADVBUD_CATEGORY_KEY = 0,
	ADVBUD_CATEGORY_NAME,
	ADVBUD_CATEGORY_TYPE,
	ADVBUD_ISROOT, // To retrieve easier the 3 tree roots
	ADVBUD_ISTOTAL, // To retrieve rows inside the Total root
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
static void ui_adv_bud_model_insert_roots(GtkTreeStore* budget, advbud_view_mode_t view_mode);
static void ui_adv_bud_model_update_monthlytotal(GtkTreeStore* budget, advbud_view_mode_t view_mode);
static GtkTreeModel * ui_adv_bud_model_new (advbud_view_mode_t view_mode);

// GtkTreeView widget
static void ui_adv_bud_view_display_amount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_display_issameamount (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_display_isdisplayforced (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_display_annualtotal (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data);
static void ui_adv_bud_view_toggle (gpointer user_data, advbud_view_mode_t view_mode);
static GtkWidget *ui_adv_bud_view_new (gpointer user_data);

// UI actions
static void ui_adv_bud_cell_update_amount(GtkCellRendererText *renderer, gchar *path_string, gchar *new_text, gpointer user_data);
static void ui_adv_bud_cell_update_issameamount(GtkCellRendererText *renderer, gchar *path_string, gpointer user_data);
static void ui_adv_bud_cell_update_isdisplayforced(GtkCellRendererText *renderer, gchar *path_string, gpointer user_data);
static void ui_adv_bud_view_update_mode (GtkToggleButton *button, gpointer user_data);
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

	DB(g_print("insert new category %d\n", bdg_category->key));

	gtk_tree_store_set(
		budget,
		&child,
		ADVBUD_CATEGORY_KEY, bdg_category->key,
		ADVBUD_CATEGORY_NAME, bdg_category->name,
		ADVBUD_CATEGORY_TYPE, category_type_get ((bdg_category)),
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
gint category_type;
gboolean result = FALSE, is_root, is_total;

	gtk_tree_model_get (model, iter,
		ADVBUD_CATEGORY_TYPE, &category_type,
		ADVBUD_ISROOT, &is_root,
		ADVBUD_ISTOTAL, &is_total,
		-1);

	budget_iter->iterator = NULL;

	if (is_root == budget_iter->category_isroot
		&& is_total == budget_iter->category_istotal
		&& category_type == budget_iter->category_type
	)
	{
		budget_iter->iterator = g_malloc0(sizeof(GtkTreeIter));
		*budget_iter->iterator = *iter;

		result = TRUE;
	}

	return result;
}

// Create tree roots for the store
static void ui_adv_bud_model_insert_roots(GtkTreeStore* budget, advbud_view_mode_t view_mode)
{
GtkTreeIter iter;
	if (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_INCOME)
	{
		gtk_tree_store_insert_with_values (
			budget,
			&iter,
			NULL,
			-1,
			ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_INCOME]),
			ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_INCOME,
			ADVBUD_ISROOT, TRUE,
			ADVBUD_ISTOTAL, FALSE,
			-1);
	}

	if (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_EXPENSE)
	{
		gtk_tree_store_insert_with_values (
			budget,
			&iter,
			NULL,
			-1,
			ADVBUD_CATEGORY_NAME, _(ADVBUD_VIEW_MODE[ADVBUD_VIEW_EXPENSE]),
			ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_EXPENSE,
			ADVBUD_ISROOT, TRUE,
			ADVBUD_ISTOTAL, FALSE,
			-1);
	}

	gtk_tree_store_insert_with_values (
		budget,
		&iter,
		NULL,
		-1,
		ADVBUD_CATEGORY_NAME, _(N_("Totals")),
		ADVBUD_CATEGORY_TYPE, ADVBUD_CAT_TYPE_NONE,
		ADVBUD_ISROOT, TRUE,
		ADVBUD_ISTOTAL, FALSE,
		-1);

	return;
}

// Update (or insert) total rows for a budget according to the view  mode
// This function will is used to initiate model and to refresh it after change by user
static void ui_adv_bud_model_update_monthlytotal(GtkTreeStore* budget, advbud_view_mode_t view_mode)
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

	if (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_INCOME)
	{
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
	}

	if (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_EXPENSE)
	{
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
	}

	if (view_mode == ADVBUD_VIEW_BALANCE)
	{
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
	}

	g_free(budget_iter->iterator);
	g_free(budget_iter);

	return;
}

// the budget model creation
static GtkTreeModel * ui_adv_bud_model_new (advbud_view_mode_t view_mode)
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
	ui_adv_bud_model_insert_roots (budget, view_mode);

	// Retrieve required root
	budget_iter = g_malloc0(sizeof(advbud_budget_iterator_t));
	budget_iter->iterator = g_malloc0(sizeof(GtkTreeIter));
	budget_iter->category_isroot = TRUE;
	budget_iter->category_istotal = FALSE;

	if (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_INCOME)
	{
		budget_iter->category_type = ADVBUD_CAT_TYPE_INCOME;

		gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
			(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
			budget_iter);

		iter_income = budget_iter->iterator;
	}

	if (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_EXPENSE)
	{
		budget_iter->category_type = ADVBUD_CAT_TYPE_EXPENSE;

		gtk_tree_model_foreach(GTK_TREE_MODEL(budget),
			(GtkTreeModelForeachFunc) ui_adv_bud_model_get_budget_iterator,
			budget_iter);

		iter_expense = budget_iter->iterator;
	}

	/* Create rows for real categories */
	n_category = da_cat_get_max_key();

	for(guint32 i=1; i<=n_category; ++i)
	{
	Category *bdg_category;
	gboolean cat_is_displayed;
	gboolean cat_is_sameamount;
	gboolean cat_is_income;

		bdg_category = da_cat_get(i);

		if (bdg_category == NULL)
		{
			continue;
		}

		cat_is_displayed = (bdg_category->flags & (GF_BUDGET|GF_FORCED));
		cat_is_income = (category_type_get (bdg_category) == 1);
		cat_is_sameamount = (! (bdg_category->flags & GF_CUSTOM));

		/* Display category only if forced or if a budget has been defined. */
		if ( view_mode == ADVBUD_VIEW_BALANCE
			&& !cat_is_displayed)
		{
			continue;
		}

		DB(g_print(" category %d:'%s' isincome=%d, issub=%d hasbudget=%d issameamount=%d parent=%d\n",
			bdg_category->key, bdg_category->name,
			cat_is_income, (bdg_category->flags & GF_SUB),
			(bdg_category->flags & GF_BUDGET), cat_is_sameamount, bdg_category->parent));

		// Compute totals and initiate category in right tree root
		if (cat_is_income
				&& (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_INCOME)
		)
		{
			ui_adv_bud_model_add_category_with_lineage(budget, iter_income, &(bdg_category->key));
		}
		else if (!cat_is_income
						 && (view_mode == ADVBUD_VIEW_BALANCE || view_mode == ADVBUD_VIEW_EXPENSE)
		)
		{
			ui_adv_bud_model_add_category_with_lineage(budget, iter_expense, &(bdg_category->key));
		}
	}


	/* Create rows for total root */
	ui_adv_bud_model_update_monthlytotal(budget, view_mode);

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
gboolean row_category_type;
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
// - compute again the model to add required rows
// - update the view columns to show only the required ones
static void ui_adv_bud_view_toggle (gpointer user_data, advbud_view_mode_t view_mode)
{
adv_bud_data_t *data = user_data;
GtkWidget *budget, *scrolledwindow;
GtkTreeModel *model;
gint w, h;

	budget = data->TV_budget;

	// Replace model with the new one
	model = ui_adv_bud_model_new(view_mode);
	gtk_tree_view_set_model(GTK_TREE_VIEW(budget), model);
	gtk_tree_view_expand_all(GTK_TREE_VIEW(budget));

	// Resize the window to get natural width for the dialog and keep the current height
	scrolledwindow = gtk_widget_get_parent(GTK_WIDGET(budget));
	g_object_ref(budget); // Add temporary a manual ref to keep the view alive
	gtk_container_remove(GTK_CONTAINER(scrolledwindow), budget);
	gtk_window_get_size(GTK_WINDOW(data->dialog), &w, &h);
	gtk_window_resize (GTK_WINDOW(data->dialog), 1, h);
	gtk_container_add(GTK_CONTAINER(scrolledwindow), budget);
	g_object_unref(budget);

	/* to automatically destroy then model with view */
	g_object_unref(model);

	DB(g_print("[ui_adv_bud] : button state changed to: %d\n", view_mode));

	// For each view mode, apply specific modifications of the budget view
	switch(view_mode) {
		case ADVBUD_VIEW_BALANCE:
			gtk_tree_view_column_set_visible(data->TVC_isdisplayforced, FALSE);
			break;
		case ADVBUD_VIEW_INCOME:
			gtk_tree_view_column_set_visible(data->TVC_isdisplayforced, TRUE);
			break;
		case ADVBUD_VIEW_EXPENSE:
			gtk_tree_view_column_set_visible(data->TVC_isdisplayforced, TRUE);
			break;
	}

	return;
}

// the budget view creation which run the model creation tool

static GtkWidget *ui_adv_bud_view_new (gpointer user_data)
{
GtkTreeViewColumn *col;
GtkCellRenderer *renderer;
GtkWidget *view;
adv_bud_data_t *data = user_data;

	view = gtk_tree_view_new();

	/* --- Category --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Category")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_text_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);

	gtk_tree_view_column_add_attribute(col, renderer, "text", ADVBUD_CATEGORY_NAME);

	/* --- Display forced ? ---*/
	col = gtk_tree_view_column_new();
	data->TVC_isdisplayforced = col;
	gtk_tree_view_column_set_title(col, _(N_("Force display")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_isdisplayforced, NULL, NULL);

	g_signal_connect (renderer, "toggled", ui_adv_bud_cell_update_isdisplayforced, (gpointer) data);

	/* --- Is same amount each month ? --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Same amount")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_toggle_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_issameamount, NULL, NULL);

	g_signal_connect (renderer, "toggled", ui_adv_bud_cell_update_issameamount, (gpointer) data);

	/* --- Monthly --- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Monthly")));

	gtk_tree_view_append_column(GTK_TREE_VIEW(view), col);
	renderer = gtk_cell_renderer_spin_new();
	gtk_tree_view_column_pack_start(col, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(col, renderer, ui_adv_bud_view_display_amount, GINT_TO_POINTER(ADVBUD_SAMEAMOUNT), NULL);

	g_object_set_data(G_OBJECT(renderer), "ui_adv_bud_column_id", GINT_TO_POINTER(ADVBUD_SAMEAMOUNT));
	g_signal_connect(renderer, "edited", ui_adv_bud_cell_update_amount, (gpointer) data);

	/* --- Each month --- */
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
		g_signal_connect(renderer, "edited", ui_adv_bud_cell_update_amount, (gpointer) data);
	}

	/* --- Year Total -- */
	col = gtk_tree_view_column_new();
	gtk_tree_view_column_set_title(col, _(N_("Total")));

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

// Update amount in budget model and homebank category on user change
static void ui_adv_bud_cell_update_amount(GtkCellRendererText *renderer, gchar *path_string, gchar *new_text, gpointer user_data)
{
const advbud_store_t column_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(renderer), "ui_adv_bud_column_id"));
adv_bud_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter iter;
GtkTreeModel *budget;
Category* category;
gdouble amount;
guint32 category_key;
advbud_view_mode_t view_mode = ADVBUD_VIEW_BALANCE;

	DB(g_print("\n[ui_adv_bud] amount updated:\n"));

	view = data->TV_budget;
	budget = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));

	gtk_tree_model_get_iter_from_string (budget, &iter, path_string);

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
	ui_adv_bud_model_update_monthlytotal (GTK_TREE_STORE(budget), view_mode);

	return;
}

// Update the row to (dis/enable) same amount for this category
static void ui_adv_bud_cell_update_issameamount(GtkCellRendererText *renderer, gchar *path_string, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter iter;
GtkTreeModel *budget;
Category* category;
gboolean issame;
guint32 category_key;
advbud_view_mode_t view_mode = ADVBUD_VIEW_BALANCE;

	DB(g_print("\n[ui_adv_bud] Is same amount updated:\n"));

	view = data->TV_budget;
	budget = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));

	gtk_tree_model_get_iter_from_string (budget, &iter, path_string);

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
	ui_adv_bud_model_update_monthlytotal (GTK_TREE_STORE(budget), view_mode);

	return;
}

// Update the row to (dis/enable) forced display for this category
static void ui_adv_bud_cell_update_isdisplayforced(GtkCellRendererText *renderer, gchar *path_string, gpointer user_data)
{
adv_bud_data_t *data = user_data;
GtkWidget *view;
GtkTreeIter iter;
GtkTreeModel *budget;
Category* category;
gboolean isdisplayforced;
guint32 category_key;
advbud_view_mode_t view_mode = ADVBUD_VIEW_BALANCE;

	DB(g_print("\n[ui_adv_bud] Is display forced updated:\n"));

	view = data->TV_budget;
	budget = gtk_tree_view_get_model (GTK_TREE_VIEW(view));
	view_mode = radio_get_active(GTK_CONTAINER(data->RA_mode));

	gtk_tree_model_get_iter_from_string (budget, &iter, path_string);

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
	ui_adv_bud_model_update_monthlytotal (GTK_TREE_STORE(budget), view_mode);

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
GtkWidget *scrolledwindow, *treeview;
gint response;
gint gridrow, w, h;

	data = g_malloc0(sizeof(adv_bud_data_t));
	data->change = 0;
	if(!data) return NULL;

	DB( g_print("\n[ui_adv_bud] open dialog\n") );

	// create window
	dialog = gtk_dialog_new_with_buttons (_("Advanced Budget Management"),
		GTK_WINDOW(GLOBALS->mainwindow),
		0,
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

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	gtk_window_set_default_size (GTK_WINDOW(dialog), -1, h * 0.8);

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

	// connect every radio button to the toggled signal to correctly update the view
	for (int i=0; ADVBUD_VIEW_MODE[i] != NULL; i++)
	{
		widget = radio_get_nth_widget (GTK_CONTAINER(radiomode), i);

		if (widget)
		{
			g_signal_connect (widget, "toggled", G_CALLBACK (ui_adv_bud_view_update_mode), (gpointer)data);
		}
	}

	// Next row displays the budget tree
	gridrow++;

	// Scrolled Window will permit to display budgets with a lot of active categories
	scrolledwindow = gtk_scrolled_window_new(NULL, NULL);
	gtk_widget_set_hexpand (scrolledwindow, TRUE);
	gtk_widget_set_vexpand (scrolledwindow, TRUE);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (scrolledwindow), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	gtk_grid_attach (GTK_GRID (grid), scrolledwindow, 0, gridrow, 1, 1);

	treeview = ui_adv_bud_view_new ((gpointer) data);
	data->TV_budget = treeview;
	gtk_container_add(GTK_CONTAINER(scrolledwindow), treeview);

	// By default, show the reader mode
	ui_adv_bud_view_toggle((gpointer) data, ADVBUD_VIEW_BALANCE);

	/* signal connect */
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);

	gtk_widget_show_all (dialog);

	response = gtk_dialog_run (GTK_DIALOG (dialog));

	ui_adv_bud_dialog_close(data, response);
	gtk_widget_destroy (dialog);

	return NULL;
}

