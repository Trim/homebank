/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2016 Maxime DOYEN
 *
 *  This file is part of HomeBank.
 *
 *  HomeBank is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  HomeBank is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty ofdeftransaction_amountchanged
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "homebank.h"

#include "ui-transaction.h"
#include "hb-transaction.h"
#include "gtk-dateentry.h"
#include "ui-payee.h"
#include "ui-category.h"
#include "ui-account.h"
#include "ui-split.h"


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


extern gchar *CYA_TYPE[];

gchar *CYA_OPERATION[] = {
	N_("Add transaction"),
	N_("Inherit transaction"),
	N_("Modify transaction")
};


gchar *CYA_TXN_STATUS[] = {
	N_("None"),
	N_("Cleared"),
	N_("Reconciled"),
	N_("Remind"),
	NULL
};


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

static void deftransaction_update(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gboolean sensitive;

	DB( g_print("\n[ui-transaction] update\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	//# 1419476 empty category when no split either...
	if( (data->ope->flags & (OF_SPLIT)) )
	{
		//# 1416624 empty category when split
		ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_grp), 0);
	}

	/* disable amount+category if split is set */
	sensitive = (data->ope->flags & (OF_SPLIT)) ? FALSE : TRUE;
	gtk_widget_set_sensitive(data->ST_amount, sensitive);
	gtk_widget_set_sensitive(data->PO_grp, sensitive);
}

static void deftransaction_update_warnsign(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gboolean warning = FALSE;
gdouble amount;
gint amttype;
Category *cat;

	DB( g_print("\n[ui-transaction] update warning sign\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	cat = ui_cat_comboboxentry_get(GTK_COMBO_BOX(data->PO_grp));
	if(cat != NULL && cat->key > 0)
	{
		amount = hb_amount_round(gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount)), 2);
		if(amount != 0.0)
		{
			amttype = (amount > 0) ? 1 : -1;
			warning = (category_type_get(cat) != amttype) ? TRUE : FALSE;
		}
	}

	if(warning)
		gtk_widget_show_all(data->IB_warnsign);
	else
		gtk_widget_hide(data->IB_warnsign);

}


static void deftransaction_update_transfer(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gboolean sensitive;
guint kacc, kdst;

	DB( g_print("\n[ui-transaction] update transfer\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	sensitive = TRUE;

	kacc = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));

	if(kacc == 0) { sensitive = FALSE; goto end; }

	/* coherent seizure
	 * - target account selected
	 * - source != target
	 * - same currency
	 */
	if( gtk_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode)) == PAYMODE_INTXFER )
	{
	Account *srcacc, *dstacc;

		kdst = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_accto));

		if(kdst == 0) { sensitive = FALSE; goto end; }
		if(kdst == kacc) {
			sensitive = FALSE;
			goto end;
		}

		srcacc = da_acc_get(kacc);
		dstacc = da_acc_get(kdst);
		if(srcacc->kcur != dstacc->kcur) {
			sensitive = FALSE;
		}
	}

end:
	DB( g_print(" sensitive %d\n", sensitive) );

	//#1437551
	//gtk_widget_set_sensitive(gtk_dialog_get_action_area(GTK_DIALOG (data->window)), sensitive);
	gtk_dialog_set_response_sensitive(GTK_DIALOG (data->window), GTK_RESPONSE_ACCEPT, sensitive);
	gtk_dialog_set_response_sensitive(GTK_DIALOG (data->window), GTK_RESPONSE_ADD, sensitive);
	gtk_dialog_set_response_sensitive(GTK_DIALOG (data->window), GTK_RESPONSE_ADDKEEP, sensitive);

}


