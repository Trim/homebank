/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2013 Maxime DOYEN
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
#include "hb-hbfile.h"
#include "hb-archive.h"
#include "hb-transaction.h"

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



gboolean hbfile_file_hasbackup(gchar *filepath)
{
gchar *bakfilepath;

	bakfilepath = homebank_filepath_with_extention(GLOBALS->xhb_filepath, "xhb~");
	GLOBALS->xhb_hasbak = g_file_test(bakfilepath, G_FILE_TEST_EXISTS);
	g_free(bakfilepath);
	//todo check here if need to return something
	return GLOBALS->xhb_hasbak;
}





/*
static gint hbfile_file_load_xhb(gchar *filepath)
{






}


static void hbfile_file_load_backup_xhb(void)
{
//todo: get from dialog.c, and split between dilaog.c/hbfile.c






}
*/

gint hbfile_insert_scheduled_transactions(void)
{
GList *list;
gint count;
guint32 maxdate;
gint nb_days;

	DB( g_printf("\n[hbfile] insert_scheduled_transactions\n") );

	count = 0;
	nb_days = archive_add_get_nbdays();

	maxdate = GLOBALS->today + nb_days;

	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *arc = list->data;

		if((arc->flags & OF_AUTO) && arc->kacc > 0)
		{

			/*#if MYDEBUG == 1
			gchar buffer1[128]; GDate *date;
			date = g_date_new_julian(arc->nextdate);
			g_date_strftime (buffer1, 128-1, "%x", date);
			g_date_free(date);
			g_print("  -> '%s' - every %d %s - next %s limit %d\n", arc->wording, arc->every, CYA_UNIT[arc->unit], buffer1, arc->limit);
			#endif*/

			if(arc->nextdate < maxdate)
			{
			guint32 mydate = arc->nextdate;

				while(mydate < maxdate)
				{
				Transaction ope;

				/*#if MYDEBUG == 1
					gchar buffer1[128]; GDate *date;
					date = g_date_new_julian(mydate);
					g_date_strftime (buffer1, 128-1, "%x", date);
					g_date_free(date);
					g_printf("  -> adding '%s' on %s\n", arc->wording, buffer1);
				#endif*/

					/* fill in the transaction */
					memset(&ope, 0, sizeof(ope));
					ope.date		= mydate;
					ope.amount		= arc->amount;
					ope.kacc		= arc->kacc;
					ope.paymode		= arc->paymode;
					ope.flags		= arc->flags | OF_ADDED;
					ope.kpay		= arc->kpay;
					ope.kcat		= arc->kcat;
					ope.kxferacc	= arc->kxferacc;
					ope.wording		= g_strdup(arc->wording);
					ope.info		= NULL;

					/* todo: ? fill in cheque number */

					transaction_add(&ope, NULL, 0);
					GLOBALS->changes_count++;
					count++;

					/* compute next occurence */
					switch(arc->unit)
					{
						case AUTO_UNIT_DAY:
							mydate += arc->every;
							break;
						case AUTO_UNIT_WEEK:
							mydate += (7*arc->every);
							break;
						case AUTO_UNIT_MONTH:
						{
						GDate *date = g_date_new_julian(mydate);
							g_date_add_months(date, (gint)arc->every);
							mydate = g_date_get_julian(date);
							g_date_free(date);
							}
							break;
						case AUTO_UNIT_YEAR:
							mydate += (365*arc->every);
							break;
					}

					/* check limit, update and maybe break */
					if(arc->flags & OF_LIMIT)
					{
						arc->limit--;
						if(arc->limit <= 0)
						{
							arc->flags ^= (OF_LIMIT | OF_AUTO);	// invert flags
							goto nextarchive;
						}
					}



				}

				/* store next occurence */
				arc->nextdate = mydate;

			}


		}
nextarchive:

		list = g_list_next(list);


	}


	return count;
}


/*
void hbfile_change_basecurrency(guint32 key)
{
GList *list;

	list = g_hash_table_get_values(GLOBALS->h_cur);
	while (list != NULL)
	{
	Currency *entry = list->data;

		if(entry->key != GLOBALS->kcur)
			entry->rate = 0.0;
			
		list = g_list_next(list);
	}
	g_list_free(list);	


	GLOBALS->changes_count++;
	GLOBALS->kcur = key;

}
*/


void hbfile_change_owner(gchar *owner)
{
	g_free(GLOBALS->owner);
	GLOBALS->owner = (owner != NULL) ? owner : NULL;
}


void hbfile_change_filepath(gchar *filepath)
{
	g_free(GLOBALS->xhb_filepath);
	GLOBALS->xhb_filepath = (filepath != NULL) ? filepath : NULL;
}


void hbfile_sanity_check(void)
{
GList *list;

	DB( g_printf("\n[hbfile] sanity_check\n") );


	list = g_list_first(GLOBALS->ope_list);
	while (list != NULL)
	{
	Transaction *entry = list->data;

		da_transaction_consistency(entry);
		list = g_list_next(list);
	}


	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *entry = list->data;

		da_archive_consistency(entry);
		list = g_list_next(list);
	}


	list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *item = list->data;

		da_acc_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(list);

	
	list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *item = list->data;

		da_pay_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(list);

	
	list = g_hash_table_get_values(GLOBALS->h_cat);
	while (list != NULL)
	{
	Category *item = list->data;

		da_cat_consistency(item);
		list = g_list_next(list);
	}
	g_list_free(list);
	
}


