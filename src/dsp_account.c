/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2018 Maxime DOYEN
 *
 *	This file is part of HomeBank.
 *
 *	HomeBank is free software; you can redistribute it and/or modify
 *	it under the terms of the GNU General Public License as published by
 *	the Free Software Foundation; either version 2 of the License, or
 *	(at your option) any later version.
 *
 *	HomeBank is distributed in the hope that it will be useful,
 *	but WITHOUT ANY WARRANTY; without even the implied warranty of
 *	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
 *	GNU General Public License for more details.
 *
 *	You should have received a copy of the GNU General Public License
 *	along with this program.	If not, see <http://www.gnu.org/licenses/>.
 */


#include "homebank.h"

#include "dsp_account.h"
#include "dsp_mainwindow.h"

#include "list_operation.h"
#include "ui-widgets.h"
#include "ui-filter.h"
#include "ui-transaction.h"
#include "ui-txn-multi.h"

/****************************************************************************/
/* Debug macros											                    */
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

//debug
#define UI 1

//extern gchar *CYA_FLT_SELECT[];

extern gchar *CYA_FLT_TYPE[];
extern gchar *CYA_FLT_STATUS[];


static void register_panel_collect_filtered_txn(GtkWidget *view);
static void register_panel_listview_populate(GtkWidget *view);
static void register_panel_action(GtkWidget *widget, gpointer user_data);
static void register_panel_update(GtkWidget *widget, gpointer user_data);

static void register_panel_export_csv(GtkWidget *widget, gpointer user_data);

static void register_panel_make_archive(GtkWidget *widget, gpointer user_data);

static void status_selected_foreach_func (GtkTreeModel	*model, GtkTreePath	 *path, GtkTreeIter	 *iter, gpointer userdata);

static void register_panel_edit_multiple(GtkWidget *widget, Transaction *txn, gint column_id, gpointer user_data);

static void register_panel_selection(GtkTreeSelection *treeselection, gpointer user_data);
static void register_panel_onRowActivated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata);


/* account action functions -------------------- */

static void register_panel_action_editfilter(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_FILTER));
}


static void register_panel_action_add(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_ADD));
}

static void register_panel_action_inherit(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_INHERIT));
}

static void register_panel_action_edit(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_EDIT));
}

static void register_panel_action_multiedit(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_edit_multiple(data->window, NULL, 0, user_data);
}

static void register_panel_action_reconcile(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_RECONCILE));
}

static void register_panel_action_clear(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_CLEAR));
}

static void register_panel_action_none(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_NONE));
}

static void register_panel_action_remove(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_action(data->window, GINT_TO_POINTER(ACTION_ACCOUNT_DELETE));
}



static void register_panel_action_createtemplate(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_make_archive(data->window, NULL);
}



static void register_panel_action_exportcsv(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_export_csv(data->window, NULL);
}


/* = = = = = = = = future version = = = = = = = = */

static void register_panel_action_exportpdf(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;
gchar *name, *filepath;

	if(data->showall == FALSE)
	{
		name = g_strdup_printf("%s.pdf", data->acc->name);
		filepath = g_build_filename(PREFS->path_export, name, NULL);
		g_free(name);

		if( ui_dialog_export_pdf(GTK_WINDOW(data->window), &filepath) == GTK_RESPONSE_ACCEPT )
		{
			DB( g_printf(" filename is'%s'\n", filepath) );
	
			
			hb_export_pdf_listview(GTK_TREE_VIEW(data->LV_ope), filepath, data->acc->name);
		}

		g_free(filepath);
		
	}
}


static void register_panel_action_duplicate_mark(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;
	
	DB( g_print("check duplicate\n\n") );

	// open dialog to select date tolerance in days
	//  with info message
	//  with check/fix button and progress bar
	// parse listview txn, clear/mark duplicate
	// apply filter

	if(data->showall == FALSE)
	{
	gint daygap;

		daygap = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_txn_daygap));
		data->similar = transaction_similar_mark (data->acc, daygap);
		if( data->similar > 0 )
		{
		gchar *text = g_strdup_printf(_("There is %d group of similar transactions"), data->similar);
			gtk_label_set_text(GTK_LABEL(data->LB_duplicate), text);
			g_free(text);
		}
		else
			gtk_label_set_text(GTK_LABEL(data->LB_duplicate), _("No similar transaction were found !"));

		gtk_widget_show(data->IB_duplicate);
		//#GTK+710888: hack waiting a fix
		gtk_widget_queue_resize (data->IB_duplicate);

		gtk_widget_queue_draw (data->LV_ope);
	}


}


static void register_panel_action_duplicate_unmark(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	DB( g_print("uncheck duplicate\n\n") );

	if(data->showall == FALSE)
	{   
		data->similar = 0;
		gtk_widget_hide(data->IB_duplicate);
		transaction_similar_unmark(data->acc);
		gtk_widget_queue_draw (data->LV_ope);
	}
}


static void register_panel_action_check_internal_xfer(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;
GtkTreeModel *model;
GtkTreeIter iter;
GList *badxferlist;
gboolean valid;
gint count;

	DB( g_print("check intenal xfer\n\n") );

	badxferlist = NULL;
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Transaction *txn;

		gtk_tree_model_get (model, &iter,
			LST_DSPOPE_DATAS, &txn,
			-1);

		if( txn->paymode == PAYMODE_INTXFER )
		{
			if( transaction_xfer_child_strong_get(txn) == NULL )
			{
				DB( g_print(" - invalid xfer: '%s'\n", txn->memo) );
				
				//test unrecoverable (kxferacc = 0)
				if( txn->kxferacc <= 0 )
				{
					DB( g_print(" - unrecoverable, revert to normal xfer\n") );
					txn->flags |= OF_CHANGED;
					txn->paymode = PAYMODE_XFER;
					txn->kxfer = 0;
					txn->kxferacc = 0;
				}
				else
				{
					badxferlist = g_list_append(badxferlist, txn);
				}
			}
		}
		
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}

	count = g_list_length (badxferlist);
	DB( g_print(" - found %d invalid int xfer\n", count) );

	if(count <= 0)
	{
		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
		_("Check internal transfert result"),
		_("No inconsistency found !")
		);
	}
	else
	{
	gboolean do_fix;
	
		do_fix = ui_dialog_msg_question(
			GTK_WINDOW(data->window),
			_("Check internal transfert result"),
			_("Inconsistency were found: %d\n"
			  "do you want to review and fix ?"),
			count
			);

		if(do_fix == GTK_RESPONSE_YES)
		{	
		GList *tmplist = g_list_first(badxferlist);

			while (tmplist != NULL)
			{
			Transaction *stxn = tmplist->data;

				//future (open dialog to select date tolerance in days)
				transaction_xfer_search_or_add_child(GTK_WINDOW(data->window), stxn, 0);

				tmplist = g_list_next(tmplist);
			}	
		}
	}

	g_list_free (badxferlist);

}


static void register_panel_action_exportqif(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;
gchar *filename;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("(qif) test qif export\n\n") );

	if( ui_file_chooser_qif(GTK_WINDOW(data->window), &filename) == TRUE )
	{
		hb_export_qif_account_single(filename, data->acc);
		g_free( filename );
	}
}


static void register_panel_action_converttoeuro(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;
gchar *msg;
gint result;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("action convert to euro\n") );

	msg = g_strdup_printf(_("Every transaction amount will be divided by %.6f."), PREFS->euro_value);

	result = ui_dialog_msg_confirm_alert(
			GTK_WINDOW(data->window),
			_("Are you sure you want to convert this account\nto Euro as Major currency?"),
			msg,
			_("_Convert")
		);

	g_free(msg);

	if(result == GTK_RESPONSE_OK)
	{
		account_convert_euro(data->acc);
		register_panel_update(data->LV_ope, GINT_TO_POINTER(UF_BALANCE));
	}
}


static void register_panel_action_assign(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;
gint count;
gboolean usermode = TRUE;

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("action assign\n") );

	count = transaction_auto_assign(g_queue_peek_head_link(data->acc->txn_queue), data->acc->key);
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_ope));
	GLOBALS->changes_count += count;

	//inform the user
	if(usermode == TRUE)
	{
	gchar *txt;

		if(count == 0)
			txt = _("No transaction changed");
		else
			txt = _("transaction changed: %d");

		ui_dialog_msg_infoerror(GTK_WINDOW(data->window), GTK_MESSAGE_INFO,
			_("Automatic assignment result"),
			txt,
			count);
	}

	//refresh main
	if( count > 0 )
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}


static void register_panel_action_close(GtkAction *action, gpointer user_data)
{
struct register_panel_data *data = user_data;

	DB( g_print("action close\n") );

	DB( g_print("window %p\n", data->window) );

	gtk_widget_destroy (GTK_WIDGET (data->window));

	//g_signal_emit_by_name(data->window, "delete-event", NULL, &result);

}


/* these 5 functions are independant from account window */

/* account functions -------------------- */