static void deftransaction_update_accto(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
guint kacc;

	DB( g_print("\n[ui-transaction] update accto\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	kacc = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));

	DB( g_print(" acc is %d\n", kacc) );

	g_signal_handlers_block_by_func (G_OBJECT (data->PO_accto), G_CALLBACK (deftransaction_update_accto), NULL);
	ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_accto), 0);
	g_signal_handlers_unblock_by_func (G_OBJECT (data->PO_accto), G_CALLBACK (deftransaction_update_accto), NULL);

	ui_acc_comboboxentry_populate_except(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, kacc, ACC_LST_INSERT_NORMAL);

	deftransaction_update_transfer(widget, user_data);
}



void deftransaction_set_amount_from_split(GtkWidget *widget, gdouble amount)
{
struct deftransaction_data *data;

	DB( g_print("\n[ui-transaction] set_amount_from_split\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	DB( g_print("- amount=%.2f\n", amount) );

	data->ope->amount = amount;
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), amount);
	
	deftransaction_update(widget, NULL);
	
}


static void deftransaction_set(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Transaction *entry;
gchar *tagstr, *txt;

	DB( g_print("\n[ui-transaction] set\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	entry = data->ope;

	DB( g_print(" - ope=%p data=%p tags:%p\n", data->ope, data, entry->tags) );

	//DB( g_print(" set date to %d\n", entry->date) );
	//g_object_set(GTK_DATE_ENTRY(data->PO_date), "date", (guint32)entry->ope_Date);
	gtk_date_entry_set_date(GTK_DATE_ENTRY(data->PO_date), (guint)entry->date);

	txt = (entry->wording != NULL) ? entry->wording : "";
	gtk_entry_set_text(GTK_ENTRY(data->ST_word), txt);
	gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), entry->amount);
	//gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_amount), (entry->ope_Flags & OF_INCOME) ? 1 : 0);

	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->CM_cheque), (entry->flags & OF_CHEQ2) ? 1 : 0);

	txt = (entry->info != NULL) ? entry->info : "";
	gtk_entry_set_text(GTK_ENTRY(data->ST_info), txt);
	ui_cat_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_grp), entry->kcat);
	ui_pay_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_pay), entry->kpay);

	tagstr = transaction_tags_tostring(entry);

	DB( g_print(" - tags: '%s'\n", txt) );

	txt = (tagstr != NULL) ? tagstr : "";
	gtk_entry_set_text(GTK_ENTRY(data->ST_tags), txt);
	g_free(tagstr);

	gtk_combo_box_set_active(GTK_COMBO_BOX(data->CY_status), entry->status );
	
	//as we trigger an event on this
	//let's place it at the end to avoid misvalue on the trigger function

	ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_acc), entry->kacc);
	ui_acc_comboboxentry_set_active(GTK_COMBO_BOX(data->PO_accto), entry->kxferacc);

	gtk_combo_box_set_active(GTK_COMBO_BOX(data->NU_mode), entry->paymode);

	DB( g_print(" - acc is: %d\n", gtk_combo_box_get_active(GTK_COMBO_BOX(data->PO_acc)) ) );
}


