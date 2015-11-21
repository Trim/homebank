/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2015 Maxime DOYEN
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

#include "ui-archive.h"
#include "ui-account.h"
#include "ui-category.h"
#include "ui-payee.h"

#include "gtk-dateentry.h"

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



gchar *CYA_UNIT[] = { N_("Day"), N_("Week"), N_("Month"), N_("Year"), NULL };

gchar *CYA_SCHED_WEEKEND[] = { N_("Possible"), N_("Before"), N_("After"), NULL };

extern gchar *CYA_TXN_STATUS[];

static GtkWidget *ui_arc_listview_new(void);




/*
**
** The function should return:
** a negative integer if the first value comes before the second,
** 0 if they are equal,
** or a positive integer if the first value comes after the second.
*/
static gint ui_arc_listview_compare_func (GtkTreeModel *model, GtkTreeIter *a, GtkTreeIter *b, gpointer userdata)
{
Archive *entry1, *entry2;

	gtk_tree_model_get(model, a, LST_DEFARC_DATAS, &entry1, -1);
	gtk_tree_model_get(model, b, LST_DEFARC_DATAS, &entry2, -1);

    return hb_string_utf8_compare(entry1->wording, entry2->wording);
}


/*
**
*/
static void ui_arc_listview_auto_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
Archive *item;
gchar *iconname = NULL;

	// get the transaction
	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &item, -1);

	iconname = ( item->flags & OF_AUTO ) ? ICONNAME_HB_OPE_AUTO : NULL;

	g_object_set(renderer, "icon-name", iconname, NULL);
}


/*
** draw some text from the stored data structure
*/
static void ui_arc_listview_text_cell_data_function (GtkTreeViewColumn *col,
				GtkCellRenderer *renderer,
				GtkTreeModel *model,
				GtkTreeIter *iter,
				gpointer user_data)
{
Archive *item;
gchar *name;

	gtk_tree_model_get(model, iter, LST_DEFARC_DATAS, &item, -1);

	name = item->wording;

	g_object_set(renderer, "text", name, NULL);
}



/*
**
*/
static GtkWidget *ui_arc_listview_new(void)
{
GtkListStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	//store
	store = gtk_list_store_new (
		NUM_LST_DEFARC,
		G_TYPE_POINTER,
		G_TYPE_UINT,
		G_TYPE_BOOLEAN
		);

	//sortable
	gtk_tree_sortable_set_sort_func(GTK_TREE_SORTABLE(store), LST_DEFARC_DATAS, ui_arc_listview_compare_func, GINT_TO_POINTER(LST_DEFARC_DATAS), NULL);


	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

	/* text column */
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_text_cell_data_function, GINT_TO_POINTER(1), NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	/* icon column */
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	//gtk_cell_renderer_set_fixed_size(renderer, GLOBALS->lst_pixbuf_maxwidth, -1);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, ui_arc_listview_auto_cell_data_function, NULL, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(view), FALSE);
	//gtk_tree_view_set_reorderable (GTK_TREE_VIEW(view), TRUE);

	return(view);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */



/*
** add an empty new account to our temp GList and treeview
*/
static void ui_arc_manage_add(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel *model;
GtkTreeIter  iter;

Archive *item;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui_scheduled] add\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_arc));

	item = da_archive_malloc();
	item->wording = g_strdup_printf(_("(template %d)"), g_list_length(GLOBALS->arc_list) + 1);
	item->unit = 2;

	GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);

	gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter,
		LST_DEFARC_DATAS, item,
		LST_DEFARC_OLDPOS, 0,
		-1);

	gtk_tree_selection_select_iter (gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &iter);

	data->change++;
}