static void register_panel_export_csv(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gchar *filename = NULL;
GString *node;
GIOChannel *io;

	DB( g_print("\n[account] export csv\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	if( ui_file_chooser_csv(GTK_WINDOW(data->window), GTK_FILE_CHOOSER_ACTION_SAVE, &filename, NULL) == TRUE )
	{
		DB( g_print(" + filename is %s\n", filename) );
		io = g_io_channel_new_file(filename, "w", NULL);
		if(io != NULL)
		{
			node = list_txn_to_string(GTK_TREE_VIEW(data->LV_ope), FALSE);
			g_io_channel_write_chars(io, node->str, -1, NULL, NULL);
			g_io_channel_unref (io);
			g_string_free(node, TRUE);
		}
		g_free( filename );
	}
}


static void register_panel_edit_multiple(GtkWidget *widget, Transaction *txn, gint column_id, gpointer user_data)
{
struct register_panel_data *data;
GtkWidget *dialog;

	DB( g_print("\n[account] edit multiple\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print(" - txn:%p, column: %d\n", txn, column_id) );

	dialog = ui_multipleedit_dialog_new(GTK_WINDOW(data->window), GTK_TREE_VIEW(data->LV_ope));

	if(txn != NULL && column_id != 0)
	{
		ui_multipleedit_dialog_prefill(dialog, txn, column_id);
	}

	//wait for the user
	gint result = gtk_dialog_run (GTK_DIALOG (dialog));

	if( result == GTK_RESPONSE_ACCEPT )
	{
	gint changes;
		
		changes = ui_multipleedit_dialog_apply (dialog, NULL);
		if( changes > 0 )
		{
			//#1782749 update account status
			if( data->acc != NULL )
				data->acc->flags |= AF_CHANGED;

			ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));
		}
	}

	gtk_widget_destroy (dialog);
	
	register_panel_update(data->LV_ope, GINT_TO_POINTER(UF_BALANCE));
}


/*
** make an archive with the active transaction
*/
static void register_panel_make_archive(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[account] make archive\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

GtkWidget *p_dialog = NULL;
GtkTreeModel *model;
GList *selection, *list;
gint result, count;

	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)));

	if( count > 0 )
	{
			p_dialog = gtk_message_dialog_new
			(
				GTK_WINDOW(data->window),
				GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				GTK_MESSAGE_WARNING,
			GTK_BUTTONS_YES_NO,
			_("Do you want to create a template with\neach of the selected transaction ?")
			);

	/*
		gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG (dialog),
			_("%d archives will be created"),
			GLOBALS->changes_count
			);
	*/

		result = gtk_dialog_run( GTK_DIALOG( p_dialog ) );
		gtk_widget_destroy( p_dialog );


		if(result == GTK_RESPONSE_YES)
		{

			model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
			selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

			list = g_list_first(selection);
			while(list != NULL)
			{
			Archive *item;
			Transaction *ope;
			GtkTreeIter iter;

				gtk_tree_model_get_iter(model, &iter, list->data);
				gtk_tree_model_get(model, &iter, LST_DSPOPE_DATAS, &ope, -1);

				DB( g_print(" create archive %s %.2f\n", ope->memo, ope->amount) );

				item = da_archive_malloc();

				da_archive_init_from_transaction(item, ope);

				//GLOBALS->arc_list = g_list_append(GLOBALS->arc_list, item);
				da_archive_append_new(item);
				GLOBALS->changes_count++;

				list = g_list_next(list);
			}

			g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
			g_list_free(selection);
		}
	}
}


static void register_panel_cb_bar_duplicate_response(GtkWidget *info_bar, gint response_id, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[account] bar_duplicate_response\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(info_bar, GTK_TYPE_WINDOW)), "inst_data");

	switch( response_id )
	{
		case HB_RESPONSE_REFRESH:
			register_panel_action_duplicate_mark(NULL, data);
			break;
		case GTK_RESPONSE_CLOSE:
			register_panel_action_duplicate_unmark(NULL, data);
			gtk_widget_hide (GTK_WIDGET (info_bar));	
			break;
	}
}


static void register_panel_cb_filter_daterange(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gboolean future;
gint range;

	DB( g_print("\n[account] filter_daterange\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	range  = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_range));
	future = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_future));

	data->filter->nbdaysfuture = 0;

	if(range != FLT_RANGE_OTHER)
	{
		filter_preset_daterange_set(data->filter, range, (data->showall == FALSE) ? data->acc->key : 0);
		// add eventual x days into future display
		if( future && (PREFS->date_future_nbdays > 0) )
			filter_preset_daterange_add_futuregap(data->filter, PREFS->date_future_nbdays);
		
		register_panel_collect_filtered_txn(data->LV_ope);
		register_panel_listview_populate(data->LV_ope);
	}
	else
	{
		if(ui_flt_manage_dialog_new(data->window, data->filter, data->showall) != GTK_RESPONSE_REJECT)
		{
			register_panel_collect_filtered_txn(data->LV_ope);
			register_panel_listview_populate(data->LV_ope);
			register_panel_update(data->LV_ope, GINT_TO_POINTER(UF_SENSITIVE+UF_BALANCE));
		}
	}
}


static void register_panel_cb_filter_type(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gint type;

	DB( g_print("\n[account] filter_type\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	type = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_type));

	filter_preset_type_set(data->filter, type);

	register_panel_collect_filtered_txn(data->LV_ope);
	register_panel_listview_populate(data->LV_ope);
}


static void register_panel_cb_filter_status(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gint status;

	DB( g_print("\n[account] filter_status\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	status = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_status));

	filter_preset_status_set(data->filter, status);

	register_panel_collect_filtered_txn(data->LV_ope);
	register_panel_listview_populate(data->LV_ope);
}


static void register_panel_cb_filter_reset(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[account] filter_reset\n") );
	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	filter_default_all_set(data->filter);

	filter_preset_daterange_set (data->filter, PREFS->date_range_txn, (data->showall == FALSE) ? data->acc->key : 0);

	if(PREFS->hidereconciled)
		filter_preset_status_set (data->filter, 1);

	// add eventual x days into future display
	if( PREFS->date_future_nbdays > 0 )
		filter_preset_daterange_add_futuregap(data->filter, PREFS->date_future_nbdays);

	register_panel_collect_filtered_txn(data->LV_ope);
	register_panel_listview_populate(data->LV_ope);
	
	g_signal_handler_block(data->CY_range, data->handler_id[HID_RANGE]);
	g_signal_handler_block(data->CY_type, data->handler_id[HID_TYPE]);
	g_signal_handler_block(data->CY_status, data->handler_id[HID_STATUS]);

	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_range), data->filter->range);
	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_type), data->filter->type);
	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_status), data->filter->status);

	g_signal_handler_unblock(data->CY_status, data->handler_id[HID_STATUS]);
	g_signal_handler_unblock(data->CY_type, data->handler_id[HID_TYPE]);
	g_signal_handler_unblock(data->CY_range, data->handler_id[HID_RANGE]);
	
}


static void register_panel_balance_refresh(GtkWidget *view)
{
struct register_panel_data *data;
GList *list;
gdouble balance;
GtkTreeModel *model;
guint32 ldate = 0;
gushort lpos = 1;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(view, GTK_TYPE_WINDOW)), "inst_data");

	// noaction if show all account
	if(data->showall)
		return;

	DB( g_print("\n[account] balance refresh\n") );

	balance = data->acc->initial;

	//#1270687: sort if date changed
	if(data->do_sort)
	{
		DB( g_print(" - complete txn sort\n") );
		
		da_transaction_queue_sort(data->acc->txn_queue);
		data->do_sort = FALSE;
	}

	list = g_queue_peek_head_link(data->acc->txn_queue);
	while (list != NULL)
	{
	Transaction *ope = list->data;
	gdouble value;

		//#1267344
		if(!(ope->status == TXN_STATUS_REMIND))
			balance += ope->amount;

		ope->balance = balance;

		//#1661806 add show overdraft
		//#1672209 added round like for #400483
		ope->overdraft = FALSE;
		value = hb_amount_round(balance, 2);
		if( value != 0.0 && value < data->acc->minimum )
			ope->overdraft = TRUE;

		if(ope->date == ldate)
		{
			ope->pos = ++lpos;	
		}
		else
		{
			ope->pos = lpos = 1;
		}
		ldate = ope->date;

		list = g_list_next(list);
	}
	
	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
	list_txn_sort_force(GTK_TREE_SORTABLE(model), NULL);
	
}


