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
#include "hb-archive.h"

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


/* = = = = = = = = = = = = = = = = = = = = */
/* Archive */

Archive *da_archive_malloc(void)
{
	return g_malloc0(sizeof(Archive));
}

Archive *da_archive_clone(Archive *src_item)
{
Archive *new_item = g_memdup(src_item, sizeof(Archive));

	if(new_item)
	{
		//duplicate the string
		new_item->wording = g_strdup(src_item->wording);
	}
	return new_item;
}

void da_archive_free(Archive *item)
{
	if(item != NULL)
	{
		if(item->wording != NULL)
			g_free(item->wording);

		g_free(item);
	}
}

void da_archive_destroy(GList *list)
{
GList *tmplist = g_list_first(list);

	while (tmplist != NULL)
	{
	Archive *item = tmplist->data;
		da_archive_free(item);
		tmplist = g_list_next(tmplist);
	}
	g_list_free(list);
}

static gint da_archive_glist_compare_func(Archive *a, Archive *b)
{
	return (g_utf8_collate(a->wording, b->wording));
}


GList *da_archive_sort(GList *list)
{
	return g_list_sort(list, (GCompareFunc)da_archive_glist_compare_func);
}

guint da_archive_length(void)
{
	return g_list_length(GLOBALS->arc_list);
}

void da_archive_consistency(Archive *item)
{
Account *acc;
Category *cat;
Payee *pay;

	// check category exists
	cat = da_cat_get(item->kcat);
	if(cat == NULL)
	{
		g_warning("arc consistency: fixed invalid cat %d", item->kcat);
		item->kcat = 0;
	}

	// check payee exists
	pay = da_pay_get(item->kpay);
	if(pay == NULL)
	{
		g_warning("arc consistency: fixed invalid pay %d", item->kpay);
		item->kpay = 0;
	}

	// reset dst acc for non xfer transaction
	if( item->paymode != PAYMODE_INTXFER )
		item->kxferacc = 0;

	// remove automation if dst_acc not exists
	if(item->paymode == PAYMODE_INTXFER)
	{
		acc = da_acc_get(item->kxferacc);
		if(acc == NULL)
		{
			item->flags &= ~(OF_AUTO);	//remove flag
		}
	}

}

/* = = = = = = = = = = = = = = = = = = = = */

/* return the nb of days from today scheduled transaction can be inserted */
guint archive_add_get_nbdays(void)
{
guint nbdays;
GDate *today, *maxdate;

	DB( g_print("(archive_add_get_nbdays)") );

	//add until xx of the next month (excluded)
	if(GLOBALS->auto_smode == 0)	
	{
		DB( g_print("- set to %d of next month", GLOBALS->auto_weekday) );
		
		today = g_date_new_julian(GLOBALS->today);

		//we compute user xx weekday of next month
		maxdate = g_date_new_julian(GLOBALS->today);
		g_date_set_day(maxdate, GLOBALS->auto_weekday);
		if(g_date_get_day (today) >= GLOBALS->auto_weekday)
			g_date_add_months(maxdate, 1);
		
		nbdays = g_date_days_between(today, maxdate);
	
		g_date_free(maxdate);
		g_date_free(today);
	}
	else
	{
		nbdays = GLOBALS->auto_nbdays;
	}

	return nbdays;
}


