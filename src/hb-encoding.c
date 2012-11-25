/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2012 Maxime DOYEN
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
#include "hb-encoding.h"

#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

/* our global datas */
extern struct HomeBank *GLOBALS;

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */



/*
 * The original versions of the following tables are taken from profterm
 *
 * Copyright (C) 2002 Red Hat, Inc.
 */



static const GeditEncoding utf8_encoding =  {
	GEDIT_ENCODING_UTF_8,
	"UTF-8",
	N_("Unicode")
};

static const GeditEncoding encodings [] = {

  { GEDIT_ENCODING_ISO_8859_1,
    "ISO-8859-1", N_("Western") },
  { GEDIT_ENCODING_ISO_8859_2,
   "ISO-8859-2", N_("Central European") },
  { GEDIT_ENCODING_ISO_8859_3,
    "ISO-8859-3", N_("South European") },
  { GEDIT_ENCODING_ISO_8859_4,
    "ISO-8859-4", N_("Baltic") },
  { GEDIT_ENCODING_ISO_8859_5,
    "ISO-8859-5", N_("Cyrillic") },
  { GEDIT_ENCODING_ISO_8859_6,
    "ISO-8859-6", N_("Arabic") },
  { GEDIT_ENCODING_ISO_8859_7,
    "ISO-8859-7", N_("Greek") },
  { GEDIT_ENCODING_ISO_8859_8,
    "ISO-8859-8", N_("Hebrew Visual") },
  { GEDIT_ENCODING_ISO_8859_8_I,
    "ISO-8859-8-I", N_("Hebrew") },
  { GEDIT_ENCODING_ISO_8859_9,
    "ISO-8859-9", N_("Turkish") },
  { GEDIT_ENCODING_ISO_8859_10,
    "ISO-8859-10", N_("Nordic") },
  { GEDIT_ENCODING_ISO_8859_13,
    "ISO-8859-13", N_("Baltic") },
  { GEDIT_ENCODING_ISO_8859_14,
    "ISO-8859-14", N_("Celtic") },
  { GEDIT_ENCODING_ISO_8859_15,
    "ISO-8859-15", N_("Western") },
  { GEDIT_ENCODING_ISO_8859_16,
    "ISO-8859-16", N_("Romanian") },

  { GEDIT_ENCODING_UTF_7,
    "UTF-7", N_("Unicode") },
  { GEDIT_ENCODING_UTF_16,
    "UTF-16", N_("Unicode") },
  { GEDIT_ENCODING_UTF_16_BE,
    "UTF-16BE", N_("Unicode") },
  { GEDIT_ENCODING_UTF_16_LE,
    "UTF-16LE", N_("Unicode") },
  { GEDIT_ENCODING_UTF_32,
    "UTF-32", N_("Unicode") },
  { GEDIT_ENCODING_UCS_2,
    "UCS-2", N_("Unicode") },
  { GEDIT_ENCODING_UCS_4,
    "UCS-4", N_("Unicode") },

  { GEDIT_ENCODING_ARMSCII_8,
    "ARMSCII-8", N_("Armenian") },
  { GEDIT_ENCODING_BIG5,
    "BIG5", N_("Chinese Traditional") },
  { GEDIT_ENCODING_BIG5_HKSCS,
    "BIG5-HKSCS", N_("Chinese Traditional") },
  { GEDIT_ENCODING_CP_866,
    "CP866", N_("Cyrillic/Russian") },

  { GEDIT_ENCODING_EUC_JP,
    "EUC-JP", N_("Japanese") },
  { GEDIT_ENCODING_EUC_JP_MS,
    "EUC-JP-MS", N_("Japanese") },
  { GEDIT_ENCODING_CP932,
    "CP932", N_("Japanese") },

  { GEDIT_ENCODING_EUC_KR,
    "EUC-KR", N_("Korean") },
  { GEDIT_ENCODING_EUC_TW,
    "EUC-TW", N_("Chinese Traditional") },

  { GEDIT_ENCODING_GB18030,
    "GB18030", N_("Chinese Simplified") },
  { GEDIT_ENCODING_GB2312,
    "GB2312", N_("Chinese Simplified") },
  { GEDIT_ENCODING_GBK,
    "GBK", N_("Chinese Simplified") },
  { GEDIT_ENCODING_GEOSTD8,
    "GEORGIAN-ACADEMY", N_("Georgian") }, /* FIXME GEOSTD8 ? */
  { GEDIT_ENCODING_HZ,
    "HZ", N_("Chinese Simplified") },

  { GEDIT_ENCODING_IBM_850,
    "IBM850", N_("Western") },
  { GEDIT_ENCODING_IBM_852,
    "IBM852", N_("Central European") },
  { GEDIT_ENCODING_IBM_855,
    "IBM855", N_("Cyrillic") },
  { GEDIT_ENCODING_IBM_857,
    "IBM857", N_("Turkish") },
  { GEDIT_ENCODING_IBM_862,
    "IBM862", N_("Hebrew") },
  { GEDIT_ENCODING_IBM_864,
    "IBM864", N_("Arabic") },

  { GEDIT_ENCODING_ISO_2022_JP,
    "ISO-2022-JP", N_("Japanese") },
  { GEDIT_ENCODING_ISO_2022_KR,
    "ISO-2022-KR", N_("Korean") },
  { GEDIT_ENCODING_ISO_IR_111,
    "ISO-IR-111", N_("Cyrillic") },
  { GEDIT_ENCODING_JOHAB,
    "JOHAB", N_("Korean") },
  { GEDIT_ENCODING_KOI8_R,
    "KOI8R", N_("Cyrillic") },
  { GEDIT_ENCODING_KOI8__R,
    "KOI8-R", N_("Cyrillic") },
  { GEDIT_ENCODING_KOI8_U,
    "KOI8U", N_("Cyrillic/Ukrainian") },

  { GEDIT_ENCODING_SHIFT_JIS,
    "SHIFT_JIS", N_("Japanese") },
  { GEDIT_ENCODING_TCVN,
    "TCVN", N_("Vietnamese") },
  { GEDIT_ENCODING_TIS_620,
    "TIS-620", N_("Thai") },
  { GEDIT_ENCODING_UHC,
    "UHC", N_("Korean") },
  { GEDIT_ENCODING_VISCII,
    "VISCII", N_("Vietnamese") },

  { GEDIT_ENCODING_WINDOWS_1250,
    "WINDOWS-1250", N_("Central European") },
  { GEDIT_ENCODING_WINDOWS_1251,
    "WINDOWS-1251", N_("Cyrillic") },
  { GEDIT_ENCODING_WINDOWS_1252,
    "WINDOWS-1252", N_("Western") },
  { GEDIT_ENCODING_WINDOWS_1253,
    "WINDOWS-1253", N_("Greek") },
  { GEDIT_ENCODING_WINDOWS_1254,
    "WINDOWS-1254", N_("Turkish") },
  { GEDIT_ENCODING_WINDOWS_1255,
    "WINDOWS-1255", N_("Hebrew") },
  { GEDIT_ENCODING_WINDOWS_1256,
    "WINDOWS-1256", N_("Arabic") },
  { GEDIT_ENCODING_WINDOWS_1257,
    "WINDOWS-1257", N_("Baltic") },
  { GEDIT_ENCODING_WINDOWS_1258,
    "WINDOWS-1258", N_("Vietnamese") }
};

