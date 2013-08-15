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
#include "hb-filter.h"

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




/* = = = = = = = = = = = = = = = = = = = = */
/* Filter */

Filter *da_filter_malloc(void)
{
	return g_malloc0(sizeof(Filter));
}

void da_filter_free(Filter *flt)
{
	if(flt != NULL)
	{
		g_free(flt->wording);
		g_free(flt->info);
		g_free(flt->tag);
		g_free(flt);
	}
}

/* = = = = = = = = = = = = = = = = = = = = */

gchar *filter_daterange_text_get(Filter *flt)
{
gchar buffer1[128];
gchar buffer2[128];
GDate *date;

	date = g_date_new_julian(flt->mindate);
	g_date_strftime (buffer1, 128-1, PREFS->date_format, date);
	g_date_set_julian(date, flt->maxdate);
	g_date_strftime (buffer2, 128-1, PREFS->date_format, date);
	g_date_free(date);

	return g_strdup_printf(_("<i>from</i> %s <i>to</i> %s"), buffer1, buffer2);
}



static void filter_default_date_set(Filter *flt)
{
	flt->mindate = 693596;	//01/01/1900
	flt->maxdate = 803533;	//31/12/2200
}


static void filter_clear(Filter *flt)
{
guint i;

	for(i=0;i<FILTER_MAX;i++)
	{
		flt->option[i] = 0;
	}
	
	g_free(flt->info);
	g_free(flt->wording);
	g_free(flt->tag);
	flt->info = NULL;
	flt->wording = NULL;
	flt->tag = NULL;

}


void filter_default_all_set(Filter *flt)
{
gint i;

	DB( g_printf("(filter) reset %p\n", flt) );

	filter_clear(flt);

	flt->range  = FLT_RANGE_LAST12MONTHS;
	flt->type   = FLT_TYPE_ALL;
	flt->status = FLT_STATUS_ALL;

	flt->option[FILTER_DATE] = 1;
	filter_default_date_set(flt);

	for(i=0;i<NUM_PAYMODE_MAX;i++)
		flt->paymode[i] = TRUE;

	filter_preset_daterange_set(flt, flt->range);

}


void filter_preset_daterange_set(Filter *flt, gint range)
{
GDate *date;
GList *list;
guint32 refdate, month, year, qnum;

	// any date :: todo : get date of current accout only when account 
	flt->range = range;
	if(g_list_length(GLOBALS->ope_list) > 0) // get all transaction date bound
	{
		GLOBALS->ope_list = da_transaction_sort(GLOBALS->ope_list);
		list = g_list_first(GLOBALS->ope_list);
		flt->mindate = ((Transaction *)list->data)->date;
		list = g_list_last(GLOBALS->ope_list);
		flt->maxdate = ((Transaction *)list->data)->date;
	}
	else
		filter_default_date_set(flt);
	
	
	// by default refdate is today
	// but we adjust if to max transaction date found
	refdate = GLOBALS->today;
	if(flt->maxdate < refdate)
		refdate = flt->maxdate;

	date  = g_date_new_julian(refdate);
	month = g_date_get_month(date);
	year  = g_date_get_year(date);
	qnum  = ((month - 1) / 3) + 1;

	DB( g_printf("m=%d, y=%d, qnum=%d\n", month, year, qnum) );

	switch( range )
	{
		case FLT_RANGE_THISMONTH:
			g_date_set_day(date, 1);
			flt->mindate = g_date_get_julian(date);
			g_date_add_days(date, g_date_get_days_in_month(month, year)-1);
			flt->maxdate = g_date_get_julian(date);
			break;

		case FLT_RANGE_LASTMONTH:
			g_date_set_day(date, 1);
			g_date_subtract_months(date, 1);
			flt->mindate = g_date_get_julian(date);
			month = g_date_get_month(date);
			year = g_date_get_year(date);
			g_date_add_days(date, g_date_get_days_in_month(month, year)-1);
			flt->maxdate = g_date_get_julian(date);
			break;

		case FLT_RANGE_THISQUARTER:
			g_date_set_day(date, 1);
			g_date_set_month(date, (qnum-1)*3+1);
			flt->mindate = g_date_get_julian(date);
			g_date_add_months(date, 3);
			g_date_subtract_days(date, 1);
			flt->maxdate = g_date_get_julian(date);
			break;

		case FLT_RANGE_LASTQUARTER:
			g_date_set_day(date, 1);
			g_date_set_month(date, (qnum-1)*3+1);
			g_date_subtract_months(date, 3);
			flt->mindate = g_date_get_julian(date);
			g_date_add_months(date, 3);
			g_date_subtract_days(date, 1);
			flt->maxdate = g_date_get_julian(date);
			break;

		case FLT_RANGE_THISYEAR:
			g_date_set_dmy(date, 1, 1, year);
			flt->mindate = g_date_get_julian(date);
			g_date_set_dmy(date, 31, 12, year);
			flt->maxdate = g_date_get_julian(date);
			break;

		case FLT_RANGE_LASTYEAR:
			g_date_set_dmy(date, 1, 1, year-1);
			flt->mindate = g_date_get_julian(date);
			g_date_set_dmy(date, 31, 12, year-1);
			flt->maxdate = g_date_get_julian(date);
			break;

		case FLT_RANGE_LAST30DAYS:
			flt->mindate = refdate - 30;
			flt->maxdate = refdate;
			break;

		case FLT_RANGE_LAST60DAYS:
			flt->mindate = refdate - 60;
			flt->maxdate = refdate;
			break;

		case FLT_RANGE_LAST90DAYS:
			flt->mindate = refdate - 90;
			flt->maxdate = refdate;
			break;

		case FLT_RANGE_LAST12MONTHS:
			g_date_subtract_months(date, 12);
			flt->mindate = g_date_get_julian(date);
			flt->maxdate = refdate;
			break;

		// case FLT_RANGE_OTHER:

		// case FLT_RANGE_ALLDATE:


	}
	g_date_free(date);

}