static void register_panel_collect_filtered_txn(GtkWidget *view)
{
struct register_panel_data *data;
GList *lst_acc, *lnk_acc;
GList *lnk_txn;

	DB( g_print("\n[register_panel] collect_filtered_txn\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(view, GTK_TYPE_WINDOW)), "inst_data");

	if(data->gpatxn != NULL)
		g_ptr_array_free (data->gpatxn, TRUE);

	//todo: why this ?
	data->gpatxn = g_ptr_array_sized_new(64);

	lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
	lnk_acc = g_list_first(lst_acc);
	while (lnk_acc != NULL)
	{
	Account *acc = lnk_acc->data;

		// skip closed in showall mode
		if( data->showall == TRUE && (acc->flags & AF_CLOSED) )
			goto next_acc;

		// skip other than current in normal mode
		if( (data->showall == FALSE) && (data->acc != NULL) && (acc->key != data->acc->key) )
			goto next_acc;

		lnk_txn = g_queue_peek_head_link(acc->txn_queue);
		while (lnk_txn != NULL)
		{
		Transaction *ope = lnk_txn->data;

			if(filter_test(data->filter, ope) == 1)
			{
				//add to the list
				g_ptr_array_add(data->gpatxn, (gpointer)ope);
			}
			lnk_txn = g_list_next(lnk_txn);
		}
	
	next_acc:
		lnk_acc = g_list_next(lnk_acc);
	}
	g_list_free(lst_acc);

	g_signal_handler_block(data->ST_search, data->handler_id[HID_SEARCH]);
	gtk_entry_set_text (GTK_ENTRY(data->ST_search), "");
	g_signal_handler_unblock(data->ST_search, data->handler_id[HID_SEARCH]);
	
}


static void register_panel_listview_populate(GtkWidget *widget)
{
struct register_panel_data *data;
GtkTreeModel *model;
GtkTreeIter	iter;
gboolean hastext;
gchar *needle;
gint sort_column_id;
GtkSortType order;
guint i, qs_flag;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[register_panel] listview_populate\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));

	// ref model to keep it
	//g_object_ref(model);
	//gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_ope), NULL);
	gtk_list_store_clear (GTK_LIST_STORE(model));


	// perf: if you leave the sort, insert is damned slow
	gtk_tree_sortable_get_sort_column_id (GTK_TREE_SORTABLE(GTK_LIST_STORE(model)), &sort_column_id, &order);
	
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GTK_LIST_STORE(model)), GTK_TREE_SORTABLE_UNSORTED_SORT_COLUMN_ID, PREFS->lst_ope_sort_order);

	hastext = gtk_entry_get_text_length (GTK_ENTRY(data->ST_search)) >= 2;
	needle = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_search));

	//build the mask flag for quick search
	qs_flag = 0;
	if(hastext)
	{
		qs_flag = list_txn_get_quicksearch_column_mask(GTK_TREE_VIEW(data->LV_ope));
	}
	
	data->total = 0;
	data->totalsum = 0.0;

	for(i=0;i<data->gpatxn->len;i++)
	{
	Transaction *txn = g_ptr_array_index(data->gpatxn, i);
	gboolean insert = TRUE;
		
		if(hastext)
		{
			insert = filter_txn_search_match(needle, txn, qs_flag);
		}

		if(insert)
		{
			//gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	 		//gtk_list_store_set (GTK_LIST_STORE(model), &iter,
	 		gtk_list_store_insert_with_values(GTK_LIST_STORE(model), &iter, -1,
				LST_DSPOPE_DATAS, txn,
				-1);

			if( data->showall == FALSE )
				data->totalsum += txn->amount;
			else
				data->totalsum += hb_amount_base (txn->amount, txn->kcur);

			data->total++;
		}
	}
	
	//gtk_tree_view_set_model(GTK_TREE_VIEW(data->LV_ope), model); /* Re-attach model to view */
	//g_object_unref(model);

	// push back the sort id
	gtk_tree_sortable_set_sort_column_id(GTK_TREE_SORTABLE(GTK_LIST_STORE(model)), sort_column_id, order);

	/* update info range text */
	{
	gchar *daterange;
		
		daterange = filter_daterange_text_get(data->filter);
		gtk_widget_set_tooltip_markup(GTK_WIDGET(data->CY_range), daterange);

		g_free(daterange);
	}
	
	register_panel_update(data->LV_ope, GINT_TO_POINTER(UF_SENSITIVE+UF_BALANCE));

}

static gint txn_list_get_count_reconciled(GtkTreeView *treeview)
{
GtkTreeModel *model;
GList *lselection, *list;
gint count = 0;
	
	model = gtk_tree_view_get_model(treeview);
	lselection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(treeview), &model);

	list = g_list_last(lselection);
	while(list != NULL)
	{
	GtkTreeIter iter;
	Transaction *txn;


		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_tree_model_get(model, &iter, LST_DSPOPE_DATAS, &txn, -1);
		if(txn->status == TXN_STATUS_RECONCILED)
			count++;
		
		list = g_list_previous(list);
	}

	g_list_foreach(lselection, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(lselection);

	return count;
}


static void txn_list_add_by_value(GtkTreeView *treeview, Transaction *ope)
{
GtkTreeModel *model;
GtkTreeIter  iter;
//GtkTreePath *path;
//GtkTreeSelection *sel;

	DB( g_print("\n[transaction] add_treeview\n") );

	model = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gtk_list_store_append (GTK_LIST_STORE(model), &iter);
	gtk_list_store_set (GTK_LIST_STORE(model), &iter,
		LST_DSPOPE_DATAS, ope,
		-1);

	//activate that new line
	//path = gtk_tree_model_get_path(model, &iter);
	//gtk_tree_view_expand_to_path(GTK_TREE_VIEW(treeview), path);

	//sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview));
	//gtk_tree_selection_select_iter(sel, &iter);

	//gtk_tree_path_free(path);

}




/* used to remove a intxfer child from a treeview */
static void txn_list_remove_by_value(GtkTreeModel *model, Transaction *txn)
{
GtkTreeIter iter;
gboolean valid;

	if( txn == NULL )
		return;

	DB( g_print("remove by value %p\n\n", txn) );

	valid = gtk_tree_model_get_iter_first(GTK_TREE_MODEL(model), &iter);
	while (valid)
	{
	Transaction *tmp;

		gtk_tree_model_get (model, &iter,
			LST_DSPOPE_DATAS, &tmp,
			-1);

		if( txn == tmp )
		{
			gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
			break;
		}
		valid = gtk_tree_model_iter_next(GTK_TREE_MODEL(model), &iter);
	}
}


static void status_selected_foreach_func (GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer userdata)
{
gint targetstatus = GPOINTER_TO_INT(userdata);
Transaction *txn;

	gtk_tree_model_get(model, iter, LST_DSPOPE_DATAS, &txn, -1);

	account_balances_sub(txn);
	
	switch(targetstatus)
	{
		case TXN_STATUS_NONE:
			switch(txn->status)
			{
				case TXN_STATUS_CLEARED:
				case TXN_STATUS_RECONCILED:
					txn->status = TXN_STATUS_NONE;
					txn->flags |= OF_CHANGED;
					break;
			}
			break;

		case TXN_STATUS_CLEARED:
			switch(txn->status)
			{
				case TXN_STATUS_NONE:
					txn->status = TXN_STATUS_CLEARED;
					txn->flags |= OF_CHANGED;
					break;
				case TXN_STATUS_CLEARED:
					txn->status = TXN_STATUS_NONE;
					txn->flags |= OF_CHANGED;
					break;
			}
			break;
			
		case TXN_STATUS_RECONCILED:
			switch(txn->status)
			{
				case TXN_STATUS_NONE:
				case TXN_STATUS_CLEARED:
					txn->status = TXN_STATUS_RECONCILED;
					txn->flags |= OF_CHANGED;
					break;
				case TXN_STATUS_RECONCILED:
					txn->status = TXN_STATUS_CLEARED;
					txn->flags |= OF_CHANGED;
					break;
			}
			break;

	}


	account_balances_add(txn);
	
	/* #492755 let the child transfer unchanged */

}


static void delete_active_transaction(GtkTreeView *treeview)
{
GtkTreeModel *model;
GList *list;

	model = gtk_tree_view_get_model(treeview);
	list = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(treeview), &model);

	if(list != NULL)
	{
	GtkTreeIter iter;

		gtk_tree_model_get_iter(model, &iter, list->data);
		gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	}

	g_list_foreach(list, (GFunc)gtk_tree_path_free, NULL);
	g_list_free(list);


}


static void register_panel_add_single_transaction(GtkWindow *window, Transaction *txn)
{
struct register_panel_data *data;

	DB( g_print("\n[account] add single txn\n") );

	data = g_object_get_data(G_OBJECT(window), "inst_data");

	txn_list_add_by_value(GTK_TREE_VIEW(data->LV_ope), txn);
}