void deftransaction_get(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Transaction *entry;
gchar *txt;
gdouble value;
gint active;

	DB( g_print("\n[ui-transaction] get\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	entry = data->ope;

	DB( g_print(" - ope = %p\n", entry) );

	//DB( g_print(" get date to %d\n", entry->ope_Date) );
	entry->date = gtk_date_entry_get_date(GTK_DATE_ENTRY(data->PO_date));
	//g_object_get(GTK_DATE_ENTRY(data->PO_date), "date", entry->ope_Date);

	//free any previous string
	if(	entry->wording )
	{
		g_free(entry->wording);
		entry->wording = NULL;
	}
	txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_word));
	// ignore if entry is empty
	if (txt && *txt)
	{
		entry->wording = g_strdup(txt);
	}

	entry->paymode    = gtk_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));
	if( entry->paymode != PAYMODE_INTXFER )
	{
		//#677351: revert kxferacc to 0
		entry->kxferacc = 0;
	}

	value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
	entry->amount = value;

	/* for internal transfer add, amount must be expense */
	// #617936
	/*
	if( entry->paymode == PAYMODE_INTXFER && data->type == OPERATION_EDIT_ADD )
	{
		if( entry->amount > 0 )
			entry->amount *= -1;
	}
	*/

	//free any previous string
	if(	entry->info )
	{
		g_free(entry->info);
		entry->info = NULL;
	}
	txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_info));
	// ignore if entry is empty
	if (txt && *txt)
	{
		entry->info = g_strdup(txt);
	}

	entry->kcat     = ui_cat_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_grp));
	entry->kpay     = ui_pay_comboboxentry_get_key_add_new(GTK_COMBO_BOX(data->PO_pay));
	entry->kacc     = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));
	entry->kxferacc = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_accto));

	/* tags */
	txt = (gchar *)gtk_entry_get_text(GTK_ENTRY(data->ST_tags));
	DB( g_print(" - tags: '%s'\n", txt) );
	transaction_tags_parse(entry, txt);

	entry->status = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_status));
	
	/* flags */
	//entry->flags = 0;
	entry->flags &= (OF_SPLIT);	//(split is set in hb_transaction)

	if(	data->type == TRANSACTION_EDIT_ADD || data->type == TRANSACTION_EDIT_INHERIT)
	entry->flags |= OF_ADDED;

	if(	data->type == TRANSACTION_EDIT_MODIFY)
	entry->flags |= OF_CHANGED;

	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cheque));
	if(active == 1) entry->flags |= OF_CHEQ2;

	//active = gtk_combo_box_get_active(GTK_COMBO_BOX(data->CY_amount));
	active = entry->amount > 0 ? TRUE : FALSE;
	if(active == TRUE) entry->flags |= OF_INCOME;

}




static gboolean deftransaction_amount_focusout(GtkWidget *widget, GdkEventFocus *event, gpointer user_data)
{
struct deftransaction_data *data;
gushort paymode;
gdouble amount;

	DB( g_print("\n[ui-transaction] amount focus-out-event %d\n", gtk_widget_is_focus(widget)) );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	paymode    = gtk_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));

	// for internal transfer add, amount must be expense by default
	if( paymode == PAYMODE_INTXFER && data->type == TRANSACTION_EDIT_ADD )
	{
		amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		if(amount > 0)
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), amount *= -1);
	}

	deftransaction_update_warnsign(widget, NULL);
	
	return FALSE;
}


static void deftransaction_toggleamount(GtkWidget *widget, GtkEntryIconPosition icon_pos, GdkEvent *event, gpointer user_data)
{
struct deftransaction_data *data;
guint count, i;
Split *split;
gdouble value;

	DB( g_print("\n[ui-transaction] toggleamount\n") );

	if(icon_pos == GTK_ENTRY_ICON_PRIMARY)
	{
		data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

		gtk_spin_button_update(GTK_SPIN_BUTTON(data->ST_amount));
		
		value = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		value *= -1;
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), value);
	
		if( data->ope->flags & OF_SPLIT )
		{
			count = da_splits_count(data->ope->splits);
			DB( g_print("- count = %d\n", count) );
			for(i=0;i<count;i++)
			{
				split = data->ope->splits[i];
				split->amount *= -1;
			}
		}

		deftransaction_update_warnsign(widget, NULL);
	}

}