void filter_preset_type_set(Filter *flt, gint type)
{

	/* any type */
	flt->type = type;
	flt->option[FILTER_AMOUNT] = 0;
	flt->minamount = G_MINDOUBLE;
	flt->maxamount = G_MINDOUBLE;

	switch( type )
	{
		case FLT_TYPE_EXPENSE:
			flt->option[FILTER_AMOUNT] = 1;
			flt->minamount = -G_MAXDOUBLE;
			flt->maxamount = G_MINDOUBLE;
			break;

		case FLT_TYPE_INCOME:
			flt->option[FILTER_AMOUNT] = 1;
			flt->minamount = G_MINDOUBLE;
			flt->maxamount = G_MAXDOUBLE;
			break;
	}

}


void filter_preset_status_set(Filter *flt, gint status)
{
Category *catitem;
GList *list;

	/* any status */
	flt->status = status;
	flt->option[FILTER_STATUS] = 0;
	flt->reconciled = TRUE;
	flt->reminded = TRUE;
	flt->forceadd = FALSE;
	flt->forcechg = FALSE;

	flt->option[FILTER_CATEGORY] = 0;
	list = g_hash_table_get_values(GLOBALS->h_cat);
	while (list != NULL)
	{
		catitem = list->data;
		catitem->filter = FALSE;
		list = g_list_next(list);
	}
	g_list_free(list);

	switch( status )
	{
		case FLT_STATUS_UNCATEGORIZED:
			flt->option[FILTER_CATEGORY] = 1;
			catitem = da_cat_get(0);	// no category
			catitem->filter = TRUE;
			break;

		case FLT_STATUS_UNRECONCILED:
			flt->option[FILTER_STATUS] = 2;
			flt->reconciled = TRUE;
			break;
	}

}