/*
** delete the selected account to our treeview and temp GList
*/
static void ui_arc_manage_delete(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
Archive *item;
gint result;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	DB( g_print("\n[ui_scheduled] delete (data=%p)\n", data) );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
	gchar *title;
	gchar *secondtext;
		
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &item, -1);

		title = g_strdup_printf (
			_("Are you sure you want to permanently delete '%s'?"), item->wording);

		secondtext = _("If you delete a scheduled/template, it will be permanently lost.");
		
		result = ui_dialog_msg_confirm_alert(
				GTK_WINDOW(data->window),
				title,
				secondtext,
				_("_Delete")
			);

		g_free(title);
		
		if( result == GTK_RESPONSE_OK )
		{
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);

			GLOBALS->arc_list = g_list_remove(GLOBALS->arc_list, item);

			data->change++;

		}
		//DB( g_print(" delete =%08x (pos=%d)\n", entry, g_list_index(data->tmp_list, entry) ) );
	}
}


/*
** update the archive name everytime it changes
*/
static void ui_arc_manage_rename(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
gchar *txt;
Archive *item;

	DB( g_print("\n[ui_scheduled] rename\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &item, -1);

		DB( g_print(" -> %s\n", item->wording) );

		txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_word));
		// ignore if entry is empty
		if (txt && *txt)
		{
			g_free(item->wording);
			item->wording = g_strdup(txt);
		}

		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_arc));

	}

}




/*
** set widgets contents from the selected account
*/
static void ui_arc_manage_set(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeSelection *selection;
GtkTreeModel		 *model;
GtkTreeIter			 iter;

Archive *item;

	DB( g_print("\n[ui_scheduled] set\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc));
	//if true there is a selected node
	if (gtk_tree_selection_get_selected(selection, &model, &iter))
	{
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &item, -1);

		g_signal_handler_block(data->ST_word, data->handler_id[HID_ARC_MEMO]);
		gtk_entry_set_text(GTK_ENTRY(data->ST_word), item->wording);
		g_signal_handler_unblock(data->ST_word, data->handler_id[HID_ARC_MEMO]);

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), item->amount);


		
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_status), item->status );

		
		/*g_signal_handler_block(data->CM_valid, data->handler_id[HID_ARC_VALID]);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_valid), (item->flags & OF_VALID) ? 1 : 0);
		g_signal_handler_unblock(data->CM_valid, data->handler_id[HID_ARC_VALID]);

		g_signal_handler_block(data->CM_remind, data->handler_id[HID_ARC_REMIND]);
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_remind), (item->flags & OF_REMIND) ? 1 : 0);
		g_signal_handler_unblock(data->CM_remind, data->handler_id[HID_ARC_REMIND]);
		*/
		
		gtk_combo_box_set_active(GTK_COMBO_BOX(data->NU_mode), item->paymode);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_cheque), (item->flags & OF_CHEQ2) ? 1 : 0);

		ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_grp), item->kcat);

	DB( g_print(" -> set payee %d\n", item->kpay) );
		ui_pay_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_pay), item->kpay);

	DB( g_print(" -> PO_acc %d\n", item->kacc) );
		ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_acc), item->kacc);

	DB( g_print(" -> PO_accto %d\n", item->kxferacc) );
		ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_accto), item->kxferacc);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_auto), (item->flags & OF_AUTO) ? 1 : 0);

		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_every), item->every);

		gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_unit), item->unit);

		gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_next), item->nextdate);

		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_limit), (item->flags & OF_LIMIT) ? 1 : 0);


		DB( g_print("nb_limit = %d %g\n", item->limit, (gdouble)item->limit) );
		
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->NB_limit), (gdouble)item->limit);

		radio_set_active(GTK_CONTAINER(data->CY_weekend), item->weekend);

	}

}