static void deftransaction_button_split_cb(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gdouble amount;

	DB( g_print("\n[ui-transaction] doing split\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));

	ui_split_dialog(data->window, data->ope->splits, amount, &deftransaction_set_amount_from_split);

	deftransaction_update(data->window, NULL);	
}




/*
**
*/
static void deftransaction_paymode(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
gint payment;
gint page;
gboolean sensitive;

	DB( g_print("\n[ui-transaction] paymode change\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	payment = gtk_combo_box_get_active(GTK_COMBO_BOX(data->NU_mode));
	page = 0;

	/* todo: prefill the cheque number ? */
	if( data->type != TRANSACTION_EDIT_MODIFY )
	{
	gboolean expense = (gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount)) > 0 ? FALSE : TRUE);

		DB( g_print(" - payment: %d\n", PAYMODE_CHECK) );
		DB( g_print(" - expense: %d\n", expense) );
		DB( g_print(" - acc is: %d\n", ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc)) ) );

		if(payment == PAYMODE_CHECK)
		{
			if(expense == TRUE)
			{
			Account *acc;
			gint active = ui_acc_comboboxentry_get_key(GTK_COMBO_BOX(data->PO_acc));
			guint cheque;
			gchar *cheque_str;

				DB( g_print(" - should fill cheque number for account %d\n", active) );

				//#1410166
				if( active > 0 )
				{
					acc = da_acc_get( active );
					if(acc != NULL)
					{
						cheque = ( gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->CM_cheque))==TRUE ? acc->cheque2 : acc->cheque1 );
						cheque_str = g_strdup_printf("%d", cheque + 1);
						gtk_entry_set_text(GTK_ENTRY(data->ST_info), cheque_str);
						g_free(cheque_str);
					}
				}
			}
		}
	}


	if(payment == PAYMODE_CHECK)
		page = 1;

	sensitive = (payment == PAYMODE_INTXFER) ? FALSE : TRUE;
	gtk_widget_set_sensitive(data->BT_split, sensitive);

	sensitive = page == 1 ? TRUE : FALSE;
	hb_widget_visible(data->CM_cheque, sensitive);
	
	if(payment == PAYMODE_INTXFER)
	{
		page = 2;
		// for internal transfer add, amount must be expense by default
		gdouble amount = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->ST_amount));
		if( data->type == TRANSACTION_EDIT_ADD )
		{
			if(amount > 0)
				gtk_spin_button_set_value(GTK_SPIN_BUTTON(data->ST_amount), amount *= -1);
		}
		//else //#1460370
		//if( data->type == TRANSACTION_EDIT_MODIFY )
	//	{
			if(amount > 0)
			{
				gtk_label_set_text_with_mnemonic (GTK_LABEL(data->LB_accto), _("From acc_ount:"));
			}
			else
			{
				gtk_label_set_text_with_mnemonic (GTK_LABEL(data->LB_accto), _("To acc_ount:"));	
			}
		//}		
		
		
	}

	deftransaction_update_accto(widget, user_data);
	DB( g_print(" payment: %d, page: %d\n", payment, page) );

	sensitive = page == 2 ? TRUE : FALSE;
	hb_widget_visible(data->LB_accto, sensitive);
	hb_widget_visible(data->PO_accto, sensitive);

}


