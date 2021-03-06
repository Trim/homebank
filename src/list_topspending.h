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

#ifndef __LIST_TOPSPENDING__H__
#define __LIST_TOPSPENDING__H__


/* list top spending */
enum
{
	LST_TOPSPEND_ID,	//fake for pie
	LST_TOPSPEND_KEY,	//fake for pie
	LST_TOPSPEND_NAME,
	LST_TOPSPEND_AMOUNT,
	LST_TOPSPEND_RATE,

	NUM_LST_TOPSPEND
};


GtkWidget *create_list_topspending(void);

#endif