/*
** get widgets contents to the selected account
*/
static void ui_arc_manage_getlast(struct ui_arc_manage_data *data)
{
gchar *txt;
gboolean bool;
gdouble value;
gint active;
Archive *item;

	DB( g_print("\n[ui_scheduled] getlast\n") );

	item = data->lastarcitem;

	if( item != NULL )
	{
		DB( g_print(" -> %s\n", item->wording) );

		txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_word));
		// ignore if entry is empty
		if (txt && *txt)
		{
			g_free(item->wording);
			item->wording = g_strdup(txt);
		}

		gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_arc));

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_amount));
		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		item->amount = value;

		item->flags = 0;

		active = item->amount > 0 ? TRUE : FALSE;
		//active = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_amount));
		if(active == 1) item->flags |= OF_INCOME;

		bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cheque));
		if(bool) item->flags |= OF_CHEQ2;

		item->status = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_status));

		/*bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_valid));
		if(bool) item->flags |= OF_VALID;

		bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_remind));
		if(bool == 1) item->flags |= OF_REMIND;*/

		item->paymode		= gtk_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));
		item->kcat		= ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_grp));
		item->kpay			= ui_pay_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_pay));
		item->kacc		= ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));
		item->kxferacc	= ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_accto));

		DB( g_print(" -> PO_acc %d\n", item->kacc) );
		DB( g_print(" -> PO_accto %d\n", item->kxferacc) );

		bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_auto));
		if(bool) item->flags |= OF_AUTO;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->NB_every));
		item->every   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->NB_every));
		item->unit    = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_unit));
		item->nextdate	= gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_next));

		bool = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_limit));
		if(bool) item->flags |= OF_LIMIT;

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->NB_limit));
		item->limit   = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(data->NB_limit));

		item->weekend = radio_get_active(GTK_CONTAINER(data->CY_weekend));
		
		data->change++;
	}
}


static void ui_arc_manage_update_accto(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
guint kacc, kdst;

	DB( g_print("\n\n[ui_scheduled] update accto\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	kacc = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));
	kdst = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_accto));
	
	DB( g_print(" ksrc=%d, kdst=%d\n", kacc, kdst) );

	ui_acc_comboboxentry_populate_except(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, kacc, ACC_LST_INSERT_NORMAL);

	if( (kacc == 0) || (kacc == kdst) )
	{
		ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_accto), 0);
	}
	
}


/*
**
*/
static void ui_arc_manage_paymode(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
gint payment;
gint page;
gboolean sensitive;

	DB( g_print("\n[ui_scheduled] paymode\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//DB( g_print(" widget=%p, data=%p\n", widget, data) );

	
	payment = gtk_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));
	page = 0;

	if(payment == PAYMODE_CHECK)
		page = 1;

	sensitive = page == 1 ? TRUE : FALSE;
	hb_widget_visible(data->CM_cheque, sensitive);

	if(payment == PAYMODE_INTXFER)
	{
		page = 2;
		ui_arc_manage_update_accto(widget, user_data);
	}

	DB( g_print(" payment: %d, page: %d\n", payment, page) );

	sensitive = page == 2 ? TRUE : FALSE;
	hb_widget_visible(data->LB_accto, sensitive);
	hb_widget_visible(data->PO_accto, sensitive);

	DB( g_print(" visible: %d\n", sensitive) );

}

/*
**
*/
static void ui_arc_manage_scheduled(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
Archive *arcitem;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
GtkTreePath			*path;
gboolean selected, sensitive;

	DB( g_print("\n[ui_scheduled] scheduled\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = FALSE;
	
	
	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);
	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &arcitem, -1);

		arcitem->flags &= ~(OF_AUTO);
		sensitive = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_auto)) ? TRUE : FALSE;
		if(sensitive)
			arcitem->flags |= OF_AUTO;
	}
	

	
	gtk_widget_set_sensitive(data->LB_next, sensitive);
	gtk_widget_set_sensitive(data->PO_next, sensitive);

	gtk_widget_set_sensitive(data->LB_every, sensitive);
	gtk_widget_set_sensitive(data->NB_every, sensitive);

	gtk_widget_set_sensitive(data->LB_weekend, sensitive);
	gtk_widget_set_sensitive(data->CY_weekend, sensitive);

	gtk_widget_set_sensitive(data->CY_unit, sensitive);
	gtk_widget_set_sensitive(data->CM_limit, sensitive);

	gtk_widget_set_sensitive(data->LB_posts, sensitive);

	
	sensitive = (sensitive == TRUE) ? gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_limit)) : sensitive;
	gtk_widget_set_sensitive(data->NB_limit, sensitive);

	if(selected)
	{
		/* redraw the row to display/hide the icon */
		path = gtk_tree_model_get_path(model, &iter);
		gtk_tree_model_row_changed(model, path, &iter);
		gtk_tree_path_free (path);

		//	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_arc));
		//gtk_widget_queue_draw (GTK_WIDGET(data->LV_arc));
	}	
		
}


