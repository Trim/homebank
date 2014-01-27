/*	HomeBank -- Free, easy, personal accounting for everyone.
 *	Copyright (C) 1995-2014 Maxime DOYEN
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

#include "import.h"
#include "imp_qif.h"

/****************************************************************************/
/* Debug macros																														 */
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


/* = = = = = = = = = = = = = = = = */
static QIF_Tran *
da_qif_tran_malloc(void)
{
	return g_malloc0(sizeof(QIF_Tran));
}

static void
da_qif_tran_free(QIF_Tran *item)
{
gint i;

	if(item != NULL)
	{
		if(item->date != NULL)
			g_free(item->date);
		if(item->info != NULL)
			g_free(item->info);
		if(item->payee != NULL)
			g_free(item->payee);
		if(item->memo != NULL)
			g_free(item->memo);
		if(item->category != NULL)
			g_free(item->category);
		if(item->account != NULL)
			g_free(item->account);

		for(i=0;i<TXN_MAX_SPLIT;i++)
		{
		QIFSplit *s = &item->splits[i];
		
			if(s->memo != NULL)
				g_free(s->memo);
			if(s->category != NULL)
				g_free(s->category);	
		}

		g_free(item);
	}
}




static void
da_qif_tran_destroy(QifContext *ctx)
{
GList *qiflist = g_list_first(ctx->q_tra);

	while (qiflist != NULL)
	{
	QIF_Tran *item = qiflist->data;
		da_qif_tran_free(item);
		qiflist = g_list_next(qiflist);
	}
	g_list_free(ctx->q_tra);
	ctx->q_tra = NULL;
}

static void
da_qif_tran_new(QifContext *ctx)
{
	ctx->q_tra = NULL;
}



static void
da_qif_tran_move(QIF_Tran *sitem, QIF_Tran *ditem)
{
	if(sitem != NULL && ditem != NULL)
	{
		memcpy(ditem, sitem, sizeof(QIF_Tran));
		memset(sitem, 0, sizeof(QIF_Tran));
	}
}


static void
da_qif_tran_append(QifContext *ctx, QIF_Tran *item)
{
	ctx->q_tra = g_list_append(ctx->q_tra, item);
}


/* = = = = = = = = = = = = = = = = */

gdouble
hb_qif_parser_get_amount(gchar *string)
{
gdouble amount;
gint l, i;
gchar *new_str, *p;
gint  ndcount = 0;
gchar dc;

	DB( g_print("\n(qif) hb_qif_parser_get_amount\n") );


	amount = 0.0;
	dc = '?';

	l = strlen(string) - 1;

	// the first non-digit is a grouping, or a decimal separator
	// if the non-digit is after a 3 digit serie, it might be a grouping

	for(i=l;i>=0;i--)
	{
		DB( g_print(" %d :: %c :: ds='%c' ndcount=%d\n", i, string[i], dc, ndcount) );
		
		if( string[i] == '-' || string[i] == '+' ) continue;

		if( g_ascii_isdigit( string[i] ))
		{
			ndcount++;
		}
		else
		{
			if( (ndcount != 3) && (string[i] == '.' || string[i]==',') )
			{	
				dc = string[i];
			}
			ndcount = 0;
		}
	}

	DB( g_print(" s='%s' :: ds='%c'\n", string, dc) );


	new_str = g_malloc (l+3);   //#1214077
	p = new_str;
	for(i=0;i<=l;i++)
	{
		if( g_ascii_isdigit( string[i] ) || string[i] == '-' )
		{
			*p++ = string[i];
		}
		else
			if( string[i] == dc )
				*p++ = '.';
	}
	*p++ = '\0';
	amount = g_ascii_strtod(new_str, NULL);

	DB( g_print(" -> amount was='%s' => to='%s' double='%f'\n", string, new_str, amount) );

	g_free(new_str);

	return amount;
}

/*	O if m-d-y (american)
	1 if d-m-y (european) */