const GeditEncoding *
gedit_encoding_get_from_index (gint index)
{
	//g_return_val_if_fail (index >= 0, NULL);

	if (index >= GEDIT_ENCODING_LAST)
		return NULL;

	//gedit_encoding_lazy_init ();

	return &encodings [index];
}

const GeditEncoding *
gedit_encoding_get_utf8 (void)
{
	//gedit_encoding_lazy_init ();

	return &utf8_encoding;
}


static gchar *homebank_utf8_convert(gchar *buffer, const gchar **charset)
{
GError *conv_error;
gchar* conv_buffer = NULL;
gsize new_len;
guint i;
gboolean valid;
const struct _GeditEncoding *enc;

	DB( g_print("(homebank) homebank_utf8_convert\n") );

	for (i=0 ; i<GEDIT_ENCODING_LAST ; i++)
	{
		conv_error = NULL;

		enc = gedit_encoding_get_from_index(i);
		DB( g_print("-> should try %s\n",  enc->charset) );

		conv_buffer = g_convert(buffer, -1, "UTF-8", enc->charset, NULL, &new_len, &conv_error);
		valid = g_utf8_validate (conv_buffer, -1, NULL);
		if ((conv_error != NULL) || !valid )
		{
			DB( g_print ("  -> Couldn't convert from %s to UTF-8.\n", enc->charset) );
		}
		else
		{
			DB( g_print ("  -> file compatible with '%s'\n", enc->charset) );
			if(charset != NULL)
				*charset = enc->charset;
			return conv_buffer;
		}
	}

	if(charset != NULL)
		*charset = NULL;
	return NULL;
}


/*
 * Ensure a buffer to be utf-8, and convert if necessary
 *
 */
gchar *homebank_utf8_ensure(gchar *buffer)
{
gboolean isvalid;
gchar *converted;

	DB( g_print("(homebank) homebank_utf8_ensure\n") );

	if(buffer == NULL)
		return NULL;

	isvalid = g_utf8_validate(buffer, -1, NULL);
	DB( g_print(" -> is valid utf8: %d\n", isvalid) );

	if(!isvalid)
	{
		converted = homebank_utf8_convert(buffer, NULL);
		if(converted != NULL)
		{
			//g_warn here ?
			g_free(buffer);
			return converted;
		}
		//g_warn here ?
	}
	return buffer;
}


const gchar *homebank_file_getencoding(gchar *filename)
{
const gchar *charset = NULL;
gchar *buffer;
gsize length;
GError *error = NULL;
gboolean isutf8;
const gchar *locale_charset;
const struct _GeditEncoding *enc;

	DB( g_print("(homebank) test encoding\n") );

	if (g_get_charset (&locale_charset) == FALSE)
	{
		//unknown_encoding.charset = g_strdup (locale_charset);

	}

	DB( g_print(" -> locale charset is '%s'\n", locale_charset) );

	if (g_file_get_contents (filename, &buffer, &length, &error))
	{

		isutf8 = g_utf8_validate(buffer, -1, NULL);
		DB( g_print(" -> is valid utf8: %d\n", isutf8) );

		if( isutf8 == FALSE )
		{
		gchar *converted;

			converted = homebank_utf8_convert(buffer, &charset);

			DB( g_print(" -> converted charset match: '%s'\n", charset) );
			DB( g_print(" -> converted: '%p' %s\n", converted, converted) );

			if(converted != NULL)
				g_free(converted);
		}
		else
		{
			enc = gedit_encoding_get_utf8();
			charset = enc->charset;
		}


		g_free(buffer);
	}

	DB( g_print ("  -> charset is '%s'\n", charset) );

	return charset;
}