/*
** update the widgets status and contents from action/selection value
*/
static void ui_arc_manage_update(GtkWidget *widget, gpointer user_data)
{
struct ui_arc_manage_data *data;
GtkTreeModel		 *model;
GtkTreeIter			 iter;
gboolean selected, sensitive;
Archive *arcitem;


	DB( g_print("\n[ui_scheduled] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//window = gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW);
	//DB( g_print("\n[ui_scheduled] widget=%08lx, window=%08lx, inst_data=%08lx\n", treeview, window, data) );

	//if true there is a selected node
	selected = gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_arc)), &model, &iter);

	DB( g_print(" selected = %d\n", selected) );

	sensitive = (selected == TRUE) ? TRUE : FALSE;

	gtk_widget_set_sensitive(data->GR_txnleft, sensitive);
	gtk_widget_set_sensitive(data->GR_txnright, sensitive);

	gtk_widget_set_sensitive(data->CM_auto, sensitive);

	gtk_widget_set_sensitive(data->BT_rem, sensitive);

	if(selected)
	{
		gtk_tree_model_get(model, &iter, LST_DEFARC_DATAS, &arcitem, -1);

		if(data->lastarcitem != NULL && arcitem != data->lastarcitem)
		{
			DB( g_print(" -> should do a get for last selected (%s)\n", data->lastarcitem->wording) );
			ui_arc_manage_getlast(data);
		}
		data->lastarcitem = arcitem;

		DB( g_print(" - call set\n") );
		ui_arc_manage_set(widget, NULL);
	}
	else
	{
		data->lastarcitem = NULL;
	}

	gtk_widget_set_sensitive(data->LB_schedinsert, sensitive);


	DB( g_print(" - call scheduled\n") );
	ui_arc_manage_scheduled(widget, NULL);
	DB( g_print(" - call paymode\n") );
	ui_arc_manage_paymode(widget,NULL);


}



/*
**
*/
static void ui_arc_manage_toggleamount(GtkWidget *widget, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
struct ui_arc_manage_data *data;
gdouble value;

	DB( g_print("\n[ui_scheduled] toggleamount\n") );

	if(icon_pos == GTK_ENTRY_ICON_PRIMARY)
	{

		data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_amount));

		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		value *= -1;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), value);

	/*
	value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
	type = gtk_widget_get_sensitive(data->CY_amount);

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), value * type);
	*/
	}

}


/*
**
*/
static void ui_arc_manage_selection(GtkTreeSelection *treeselection, gpointer user_data)
{
	DB( g_print("\n[ui_scheduled] selection\n") );

	DB( g_print(" - call update\n") );
	ui_arc_manage_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), NULL);
}

/*
**
*/
static gboolean ui_arc_manage_cleanup(struct ui_arc_manage_data *data, gint result)
{
gboolean doupdate = FALSE;

	DB( g_print("\n[ui_scheduled] cleanup\n") );


	if(data->lastarcitem != NULL)
	{
		DB( g_print(" -> should do a get for last selected (%s)\n", data->lastarcitem->wording) );
		ui_arc_manage_getlast(data);
	}

	GLOBALS->arc_list = da_archive_sort(GLOBALS->arc_list);

	GLOBALS->changes_count += data->change;

	return doupdate;
}