/* obsolete 4.5
static gint
hb_qif_parser_guess_datefmt(QifContext *ctx)
{
gboolean retval = TRUE;
GList *qiflist;
gboolean r, valid;
gint d, m, y;

	DB( g_print("(qif) get_datetype\n") );

	qiflist = g_list_first(ctx->q_tra);
	while (qiflist != NULL)
	{
	QIF_Tran *item = qiflist->data;

		r = hb_qif_parser_get_dmy(item->date, &d, &m, &y);
		valid = g_date_valid_dmy(d, m, y);

		DB( g_print(" -> date: %s :: %d %d %d :: %d\n", item->date, d, m, y, valid ) );

		if(valid == FALSE)
		{
			retval = FALSE;
			break;
		}

		qiflist = g_list_next(qiflist);
	}

	return retval;
}
*/

static Transaction *
account_qif_get_child_transfer(Transaction *src, GList *list)
{
Transaction *item;

	//DB( g_print("(transaction) transaction_get_child_transfer\n") );

	//DB( g_print(" search: %d %s %f %d=>%d\n", src->date, src->wording, src->amount, src->account, src->kxferacc) );

	list = g_list_first(list);
	while (list != NULL)
	{
		item = list->data;
		if( item->paymode == PAYMODE_INTXFER)
		{
			if( src->date == item->date &&
			    src->kacc == item->kxferacc &&
			    src->kxferacc == item->kacc &&
			    ABS(src->amount) == ABS(item->amount) )
			{
				//DB( g_print(" found : %d %s %f %d=>%d\n", item->date, item->wording, item->amount, item->account, item->kxferacc) );

				return item;
			}
		}
		list = g_list_next(list);
	}

	//DB( g_print(" not found...\n") );

	return NULL;
}


static gint
hb_qif_parser_get_block_type(gchar *tmpstr)
{
gchar **typestr;
gint type = QIF_NONE;

	DB( g_print("--------\n(account) block type\n") );

	//DB( g_print(" -> str: %s type: %d\n", tmpstr, type) );


	if(g_str_has_prefix(tmpstr, "!Account") || g_str_has_prefix(tmpstr, "!account"))
	{
		type = QIF_ACCOUNT;
	}
	else
	{
		typestr = g_strsplit(tmpstr, ":", 2);

		if( g_strv_length(typestr) == 2 )
		{
			gchar *tmpstr = g_utf8_casefold(typestr[1], -1);

			//DB( g_print(" -> str[1]: %s\n", typestr[1]) );

			if( g_str_has_prefix(tmpstr, "bank") )
			{
				type = QIF_TRANSACTION;
			}
			else
			if( g_str_has_prefix(tmpstr, "cash") )
			{
				type = QIF_TRANSACTION;
			}
			else
			if( g_str_has_prefix(tmpstr, "ccard") )
			{
				type = QIF_TRANSACTION;
			}
			else
			if( g_str_has_prefix(tmpstr, "invst") )
			{
				type = QIF_TRANSACTION;
			}
			else
			if( g_str_has_prefix(tmpstr, "oth a") )
			{
				type = QIF_TRANSACTION;
			}
			else
			if( g_str_has_prefix(tmpstr, "oth l") )
			{
				type = QIF_TRANSACTION;
			}
			else
			if( g_str_has_prefix(tmpstr, "security") )
			{
				type = QIF_SECURITY;
			}
			else
			if( g_str_has_prefix(tmpstr, "prices") )
			{
				type = QIF_PRICES;
			}

			g_free(tmpstr);
		}
		g_strfreev(typestr);
	}

	//DB( g_print(" -> return type: %d\n", type) );


	return type;
}