static void register_panel_action(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
gint action = GPOINTER_TO_INT(user_data);
guint changes = GLOBALS->changes_count;
gboolean result;

	DB( g_print("\n[account] action\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//data = INST_DATA(widget);

	DB( g_print(" - action=%d\n", action) );

	switch(action)
	{
		//add
		case ACTION_ACCOUNT_ADD:
		//inherit
		case ACTION_ACCOUNT_INHERIT:
		{
		GtkWidget *dialog;
		Transaction *src_txn;
		gint type = 0;

			homebank_app_date_get_julian();
			
			if(action == ACTION_ACCOUNT_ADD)
			{
				DB( g_print("(transaction) add multiple\n") );
				src_txn = da_transaction_malloc();
				src_txn->date = GLOBALS->today;
				if( data->acc != NULL )
					src_txn->kacc = data->acc->key;
				da_transaction_set_default_template(src_txn);
				type = TRANSACTION_EDIT_ADD;
			}
			else
			{
				DB( g_print("(transaction) inherit multiple\n") );
				src_txn = da_transaction_clone(list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope)));
				//#1432204
				src_txn->status = TXN_STATUS_NONE;
				type = TRANSACTION_EDIT_INHERIT;
			}

			dialog = create_deftransaction_window(GTK_WINDOW(data->window), type, FALSE, (data->acc != NULL) ? data->acc->key : 0 );
			result = HB_RESPONSE_ADD;
			while(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP)
			{
				/* clone source transaction */
				if( result == HB_RESPONSE_ADD )
				{
					data->cur_ope = da_transaction_clone (src_txn);

					if( PREFS->heritdate == FALSE ) //fix: 318733 / 1335285
						data->cur_ope->date = GLOBALS->today;
				}

				deftransaction_set_transaction(dialog, data->cur_ope);

				result = gtk_dialog_run (GTK_DIALOG (dialog));
				if(result == HB_RESPONSE_ADD || result == HB_RESPONSE_ADDKEEP || result == GTK_RESPONSE_ACCEPT)
				{
				Transaction *add_txn;
				
					deftransaction_get(dialog, NULL);
					add_txn = transaction_add(GTK_WINDOW(data->window), data->cur_ope);
					if((data->showall == TRUE) || ( (data->acc != NULL) && (data->cur_ope->kacc == data->acc->key) ) )
					{
						txn_list_add_by_value(GTK_TREE_VIEW(data->LV_ope), add_txn);
						//#1716181 also add to the ptr_array (quickfilter)
						g_ptr_array_add(data->gpatxn, (gpointer)add_txn);
					}
					register_panel_update(widget, GINT_TO_POINTER(UF_BALANCE));
					//#1667201 already done into transaction_add
					//data->acc->flags |= AF_ADDED;
					GLOBALS->changes_count++;
					//store last date
					src_txn->date = data->cur_ope->date;
				}

				if( result == HB_RESPONSE_ADD )
				{
					da_transaction_free (data->cur_ope);
				}

			}

			deftransaction_dispose(dialog, NULL);
			da_transaction_free (src_txn);

			gtk_widget_destroy (dialog);
		}
		break;

		case ACTION_ACCOUNT_EDIT:
			{
		Transaction *active_txn;

			DB( g_print(" - edit\n") );
				
			active_txn = list_txn_get_active_transaction(GTK_TREE_VIEW(data->LV_ope));
					
			if(active_txn)
			{
			Transaction *old_txn, *new_txn;

				old_txn = da_transaction_clone (active_txn);
				new_txn = active_txn;
				
				result = deftransaction_external_edit(GTK_WINDOW(data->window), old_txn, new_txn);

				if(result == GTK_RESPONSE_ACCEPT)
				{
					//manage current window display stuff
					
					//#1270687: sort if date changed
					if(old_txn->date != new_txn->date)
						data->do_sort = TRUE;

					// manage account change
					//maybe this should move to deftransaction_external_edit
					if( data->acc != NULL && (new_txn->kacc != data->acc->key) )
					{
					Account *nacc;
						
						delete_active_transaction(GTK_TREE_VIEW(data->LV_ope));
						//#1667501 update target account window is open
						nacc = da_acc_get(new_txn->kacc);
						if( nacc->window != NULL )
						{
							DB( g_print("- account changed and window is open\n") );
							if( GTK_IS_WINDOW(nacc->window) )
							{
								register_panel_add_single_transaction(nacc->window, new_txn);
								register_panel_update(GTK_WIDGET(nacc->window), GINT_TO_POINTER(UF_BALANCE));
							}
						}
					}
					
					//da_transaction_copy(new_txn, old_txn);

					register_panel_update(widget, GINT_TO_POINTER(UF_BALANCE));

					if( data->acc != NULL )
						data->acc->flags |= AF_CHANGED;
					GLOBALS->changes_count++;
				}

				da_transaction_free (old_txn);
			}
		}
		break;

		case ACTION_ACCOUNT_DELETE:
		{
		GtkWidget *p_dialog = NULL;
		GtkTreeModel *model;
		GList *selection, *list;
		gint result;

			DB( g_print(" - delete\n") );

			//todo: replace with a call to ui_dialog_msg_question ?
			p_dialog = gtk_message_dialog_new
			(
				GTK_WINDOW(data->window),
				GTK_DIALOG_MODAL,
				GTK_MESSAGE_WARNING,
			GTK_BUTTONS_YES_NO,
			_("Do you want to delete\neach of the selected transaction ?")
			);

			result = gtk_dialog_run( GTK_DIALOG( p_dialog ) );
			gtk_widget_destroy( p_dialog );

			if(result == GTK_RESPONSE_YES)
			{
				model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
				selection = gtk_tree_selection_get_selected_rows(gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope)), &model);

				list = g_list_last(selection);
				while(list != NULL)
				{
				Transaction *entry;
				GtkTreeIter iter;

					gtk_tree_model_get_iter(model, &iter, list->data);
					gtk_tree_model_get(model, &iter, LST_DSPOPE_DATAS, &entry, -1);

					DB( g_print(" delete %s %.2f\n", entry->memo, entry->amount) );

					//#1716181 also remove from the ptr_array (quickfilter)
					g_ptr_array_remove(data->gpatxn, (gpointer)entry);

					// 1) remove visible current and potential xfer
					gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
					if(data->showall && entry->paymode == PAYMODE_INTXFER)
					{
					Transaction *child = transaction_xfer_child_strong_get(entry);
						if( child )
						{
							txn_list_remove_by_value(model, child);
							//#1716181 also remove from the ptr_array (quickfilter)				
							g_ptr_array_remove(data->gpatxn, (gpointer)child);
						}
					}

					// 2) remove datamodel
					transaction_remove(entry);
					GLOBALS->changes_count++;

					list = g_list_previous(list);
				}

				g_list_foreach(selection, (GFunc)gtk_tree_path_free, NULL);
				g_list_free(selection);

				register_panel_update(widget, GINT_TO_POINTER(UF_BALANCE));
			}
		}
		break;

		//none
		case ACTION_ACCOUNT_NONE:
		{
		GtkTreeSelection *selection;
		gint count, result;
			
			count = txn_list_get_count_reconciled(GTK_TREE_VIEW(data->LV_ope));

			if(count > 0 )
			{
			
			result = ui_dialog_msg_confirm_alert(
					GTK_WINDOW(data->window),
					_("Are you sure you want to change the status to None?"),
					_("Some transaction in your selection are already Reconciled."),
					_("_Change")
				);
			}
			else
				result = GTK_RESPONSE_OK;
				
			if( result == GTK_RESPONSE_OK )
			{
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
				gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)status_selected_foreach_func, 
					GINT_TO_POINTER(TXN_STATUS_NONE));

				DB( g_print(" - none\n") );

				gtk_widget_queue_draw (data->LV_ope);
				//gtk_widget_queue_resize (data->LV_acc);

				register_panel_update(widget, GINT_TO_POINTER(UF_BALANCE));
				if( data->acc != NULL )
					data->acc->flags |= AF_CHANGED;
				GLOBALS->changes_count++;
			}

		}
		break;
		//clear
		case ACTION_ACCOUNT_CLEAR:
		{
			GtkTreeSelection *selection;
			
			selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
			gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)status_selected_foreach_func, 
				GINT_TO_POINTER(TXN_STATUS_CLEARED));

			DB( g_print(" - clear\n") );

			gtk_widget_queue_draw (data->LV_ope);
			//gtk_widget_queue_resize (data->LV_acc);

			register_panel_update(widget, GINT_TO_POINTER(UF_BALANCE));

			if( data->acc != NULL )
				data->acc->flags |= AF_CHANGED;
			GLOBALS->changes_count++;
		}
		break;


		//reconcile
		case ACTION_ACCOUNT_RECONCILE:
		{
		GtkTreeSelection *selection;
		gint count, result;
			
			count = txn_list_get_count_reconciled(GTK_TREE_VIEW(data->LV_ope));

			if(count > 0 )
			{
			
			result = ui_dialog_msg_confirm_alert(
					GTK_WINDOW(data->window),
					_("Are you sure you want to toggle the status Reconciled?"),
					_("Some transaction in your selection are already Reconciled."),
					_("_Toggle")
				);
			}
			else
				result = GTK_RESPONSE_OK;
				
			if( result == GTK_RESPONSE_OK )
			{
				selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
				gtk_tree_selection_selected_foreach(selection, (GtkTreeSelectionForeachFunc)status_selected_foreach_func, 
					GINT_TO_POINTER(TXN_STATUS_RECONCILED));

				DB( g_print(" - reconcile\n") );

				gtk_widget_queue_draw (data->LV_ope);
				//gtk_widget_queue_resize (data->LV_acc);

				register_panel_update(widget, GINT_TO_POINTER(UF_BALANCE));
				if( data->acc != NULL )
					data->acc->flags |= AF_CHANGED;
				GLOBALS->changes_count++;
			}

		}
		break;


		case ACTION_ACCOUNT_FILTER:
		{

			if(ui_flt_manage_dialog_new(data->window, data->filter, data->showall) != GTK_RESPONSE_REJECT)
			{
				register_panel_collect_filtered_txn(data->LV_ope);
				register_panel_listview_populate(data->LV_ope);
				register_panel_update(data->LV_ope, GINT_TO_POINTER(UF_SENSITIVE+UF_BALANCE));

				g_signal_handler_block(data->CY_range, data->handler_id[HID_RANGE]);
				gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_range), FLT_RANGE_OTHER);
				g_signal_handler_unblock(data->CY_range, data->handler_id[HID_RANGE]);
			}

		}
		break;

	}

	//refresh main
	if( GLOBALS->changes_count > changes )
		ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE));

}