/*
**
*/
static void ui_arc_manage_setup(struct ui_arc_manage_data *data)
{
GtkTreeModel *model;
GtkTreeIter  iter;
GList *list;
gint i;

	DB( g_print("\n[ui_scheduled] setup\n") );

	//init GList
	data->tmp_list = NULL; //hb-glist_clone_list(GLOBALS->arc_list, sizeof(struct _Archive));
	data->change = 0;
	data->lastarcitem = NULL;

	//hb-glist_populate_treeview(data->tmp_list, data->LV_arc, LST_DEFARC_DATAS, LST_DEFARC_OLDPOS);

	//insert all glist item into treeview
	model  = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_arc));
	i=0;
	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *item = list->data;

		gtk_list_store_append (GTK_LIST_STORE(model), &iter);
		gtk_list_store_set (GTK_LIST_STORE(model), &iter,
			LST_DEFARC_DATAS, item,	//data struct
			LST_DEFARC_OLDPOS, i,		//oldpos
			-1);

		//DB( g_print(" populate_treeview: %d %08x\n", i, list->data) );

		i++; list = g_list_next(list);
	}

	DB( g_print(" - populate boxentries\n") );

	ui_pay_comboboxentry_populate(GTK_COMBO_BOX(data->PO_pay)  , GLOBALS->h_pay);
	ui_cat_comboboxentry_populate(GTK_COMBO_BOX(data->PO_grp)  , GLOBALS->h_cat);
	ui_acc_comboboxentry_populate(GTK_COMBO_BOX(data->PO_acc)  , GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);
	ui_acc_comboboxentry_populate(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);


}


static GtkWidget *ui_arc_manage_create_left_txn(struct ui_arc_manage_data *data)
{
GtkWidget *group_grid, *hbox, *label, *widget;
gint row;
	
	// group :: Transaction detail
    group_grid = gtk_grid_new ();
    data->GR_txnleft = group_grid;
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);

	label = make_label_group(_("Transaction detail"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label(_("_Amount:"), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_widget_set_hexpand (hbox, TRUE);

	widget = make_amount(label);
	data->ST_amount = widget;
	gtk_entry_set_icon_from_icon_name(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, ICONNAME_HB_TOGGLE_SIGN);
	gtk_entry_set_icon_tooltip_text(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, _("Toggle amount sign"));
	gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);

	row++;
	label = make_label(_("Pay_ment:"), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_paymode(label);
	gtk_widget_set_hexpand (widget, TRUE);
	data->NU_mode = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic(_("Of notebook _2"));
	data->CM_cheque = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 2, 1);

	/* info should be here some day */

	row++;
	label = make_label(_("A_ccount:"), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_acc_comboboxentry_new(label);
	gtk_widget_set_hexpand (widget, TRUE);
	data->PO_acc = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label(_("_To account:"), 0.0, 0.5);
	data->LB_accto = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_acc_comboboxentry_new(label);
	gtk_widget_set_hexpand (widget, TRUE);
	data->PO_accto = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	return group_grid;
}


static GtkWidget *ui_arc_manage_create_right_txn(struct ui_arc_manage_data *data)
{
GtkWidget *group_grid, *label, *widget;
gint row;

	// group :: <empty> 
	group_grid = gtk_grid_new ();
	data->GR_txnright = group_grid;
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);

	// keep this to avoid a shift
	label = make_label_group(NULL);
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	label = make_label(_("_Payee:"), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_pay_comboboxentry_new(label);
	gtk_widget_set_hexpand (widget, TRUE);
	data->PO_pay = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label(_("_Category:"), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = ui_cat_comboboxentry_new(label);
	gtk_widget_set_hexpand (widget, TRUE);
	data->PO_grp = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = make_label(_("_Memo:"), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_string(label);
	gtk_widget_set_hexpand (widget, TRUE);
	data->ST_word = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	/* tags should be here some day */

	row++;
	label = make_label(_("_Status:"), 0.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = make_cycle(label, CYA_TXN_STATUS);
	data->CY_status = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	return group_grid;
}


static GtkWidget *ui_arc_manage_create_scheduling(struct ui_arc_manage_data *data)
{
GtkWidget *group_grid, *hbox, *label, *widget;
gint row;

	// group :: Scheduled insertion
	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);

	label = make_label_group(_("Scheduled insertion"));
	data->LB_schedinsert = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, 0, 3, 1);

	row = 1;
	widget = gtk_check_button_new_with_mnemonic(_("_Activate"));
	data->CM_auto = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 2, 1);

	row++;
	label = gtk_label_new_with_mnemonic (_("Next _date:"));
	data->LB_next = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);
	widget = gtk_date_entry_new();
	data->PO_next = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	label = gtk_label_new_with_mnemonic (_("Ever_y:"));
	data->LB_every = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 2, row, 1, 1);
	widget = make_numeric(label, 1, 100);
	data->NB_every = widget;
    gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
	//label = gtk_label_new_with_mnemonic (_("_Unit:"));
    //gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	widget = make_cycle(label, CYA_UNIT);
	data->CY_unit = widget;
    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

	row++;
	label = make_label(_("Week end:"), 0.0, 0.5);
	data->LB_weekend = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 1, row, 1, 1);

	widget = make_radio(CYA_SCHED_WEEKEND, FALSE, GTK_ORIENTATION_HORIZONTAL);
	data->CY_weekend = widget;
	gtk_grid_attach (GTK_GRID (group_grid), widget, 2, row, 1, 1);

	row++;
	hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, SPACING_SMALL);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 3, 1);

		widget = gtk_check_button_new_with_mnemonic(_("_Stop after:"));
		data->CM_limit = widget;
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);

		widget = make_numeric(label, 1, 366);
		data->NB_limit = widget;
	    gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

		label = gtk_label_new_with_mnemonic (_("posts"));
		data->LB_posts = label;
	    gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);

	return group_grid;
}


