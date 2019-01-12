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

#ifndef __HOMEBANK_ADVBUDGET_H__
#define __HOMEBANK_ADVBUDGET_H__

struct adv_bud_data
{
	GtkWidget *dialog;

	// Number of changes to notify globally
	gint change;

	// Scrolled Window with the tree view
	GtkWidget * SW_treeview;

	// Tree view with budget
	GtkWidget *TV_budget;

	// Radio buttons of view mode
	GtkWidget *RA_mode;

  // Tool bar
  GtkWidget *BT_category_add, *BT_category_delete, *BT_expand, *BT_collapse;

  // Should the tree be collapsed
  gboolean TV_isexpanded;

	GtkUIManager *ui;
};
typedef struct adv_bud_data adv_bud_data_t;

GtkWidget *ui_adv_bud_manage_dialog(void);


#endif