static void register_panel_toggle_minor(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;

	DB( g_print("\n[account] toggle\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	register_panel_update(data->LV_ope, GINT_TO_POINTER(UF_BALANCE));
	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(data->LV_ope));
}


static void register_panel_selection(GtkTreeSelection *treeselection, gpointer user_data)
{

	DB( g_print("\n[account] selection changed cb\n") );


	register_panel_update(GTK_WIDGET(gtk_tree_selection_get_tree_view (treeselection)), GINT_TO_POINTER(UF_SENSITIVE));

}


static void register_panel_update(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;
GtkTreeSelection *selection;
gint flags = GPOINTER_TO_INT(user_data);
gint count = 0;

	DB( g_print("\n[account] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	//data = INST_DATA(widget);

	GLOBALS->minor = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_minor));

	/* set window title */
	if(flags & UF_TITLE)
	{
		DB( g_print(" - UF_TITLE\n") );

	}

	/* update disabled things */
	if(flags & UF_SENSITIVE)
	{
	gboolean	sensitive;

		DB( g_print(" - UF_SENSITIVE\n") );

		selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
		count = gtk_tree_selection_count_selected_rows(selection);
		DB( g_print(" - count = %d\n", count) );

		//showall part
		sensitive = !data->showall;
		gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/MenuBar/AccountMenu/ExportPDF"), sensitive);
		gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/MenuBar/AccountMenu/ExportQIF"), sensitive);
		gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/MenuBar/AccountMenu/ExportCSV"), sensitive);
		gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/MenuBar/ToolsMenu/ChkIntXfer"), sensitive);
		gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/MenuBar/ToolsMenu/DuplicateMark"), sensitive);
		//gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/MenuBar/ToolsMenu/DuplicateClear"), sensitive);

		//5.3.1 if closed account : disable any change
		sensitive =  TRUE;
		if( (data->showall == FALSE) && (data->acc->flags & AF_CLOSED) )
			sensitive = FALSE;
		
		gtk_widget_set_sensitive (data->TB_bar, sensitive);
		//gtk_widget_set_sensitive (data->TB_tools, sensitive);
		gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/ToolBar/Assign"), data->showall ? FALSE : sensitive);

		gtk_widget_set_sensitive(gtk_ui_manager_get_widget(data->ui, "/MenuBar/TxnMenu"), sensitive);
		gtk_widget_set_sensitive(gtk_ui_manager_get_widget(data->ui, "/MenuBar/ToolsMenu"), sensitive);

		// multiple: disable inherit, edit
		sensitive = (count != 1 ) ? FALSE : TRUE;
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/MenuBar/TxnMenu/Inherit"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/MenuBar/TxnMenu/Edit"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/TxnBar/Inherit"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/TxnBar/Edit"), sensitive);

		// single: disable multiedit
		sensitive = (count <= 1 ) ? FALSE : TRUE;
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/MenuBar/TxnMenu/MultiEdit"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/TxnBar/MultiEdit"), sensitive);

		// no selection: disable reconcile, delete
		sensitive = (count > 0 ) ? TRUE : FALSE;
		gtk_widget_set_sensitive(gtk_ui_manager_get_widget(data->ui, "/MenuBar/TxnMenu/TxnStatusMenu"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/MenuBar/TxnMenu/Delete"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/MenuBar/TxnMenu/Template"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/TxnBar/Delete"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/TxnBar/Cleared"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/TxnBar/Reconciled"), sensitive);
		gtk_action_set_sensitive(gtk_ui_manager_get_action(data->ui, "/TxnBar/Template"), sensitive);

		// euro convert
		sensitive = (data->showall == TRUE) ? FALSE : PREFS->euro_active;
		if( (data->acc != NULL) && currency_is_euro(data->acc->kcur) )
			sensitive = FALSE;
		gtk_action_set_visible(gtk_ui_manager_get_action(data->ui, "/MenuBar/ToolsMenu/ConvToEuro"), sensitive);

	}

	/* update toolbar & list */
	if(flags & UF_VISUAL)
	{
		DB( g_print(" - UF_VISUAL\n") );

		if(PREFS->toolbar_style == 0)
			gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_bar));
		else
			gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_bar), PREFS->toolbar_style-1);

		//minor ?
		if( PREFS->euro_active )
		{
			gtk_widget_show(data->CM_minor);
		}
		else
		{
			gtk_widget_hide(data->CM_minor);
		}
	}

	/* update balances */
	if(flags & UF_BALANCE)
	{
		DB( g_print(" - UF_BALANCE\n") );

		if(data->showall == FALSE)
		{
		Account *acc = data->acc;

			register_panel_balance_refresh(widget);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[0]), acc->bal_bank, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[1]), acc->bal_today, acc->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[2]), acc->bal_future, acc->kcur, GLOBALS->minor);
		}
		else
		{
		GList *lst_acc, *lnk_acc;
		gdouble bank, today, future;
	
			bank = today = future = 0.0;
			lst_acc = g_hash_table_get_values(GLOBALS->h_acc);
			lnk_acc = g_list_first(lst_acc);
			while (lnk_acc != NULL)
			{
			Account *acc = lnk_acc->data;

				bank += hb_amount_base(acc->bal_bank, acc->kcur);
				today += hb_amount_base(acc->bal_today, acc->kcur);
				future += hb_amount_base(acc->bal_future, acc->kcur);
				
				lnk_acc = g_list_next(lnk_acc);
			}
			g_list_free(lst_acc);
		
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[0]), bank, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[1]), today, GLOBALS->kcur, GLOBALS->minor);
			hb_label_set_colvalue(GTK_LABEL(data->TX_balance[2]), future, GLOBALS->kcur, GLOBALS->minor);
		}
		ui_mainwindow_populate_accounts(GLOBALS->mainwindow, NULL);
	}
	
	/* update fltinfo */
	DB( g_print(" - statusbar\n") );

	selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(data->LV_ope));
	count = gtk_tree_selection_count_selected_rows(selection);
	DB( g_print(" - nb selected = %d\n", count) );

	/* if more than one ope selected, we make a sum to display to the user */
	gdouble opeexp = 0.0;
	gdouble opeinc = 0.0;
	gchar buf1[64];
	gchar buf2[64];
	gchar buf3[64];
	gchar fbufavg[64];
	guint32 kcur;

	kcur = (data->showall == TRUE) ? GLOBALS->kcur : data->acc->kcur;


	if( count >= 1 )
	{
	GList *list, *tmplist;
	GtkTreeModel *model;
	GtkTreeIter iter;

		model = gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope));
		list = gtk_tree_selection_get_selected_rows(selection, &model);
		tmplist = g_list_first(list);
		while (tmplist != NULL)
		{
		Transaction *item;

			gtk_tree_model_get_iter(model, &iter, tmplist->data);
			gtk_tree_model_get(model, &iter, LST_DSPOPE_DATAS, &item, -1);

			if( data->showall == FALSE )
			{
				if( item->flags & OF_INCOME )
					opeinc += item->amount;
				else
					opeexp += item->amount;
			}
			else
			{
				if( item->flags & OF_INCOME )
					opeinc += hb_amount_base(item->amount, item->kcur);
				else
					opeexp += hb_amount_base(item->amount, item->kcur);
			}

			DB( g_print(" - %s, %.2f\n", item->memo, item->amount ) );

			tmplist = g_list_next(tmplist);
		}
		g_list_free(list);

		DB( g_print(" %f - %f = %f\n", opeinc, opeexp, opeinc + opeexp) );


		hb_strfmon(buf1, 64-1, opeinc, kcur, GLOBALS->minor);
		hb_strfmon(buf2, 64-1, -opeexp, kcur, GLOBALS->minor);
		hb_strfmon(buf3, 64-1, opeinc + opeexp, kcur, GLOBALS->minor);
		hb_strfmon(fbufavg, 64-1, (opeinc + opeexp) / count, kcur, GLOBALS->minor);
	}

	gchar *msg;

	if( count <= 1 )
	{
		msg = g_strdup_printf(_("%d transactions"), data->total);
	}
	else
		msg = g_strdup_printf(_("%d transactions, %d selected, avg: %s, sum: %s (%s - %s)"), data->total, count, fbufavg, buf3, buf1, buf2);

	gtk_label_set_markup(GTK_LABEL(data->TX_selection), msg);
	g_free (msg);

}


