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

#ifndef __HB_FILTER_H__
#define __HB_FILTER_H__


/*
** filter options
*/
enum
{
	FILTER_DATE,
	FILTER_STATUS,
	FILTER_PAYMODE,
	FILTER_AMOUNT,
	FILTER_ACCOUNT,
	FILTER_CATEGORY,
	FILTER_PAYEE,
	FILTER_TEXT,
	FILTER_MAX
};


enum
{
	FLT_RANGE_THISMONTH = 0,
	FLT_RANGE_LASTMONTH = 1,
	FLT_RANGE_THISQUARTER = 2,
	FLT_RANGE_LASTQUARTER = 3,
	FLT_RANGE_THISYEAR = 4,
	
	FLT_RANGE_LAST30DAYS = 6,
	FLT_RANGE_LAST60DAYS = 7,
	FLT_RANGE_LAST90DAYS = 8,
	FLT_RANGE_LAST12MONTHS = 9,
	
	FLT_RANGE_OTHER = 11,
	
	FLT_RANGE_ALLDATE = 13
};

//default settings for account window
#define FLT_TYPE_ALL 3
#define FLT_STATUS_ALL 3


typedef struct _filter	Filter;

struct _filter
{
	guint32		mindate, maxdate;
	gint		range;
	gint		type;
	gint		status;

	gshort		option[FILTER_MAX];
	gboolean	reconciled;
	gboolean	reminded;
	gboolean	forceadd;
	gboolean	forcechg;
	gboolean	paymode[NUM_PAYMODE_MAX];
	gdouble		minamount, maxamount;
	gchar		*info;
	gchar		*wording;
	gchar		*tag;
	guint		last_tab;
};


Filter *da_filter_malloc(void);
void da_filter_free(Filter *flt);

void filter_default_all_set(Filter *flt);
void filter_preset_daterange_set(Filter *flt, gint range);
void filter_preset_type_set(Filter *flt, gint value);
void filter_preset_status_set(Filter *flt, gint value);
gchar *filter_daterange_text_get(Filter *flt);

gint filter_test(Filter *flt, Transaction *ope);

#endif
