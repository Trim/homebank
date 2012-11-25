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

#ifndef __LIST_UPCOMING__H__
#define __LIST_UPCOMING__H__

GtkWidget *create_list_upcoming(void);

enum
{
	LST_DSPUPC_DATAS,
	LST_DSPUPC_PAYEE,
	LST_DSPUPC_WORDING,
	LST_DSPUPC_AMOUNT,
	LST_DSPUPC_ACCOUNT,
	LST_DSPUPC_NEXTON,
	LST_DSPUPC_REMAINING,
	NUM_LST_DSPUPC
};

#endif