void register_panel_onRowActivated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *col, gpointer userdata)
{
struct register_panel_data *data;
GtkTreeModel *model;
GtkTreeIter iter;
gint col_id, count;
Transaction *ope;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(GTK_WIDGET(treeview), GTK_TYPE_WINDOW)), "inst_data");

	//5.3.1 if closed account : disable any change
	if( (data->showall == FALSE) && (data->acc->flags & AF_CLOSED) )
		return;


	col_id = gtk_tree_view_column_get_sort_column_id (col);

	count = gtk_tree_selection_count_selected_rows(gtk_tree_view_get_selection(treeview));

	model = gtk_tree_view_get_model(treeview);

	//get transaction double clicked to initiate the widget
	gtk_tree_model_get_iter(model, &iter, path);
	gtk_tree_model_get(model, &iter, LST_DSPOPE_DATAS, &ope, -1);


	DB( g_print ("%d rows been double-clicked on column=%d! ope=%s\n", count, col_id, ope->memo) );

	if( count == 1)
	{
		register_panel_action(GTK_WIDGET(treeview), GINT_TO_POINTER(ACTION_ACCOUNT_EDIT));
	}
	else
	{
		if( data->showall == FALSE )
		{
			if(col_id >= LST_DSPOPE_DATE && col_id != LST_DSPOPE_BALANCE)
			{
				register_panel_edit_multiple (data->window, ope, col_id, data);
			}
		}
	}
}


/*
static gint listview_context_cb (GtkWidget *widget, GdkEventButton *event, GtkWidget *menu)
{

	if (event->button == 3)
	{


		if (gtk_tree_view_get_path_at_pos (GTK_TREE_VIEW (treeview),
			(gint) event->x, (gint) event->y, &path, NULL, NULL, NULL))
		{
			gtk_tree_view_set_cursor (GTK_TREE_VIEW (treeview), path, NULL, FALSE);
			gtk_tree_path_free (path);
		}




				gtk_menu_popup (GTK_MENU(menu), NULL, NULL, NULL, NULL,
												event->button, event->time);

				// On indique à l'appelant que l'on a géré cet événement.

				return TRUE;
		}

		// On indique à l'appelant que l'on n'a pas géré cet événement.

		return FALSE;
}
*/


/*
** populate the account window
*/
void register_panel_window_init(GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data;

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("\n[account] init window\n") );

	if( data->showall == TRUE )
	{
		gtk_label_set_text (GTK_LABEL(data->LB_name), _("All transactions"));
		hb_widget_visible (data->IM_closed, FALSE);
	}
	else
	{
		gtk_label_set_text (GTK_LABEL(data->LB_name), data->acc->name);
		hb_widget_visible (data->IM_closed, (data->acc->flags & AF_CLOSED) ? TRUE : FALSE);

		DB( g_print(" - sort transactions\n") );
		da_transaction_queue_sort(data->acc->txn_queue);
	}

	list_txn_set_column_acc_visible(GTK_TREE_VIEW(data->LV_ope), data->showall);

	//DB( g_print(" mindate=%d, maxdate=%d %x\n", data->filter->mindate,data->filter->maxdate) );

	DB( g_print(" - call update visual\n") );
	register_panel_update(widget, GINT_TO_POINTER(UF_VISUAL|UF_SENSITIVE));

	DB( g_print(" - set range or populate+update sensitive+balance\n") );
	
	register_panel_cb_filter_reset(widget, user_data);

}

/*
**
*/
static gboolean 
register_panel_getgeometry(GtkWidget *widget, GdkEventConfigure *event, gpointer user_data)
{
//struct register_panel_data *data = user_data;
struct WinGeometry *wg;

	DB( g_print("\n[account] get geometry\n") );

	//store position and size
	wg = &PREFS->acc_wg;
	gtk_window_get_position(GTK_WINDOW(widget), &wg->l, &wg->t);
	gtk_window_get_size(GTK_WINDOW(widget), &wg->w, &wg->h);
	GdkWindow *gdk_window = gtk_widget_get_window(GTK_WIDGET(widget));
	GdkWindowState state = gdk_window_get_state(gdk_window);
	wg->s = (state & GDK_WINDOW_STATE_MAXIMIZED) ? 1 : 0;
	
	DB( g_print(" window: l=%d, t=%d, w=%d, h=%d s=%d, state=%d\n", wg->l, wg->t, wg->w, wg->h, wg->s, state & GDK_WINDOW_STATE_MAXIMIZED) );

	return FALSE;
}

/*
**
*/
static gboolean register_panel_dispose(GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
//struct register_panel_data *data = user_data;

	//data = g_object_get_data(G_OBJECT(widget), "inst_data");

	DB( g_print("\n[account] delete-event\n") );

	return FALSE;
}

/* Another callback */
static gboolean register_panel_destroy( GtkWidget *widget,
										 gpointer	 user_data )
{
struct register_panel_data *data;

	data = g_object_get_data(G_OBJECT(widget), "inst_data");


	DB( g_print ("\n[account] destroy event occurred\n") );



	//enable define windows
	GLOBALS->define_off--;

	/* unset transaction edit mutex */
	if(data->showall == FALSE)
		data->acc->window = NULL;
	else
		GLOBALS->alltxnwindow = NULL;

	/* free title and filter */
	DB( g_print(" user_data=%p to be free\n", user_data) );
	g_free(data->wintitle);

	if(data->gpatxn != NULL)
		g_ptr_array_free (data->gpatxn, TRUE);

	da_filter_free(data->filter);

	g_free(data);


	//our global list has changed, so update the treeview
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_TITLE+UF_SENSITIVE+UF_BALANCE+UF_REFRESHALL));

	return FALSE;
}


static void quick_search_text_changed_cb (GtkWidget *widget, gpointer user_data)
{
struct register_panel_data *data = user_data;

	register_panel_listview_populate (data->window);
}


static GtkActionEntry entries[] = {

	/* name, icon-name, label */
	{ "AccountMenu"	 , NULL, N_("A_ccount"), NULL, NULL, NULL },
	{ "TxnMenu"      , NULL, N_("Transacti_on"), NULL, NULL, NULL },
	{ "TxnStatusMenu", NULL, N_("_Status"), NULL, NULL, NULL },
	{ "ToolsMenu"	 , NULL, N_("_Tools"), NULL, NULL, NULL },

	
	/* name, icon-name, label, accelerator, tooltip */
	{ "ExportPDF"	, NULL               		, N_("Export as PDF..."), NULL,		N_("Export to a PDF file"), G_CALLBACK (register_panel_action_exportpdf) },
	{ "ExportQIF"	, NULL						, N_("Export QIF..."), NULL,		N_("Export as QIF"), G_CALLBACK (register_panel_action_exportqif) },
	{ "ExportCSV"	, NULL				   		, N_("Export CSV..."), NULL,		N_("Export as CSV"), G_CALLBACK (register_panel_action_exportcsv) },
	{ "Close" 	    , ICONNAME_CLOSE	        , N_("_Close")				, "<control>W", N_("Close the current account"),		G_CALLBACK (register_panel_action_close) },

	{ "Add"			, ICONNAME_HB_OPE_ADD	    , N_("_Add..."), NULL,		N_("Add a new transaction"), G_CALLBACK (register_panel_action_add) },
	{ "Inherit"		, ICONNAME_HB_OPE_HERIT	    , N_("_Inherit..."), NULL, N_("Inherit from the active transaction"), G_CALLBACK (register_panel_action_inherit) },
	{ "Edit"		, ICONNAME_HB_OPE_EDIT	    , N_("_Edit..."), NULL, N_("Edit the active transaction"),	G_CALLBACK (register_panel_action_edit) },

	{ "None"	    , NULL                      , N_("_None"), "<control>N",		N_("Toggle none for selected transaction(s)"), G_CALLBACK (register_panel_action_none) },
	{ "Cleared"	    , ICONNAME_HB_OPE_CLEARED   , N_("_Cleared"), "<control>C",		N_("Toggle cleared for selected transaction(s)"), G_CALLBACK (register_panel_action_clear) },
	{ "Reconciled"	, ICONNAME_HB_OPE_RECONCILED, N_("_Reconciled"), "<control>R",		N_("Toggle reconciled for selected transaction(s)"), G_CALLBACK (register_panel_action_reconcile) },

	{ "MultiEdit"	, ICONNAME_HB_OPE_MULTIEDIT , N_("_Multiple Edit..."), NULL, N_("Edit multiple transaction"),	G_CALLBACK (register_panel_action_multiedit) },
	{ "Template"    , ICONNAME_CONVERT          , N_("Create template..."), NULL,		N_("Create template"), G_CALLBACK (register_panel_action_createtemplate) },
	{ "Delete"		, ICONNAME_HB_OPE_DELETE    , N_("_Delete..."), NULL,		N_("Delete selected transaction(s)"), G_CALLBACK (register_panel_action_remove) },

	{ "DuplicateMark", NULL                      , N_("Mark duplicate..."), NULL,	NULL, G_CALLBACK (register_panel_action_duplicate_mark) },
//	{ "DuplicateClear", NULL                      , N_("Unmark duplicate"), NULL,	NULL, G_CALLBACK (register_panel_action_duplicate_unmark) },

	{ "ChkIntXfer"  , NULL                      , N_("Check internal xfer"), NULL,	NULL, G_CALLBACK (register_panel_action_check_internal_xfer) },
	{ "Assign"		, ICONNAME_HB_ASSIGN_RUN    , N_("Auto. assignments"), NULL,	N_("Run automatic assignments"), G_CALLBACK (register_panel_action_assign) },

	{ "Filter"		, ICONNAME_HB_FILTER        , N_("_Filter..."), NULL,			N_("Open the list filter"), G_CALLBACK (register_panel_action_editfilter) },
	{ "ConvToEuro"	, NULL                      , N_("Convert to Euro..."), NULL,	N_("Convert this account to Euro currency"), G_CALLBACK (register_panel_action_converttoeuro) },

};
static guint n_entries = G_N_ELEMENTS (entries);