GtkWidget *ui_arc_manage_dialog (void)
{
struct ui_arc_manage_data data;
GtkWidget *dialog, *content_area;
GtkWidget *content_grid, *group_grid, *hgrid, *treeview, *scrollwin;
GtkWidget *widget, *hpaned;
gint w, h;

	dialog = gtk_dialog_new_with_buttons (_("Manage scheduled/template transactions"),
					    GTK_WINDOW(GLOBALS->mainwindow),
					    0,
					    _("_Close"),
					    GTK_RESPONSE_ACCEPT,
					    NULL);

	data.window = dialog;
	
	gtk_window_set_icon_name(GTK_WINDOW (dialog), ICONNAME_HB_ARCHIVE);

	//set a nice dialog size
	gtk_window_get_size(GTK_WINDOW(GLOBALS->mainwindow), &w, &h);
	gtk_window_set_default_size (GTK_WINDOW(dialog), -1, h/PHI);

	
	//store our dialog private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)&data);
	DB( g_print("\n[ui_scheduled] dialog=%p, inst_data=%p\n", dialog, &data) );

	//dialog contents
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));	 	// return a vbox

	hpaned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_container_set_border_width (GTK_CONTAINER(hpaned), SPACING_MEDIUM);
	gtk_container_add(GTK_CONTAINER(content_area), hpaned);
	
	/* left area */
	hgrid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (hgrid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (hgrid), SPACING_MEDIUM);
	gtk_widget_set_margin_right(hgrid, SPACING_SMALL);
	gtk_paned_pack1 (GTK_PANED(hpaned), hgrid, FALSE, FALSE);

	// listview
	scrollwin = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
 	treeview = (GtkWidget *)ui_arc_listview_new();
  	data.LV_arc = treeview;
	gtk_widget_set_size_request(treeview, HB_MINWIDTH_LIST, -1);
	gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
	gtk_widget_set_hexpand (scrollwin, TRUE);
	gtk_widget_set_vexpand (scrollwin, TRUE);
	gtk_grid_attach (GTK_GRID (hgrid), scrollwin, 0, 0, 2, 1);
	
	widget = gtk_button_new_with_mnemonic(_("_Add"));
	data.BT_add = widget;
	gtk_grid_attach (GTK_GRID (hgrid), widget, 0, 1, 1, 1);
	
	widget = gtk_button_new_with_mnemonic(_("_Delete"));
	data.BT_rem = widget;
	gtk_grid_attach (GTK_GRID (hgrid), widget, 1, 1, 1, 1);


	/* right area */
	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	//gtk_container_set_border_width (GTK_CONTAINER(content_grid), SPACING_MEDIUM);
	gtk_widget_set_margin_left(content_grid, SPACING_SMALL);
	gtk_paned_pack2 (GTK_PANED(hpaned), content_grid, FALSE, FALSE);

	hgrid = gtk_grid_new ();
	gtk_grid_set_column_spacing (GTK_GRID (hgrid), SPACING_SMALL);
	gtk_grid_attach (GTK_GRID (content_grid), hgrid, 0, 0, 1, 1);

	group_grid = ui_arc_manage_create_left_txn(&data);
	gtk_widget_set_hexpand (GTK_WIDGET(group_grid), FALSE);
	gtk_grid_attach (GTK_GRID (hgrid), group_grid, 0, 0, 1, 1);

	group_grid = ui_arc_manage_create_right_txn(&data);
	gtk_widget_set_hexpand (GTK_WIDGET(group_grid), TRUE);
	gtk_grid_attach (GTK_GRID (hgrid), group_grid, 1, 0, 1, 1);
	
	/* sheduling */
	group_grid = ui_arc_manage_create_scheduling(&data);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, 1, 1, 1);

	/* set default periodicity to month */
	//todo: move elsewhere
	gtk_combo_box_set_active(GTK_COMBO_BOX(data.CY_unit), 2);

	//connect all our signals
	g_signal_connect (dialog, "destroy", G_CALLBACK (gtk_widget_destroyed), &dialog);

	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(data.LV_arc)), "changed", G_CALLBACK (ui_arc_manage_selection), NULL);
	g_signal_connect (G_OBJECT (data.ST_amount), "icon-release", G_CALLBACK (ui_arc_manage_toggleamount), NULL);

	g_signal_connect (G_OBJECT (data.BT_add), "clicked", G_CALLBACK (ui_arc_manage_add), NULL);
	g_signal_connect (G_OBJECT (data.BT_rem), "clicked", G_CALLBACK (ui_arc_manage_delete), NULL);

	data.handler_id[HID_ARC_MEMO] = g_signal_connect (G_OBJECT (data.ST_word), "changed", G_CALLBACK (ui_arc_manage_rename), NULL);
	g_signal_connect (data.NU_mode, "changed", G_CALLBACK (ui_arc_manage_paymode), NULL);
	g_signal_connect (data.PO_acc, "changed", G_CALLBACK (ui_arc_manage_update_accto), NULL);
	//data.handler_id[HID_ARC_VALID]  = g_signal_connect (data.CM_valid , "toggled", G_CALLBACK (ui_arc_manage_togglestatus), GINT_TO_POINTER(HID_ARC_VALID));
	//data.handler_id[HID_ARC_REMIND] = g_signal_connect (data.CM_remind, "toggled", G_CALLBACK (ui_arc_manage_togglestatus), GINT_TO_POINTER(HID_ARC_REMIND));

	g_signal_connect (data.CM_auto, "toggled", G_CALLBACK (ui_arc_manage_scheduled), NULL);
	g_signal_connect (data.CM_limit, "toggled", G_CALLBACK (ui_arc_manage_scheduled), NULL);

	//setup, init and show dialog
	ui_arc_manage_setup(&data);
	ui_arc_manage_update(data.LV_arc, NULL);

	gtk_widget_show_all (dialog);

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	switch (result)
    {
	case GTK_RESPONSE_ACCEPT:
	   //do_application_specific_something ();
	   break;
	default:
	   //do_nothing_since_dialog_was_cancelled ();
	   break;
    }

	// cleanup and destroy
	ui_arc_manage_cleanup(&data, result);
	gtk_widget_destroy (dialog);

	return NULL;
}