static void
hb_qif_parser_parse(QifContext *ctx, gchar *filename, const gchar *encoding)
{
GIOChannel *io;
QIF_Tran tran = { 0 };

	DB( g_print("(qif) hb_qif_parser_parse\n") );

	io = g_io_channel_new_file(filename, "r", NULL);
	if(io != NULL)
	{
	gchar *tmpstr;
	GError *err = NULL;
	gint io_stat;
	gint type = QIF_NONE;
	gchar *value = NULL;
	gchar *cur_acc;

		DB( g_print(" -> encoding should be %s\n", encoding) );
		if( encoding != NULL )
		{
			g_io_channel_set_encoding(io, encoding, NULL);
		}


		DB( g_print(" -> encoding is %s\n", g_io_channel_get_encoding(io)) );

		//g_io_channel_set_encoding(io, NULL, NULL);

		cur_acc = g_strdup(QIF_UNKNOW_ACCOUNT_NAME);

		for(;;)
		{
			io_stat = g_io_channel_read_line(io, &tmpstr, NULL, NULL, &err);


			if( io_stat == G_IO_STATUS_EOF )
				break;
			if( io_stat == G_IO_STATUS_ERROR )
			{
				DB (g_print(" + ERROR %s\n",err->message));
				break;
			}
			if( io_stat == G_IO_STATUS_NORMAL )
			{

				hb_string_strip_crlf(tmpstr);

				//DB (g_print("** new QIF line: '%s' **\n", tmpstr));


				//start qif parsing
				if(g_str_has_prefix(tmpstr, "!")) /* !Type: or !Option: or !Account otherwise ignore */
				{
					type = hb_qif_parser_get_block_type(tmpstr);
					DB ( g_print("-> ---- QIF block: '%s' (type = %d) ----\n", tmpstr, type) );
				}

				value = &tmpstr[1];

				if( type == QIF_ACCOUNT )
				{

					// Name
					if(g_str_has_prefix(tmpstr, "N"))
					{

						g_free(cur_acc);
						g_strstrip(value);
						cur_acc = g_strdup(value);


						DB ( g_print(" name: '%s'\n", value) );
					}
					else

					// Type of account
					if(g_str_has_prefix(tmpstr, "T"))
					{

						DB ( g_print(" type: '%s'\n", value) );
					}
					else

					// Credit limit (only for credit card accounts)
					if(g_str_has_prefix(tmpstr, "L"))
					{

						DB ( g_print(" credit limit: '%s'\n", value) );
					}
					else

					// Statement balance amount
					if(g_str_has_prefix(tmpstr, "$"))
					{

						DB ( g_print(" balance: '%s'\n", value) );
					}
					else

					// end
					if(g_str_has_prefix(tmpstr, "^"))
					{
						DB ( g_print("should create account '%s' here\n", cur_acc) );

						DB ( g_print(" ----------------\n") );
					}

				}

		/* transaction */
				if( type == QIF_TRANSACTION )
				{
				//date
					if(g_str_has_prefix(tmpstr, "D"))
					{
					gchar *ptr;

						// US Quicken seems to be using the ' to indicate post-2000 two-digit years
						//(such as 01/01'00 for Jan 1 2000)
						ptr = g_strrstr (value, "\'");
						if(ptr != NULL) { *ptr = '/'; }

						ptr = g_strrstr (value, " ");
						if(ptr != NULL) { *ptr = '0'; }

						g_free(tran.date);
						tran.date = g_strdup(value);
					}
					else

				// amount
					if(g_str_has_prefix(tmpstr, "T"))
					{
						tran.amount = hb_qif_parser_get_amount(value);
					}
					else

					if(tran.nb_splits < TXN_MAX_SPLIT)
					{

					// split category
						if(g_str_has_prefix(tmpstr, "S"))
						{
						QIFSplit *s = &tran.splits[tran.nb_splits];
							if(*value != '\0')
							{
								g_free(s->category);
								g_strstrip(value);
								s->category = g_strdup(value);
							}
						}
						else

					// split memo
						if(g_str_has_prefix(tmpstr, "E"))
						{
						QIFSplit *s = &tran.splits[tran.nb_splits];
							if(*value != '\0')
							{
								g_free(s->memo);
								s->memo = g_strdup(value);
							}
						}
						else

					// split amount
						if(g_str_has_prefix(tmpstr, "$"))
						{
						QIFSplit *s = &tran.splits[tran.nb_splits];
						
							s->amount = hb_qif_parser_get_amount(value);
							// $ line normally end a split
							#if MYDEBUG == 1
							g_print(" -> new split added: [%d] S=%s, E=%s, $=%.2f\n", tran.nb_splits, s->category, s->memo, s->amount);
							#endif
						
							tran.nb_splits++;						
						}
					// end split
					}
					else

				// cleared status
					if(g_str_has_prefix(tmpstr, "C"))
					{
						if(g_str_has_prefix(value, "X") || g_str_has_prefix(value, "R") )
						{
							tran.reconciled = TRUE;
						}
						else
							tran.reconciled = FALSE;
					}
					else

				// check num or reference number
					if(g_str_has_prefix(tmpstr, "N"))
					{
						if(*value != '\0')
						{
							g_free(tran.info);
							g_strstrip(value);
							tran.info = g_strdup(value);
						}
					}
					else

				// payee
					if(g_str_has_prefix(tmpstr, "P"))
					{
						if(*value != '\0')
						{
							g_free(tran.payee);
							g_strstrip(value);
							tran.payee = g_strdup(value);
						}
					}
					else

				// memo
					if(g_str_has_prefix(tmpstr, "M"))
					{
						if(*value != '\0')
						{
							g_free(tran.memo);
							tran.memo = g_strdup(value);
						}
					}
					else

				// category
					if(g_str_has_prefix(tmpstr, "L"))
					{
						// LCategory of transaction
						// L[Transfer account name]
						// LCategory of transaction/Class of transaction
						// L[Transfer account]/Class of transaction
						// this is managed at insertion

						if(*value != '\0')
						{
							g_free(tran.category);
							g_strstrip(value);
							tran.category = g_strdup(value);
						}
					}
					else

				// end
					if(g_str_has_prefix(tmpstr, "^"))
					{
					QIF_Tran *newitem;

						//fix: 380550
						if( tran.date )
						{
							tran.account = g_strdup(cur_acc);

							DB ( g_print(" -> store qif txn: dat:'%s' amt:%.2f pay:'%s' mem:'%s' cat:'%s' acc:'%s' nbsplit:%d\n", tran.date, tran.amount, tran.payee, tran.memo, tran.category, tran.account, tran.nb_splits) );

							newitem = da_qif_tran_malloc();
							da_qif_tran_move(&tran, newitem);
							da_qif_tran_append(ctx, newitem);
						}

						//unvalid tran
						tran.date = 0;
						//todo: should clear mem alloc here
						
						tran.nb_splits = 0;

					}
				}
				// end QIF_TRANSACTION


			}
			g_free(tmpstr);
		}
		g_io_channel_unref (io);

		g_free(cur_acc);

	}


}