static const gchar *ui_info =
"<ui>"
"<menubar name='MenuBar'>"
"	<menu action='AccountMenu'>"
"		<menuitem action='ExportQIF'/>"
"		<menuitem action='ExportCSV'/>"
"			<separator/>"
"		<menuitem action='ExportPDF'/>"
"			<separator/>"
"		<menuitem action='Close'/>"
"	</menu>"
"	<menu action='TxnMenu'>"
"		<menuitem action='Add'/>"
"		<menuitem action='Inherit'/>"
"		<menuitem action='Edit'/>"
"			<separator/>"
"	   <menu action='TxnStatusMenu'>"
"			<menuitem action='None'/>"
"			<menuitem action='Cleared'/>"
"			<menuitem action='Reconciled'/>"
"	   </menu>"
"			<separator/>"
"		<menuitem action='MultiEdit'/>"
"		<menuitem action='Template'/>"
"		<menuitem action='Delete'/>"
"	</menu>"
"	<menu action='ToolsMenu'>"
"		<menuitem action='DuplicateMark'/>"
//"		<menuitem action='DuplicateClear'/>"
"			<separator/>"
"		<menuitem action='ChkIntXfer'/>"
"	    	<separator/>"
"		<menuitem action='Filter'/>"
"		<menuitem action='Assign'/>"
"			<separator/>"
"		<menuitem action='ConvToEuro'/>"
"	</menu>"
"</menubar>"

"<toolbar name='TxnBar'>"
"	<toolitem action='Add'/>"
"	<toolitem action='Inherit'/>"
"	<toolitem action='Edit'/>"
"		<separator/>"
"	<toolitem action='Cleared'/>"
"	<toolitem action='Reconciled'/>"
"		<separator/>"
"	<toolitem action='MultiEdit'/>"
"	<toolitem action='Template'/>"
"	<toolitem action='Delete'/>"
"</toolbar>"
"<toolbar name='ToolBar'>"
"	<toolitem action='Filter'/>"
"	<toolitem action='Assign'/>"
"</toolbar>"
"</ui>";


/*
 * if accnum = 0 or acc is null : show all account 
 */