void hbfile_anonymize(void)
{
GList *list;
guint cnt, i;

	DB( g_printf("\n[hbfile] anonymize\n") );

	// owner
	hbfile_change_owner(g_strdup("An0nym0us"));
	GLOBALS->changes_count++;
	GLOBALS->hbfile_is_new = TRUE;

	// filename
	hbfile_change_filepath(g_build_filename(PREFS->path_hbfile, "anonymized.xhb", NULL));

	// accounts
	list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *item = list->data;
		g_free(item->name);
		item->name = g_strdup_printf("account %d", item->key);
		g_free(item->number);
		item->number = NULL;
		g_free(item->bankname);
		item->bankname = NULL;
		
		GLOBALS->changes_count++;
		list = g_list_next(list);
	}
	g_list_free(list);

	//payees
	list = g_hash_table_get_values(GLOBALS->h_pay);
	while (list != NULL)
	{
	Payee *item = list->data;

		if(item->key != 0)
		{
			g_free(item->name);
			item->name = g_strdup_printf("payee %d", item->key);
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(list);

	//categories
	list = g_hash_table_get_values(GLOBALS->h_cat);
	while (list != NULL)
	{
	Category *item = list->data;

		if(item->key != 0)
		{
			g_free(item->name);
			item->name = g_strdup_printf("category %d", item->key);
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(list);

	//tags
	list = g_hash_table_get_values(GLOBALS->h_tag);
	while (list != NULL)
	{
	Tag *item = list->data;

		if(item->key != 0)
		{
			g_free(item->name);
			item->name = g_strdup_printf("tag %d", item->key);
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(list);

	//assigns
	list = g_hash_table_get_values(GLOBALS->h_rul);
	while (list != NULL)
	{
	Assign *item = list->data;

		if(item->key != 0)
		{
			g_free(item->name);
			item->name = g_strdup_printf("assign %d", item->key);
			GLOBALS->changes_count++;
		}
		list = g_list_next(list);
	}
	g_list_free(list);

	//archives
	cnt = 0;
	list = g_list_first(GLOBALS->arc_list);
	while (list != NULL)
	{
	Archive *item = list->data;
	
		g_free(item->wording);
		item->wording = g_strdup_printf("archive %d", cnt++);
		GLOBALS->changes_count++;
		
		//later split anonymize also
		
		list = g_list_next(list);
	}

	//transaction
	list = g_list_first(GLOBALS->ope_list);
	while (list != NULL)
	{
	Transaction *item = list->data;
	Split *split;

		g_free(item->info);
		item->info = NULL;
		g_free(item->wording);
		item->wording = g_strdup_printf("memo %d", item->date);
		GLOBALS->changes_count++;
		
		if(item->flags & OF_SPLIT)
		{
			for(i=0;i<TXN_MAX_SPLIT;i++)
			{
				split = item->splits[i];
				if( split == NULL ) break;

				if(split->memo != NULL)
					g_free(split->memo);
					
				split->memo = g_strdup_printf("memo %d", i);
				GLOBALS->changes_count++;
			}		
		
		
		}
		
		list = g_list_next(list);
	}

}


void hbfile_cleanup(gboolean file_clear)
{
	DB( g_printf("\n[hbfile] cleanup\n") );
	DB( g_printf("- file clear is %d\n", file_clear) );
	
	// Free data storage
	//da_cur_destroy();
	da_acc_destroy();
	da_pay_destroy();
	da_cat_destroy();
	da_tag_destroy();
	da_asg_destroy();
	g_hash_table_destroy(GLOBALS->h_memo);
	da_archive_destroy(GLOBALS->arc_list);
	da_transaction_destroy(GLOBALS->ope_list);

	hbfile_change_owner(NULL);

	if(file_clear)
		hbfile_change_filepath(NULL);

}


void hbfile_setup(gboolean file_clear)
{

	DB( g_printf("\n[hbfile] setup\n") );
	DB( g_printf("- file clear is %d\n", file_clear) );

	// Allocate data storage
	//da_cur_new();
	da_acc_new();
	da_pay_new();
	da_cat_new();
	da_tag_new();
	da_asg_new();

	GLOBALS->h_memo = g_hash_table_new_full(g_str_hash, g_str_equal, (GDestroyNotify)g_free, NULL);
	GLOBALS->arc_list = NULL;
	GLOBALS->ope_list = NULL;


	if(file_clear == TRUE)
	{
		//todo: maybe translate this also
		hbfile_change_filepath(g_build_filename(PREFS->path_hbfile, "untitled.xhb", NULL));
		GLOBALS->hbfile_is_new = TRUE;
		
		DB( g_printf("- path_hbfile is '%s'\n", PREFS->path_hbfile) );
		DB( g_printf("- xhb_filepath is '%s'\n", GLOBALS->xhb_filepath) );
	}
	else
	{
		GLOBALS->hbfile_is_new = FALSE;
	}

	hbfile_change_owner(g_strdup(_("Unknown")));
	
	//GLOBALS->kcur = 0;

	GLOBALS->vehicle_category = 0;
	
	GLOBALS->auto_nbdays = 0;
	GLOBALS->auto_weekday = 1;
	GLOBALS->auto_smode = 0;
	
	GLOBALS->changes_count = 0;

	GLOBALS->xhb_hasbak = FALSE;

}