gint filter_test(Filter *flt, Transaction *txn)
{
Account *accitem;
Payee *payitem;
Category *catitem;
gint insert;

	//DB( g_printf("(filter) test\n") );

	insert = 1;

/*** start filtering ***/

	/* add/change force */
	if(flt->forceadd == TRUE && (txn->flags & OF_ADDED))
		goto end;

	if(flt->forcechg == TRUE && (txn->flags & OF_CHANGED))
		goto end;

/* date */
	if(flt->option[FILTER_DATE]) {
		insert = ( (txn->date >= flt->mindate) && (txn->date <= flt->maxdate) ) ? 1 : 0;
		if(flt->option[FILTER_DATE] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* account */
	if(flt->option[FILTER_ACCOUNT]) {
		accitem = da_acc_get(txn->kacc);
		if(accitem)
		{
			insert = ( accitem->filter == TRUE ) ? 1 : 0;
			if(flt->option[FILTER_ACCOUNT] == 2) insert ^= 1;
		}
	}
	if(!insert) goto end;

/* payee */
	if(flt->option[FILTER_PAYEE]) {
		payitem = da_pay_get(txn->kpay);
		if(payitem)
		{
			insert = ( payitem->filter == TRUE ) ? 1 : 0;
			if(flt->option[FILTER_PAYEE] == 2) insert ^= 1;
		}
	}
	if(!insert) goto end;

/* category */
	if(flt->option[FILTER_CATEGORY]) {
		if(txn->flags & OF_SPLIT)
		{
		guint count, i;
		Split *split;

			insert = 0;	 //fix: 1151259
			count = da_transaction_splits_count(txn);
			for(i=0;i<count;i++)
			{
				split = txn->splits[i];
				catitem = da_cat_get(split->kcat);
				if(catitem)
				{
					insert |= ( catitem->filter == TRUE ) ? 1 : 0;
				}
			}
			if(flt->option[FILTER_CATEGORY] == 2) insert ^= 1;
		}
		else
		{
			catitem = da_cat_get(txn->kcat);
			if(catitem)
			{
				insert = ( catitem->filter == TRUE ) ? 1 : 0;
				if(flt->option[FILTER_CATEGORY] == 2) insert ^= 1;
			}
		}
	}
	if(!insert) goto end;

/* status */
	if(flt->option[FILTER_STATUS]) {
	gint insert1 = 0, insert2 = 0;

		if(flt->reconciled)
			insert1 = ( txn->flags & OF_VALID ) ? 1 : 0;
		if(flt->reminded)
			insert2 = ( txn->flags & OF_REMIND ) ? 1 : 0;

		insert = insert1 | insert2;
		if(flt->option[FILTER_STATUS] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* paymode */
	if(flt->option[FILTER_PAYMODE]) {
		insert = ( flt->paymode[txn->paymode] == TRUE) ? 1 : 0;
		if(flt->option[FILTER_PAYMODE] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* amount */
	if(flt->option[FILTER_AMOUNT]) {
		insert = ( (txn->amount >= flt->minamount) && (txn->amount <= flt->maxamount) ) ? 1 : 0;

		if(flt->option[FILTER_AMOUNT] == 2) insert ^= 1;
	}
	if(!insert) goto end;

/* info/wording/tag */
	if(flt->option[FILTER_TEXT])
	{
	gchar *tags;
	gint insert1 = 0, insert2 = 0, insert3 = 0;

		if(flt->info)
		{
			if(txn->info)
			{
				if( g_strstr_len(txn->info, -1, flt->info) != NULL )
					insert1 = 1;
			}
		}
		else
			insert1 = 1;

		if(flt->wording)
		{
			if(txn->wording)
			{
				if( g_strstr_len(txn->wording, -1, flt->wording) != NULL )
					insert2 = 1;
			}
		}
		else
			insert2 = 1;

		if(flt->tag)
		{
			tags = transaction_tags_tostring(txn);
			if(tags)
			{
				if( g_strstr_len(tags, -1, flt->tag) != NULL )
					insert3 = 1;

			}
			g_free(tags);
		}
		else
			insert3 = 1;

		insert = insert1 && insert2 && insert3 ? 1 : 0;

		if(flt->option[FILTER_TEXT] == 2) insert ^= 1;

	}
	if(!insert) goto end;

end:
//	DB( g_printf(" %d :: %d :: %d\n", flt->mindate, txn->date, flt->maxdate) );
//	DB( g_printf(" [%d] %s => %d (%d)\n", txn->account, txn->wording, insert, count) );
	return(insert);
}