GtkWidget *register_panel_window_new(Account *acc)
{
struct register_panel_data *data;
struct WinGeometry *wg;
GtkWidget *window, *mainbox, *table, *sw, *bar;
GtkWidget *treeview, *label, *widget, *image;
//GtkWidget *menu, *menu_items;
GtkUIManager *ui;
GtkActionGroup *actions;
GtkAction *action;
GError *error = NULL;

	DB( g_print("\n[account] create_register_panel_window\n") );

	data = g_malloc0(sizeof(struct register_panel_data));
	if(!data) return NULL;

	//disable define windows
	GLOBALS->define_off++;
	ui_mainwindow_update(GLOBALS->mainwindow, GINT_TO_POINTER(UF_SENSITIVE));

	/* create window, etc */
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	data->window = window;

	//store our window private data
	g_object_set_data(G_OBJECT(window), "inst_data", (gpointer)data);
	DB( g_print(" - new window=%p, inst_data=%p\n", window, data) );

	data->acc = acc;
	data->showall = (acc != NULL) ? FALSE : TRUE;
	
	if(data->showall == FALSE)
	{
		data->acc->window = GTK_WINDOW(window);
		if( data->acc->flags & AF_CLOSED )
			data->wintitle = g_strdup_printf("%s %s - HomeBank", data->acc->name, _("(closed)"));
		else
			data->wintitle = g_strdup_printf("%s - HomeBank", data->acc->name);
	}
	else
	{
		GLOBALS->alltxnwindow = window;
		data->wintitle = g_strdup_printf(_("%s - HomeBank"), _("All transactions"));
	}

	gtk_window_set_title (GTK_WINDOW (window), data->wintitle);
	gtk_window_set_icon_name(GTK_WINDOW (window), ICONNAME_HB_OPE_SHOW );

	// connect our dispose function
	g_signal_connect (window, "delete-event",
		G_CALLBACK (register_panel_dispose), (gpointer)data);

	// connect our dispose function
	g_signal_connect (window, "destroy",
		G_CALLBACK (register_panel_destroy), (gpointer)data);

	// connect our dispose function
	g_signal_connect (window, "configure-event",
		G_CALLBACK (register_panel_getgeometry), (gpointer)data);

#if UI == 1
	//start test uimanager

		actions = gtk_action_group_new ("Account");

		//as we use gettext
		gtk_action_group_set_translation_domain(actions, GETTEXT_PACKAGE);


		DB( g_print(" - add actions: %p user_data: %p\n", actions, data) );
		gtk_action_group_add_actions (actions, entries, n_entries, data);

		/* set which action should have priority in the toolbar */
		action = gtk_action_group_get_action(actions, "Add");
		g_object_set(action, "is_important", TRUE, "short_label", _("Add"), NULL);

		action = gtk_action_group_get_action(actions, "Inherit");
		g_object_set(action, "is_important", TRUE, "short_label", _("Inherit"), NULL);

		action = gtk_action_group_get_action(actions, "Edit");
		g_object_set(action, "is_important", TRUE, "short_label", _("Edit"), NULL);

		action = gtk_action_group_get_action(actions, "Filter");
		g_object_set(action, "is_important", TRUE, "short_label", _("Filter"), NULL);

		//action = gtk_action_group_get_action(actions, "Reconciled");
		//g_object_set(action, "is_important", TRUE, "short_label", _("Reconciled"), NULL);


		ui = gtk_ui_manager_new ();

		DB( g_print(" - insert action group:\n") );
		gtk_ui_manager_insert_action_group (ui, actions, 0);

			GtkAccelGroup *ag = gtk_ui_manager_get_accel_group (ui);

			DB( g_print(" - add_accel_group actions=%p, ui=%p, ag=%p\n", actions, ui, ag) );

			gtk_window_add_accel_group (GTK_WINDOW (window), ag);

		DB( g_print(" - add ui from string:\n") );
	if (!gtk_ui_manager_add_ui_from_string (ui, ui_info, -1, &error))
	{
		g_message ("building menus failed: %s", error->message);
		g_error_free (error);
	}

	data->ui = ui;
	data->actions = actions;
#endif

	mainbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
	//gtk_container_set_border_width(GTK_CONTAINER(mainbox), SPACING_SMALL);
	gtk_container_add (GTK_CONTAINER (window), mainbox);

	widget = gtk_ui_manager_get_widget (ui, "/MenuBar");
	//data->menu = widget;
	gtk_box_pack_start (GTK_BOX (mainbox), widget, FALSE, FALSE, 0);

	// info bar for duplicate
	bar = gtk_info_bar_new_with_buttons (_("_Refresh"), HB_RESPONSE_REFRESH, NULL);
	data->IB_duplicate = bar;
	gtk_box_pack_start (GTK_BOX (mainbox), bar, FALSE, FALSE, 0);

	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	gtk_info_bar_set_show_close_button (GTK_INFO_BAR (bar), TRUE);
	label = gtk_label_new ("This is an info bar with message type GTK_MESSAGE_WARNING");
	data->LB_duplicate = label;
      gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
      gtk_label_set_xalign (GTK_LABEL (label), 0);
      gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, FALSE, FALSE, 0);

		widget = make_numeric(NULL, 0, HB_DATE_MAX_GAP);
		data->NB_txn_daygap = widget;
		gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), widget, FALSE, FALSE, 0);

	table = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	gtk_container_set_border_width (GTK_CONTAINER(table), SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (mainbox), table, FALSE, FALSE, 0);	

	// account name (+ balance)
	widget = gtk_image_new_from_icon_name (ICONNAME_CHANGES_PREVENT, GTK_ICON_SIZE_BUTTON);
	data->IM_closed = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 0, 0, 1, 1);

	label = gtk_label_new(NULL);
	data->LB_name = label;
	gimp_label_set_attributes (GTK_LABEL (label), PANGO_ATTR_SCALE, PANGO_SCALE_LARGE, -1);
	gtk_widget_set_halign (label, GTK_ALIGN_START);
	gtk_widget_set_hexpand (label, TRUE);
	gtk_grid_attach (GTK_GRID(table), label, 1, 0, 1, 1);

	/* balances area */
	label = gtk_label_new(_("Bank:"));
	gtk_grid_attach (GTK_GRID(table), label, 3, 0, 1, 1);
	widget = gtk_label_new(NULL);
	data->TX_balance[0] = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 4, 0, 1, 1);

	label = gtk_label_new(_("Today:"));
	gtk_grid_attach (GTK_GRID(table), label, 5, 0, 1, 1);
	widget = gtk_label_new(NULL);
	data->TX_balance[1] = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 6, 0, 1, 1);

	label = gtk_label_new(_("Future:"));
	gtk_grid_attach (GTK_GRID(table), label, 7, 0, 1, 1);

	widget = gtk_label_new(NULL);
	data->TX_balance[2] = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 8, 0, 1, 1);

	//quick search
	widget = make_search ();
	data->ST_search = widget;
	gtk_widget_set_size_request(widget, HB_MINWIDTH_SEARCH, -1);
	gtk_grid_attach (GTK_GRID(table), widget, 9, 0, 1, 1);

	data->handler_id[HID_SEARCH] = g_signal_connect (data->ST_search, "search-changed", G_CALLBACK (quick_search_text_changed_cb), data);


	// windows interior
	table = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (table), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (table), SPACING_MEDIUM);
	gtk_container_set_border_width (GTK_CONTAINER(table), SPACING_SMALL);
	gtk_box_pack_start (GTK_BOX (mainbox), table, FALSE, FALSE, 0);

	label = make_label_widget(_("_Range:"));
	gtk_grid_attach (GTK_GRID(table), label, 0, 0, 1, 1);
	data->CY_range = make_daterange(label, TRUE);
	gtk_grid_attach (GTK_GRID(table), data->CY_range, 1, 0, 1, 1);

	widget = gtk_toggle_button_new();
	image = gtk_image_new_from_icon_name (ICONNAME_HB_OPE_FUTURE, GTK_ICON_SIZE_MENU);
	g_object_set (widget, "image", image,  NULL);
	gtk_widget_set_tooltip_text (widget, _("Toggle show future transaction"));
	data->CM_future = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 2, 0, 1, 1);

	label = make_label_widget(_("_Type:"));
	gtk_grid_attach (GTK_GRID(table), label, 3, 0, 1, 1);
	data->CY_type = make_cycle(label, CYA_FLT_TYPE);
	gtk_grid_attach (GTK_GRID(table), data->CY_type, 4, 0, 1, 1);

	label = make_label_widget(_("_Status:"));
	gtk_grid_attach (GTK_GRID(table), label, 5, 0, 1, 1);
	data->CY_status = make_cycle(label, CYA_FLT_STATUS);
	gtk_grid_attach (GTK_GRID(table), data->CY_status, 6, 0, 1, 1);

	//widget = gtk_button_new_with_mnemonic (_("Reset _filters"));
	widget = gtk_button_new_with_mnemonic (_("_Reset"));
	data->BT_reset = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 7, 0, 1, 1);

	//TRANSLATORS: this is for Euro specific users, a toggle to display in 'Minor' currency
	widget = gtk_check_button_new_with_mnemonic (_("Euro _minor"));
	data->CM_minor = widget;
	gtk_grid_attach (GTK_GRID(table), widget, 8, 0, 1, 1);

	label = make_label(NULL, 0.0, 0.5);
	data->TX_selection = label;
	gtk_widget_set_halign (label, GTK_ALIGN_END);
	gtk_widget_set_hexpand (label, TRUE);
	gtk_grid_attach (GTK_GRID(table), label, 10, 0, 1, 1);

	/*
	label = make_label_widget(_("_Month:"));
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	data->CY_month = make_cycle(label, CYA_SELECT);
		gtk_box_pack_start (GTK_BOX (hbox), data->CY_month, FALSE, FALSE, 0);

	label = make_label_widget(_("_Year:"),);
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
	data->NB_year = make_year(label);
		gtk_box_pack_start (GTK_BOX (hbox), data->NB_year, FALSE, FALSE, 0);
	*/

	//list
	sw = gtk_scrolled_window_new (NULL, NULL);
	gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (sw), GTK_SHADOW_ETCHED_IN);
	gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW (sw), GTK_POLICY_AUTOMATIC, GTK_POLICY_ALWAYS);
	treeview = (GtkWidget *)create_list_transaction(LIST_TXN_TYPE_BOOK, PREFS->lst_ope_columns);
	data->LV_ope = treeview;
	gtk_container_add (GTK_CONTAINER (sw), treeview);
	gtk_box_pack_start (GTK_BOX (mainbox), sw, TRUE, TRUE, 0);

	list_txn_set_save_column_width(GTK_TREE_VIEW(treeview), TRUE);
	
	/* toolbars */
	table = gtk_grid_new();
	gtk_box_pack_start (GTK_BOX (mainbox), table, FALSE, FALSE, 0);
	
	widget = gtk_ui_manager_get_widget (ui, "/TxnBar");
	data->TB_bar = widget;
	//gtk_widget_set_halign (widget, GTK_ALIGN_START);
	//gtk_style_context_add_class (gtk_widget_get_style_context (widget), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID(table), widget, 0, 0, 1, 1);

	widget = gtk_ui_manager_get_widget (ui, "/ToolBar");
	data->TB_tools = widget;
	//gtk_widget_set_halign (widget, GTK_ALIGN_END);
	//gtk_style_context_add_class (gtk_widget_get_style_context (widget), GTK_STYLE_CLASS_INLINE_TOOLBAR);
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID(table), widget, 2, 0, 1, 1);

    #ifdef G_OS_WIN32
    if(PREFS->toolbar_style == 0)
    {
        gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_bar));
        gtk_toolbar_unset_style(GTK_TOOLBAR(data->TB_tools));
    }
    else
    {
        gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_bar), PREFS->toolbar_style-1);
        gtk_toolbar_set_style(GTK_TOOLBAR(data->TB_tools), PREFS->toolbar_style-1);
    }
    #endif

	//todo: should move this
	//setup
	g_object_set_data(G_OBJECT(gtk_tree_view_get_model(GTK_TREE_VIEW(data->LV_ope))), "minor", data->CM_minor);
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_future), (PREFS->date_future_nbdays > 0) ? TRUE : FALSE );
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_minor), GLOBALS->minor);
	gtk_widget_grab_focus(GTK_WIDGET(data->LV_ope));

	// connect signals
	g_signal_connect (data->IB_duplicate    , "response", G_CALLBACK (register_panel_cb_bar_duplicate_response), NULL);

	data->handler_id[HID_RANGE]	 = g_signal_connect (data->CY_range , "changed", G_CALLBACK (register_panel_cb_filter_daterange), NULL);
	data->handler_id[HID_TYPE]	 = g_signal_connect (data->CY_type	, "changed", G_CALLBACK (register_panel_cb_filter_type), NULL);
	data->handler_id[HID_STATUS] = g_signal_connect (data->CY_status, "changed", G_CALLBACK (register_panel_cb_filter_status), NULL);

	g_signal_connect (data->CM_future, "toggled", G_CALLBACK (register_panel_cb_filter_daterange), NULL);

	g_signal_connect (data->BT_reset , "clicked", G_CALLBACK (register_panel_cb_filter_reset), NULL);

	g_signal_connect (data->CM_minor , "toggled", G_CALLBACK (register_panel_toggle_minor), NULL);

	//g_signal_connect (GTK_TREE_VIEW(treeview), "cursor-changed", G_CALLBACK (register_panel_update), (gpointer)2);
	g_signal_connect (gtk_tree_view_get_selection(GTK_TREE_VIEW(treeview)), "changed", G_CALLBACK (register_panel_selection), NULL);
	g_signal_connect (GTK_TREE_VIEW(treeview), "row-activated", G_CALLBACK (register_panel_onRowActivated), GINT_TO_POINTER(2));


//todo: test context menu
	/*
	menu = gtk_menu_new();
	menu_items = gtk_ui_manager_get_widget (ui, "/MenuBar/TxnMenu/Add");

	menu_items = gtk_menu_item_new_with_label ("test");
	gtk_widget_show(menu_items);
	gtk_menu_shell_append (GTK_MENU (menu), menu_items);

	//todo: debug test
	g_signal_connect (treeview, "button-press-event", G_CALLBACK (listview_context_cb),
		// todo: here is not a GtkMenu but GtkImageMenuItem...
		menu
		//gtk_ui_manager_get_widget (ui, "/MenuBar")
	);
	*/

	//setup, init and show window
	wg = &PREFS->acc_wg;
	if(wg->s == 0)
	{
		gtk_window_move(GTK_WINDOW(window), wg->l, wg->t);
		gtk_window_resize(GTK_WINDOW(window), wg->w, wg->h);
	}
	else
		gtk_window_maximize(GTK_WINDOW(window));
	
	gtk_widget_show_all (window);
	gtk_widget_hide(data->IB_duplicate);
	
	/* hide showfuture */
	hb_widget_visible (data->CM_future, PREFS->date_future_nbdays > 0 ? TRUE : FALSE);


	/* make sure splash is up */
	while (gtk_events_pending ())
		gtk_main_iteration ();

	/* setup to moove later */
	data->filter = da_filter_malloc();
	DB( g_print(" - filter ok %p\n", data->filter) );


	return window;
}