/*
** this is our main qif entry point
*/
GList *
account_import_qif(gchar *filename, ImportContext *ictx)
{
QifContext ctx = { 0 };
GList *qiflist;
GList *list = NULL;

	DB( g_print("(qif) account import qif\n") );

	// allocate our GLists
	da_qif_tran_new(&ctx);
	ctx.is_ccard = FALSE;

	// parse !!
	hb_qif_parser_parse(&ctx, filename, ictx->encoding);

	// check iso date format in file
	//isodate = hb_qif_parser_check_iso_date(&ctx);
	//DB( g_print(" -> date is dd/mm/yy: %d\n", isodate) );

	DB( g_print("(qif) transform to hb txn\n") );

	DB( g_print(" -> %d qif txn\n",  g_list_length(ctx.q_tra)) );

	// transform our qif transactions to homebank ones
	qiflist = g_list_first(ctx.q_tra);
	while (qiflist != NULL)
	{
	QIF_Tran *item = qiflist->data;
	Transaction *newope, *child;
	Account *accitem, *existitem;
	Payee *payitem;
	Category *catitem;
	gchar *name;
	gint nsplit;

		newope = da_transaction_malloc();

		newope->date		 = hb_date_get_julian(item->date, ictx->datefmt);
		if( newope->date == 0 )
			ictx->cnt_err_date++;
		
		//newope->paymode	 = atoi(str_array[1]);
		//newope->info		 = g_strdup(str_array[2]);

		newope->wording		 = g_strdup(item->memo);
		newope->info		 = g_strdup(item->info);
		newope->amount		 = item->amount;

		//#773282 invert amount for ccard accounts
		if(ctx.is_ccard)
			newope->amount *= -1;

		// payee + append
		if( item->payee != NULL )
		{
			payitem = da_pay_get_by_name(item->payee);
			if(payitem == NULL)
			{
				//DB( g_print(" -> append pay: '%s'\n", item->payee ) );

				payitem = da_pay_malloc();
				payitem->name = g_strdup(item->payee);
				payitem->imported = TRUE;
				da_pay_append(payitem);

				ictx->cnt_new_pay += 1;
			}
			newope->kpay = payitem->key;
		}

		// LCategory of transaction
		// L[Transfer account name]
		// LCategory of transaction/Class of transaction
		// L[Transfer account]/Class of transaction
		if( item->category != NULL )
		{
			if(g_str_has_prefix(item->category, "["))	// this is a transfer account name
			{
			gchar *accname;

				//DB ( g_print(" -> transfer to: '%s'\n", item->category) );

				//remove brackets
				accname = hb_strdup_nobrackets(item->category);

				// account + append
				accitem = da_acc_get_by_name(accname);
				if(accitem == NULL)
				{
					DB( g_print(" -> append dest acc: '%s'\n", accname ) );

					accitem = da_acc_malloc();
					accitem->name = g_strdup(accname);
					accitem->imported = TRUE;
					accitem->imp_name = g_strdup(accname);
					da_acc_append(accitem);
				}

				newope->kxferacc = accitem->key;
				newope->paymode = PAYMODE_INTXFER;

				g_free(accname);
			}
			else
			{
				//DB ( g_print(" -> append cat: '%s'\n", item->category) );

				catitem = da_cat_append_ifnew_by_fullname(item->category, TRUE );
				if( catitem != NULL )
				{
					ictx->cnt_new_cat += 1;
					newope->kcat = catitem->key;
				}
			}
		}

		// splits, if not a xfer
		if( newope->paymode != PAYMODE_INTXFER )
		{
			for(nsplit=0;nsplit<item->nb_splits;nsplit++)
			{
			QIFSplit *s = &item->splits[nsplit];
			Split *hbs;
		
				DB( g_print(" -> append split %d: '%s' '%.2f' '%s'\n", nsplit, s->category, s->amount, s->memo) );
		
				if( s->category != NULL )
				{
					catitem = da_cat_append_ifnew_by_fullname(s->category, TRUE ); // TRUE = imported
					if( catitem != NULL )
					{
						DB( g_print(" -> append ok\n" ) );

						hbs = da_split_new(catitem->key, s->amount, s->memo);
						da_transaction_splits_append(newope, hbs);
						hbs = NULL;				
					}
				}	
		
			}
		}

		// account + append
		name = strcmp(QIF_UNKNOW_ACCOUNT_NAME, item->account) == 0 ? QIF_UNKNOW_ACCOUNT_NAME : item->account;

		DB( g_print(" -> account name is '%s'\n", name ) );

		accitem = da_acc_get_by_imp_name(name);
		if( accitem == NULL )
		{
			// check for an existing account before creating it
			existitem = da_acc_get_by_name(name);

			accitem = import_create_account(name, NULL);
			DB( g_print(" -> creating account '%s'\n", name ) );

			if( existitem != NULL )
			{
				accitem->imp_key = existitem->key;
				DB( g_print(" -> existitem is '%d' %s\n", existitem->key, existitem->name ) );
			}
		}

		newope->kacc = accitem->key;

		newope->flags |= OF_ADDED;
		if( newope->amount > 0 )
			newope->flags |= OF_INCOME;

		if( item->reconciled )
			newope->flags |= OF_VALID;

		child = NULL;

		child = account_qif_get_child_transfer(newope, list);
		if( child != NULL)
		{
			//DB( g_print(" -> transaction already exist\n" ) );

			da_transaction_free(newope);
		}
		else
		{
			//DB( g_print(" -> append trans. acc:'%s', memo:'%s', val:%.2f\n", item->account, item->memo, item->amount ) );

			list = g_list_append(list, newope);
		}

		qiflist = g_list_next(qiflist);
	}

	// destroy our GLists
	da_qif_tran_destroy(&ctx);

	DB( g_print(" -> %d txn converted\n", g_list_length(list)) );
	DB( g_print(" -> %d errors\n", ictx->cnt_err_date) );




	return list;
}