static void deftransaction_fillfrom(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;
Transaction *entry;
Archive *arc;
gint n_arc;

	DB( g_print("\n[ui-transaction] fill from\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");
	entry = data->ope;

	n_arc = gtk_combo_box_get_active(GTK_COMBO_BOX(data->PO_arc));

	DB( g_print(" fill from %d\n", n_arc) );

	if(n_arc > 0)
	{
		arc = g_list_nth_data(GLOBALS->arc_list, n_arc-1);

		da_transaction_init_from_template(entry, arc);
		if( da_splits_clone(arc->splits, entry->splits) > 0)
			entry->flags |= OF_SPLIT; //Flag that Splits are active

		DB( g_print(" calls\n") );

		deftransaction_set(widget, NULL);
		deftransaction_paymode(widget, NULL);
		deftransaction_update(widget, NULL);

		gtk_combo_box_set_active(GTK_COMBO_BOX(data->PO_arc), 0);
	}
}


/*
** called from outside
*/
void deftransaction_set_transaction(GtkWidget *widget, Transaction *ope)
{
struct deftransaction_data *data;


	DB( g_print("\n[ui-transaction] set transaction (from out)\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	data->ope = ope;

	DB( g_print(" - ope=%p data=%p\n", data->ope, data) );

	DB( g_print(" - call init\n") );

	deftransaction_set(widget, NULL);
	deftransaction_paymode(widget, NULL);
	deftransaction_update(widget, NULL);
	deftransaction_update_warnsign(widget, NULL);

}


void deftransaction_dispose(GtkWidget *widget, gpointer user_data)
{
struct deftransaction_data *data;

	DB( g_print("\n[ui-transaction] dispose\n") );

	data = g_object_get_data(G_OBJECT(gtk_widget_get_ancestor(widget, GTK_TYPE_WINDOW)), "inst_data");

	g_free(data);
}


static void deftransaction_setup(struct deftransaction_data *data)
{

	DB( g_print("\n[ui-transaction] setup\n") );

    gtk_window_set_title (GTK_WINDOW (data->window), _(CYA_OPERATION[data->type]));

	ui_pay_comboboxentry_populate(GTK_COMBO_BOX(data->PO_pay), GLOBALS->h_pay);
	ui_cat_comboboxentry_populate(GTK_COMBO_BOX(data->PO_grp), GLOBALS->h_cat);
	ui_acc_comboboxentry_populate(GTK_COMBO_BOX(data->PO_acc), GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);
	ui_acc_comboboxentry_populate(GTK_COMBO_BOX(data->PO_accto), GLOBALS->h_acc, ACC_LST_INSERT_NORMAL);

	if( data->showtemplate )
		make_poparchive_populate(GTK_COMBO_BOX(data->PO_arc), GLOBALS->arc_list);

}


static GtkWidget *deftransaction_make_block1(struct deftransaction_data *data)
{
GtkWidget *group_grid, *hbox, *label, *widget, *image;
gint row;

	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);

	row = 0;
	label = make_label_widget(_("_Date:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = gtk_date_entry_new();
	data->PO_date = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	gtk_widget_set_tooltip_text(widget, _("Date accepted here are:\nday,\nday/month or month/day,\nand complete date into your locale"));

	row++;
	label = make_label_widget(_("_Amount:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);

	hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, 0);
	gtk_grid_attach (GTK_GRID (group_grid), hbox, 1, row, 1, 1);
	gtk_widget_set_hexpand (hbox, TRUE);

		widget = make_amount(label);
		data->ST_amount = widget;
		gtk_entry_set_icon_from_icon_name(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, ICONNAME_HB_TOGGLE_SIGN);
		gtk_entry_set_icon_tooltip_text(GTK_ENTRY(widget), GTK_ENTRY_ICON_PRIMARY, _("Toggle amount sign"));
		gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

		image = gtk_image_new_from_icon_name (ICONNAME_HB_BUTTON_SPLIT, GTK_ICON_SIZE_BUTTON);
		widget = gtk_button_new();
		g_object_set (widget, "image", image, NULL);
		data->BT_split = widget;
		gtk_box_pack_start (GTK_BOX (hbox), widget, FALSE, FALSE, 0);
		gtk_widget_set_tooltip_text(widget, _("Transaction splits"));

	row++;
	label = make_label_widget(_("Pa_yment:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_paymode(label);
	data->NU_mode = widget;
		gtk_widget_set_hexpand (widget, TRUE);

	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	widget = gtk_check_button_new_with_mnemonic(_("Of notebook _2"));
	data->CM_cheque = widget;
		gtk_widget_set_hexpand (widget, TRUE);

	gtk_grid_attach (GTK_GRID (group_grid), widget, 0, row, 2, 1);

	row++;
	label = make_label_widget(_("_Info:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_string(label);
	data->ST_info = widget;
	gtk_widget_set_hexpand (widget, TRUE);

	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("Acc_ount:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = ui_acc_comboboxentry_new(label);
	data->PO_acc = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("To acc_ount:"));
	data->LB_accto = label;
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = ui_acc_comboboxentry_new(label);
	data->PO_accto = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);


	return group_grid;
}




static GtkWidget *deftransaction_make_block2(struct deftransaction_data *data)
{
GtkWidget *group_grid, *label, *widget;
gint row;

	group_grid = gtk_grid_new ();
	gtk_grid_set_row_spacing (GTK_GRID (group_grid), SPACING_SMALL);
	gtk_grid_set_column_spacing (GTK_GRID (group_grid), SPACING_MEDIUM);
	
	row = 0;
	label = make_label_widget(_("_Payee:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = ui_pay_comboboxentry_new(label);
	data->PO_pay = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	gtk_widget_set_tooltip_text(widget, _("Autocompletion and direct seizure\nis available"));

	row++;
	label = make_label_widget(_("_Category:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = ui_cat_comboboxentry_new(label);
	data->PO_grp = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);
	gtk_widget_set_tooltip_text(widget, _("Autocompletion and direct seizure\nis available"));

	row++;
	label = make_label_widget(_("M_emo:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_memo_entry(label);
	data->ST_word = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	label = make_label_widget(_("Ta_gs:"));
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_string(label);
	data->ST_tags = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	row++;
	label = make_label(_("_Status:"), 1.0, 0.5);
	gtk_grid_attach (GTK_GRID (group_grid), label, 0, row, 1, 1);
	widget = make_cycle(label, CYA_TXN_STATUS);
	data->CY_status = widget;
	gtk_widget_set_hexpand (widget, TRUE);
	gtk_grid_attach (GTK_GRID (group_grid), widget, 1, row, 1, 1);

	return group_grid;
}


GtkWidget *create_deftransaction_window (GtkWindow *parent, gint type, gboolean postmode)
{
struct deftransaction_data *data;
GtkWidget *dialog, *content_area, *content_grid, *group_grid;
GtkWidget *label, *widget, *bar;
gint crow;

	DB( g_print("\n[ui-transaction] new\n") );

	data = g_malloc0(sizeof(struct deftransaction_data));
	
	dialog = gtk_dialog_new_with_buttons (NULL,
					    GTK_WINDOW (parent),
					    0,
					    NULL,
					    NULL);

	//store our window private data
	g_object_set_data(G_OBJECT(dialog), "inst_data", (gpointer)data);
	DB( g_print(" - window=%p, inst_data=%p\n", dialog, data) );

	data->window = dialog;
	data->type = type;

	// if you add/remove response_id also change into deftransaction_update_transfer
	if(type == TRANSACTION_EDIT_MODIFY)
	{
		gtk_dialog_add_buttons (GTK_DIALOG(dialog),
		    _("_Cancel"), GTK_RESPONSE_REJECT,
			_("_OK"), GTK_RESPONSE_ACCEPT,
			NULL);
	}
	else
	{
		if(!postmode)
		{
			gtk_dialog_add_buttons (GTK_DIALOG(dialog),
				_("_Close"), GTK_RESPONSE_REJECT,
				_("_Add & Keep"), GTK_RESPONSE_ADDKEEP,
				_("_Add"), GTK_RESPONSE_ADD,
				NULL);
		}
		else
		{
			gtk_dialog_add_buttons (GTK_DIALOG(dialog),
				_("_Close"), GTK_RESPONSE_REJECT,
				_("_Post"), GTK_RESPONSE_ADD,
				NULL);
		}
	}

	switch(type)
	{
		case TRANSACTION_EDIT_ADD:
			gtk_window_set_icon_name(GTK_WINDOW (dialog), ICONNAME_HB_OPE_ADD);
			break;
		case TRANSACTION_EDIT_INHERIT:
			gtk_window_set_icon_name(GTK_WINDOW (dialog), ICONNAME_HB_OPE_HERIT);
			break;
		case TRANSACTION_EDIT_MODIFY:
			gtk_window_set_icon_name(GTK_WINDOW (dialog), ICONNAME_HB_OPE_EDIT);
			break;
	}

	//window contents
	content_area = gtk_dialog_get_content_area(GTK_DIALOG (dialog));

	content_grid = gtk_grid_new();
	gtk_grid_set_row_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	gtk_grid_set_column_spacing (GTK_GRID (content_grid), SPACING_LARGE);
	//gtk_grid_set_column_homogeneous(GTK_GRID (content_grid), TRUE);
	gtk_orientable_set_orientation(GTK_ORIENTABLE(content_grid), GTK_ORIENTATION_VERTICAL);
	gtk_container_set_border_width (GTK_CONTAINER(content_grid), SPACING_MEDIUM);
	gtk_container_add (GTK_CONTAINER (content_area), content_grid);
	gtk_widget_show_all(content_grid);
	
	crow = 0;
	group_grid = deftransaction_make_block1(data);
	gtk_widget_set_hexpand (GTK_WIDGET(group_grid), FALSE);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 0, crow, 1, 1);
	gtk_widget_show_all(group_grid);
	
	group_grid = deftransaction_make_block2(data);
	gtk_widget_set_hexpand (GTK_WIDGET(group_grid), TRUE);
	gtk_grid_attach (GTK_GRID (content_grid), group_grid, 1, crow, 1, 1);
	gtk_widget_show_all(group_grid);

	data->showtemplate = FALSE;
	if( data->type != TRANSACTION_EDIT_MODIFY && da_archive_length() > 0 && !postmode )
	{
	GtkWidget *expander, *hbox;

		data->showtemplate = TRUE;

		crow = 1;
		expander = gtk_expander_new (_("Fill in with a template"));
		gtk_expander_set_expanded (GTK_EXPANDER(expander), TRUE);
		gtk_grid_attach (GTK_GRID (content_grid), expander, 0, crow, 2, 1);

		hbox = gtk_box_new (GTK_ORIENTATION_HORIZONTAL, SPACING_MEDIUM);
		gtk_container_add (GTK_CONTAINER (expander), hbox);

		label = make_label_widget(_("_Template:"));
		widget = make_poparchive(label);
		data->PO_arc = widget;
		gtk_box_pack_start (GTK_BOX (hbox), label, FALSE, FALSE, 0);
		gtk_box_pack_start (GTK_BOX (hbox), widget, TRUE, TRUE, 0);

		g_signal_connect (data->PO_arc, "changed", G_CALLBACK (deftransaction_fillfrom), NULL);

		gtk_widget_show_all(expander);
	}

	crow = 2;
	bar = gtk_info_bar_new ();
	data->IB_warnsign = bar;
	gtk_info_bar_set_message_type (GTK_INFO_BAR (bar), GTK_MESSAGE_WARNING);
	label = gtk_label_new (_("Warning: amount and category sign don't match"));
    gtk_box_pack_start (GTK_BOX (gtk_info_bar_get_content_area (GTK_INFO_BAR (bar))), label, TRUE, TRUE, 0);
	gtk_grid_attach (GTK_GRID (content_grid), bar, 0, crow, 2, 1);

	
	//connect all our signals
	g_signal_connect (G_OBJECT (data->ST_amount), "focus-out-event", G_CALLBACK (deftransaction_amount_focusout), NULL);
	g_signal_connect (G_OBJECT (data->ST_amount), "icon-release", G_CALLBACK (deftransaction_toggleamount), NULL);
	g_signal_connect (G_OBJECT (data->BT_split), "clicked", G_CALLBACK (deftransaction_button_split_cb), NULL);

	g_signal_connect (data->NU_mode  , "changed", G_CALLBACK (deftransaction_paymode), NULL);
	g_signal_connect (data->CM_cheque, "toggled", G_CALLBACK (deftransaction_paymode), NULL);
	g_signal_connect (data->PO_acc   , "changed", G_CALLBACK (deftransaction_update_accto), NULL);
	g_signal_connect (data->PO_accto , "changed", G_CALLBACK (deftransaction_update_transfer), NULL);

	g_signal_connect (data->PO_grp   , "changed", G_CALLBACK (deftransaction_update_warnsign), NULL);


	//setup, init and show window
	deftransaction_setup(data);

	//gtk_widget_show_all (dialog);
	//gtk_widget_hide(data->IB_warnsign);

	return dialog;
}
