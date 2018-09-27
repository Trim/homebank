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

#ifndef __HOMEBANK_REPBUDGETBALANCE_H__
#define __HOMEBANK_REPBUDGETBALANCE_H__

struct repbudgetbalance_data
{
	GtkWidget *window;

	// Tree view with budget
	GtkWidget *TV_budget;

	// Tree columns to dynamically display
	GtkTreeViewColumn *TVC_isdisplayforced;

	// Radio buttons of view mode
	GtkWidget *RA_mode;

	GtkUIManager *ui;
};
typedef struct repbudgetbalance_data repbudgetbalance_data_t;

GtkWidget *repbudgetbalance_window_new(void);


#endif
