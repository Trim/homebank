/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2018 Maxime DOYEN
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
#include "hb-import.h"


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


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/
static void 
da_import_context_gen_txn_destroy(ImportContext *context)
{
GList *list;

	DB( g_print("\n[import] free gen txn list\n") );
	list = g_list_first(context->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *gentxn = list->data;
		da_gen_txn_free(gentxn);
		list = g_list_next(list);
	}
	g_list_free(context->gen_lst_txn);
	context->gen_lst_txn = NULL;
}


static void 
da_import_context_gen_acc_destroy(ImportContext *context)
{
GList *list;

	DB( g_print(" free gen acc list\n") );
	list = g_list_first(context->gen_lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;
		da_gen_acc_free(genacc);
		list = g_list_next(list);
	}
	g_list_free(context->gen_lst_acc);
	context->gen_lst_acc = NULL;

}


static void 
da_import_context_clear(ImportContext *context)
{
	DB( g_print("\n[import] context clear\n") );

	da_import_context_gen_txn_destroy(context);
	da_import_context_gen_acc_destroy(context);
	context->gen_next_acckey = 1;

}


void 
da_import_context_destroy(ImportContext *context)
{
GList *list;

	DB( g_print("\n[import] context destroy\n") );

	da_import_context_gen_txn_destroy(context);
	da_import_context_gen_acc_destroy(context);

	DB( g_print(" free gen file list\n") );
	list = g_list_first(context->gen_lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;
		da_gen_file_free(genfile);
		list = g_list_next(list);
	}
	g_list_free(context->gen_lst_file);
	context->gen_lst_file = NULL;
}


