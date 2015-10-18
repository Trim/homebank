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

#ifndef __HB_SPLIT_H__
#define __HB_SPLIT_H__

#include "hb-transaction.h"

#define TXN_MAX_SPLIT 10

typedef struct _split Split;


struct _split
{
	guint32		kcat;
	gdouble		amount;
	gchar		*memo;
};


Split *da_split_new(guint32 kcat, gdouble amount, gchar	*memo);
void da_transaction_splits_free(Transaction *txn);
void da_transaction_splits_append(Transaction *txn, Split *split);
void da_transaction_splits_clone(Transaction *stxn, Transaction *dtxn);
guint transaction_splits_parse(Transaction *ope, gchar *cats, gchar *amounts, gchar *memos);
guint transaction_splits_tostring(Transaction *ope, gchar **cats, gchar **amounts, gchar **memos);
guint da_transaction_splits_count(Transaction *txn);



#endif