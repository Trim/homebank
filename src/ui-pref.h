/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2014 Maxime DOYEN
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

#ifndef __HB_PREFERENCE_GTK_H__
#define __HB_PREFERENCE_GTK_H__


struct defpref_data
{
	GtkWidget	*window;

	GtkWidget	*LV_page;
	GtkWidget	*GR_page;

	GtkWidget	*label;
	GtkWidget	*image;
	GtkWidget   *BT_clear;

	GtkWidget	*CY_language;
	GtkWidget	*CY_toolbar;
	GtkWidget	*CY_colors;
	GtkWidget	*CM_custom_colors;
	GtkWidget	*CP_exp_color;
	GtkWidget	*CP_inc_color;
	GtkWidget	*CP_warn_color;
	GtkWidget	*CM_ruleshint;

	GtkWidget	*LV_opecolumns;
	GtkWidget	*BT_go_up;
	GtkWidget	*BT_go_down;

	GtkWidget	*CM_runwizard;

	GtkWidget	*ST_path_hbfile, *BT_path_hbfile;
	GtkWidget	*ST_path_import, *BT_path_import;
	GtkWidget	*ST_path_export, *BT_path_export;

	GtkWidget	*CM_load_last;
	GtkWidget	*CM_show_splash;
	GtkWidget	*CM_append_scheduled;
	GtkWidget	*CM_herit_date;
	GtkWidget	*CM_hide_reconciled;

	//GtkWidget	*ST_path_navigator;

	GtkWidget	*ST_datefmt;
	GtkWidget	*LB_date;

	GtkWidget	*ST_num_presymbol;
	GtkWidget	*ST_num_sufsymbol;
	GtkWidget	*ST_num_decimalchar;	
	GtkWidget	*ST_num_groupingchar;	
	GtkWidget	*NB_num_fracdigits;
	GtkWidget	*LB_numberbase;

	//GtkWidget	*NB_numnbdec;
	//GtkWidget	*CM_numseparator;
	GtkWidget	*CM_imperial;

	GtkWidget	*CY_daterange_wal;
	GtkWidget	*CY_daterange_txn;
	GtkWidget	*CY_daterange_rep;
	
	/* currencies 
	GtkWidget	*LB_default;
	GtkWidget	*BT_default; */
	
	GtkWidget	*CM_euro_enable;
	GtkWidget	*CY_euro_preset;
	GtkWidget	*ST_euro_country;
	GtkWidget	*NB_euro_value;

	GtkWidget	*ST_euro_presymbol;
	GtkWidget	*ST_euro_sufsymbol;
	GtkWidget	*ST_euro_decimalchar;	
	GtkWidget	*ST_euro_groupingchar;	
	GtkWidget	*NB_euro_fracdigits;
	GtkWidget	*LB_numbereuro;

	//GtkWidget	*ST_euro_symbol;
	//GtkWidget	*NB_euro_nbdec;
	//GtkWidget	*CM_euro_thsep;

	GtkWidget	*CM_stat_byamount;
	GtkWidget	*CM_stat_showdetail;
	GtkWidget	*CM_stat_showrate;

	GtkWidget	*CM_budg_showdetail;

	GtkWidget	*CY_color_scheme;

	GtkWidget	*CM_chartlegend;

	GtkWidget	*CY_dtex_datefmt;
	GtkWidget	*CY_dtex_ofxmemo;

};


void free_pref_icons(void);
void load_pref_icons(void);

GtkWidget *defpref_dialog_new (void);

#endif