void 
da_import_context_new(ImportContext *context)
{
	context->gen_lst_file = NULL;

	context->gen_lst_acc  = NULL;
	context->gen_lst_txn  = NULL;
	context->gen_next_acckey = 1;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/

GenFile *
da_gen_file_malloc(void)
{
	return g_malloc0(sizeof(GenFile));
}

void 
da_gen_file_free(GenFile *genfile)
{
	if(genfile != NULL)
	{
		if(genfile->filepath != NULL)
			g_free(genfile->filepath);

		g_free(genfile);
	}
}


GenFile *
da_gen_file_get(GList *lst_file, guint32 key)
{
GenFile *existfile = NULL;
GList *list;

	list = g_list_first(lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;

		if( key == genfile->key )
		{
			existfile = genfile;
			break;
		}
		list = g_list_next(list);
	}
	return existfile;
}


static GenFile *
da_gen_file_get_by_name(GList *lst_file, gchar *filepath)
{
GenFile *existfile = NULL;
GList *list;

	DB( g_print("da_gen_file_get_by_name\n") );

	list = g_list_first(lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;

		DB( g_print(" strcasecmp '%s' '%s'\n", filepath, genfile->filepath) );
	
		if(!strcasecmp(filepath, genfile->filepath))
		{
			existfile = genfile;
			DB( g_print(" found\n") );
			break;
		}
		list = g_list_next(list);
	}

	return existfile;
}


GenFile *
da_gen_file_append_from_filename(ImportContext *ictx, gchar *filename)
{
GenFile *genfile = NULL;
gint filetype;

	//todo: should check if its a file !!

	filetype = homebank_alienfile_recognize(filename);

	DB( g_print(" - filename '%s', type is %d\n", filename, filetype ) );

	// we keep everything here
	//if( (filetype == FILETYPE_OFX) || (filetype == FILETYPE_QIF) || (filetype == FILETYPE_CSV_HB) )
	//{
	GenFile *existgenfile;

		existgenfile = da_gen_file_get_by_name(ictx->gen_lst_file, filename);
		if(existgenfile == NULL)
		{
			genfile = da_gen_file_malloc();
			genfile->filepath = g_strdup(filename);
			genfile->filetype = filetype;
			
			//append to list
			genfile->key = g_list_length (ictx->gen_lst_file) + 1;
			ictx->gen_lst_file = g_list_append(ictx->gen_lst_file, genfile);

		}
	//}

	return genfile;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


GenAcc *
da_gen_acc_malloc(void)
{
	return g_malloc0(sizeof(GenAcc));
}

void 
da_gen_acc_free(GenAcc *genacc)
{
	if(genacc != NULL)
	{
		if(genacc->name != NULL)
			g_free(genacc->name);
		if(genacc->number != NULL)
			g_free(genacc->number);

		g_free(genacc);
	}
}


GenAcc *
da_gen_acc_get_by_key(GList *lst_acc, guint32 key)
{
GenAcc *existacc = NULL;
GList *list;

	list = g_list_first(lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;

		if( key == genacc->key )
		{
			existacc = genacc;
			break;
		}
		list = g_list_next(list);
	}
	return existacc;
}


static GenAcc *
da_gen_acc_get_by_name(GList *lst_acc, gchar *name)
{
GenAcc *existacc = NULL;
GList *list;

	//DB( g_print("da_gen_acc_get_by_name\n") );

	list = g_list_first(lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;

		//DB( g_print(" strcasecmp '%s' '%s'\n", name, genacc->name) );
	
		if(!strcasecmp(name, genacc->name))
		{
			existacc = genacc;
			//DB( g_print(" found\n") );
			break;
		}
		list = g_list_next(list);
	}

	return existacc;
}


Account *
hb_import_acc_find_existing(gchar *name, gchar *number)
{
Account *retacc;
GList *lacc, *list;

	DB( g_print("\n[import] acc_find_existing\n") );
	DB( g_print(" - search for '%s' or '%s'\n", name, number) );

	retacc = NULL;
	lacc = list = g_hash_table_get_values(GLOBALS->h_acc);
	while (list != NULL)
	{
	Account *acc = list->data;
		
		DB( g_print(" - acc '%s' or '%s'\n", acc->name, acc->number) );
		
		if(number != NULL && acc->number && strlen(acc->number) )
		{
			//prefer identifying with number & search number into acc->number
			if(g_strstr_len(number, -1, acc->number) != NULL)
			{
				DB( g_print(" - found '%s'\n", acc->number) );
				retacc = acc;
				break;
			}
		}

		//but if not found try with name	
		if(retacc == NULL && name != NULL)
		{
			if(g_strstr_len(name, -1, acc->name) != NULL)
			{
				DB( g_print(" - found '%s'\n", acc->name) );
				retacc = acc;
				break;
			}
		}

		list = g_list_next(list);
	}
	g_list_free(lacc);

	return retacc;
}


GenAcc *
hb_import_gen_acc_get_next(ImportContext *ictx, gint filetype, gchar *name, gchar *number)
{
GenAcc *newacc;

	DB( g_print("\n[Import] acc_get_next\n") );

	DB( g_print(" - type='%d', name='%s', number='%s'\n", filetype, name, number) );

	// try to find a same name account
	if( name != NULL )
	{
		newacc = da_gen_acc_get_by_name(ictx->gen_lst_acc, name);
		if(newacc != NULL)
		{
			DB( g_print(" - found existing '%s'\n", name) );
			goto end;
		}
	}

	newacc = da_gen_acc_malloc();
	if(newacc)
	{
		newacc->kfile = ictx->curr_kfile;
		newacc->key = ictx->gen_next_acckey++;
		newacc->kacc = DST_ACC_GLOBAL;
		
		if(name != NULL)
		{
			newacc->is_unamed = FALSE;
			newacc->name = g_strdup(name);
		}
		else
		{
		GenFile *genfile;
		gchar *basename;
		
			newacc->is_unamed = TRUE;

			genfile = da_gen_file_get (ictx->gen_lst_file, newacc->kfile);
			basename = g_path_get_basename(genfile->filepath);
			
			newacc->name = g_strdup_printf("%s %d", basename, newacc->key);
			g_free(basename);
		}
		
		if(number != NULL)
			newacc->number = g_strdup(number);

		ictx->gen_lst_acc = g_list_append(ictx->gen_lst_acc, newacc);
	}

	DB( g_print(" - create new '%s'\n", newacc->name) );

end:
	newacc->filetype = filetype;
	ictx->curr_kacc = newacc->key;

	return newacc;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = =*/


GenTxn *
da_gen_txn_malloc(void)
{
	return g_malloc0(sizeof(GenTxn));
}


void 
da_gen_txn_free(GenTxn *gentxn)
{
gint i;

	if(gentxn != NULL)
	{
		if(gentxn->account != NULL)
			g_free(gentxn->account);

		if(gentxn->rawinfo != NULL)
			g_free(gentxn->rawinfo);
		if(gentxn->rawpayee != NULL)
			g_free(gentxn->rawpayee);
		if(gentxn->rawmemo != NULL)
			g_free(gentxn->rawmemo);

		if(gentxn->date != NULL)
			g_free(gentxn->date);
		if(gentxn->info != NULL)
			g_free(gentxn->info);
		if(gentxn->payee != NULL)
			g_free(gentxn->payee);
		if(gentxn->memo != NULL)
			g_free(gentxn->memo);
		if(gentxn->category != NULL)
			g_free(gentxn->category);
		if(gentxn->tags != NULL)
			g_free(gentxn->tags);

		for(i=0;i<TXN_MAX_SPLIT;i++)
		{
		GenSplit *s = &gentxn->splits[i];
		
			if(s->memo != NULL)
				g_free(s->memo);
			if(s->category != NULL)
				g_free(s->category);	
		}

		if(gentxn->lst_existing != NULL)
		{
			g_list_free(gentxn->lst_existing);
			gentxn->lst_existing = NULL;
		}

		g_free(gentxn);
	}
}

static gint 
da_gen_txn_compare_func(GenTxn *a, GenTxn *b)
{
gint retval = (gint)(a->julian - b->julian); 

	if(!retval)
		retval = (ABS(a->amount) - ABS(b->amount)) > 0 ? 1 : -1;
	return (retval);
}


GList *
da_gen_txn_sort(GList *list)
{
	return( g_list_sort(list, (GCompareFunc)da_gen_txn_compare_func));
}


void 
da_gen_txn_move(GenTxn *sgentxn, GenTxn *dgentxn)
{
	if(sgentxn != NULL && dgentxn != NULL)
	{
		memcpy(dgentxn, sgentxn, sizeof(GenTxn));
		memset(sgentxn, 0, sizeof(GenTxn));
	}
}


void 
da_gen_txn_append(ImportContext *ctx, GenTxn *gentxn)
{
	gentxn->kfile = ctx->curr_kfile;
	gentxn->kacc  = ctx->curr_kacc;
	gentxn->to_import = TRUE;
	ctx->gen_lst_txn = g_list_append(ctx->gen_lst_txn, gentxn);
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */


static void _string_utf8_ucfirst(gchar **str)
{
gint str_len;
gchar *first, *lc;

	if( *str == NULL )
		return;
	
	str_len = strlen(*str);
	if( str_len <= 1 )
		return;

	first = g_utf8_strup(*str, 1);
	lc    = g_utf8_strdown( g_utf8_next_char(*str), -1 );
	g_free(*str);
	*str = g_strjoin(NULL, first, lc, NULL);
	g_free(first);
	g_free(lc);
}


static gchar *
_string_concat(gchar *str, gchar *addon)
{
gchar *retval;

	DB( g_print(" - concat '%s' + '%s'\n", str, addon) );

	if(str == NULL)
		retval = g_strdup(addon);
	else
	{
		retval = g_strjoin(" ", str, addon, NULL);
		g_free(str);
	}

	DB( g_print(" - retval='%s'\n", retval) );	
	return retval;
}


/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

gchar *hb_import_filetype_char_get(GenAcc *genacc)
{
gchar *retval = "";

	switch(genacc->filetype)
	{
#ifndef NOOFX
		case FILETYPE_OFX:
			retval = "OFX/QFX";
			break;
#endif
		case FILETYPE_QIF:
			retval = "QIF";
			break;

		case FILETYPE_CSV_HB:
			retval = "CSV";
			break;
	}

	return retval;
}


void 
hb_import_load_all(ImportContext *ictx)
{
GList *list;

	DB( g_print("\n[ui-import] load all\n") );

	da_import_context_clear (ictx);
	
	list = g_list_first(ictx->gen_lst_file);
	while (list != NULL)
	{
	GenFile *genfile = list->data;
	
		if(genfile->filetype != FILETYPE_UNKNOWN)
		{
			//todo: move this to alien analysis
			genfile->encoding = homebank_file_getencoding(genfile->filepath);
		
			ictx->curr_kfile = genfile->key;

			DB( g_print(" -> key = '%d'\n", genfile->key) );
			DB( g_print(" -> filepath = '%s'\n", genfile->filepath) );
			DB( g_print(" -> encoding = '%s'\n", genfile->encoding) );

			genfile->loaded = FALSE;
			genfile->invaliddatefmt = FALSE;		
	
			switch(genfile->filetype)
			{
		#ifndef NOOFX
				case FILETYPE_OFX:
					homebank_ofx_import(ictx, genfile);
					break;
		#endif
				case FILETYPE_QIF:
					homebank_qif_import(ictx, genfile);
					break;

				case FILETYPE_CSV_HB:
					homebank_csv_import(ictx, genfile);
					break;
			}

			genfile->loaded = TRUE;
		}

		list = g_list_next(list);
	}
	
	// sort by date
	ictx->gen_lst_txn = da_gen_txn_sort(ictx->gen_lst_txn);

}


gint 
hb_import_gen_acc_count_txn(ImportContext *ictx, GenAcc *genacc)
{
GList *list;
gint count = 0;

	DB( g_print("\n[import] gen_acc_count_txn\n") );
	
	genacc->n_txnall = 0;
	genacc->n_txnimp = 0;

	list = g_list_first(ictx->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *gentxn = list->data;

		if(gentxn->kacc == genacc->key)
		{
			genacc->n_txnall++;
			count++;

			DB( g_print(" count %03d: gentxn in=%d dup=%d '%s'\n", count, gentxn->to_import, gentxn->is_dst_similar, gentxn->memo) );

			if(gentxn->to_import) 
				genacc->n_txnimp++;
		}
		list = g_list_next(list);
	}
	return count;
}


/**
 * uncheck duplicate within the import context files
 */
gint 
hb_import_gen_txn_check_duplicate(ImportContext *ictx, GenAcc *genacc)
{
GList *list1, *list2;
gint count = 0;

	DB( g_print("\n[import] gen_txn_check_duplicate\n") );

	
	list1 = g_list_first(ictx->gen_lst_txn);
	while (list1 != NULL)
	{
	GenTxn *gentxn1 = list1->data;

		if( (genacc->key == gentxn1->kacc) && (gentxn1->julian != 0) ) //same account, valid date
		{
			list2 = g_list_next(list1);
			while (list2 != NULL)
			{
			GenTxn *gentxn2 = list2->data;

				if( (gentxn2->julian > gentxn1->julian) )
					break;

				//todo: maybe reinforce controls here
				if( (gentxn2->kacc == gentxn1->kacc) 
					&& (gentxn2->julian == gentxn1->julian)
					&& (gentxn2->amount == gentxn1->amount)
					&& (hb_string_compare(gentxn2->memo, gentxn1->memo) == 0)
					&& (hb_string_compare(gentxn2->payee, gentxn1->payee) == 0)
				  )
				{
					gentxn1->to_import = FALSE;
					gentxn1->is_imp_similar = TRUE;
					count++;

					DB( g_print(" found import dup  %d=%d %.2f %.2f in=%d dup=%d\n", gentxn1->julian, gentxn2->julian, gentxn2->amount, gentxn1->amount, gentxn1->to_import, gentxn1->is_imp_similar) );

				}
				list2 = g_list_next(list2);
			}
		}
		list1 = g_list_next(list1);
	}
	return count;
}


/**
 * uncheck existing txn into target account
 *
 */
gint 
hb_import_gen_txn_check_target_similar(ImportContext *ictx, GenAcc *genacc)
{
GList *list1, *list2;
gint count = 0;

	DB( g_print("\n[import] gen_txn_check_target_similar\n") );

	list1 = g_list_first(ictx->gen_lst_txn);
	while (list1 != NULL)
	{
	GenTxn *gentxn = list1->data;

		if(genacc->key == gentxn->kacc)
		{
			gentxn->to_import = TRUE;
			gentxn->is_dst_similar = FALSE;

			if(genacc->kacc == DST_ACC_SKIP)
			{		
				gentxn->to_import = FALSE;
			}
			else
			{
			Account *acc = da_acc_get(genacc->kacc);
			
				if(acc != NULL)
				{
					//clear previous existing
					if(gentxn->lst_existing != NULL)
					{
						g_list_free(gentxn->lst_existing);
						gentxn->lst_existing = NULL;
					}
					
					// try to find existing transaction
					list2 = g_queue_peek_tail_link(acc->txn_queue);
					while (list2 != NULL)
					{
					Transaction *txn = list2->data;

						//break if the date goes below the gentxn date + gap
						if( txn->date < (gentxn->julian - ictx->opt_daygap) )
							break;

						//#1586211 add of date tolerance
						//todo: maybe reinforce controls here
						if( ( txn->kacc == genacc->kacc ) 
						 && ( gentxn->julian <= (txn->date + ictx->opt_daygap) )
						 && ( gentxn->julian >= (txn->date - ictx->opt_daygap) )
						 && ( txn->amount == gentxn->amount ) 
						)
						{
							gentxn->lst_existing = g_list_append(gentxn->lst_existing, txn);
							gentxn->to_import = FALSE;
							gentxn->is_dst_similar = TRUE;
							count++;

							DB( g_print(" found dst acc dup %d %.2f '%s' in=%d, dup=%d\n", gentxn->julian, gentxn->amount, gentxn->memo, gentxn->to_import, gentxn->is_dst_similar) );
						}
						
						list2 = g_list_previous(list2);
					}
				}
				
			}
		}

		list1 = g_list_next(list1);
	}

	return count;
}


/**
 * try to indentify xfer for OFX
 *
 */
static gint 
hb_import_gen_xfer_eval(ImportContext *ictx, GList *list)
{
GList *root, *list1, *list2;
GList *match = NULL;
gint count = 0;

	DB( g_print("\n[import] gen xfer eval\n") );

	
	root = list1 = g_list_first(list);
	while (list1 != NULL)
	{
	Transaction *gtxn1 = list1->data;
	GenAcc *acc;

		acc = da_gen_acc_get_by_key(ictx->gen_lst_acc, gtxn1->kacc);
		if( (acc != NULL) && (acc->filetype == FILETYPE_OFX) )
		{
			g_list_free(match);
			match = NULL;
			count = 0;
			list2 = g_list_next(root);
			while (list2 != NULL)
			{
			Transaction *gentxn2 = list2->data;

				if( (gentxn2->date > gtxn1->date) )
					break;

				if(gentxn2 == gtxn1)
					goto next;

				//todo: maybe reinforce controls here
				if( (gentxn2->kacc != gtxn1->kacc) 
					&& (gentxn2->date == gtxn1->date)
					&& (gentxn2->amount == -gtxn1->amount)
					&& (hb_string_compare(gentxn2->memo, gtxn1->memo) == 0)
				  )
				{
					DB( g_print(" found xfer?  %d=%d %.2f %.2f\n", gtxn1->date, gentxn2->date, gtxn1->amount, gentxn2->amount) );
					match = g_list_append(match, gentxn2);
					count++;
				}
			next:
				list2 = g_list_next(list2);
			}
		}
		
		if(count == 1)  //we found a single potential xfer, transform it
		{
		Transaction *gentxn2 ;
		
			list2 = g_list_first(match);	
			gentxn2 = list2->data;
			
			gtxn1->paymode = PAYMODE_INTXFER;
			gtxn1->kxferacc = gentxn2->kacc;
			
			gentxn2->paymode = PAYMODE_INTXFER;
			gentxn2->kxferacc = gtxn1->kacc;
		}
		// if more than one, we cannot be sure
		
		list1 = g_list_next(list1);
	}
	
	g_list_free(match);
	
	return count;
}


/**
 * apply the user option: date format, payee/memo/info mapping
 *
 */
gboolean 
hb_import_option_apply(ImportContext *ictx, GenAcc *genacc)
{
GList *list;

	DB( g_print("\n[import] option apply\n") );

	DB( g_print(" - type=%d\n", genacc->filetype) );

	genacc->n_txnbaddate = 0;

	list = g_list_first(ictx->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *gentxn = list->data;

		if(gentxn->kacc == genacc->key)
		{
			if(genacc->filetype != FILETYPE_OFX)
			{
				gentxn->julian = hb_date_get_julian(gentxn->date, ictx->opt_dateorder);
				if( gentxn->julian == 0 )
				{
					genacc->n_txnbaddate++;
				}
			}

			if(genacc->filetype == FILETYPE_OFX)
			{
				DB( g_print(" - ofx option apply\n") );

				g_free(gentxn->payee);
				g_free(gentxn->memo);
				g_free(gentxn->info);
				gentxn->payee = NULL;
				gentxn->memo = NULL;
				gentxn->info = NULL;

				gentxn->info = g_strdup(gentxn->rawinfo);

				if(ictx->opt_ofxname == 2)
					gentxn->payee = g_strdup(gentxn->rawpayee);
				else if(ictx->opt_ofxname == 1)
					gentxn->memo = g_strdup(gentxn->rawpayee);

				DB( g_print(" - payee is '%s'\n", gentxn->payee) );
				DB( g_print(" - memo is '%s'\n", gentxn->memo) );

				if(gentxn->rawmemo != NULL)
				{
					switch(ictx->opt_ofxmemo)
					{
						//case 0: ignore
						case 1:	//add to info
							gentxn->info = _string_concat(gentxn->info, gentxn->rawmemo);
							break;

						case 2: //add to memo
							gentxn->memo = _string_concat(gentxn->memo, gentxn->rawmemo);					
							break;

						case 3: //add to payee
							gentxn->payee = _string_concat(gentxn->payee, gentxn->rawmemo);					
							break;
					}
				}

				DB( g_print("\n") );

			}
			else
			if(genacc->filetype == FILETYPE_QIF)
			{
				DB( g_print(" - qif option apply\n") );

				g_free(gentxn->payee);
				g_free(gentxn->memo);
				gentxn->payee = NULL;
				gentxn->memo = NULL;

				if(!ictx->opt_qifswap)
				{
					gentxn->payee = g_strdup(gentxn->rawpayee);
					if(ictx->opt_qifmemo)
						gentxn->memo = g_strdup(gentxn->rawmemo);
				}
				else
				{
					gentxn->payee = g_strdup(gentxn->rawmemo);
					if(ictx->opt_qifmemo)
						gentxn->memo = g_strdup(gentxn->rawpayee);
				}

				DB( g_print(" - payee is '%s'\n", gentxn->payee) );
				DB( g_print(" - memo is '%s'\n", gentxn->memo) );

			}
			else
			if(genacc->filetype == FILETYPE_CSV_HB)
			{
				DB( g_print(" - csv option apply\n") );

				g_free(gentxn->memo);
				gentxn->memo = g_strdup(gentxn->rawmemo);
			}
			
			//at last do ucfirst
			if( (ictx->opt_ucfirst == TRUE) )
			{
				_string_utf8_ucfirst(&gentxn->memo);
				_string_utf8_ucfirst(&gentxn->payee);
				//category ?
			}
				
		}
		list = g_list_next(list);
	}
	
	DB( g_print(" - nb_err=%d\n", genacc->n_txnbaddate) );
	
	return genacc->n_txnbaddate == 0 ? TRUE : FALSE;
}


/**
 * convert a GenTxn to a Transaction
 *
 */
Transaction *
hb_import_convert_txn(GenAcc *genacc, GenTxn *gentxn)
{
Transaction *newope;
Account *accitem;
Payee *payitem;
Category *catitem;
gint nsplit;

	DB( g_print("\n[import] convert txn\n") );

	newope = NULL;

	DB( g_print(" - gentxt %s %s %s\n", gentxn->account, gentxn->date, gentxn->memo) );
	DB( g_print(" - genacc '%s' '%p'\n", gentxn->account, genacc) );

	if( genacc != NULL)
	{
		newope = da_transaction_malloc();
	
		newope->kacc         = genacc->kacc;
		newope->date		 = gentxn->julian;
		newope->paymode		 = gentxn->paymode;
		newope->info		 = g_strdup(gentxn->info);
		newope->memo		 = g_strdup(gentxn->memo);
		newope->amount		 = gentxn->amount;

		if(newope->amount > 0)
			newope->flags |= OF_INCOME;

		//#773282 invert amount for ccard accounts
		//todo: manage this (qif), it is not set to true anywhere
		//if(ictx->is_ccard)
		//	gentxn->amount *= -1;

		// payee + append
		if( gentxn->payee != NULL )
		{
			payitem = da_pay_get_by_name(gentxn->payee);
			if(payitem == NULL)
			{
				//DB( g_print(" -> append pay: '%s'\n", item->payee ) );

				payitem = da_pay_malloc();
				payitem->name = g_strdup(gentxn->payee);
				//payitem->imported = TRUE;
				da_pay_append(payitem);

				//ictx->cnt_new_pay += 1;
			}
			newope->kpay = payitem->key;
		}

		// LCategory of transaction
		// L[Transfer account name]
		// LCategory of transaction/Class of transaction
		// L[Transfer account]/Class of transaction
		if( gentxn->category != NULL )
		{
			if(g_str_has_prefix(gentxn->category, "["))	// this is a transfer account name
			{
			gchar *accname;

				//DB ( g_print(" -> transfer to: '%s'\n", item->category) );

				//remove brackets
				accname = hb_strdup_nobrackets(gentxn->category);

				// search target account + append if not exixts
				accitem = da_acc_get_by_name(accname);
				if(accitem == NULL)
				{
					DB( g_print(" -> append int xfer dest acc: '%s'\n", accname ) );

					accitem = da_acc_malloc();
					accitem->name = g_strdup(accname);
					//accitem->imported = TRUE;
					//accitem->imp_name = g_strdup(accname);
					da_acc_append(accitem);
				}

				newope->kxferacc = accitem->key;
				newope->paymode = PAYMODE_INTXFER;

				g_free(accname);
			}
			else
			{
				//DB ( g_print(" -> append cat: '%s'\n", item->category) );

				catitem = da_cat_append_ifnew_by_fullname(gentxn->category);
				if( catitem != NULL )
				{
					//ictx->cnt_new_cat += 1;
					newope->kcat = catitem->key;
				}
			}
		}

		// splits, if not a xfer
		if( gentxn->paymode != PAYMODE_INTXFER )
		{
			if( gentxn->nb_splits > 0 )
			{
				newope->splits = da_split_new();
				for(nsplit=0;nsplit<gentxn->nb_splits;nsplit++)
				{
				GenSplit *s = &gentxn->splits[nsplit];
				Split *hbs;
				guint32 kcat = 0;
			
					DB( g_print(" -> append split %d: '%s' '%.2f' '%s'\n", nsplit, s->category, s->amount, s->memo) );

					if( s->category != NULL )
					{
						catitem = da_cat_append_ifnew_by_fullname(s->category);
						if( catitem != NULL )
						{
							kcat = catitem->key;
						}
					}

					//todo: remove this when no more use ||
					hb_string_replace_char('|', s->memo);
					hbs = da_split_malloc ();
					hbs->kcat   = kcat;
					hbs->memo   = g_strdup(s->memo);
					hbs->amount = s->amount;
					da_splits_append(newope->splits, hbs);
					hbs = NULL;				
				}
			}
		}
		
		newope->flags |= OF_ADDED;
		if( newope->amount > 0 )
			newope->flags |= OF_INCOME;

		if( gentxn->reconciled )
			newope->status = TXN_STATUS_RECONCILED;
		else
		if( gentxn->cleared )
			newope->status = TXN_STATUS_CLEARED;
		



	}
	return newope;
}

//todo: chck if not redundant with some hb_trasaciton.c fucntion
//it is :same than transaction_old_get_child_transfer
static Transaction *
hb_import_xfer_child_search(Transaction *src, GList *list)
{
	DB( g_print("([import] xfer_child_search\n") );

	DB( g_print(" search: %d %s %f %d=>%d\n", src->date, src->memo, src->amount, src->kacc, src->kxferacc) );

	list = g_list_first(list);
	while (list != NULL)
	{
	Transaction *item = list->data;
	
		if( item->paymode == PAYMODE_INTXFER)
		{
			if( src->date == item->date &&
			    src->kacc == item->kxferacc &&
			    src->kxferacc == item->kacc &&
			    ABS(src->amount) == ABS(item->amount) )
			{
				DB( g_print(" found : %d %s %f %d=>%d\n", item->date, item->memo, item->amount, item->kacc, item->kxferacc) );

				return item;
			}
		}
		list = g_list_next(list);
	}

	DB( g_print(" not found...\n") );

	return NULL;
}


void 
hb_import_apply(ImportContext *ictx)
{
GList *list, *lacc;
GList *txnlist = NULL;
guint32 kcommon = 0;
guint changes = 0;

	DB( g_print("\n[import] apply\n") );

	//create accounts
	list = g_list_first(ictx->gen_lst_acc);
	while (list != NULL)
	{
	GenAcc *genacc = list->data;

		DB( g_print(" - genacc: %d %s %s => %d\n", genacc->key, genacc->name, genacc->number, genacc->kacc) );

		//we do create the common account once
		if( (genacc->kacc == DST_ACC_GLOBAL) )
		{
			if( kcommon == 0 )
			{
			Account *acc = da_acc_malloc ();
				
				acc->name = g_strdup(_("imported account"));
				if( da_acc_append(acc) )
				{
					kcommon = acc->key;
					changes++;						
				}
			}

			genacc->kacc = kcommon;
		}
		else
		if( (genacc->kacc == DST_ACC_NEW) )
		{
		Account *acc = da_acc_malloc ();
		
			acc->name = g_strdup(genacc->name);
			if( da_acc_append(acc) )
			{
				acc->number = g_strdup(genacc->number);
				acc->initial = genacc->initial;
				
				//store the target acc key
				genacc->kacc = acc->key;
				changes++;
			}
		}
		
		list = g_list_next(list);
	}

	// insert every transactions into a temporary list
	// we do this to keep a finished real txn list for detect xfer below
	lacc = g_list_first(ictx->gen_lst_acc);
	while (lacc != NULL)
	{
	GenAcc *genacc = lacc->data;

		if(genacc->kacc != DST_ACC_SKIP)
		{
			list = g_list_first(ictx->gen_lst_txn);
			while (list != NULL)
			{
			GenTxn *gentxn = list->data;

				if(gentxn->kacc == genacc->key && gentxn->to_import == TRUE)
				{
				Transaction *txn;
		
					txn = hb_import_convert_txn(genacc, gentxn);
					if( txn )
					{
						txnlist = g_list_append(txnlist, txn);					
					}
				}
				list = g_list_next(list);
			}
		
		}
		lacc = g_list_next(lacc);
	}

	//check for ofx internal xfer
	DB( g_print(" call hb_import_gen_xfer_eval\n") );
	hb_import_gen_xfer_eval(ictx, txnlist);

	//auto assign
	DB( g_print(" call auto assign\n") );
	transaction_auto_assign(txnlist, 0);

	// last: insert transaction for real
	DB( g_print(" real import txn\n") );

	list = g_list_first(txnlist);	
	while (list != NULL)
	{
	Transaction *txn = list->data;
	Transaction *child;

		//avoid adding a child xfer
		child = hb_import_xfer_child_search(txn, txnlist);
		if( child != NULL)
		{
			DB( g_print(" -> transaction already exist\n" ) );
			da_transaction_free(txn);
		}
		else
		{
			transaction_add(NULL, txn);
			da_transaction_free(txn);
			changes++;
		}
		list = g_list_next(list);
	}
	
	g_list_free(txnlist);

	GLOBALS->changes_count += changes;
	
	/*
	// transform our qif transactions to homebank ones
	qiflist = g_list_first(ictx->gen_lst_txn);
	while (qiflist != NULL)
	{
	GenTxn *item = qiflist->data;
	Transaction *newope;
	Account *accitem;
	Category *catitem;
	gint nsplit;

		//moved was here



		// account + append
		//todo: commented
		//name = strcmp(QIF_UNKNOWN_ACCOUNT_NAME, item->account) == 0 ? "" : item->account;

		//todo: diabeldd and commented tinto hb-import
		//accitem = import_create_account(name, NULL);

		//newope->kacc = accitem->key;

		
		//todo: rework this

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

		qiflist = g_list_next(qiflist);
	}

	*/

}

#if MYDEBUG
void _import_context_debug_file_list(ImportContext *ctx)
{
GList *list;

	g_print("\n--debug-- file list %d\n", g_list_length(ctx->gen_lst_file) );

	list = g_list_first(ctx->gen_lst_file);
	while (list != NULL)
	{
	GenFile *item = list->data;

		g_print(" genfile: %d '%s' '%s'\ndf=%d invalid=%d\n", item->key, item->filepath, item->encoding, item->datefmt, item->invaliddatefmt);

		list = g_list_next(list);
	}

}

void _import_context_debug_acc_list(ImportContext *ctx)
{
GList *list;

	g_print("\n--debug-- acc list %d\n", g_list_length(ctx->gen_lst_acc) );

	list = g_list_first(ctx->gen_lst_acc);
	while (list != NULL)
	{
	GenAcc *item = list->data;

		g_print(" genacc: %d %s %s => %d\n", item->key, item->name, item->number, item->kacc);

		list = g_list_next(list);
	}

}


void _import_context_debug_txn_list(ImportContext *ctx)
{
GList *list;

	g_print("\n--debug-- txn list %d\n", g_list_length(ctx->gen_lst_txn) );

	list = g_list_first(ctx->gen_lst_txn);
	while (list != NULL)
	{
	GenTxn *item = list->data;

		g_print(" gentxn: (%d) %s %s (%d) %s %.2f\n", item->kfile, item->account, item->date, item->julian, item->memo, item->amount);

		list = g_list_next(list);
	}

}

#endif



/*Account *import_create_account(gchar *name, gchar *number)
{
Account *accitem, *existitem;

	//first check we do not have already this imported account
	existitem = da_acc_get_by_imp_name(name);
	if(existitem != NULL)
		return existitem;

	accitem = da_acc_malloc();
	accitem->key  = da_acc_get_max_key() + 1;
	accitem->pos  = da_acc_length() + 1;

	// existing named account ?
	existitem = da_acc_get_by_name(name);
	if(existitem != NULL)
		accitem->imp_key = existitem->key;

	if(!existitem && *name != 0)
			accitem->name = g_strdup(name);
	else
		
		accitem->name = g_strdup_printf(_("(account %d)"), accitem->key);

	accitem->imp_name = g_strdup(name);

	if(number)
		accitem->number = g_strdup(number);

	accitem->imported = TRUE;
	da_acc_insert(accitem);

	return accitem;
}
*/


/*
TODO: should move or be rewritten
the goal is to find already existing txn into real transaction
quesiotn is: is it possible at this stgae ??
rewritten into hb-transaction

static gboolean ui_import_page_transaction_is_duplicate(Transaction *impope, Transaction *ope, gint maxgap)
{
Account *dstacc;
guint dstkacc;
gboolean retval = FALSE;

	//common tests
	if( (impope->amount == ope->amount) &&
		(ope->date <= (impope->date + maxgap)) && (ope->date >= (impope->date - maxgap)) )
	{

		//we focus the test on impope->acc->imp_key (and not impope->kacc)
		dstkacc = impope->kacc; 
		dstacc = da_acc_get(dstkacc);


		DB( g_print("--------\n -> dstkacc=%d, amount & date are similar\n", dstkacc) );

		DB( g_print(" -> impope: kacc=%d, %s kxfer=%d, kxferacc=%d\n", impope->kacc, impope->memo, impope->kxfer, impope->kxferacc) );
		DB( g_print(" ->    ope: kacc=%d, %s kxfer=%d, kxferacc=%d\n", ope->kacc, ope->memo, ope->kxfer, ope->kxferacc) );


		if(impope->paymode != PAYMODE_INTXFER)
		{
			if( dstkacc == ope->kacc )
			{
				DB( g_print(" -> impope is not a xfer and acc are similar\n") );
				retval = TRUE;
			}
		}
		else
		{
			if( ( (impope->kxferacc == ope->kxferacc) && ope->kxfer != 0) ||
				( impope->kxferacc == 0 )
			   )
				retval = TRUE;
		}
	}
	return retval;
}


static void ui_import_page_transaction_find_duplicate(struct import_data *data)
{
ImportContext *ictx = &data->ictx;
GList *tmplist, *implist;
Transaction *item;
guint32 mindate;
guint maxgap;

	DB( g_print("\n[ui-import] find duplicate\n") );

	ictx->nb_duplicate = 0;
	if( ictx->trans_list )
	{
		// 1: get import min bound date
		tmplist = g_list_first(ictx->trans_list);
		item = tmplist->data;
		mindate = item->date;
		maxgap = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->NB_maxgap));

		// clear any previous same txn
		implist = g_list_first(ictx->trans_list);
		while (implist != NULL)
		{
		Transaction *impope = implist->data;

			if(impope->same != NULL)
			{
				g_list_free(impope->same);
				impope->same = NULL;
			}
			implist = g_list_next(implist);
		}
		
		tmplist = g_list_first(old txn list);
		while (tmplist != NULL)
		{
		Transaction *ope = tmplist->data;

			if( ope->date >= mindate )
			{
				//DB( g_print("should check here %d: %s\n", ope->date, ope->memo) );

				implist = g_list_first(ictx->trans_list);
				while (implist != NULL)
				{
				Transaction *impope = implist->data;

					if( ui_import_page_transaction_is_duplicate(impope, ope, maxgap) )
					{
						//DB( g_print(" found %d: %s\n", impope->date, impope->memo) );

						impope->same = g_list_append(impope->same, ope);
						ictx->nb_duplicate++;
					}

					implist = g_list_next(implist);
				}
			}

			tmplist = g_list_next(tmplist);
		}
	}

	DB( g_print(" nb_duplicate = %d\n", ictx->nb_duplicate) );
}
*/


/*
static Transaction *
account_qif_get_child_transfer(Transaction *src, GList *list)
{
Transaction *item;

	DB( g_print("([qif] get_child_transfer\n") );

	//DB( g_print(" search: %d %s %f %d=>%d\n", src->date, src->memo, src->amount, src->account, src->kxferacc) );

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
				//DB( g_print(" found : %d %s %f %d=>%d\n", item->date, item->memo, item->amount, item->account, item->kxferacc) );

				return item;
			}
		}
		list = g_list_next(list);
	}

	//DB( g_print(" not found...\n") );

	return NULL;
}
*/


/*
** convert after userr say ok (apply)

GList *
account_import_qif(gchar *filename, ImportContext *ictx)
{
GList *qiflist;
GList *list = NULL;

	// check iso date format in file
	//isodate = hb_qif_parser_check_iso_date(&ctx);
	//DB( g_print(" -> date is dd/mm/yy: %d\n", isodate) );

	DB( g_print("\n[import] real import\n") );

	DB( g_print(" -> %d qif txn\n",  g_list_length(ictx->gen_lst_txn)) );

	// transform our qif transactions to homebank ones
	qiflist = g_list_first(ictx->gen_lst_txn);
	while (qiflist != NULL)
	{
	GenTxn *item = qiflist->data;
	Transaction *newope;
	Account *accitem;
	Category *catitem;
	gint nsplit;

		//moved was here



		// account + append
		//todo: commented
		//name = strcmp(QIF_UNKNOWN_ACCOUNT_NAME, item->account) == 0 ? "" : item->account;

		//todo: diabeldd and commented tinto hb-import
		//accitem = import_create_account(name, NULL);

		//newope->kacc = accitem->key;

		
		//todo: rework this

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

		qiflist = g_list_next(qiflist);
	}

	DB( g_print(" -> %d txn converted\n", g_list_length(list)) );
	
	return list;
}
*/
