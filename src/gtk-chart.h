/*  HomeBank -- Free, easy, personal accounting for everyone.
 *  Copyright (C) 1995-2014 Maxime DOYEN
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

#ifndef __GTK_CHART_H__
#define __GTK_CHART_H__

#include <glib.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <cairo.h>
#include <math.h>

G_BEGIN_DECLS
#define GTK_TYPE_CHART            (gtk_chart_get_type ())
#define GTK_CHART(obj)			  (G_TYPE_CHECK_INSTANCE_CAST ((obj), GTK_TYPE_CHART, GtkChart))
#define GTK_CHART_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), GTK_TYPE_CHART, GtkChartClass)
#define GTK_IS_CHART(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), GTK_TYPE_CHART))
#define GTK_IS_CHART_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), GTK_TYPE_CHART))
#define GTK_CHART_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), GTK_TYPE_CHART, GtkChartClass))

typedef struct _GtkChart		GtkChart;
typedef struct _GtkChartClass	GtkChartClass;
//typedef struct _GtkChartPrivate	GtkChartPrivate;

typedef gchar (* GtkChartPrintIntFunc)    (gint value, gboolean minor);
typedef gchar (* GtkChartPrintDoubleFunc) (gdouble value, gboolean minor);

/* = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

enum
{
	CHART_BAR_TYPE,
	CHART_LINE_TYPE,
	CHART_PIE_TYPE,
	CHART_TYPE_MAX
};

/* default zoomx for charts */
#define GTK_CHART_BARW 		24
#define GTK_CHART_MINBARW 	8
#define GTK_CHART_MAXBARW 	64

#define CHART_BUFFER_LENGTH 128


#define DEFAULT_DELAY 500           /* Default delay in ms */

// for cairo pie
#define PIE_LINE_SLICE 0
#define SOFT_LIGHT  0
#define GRADIENT	0
#define CHART_PIE_DONUT	 0


/* new stuff */


#define MARGIN 8

//#define PROP_SHOW_MINOR		6
//#define PROP_SHOW_LEGEND	7


/* end */

enum
{
	LST_LEGEND_FAKE,
	LST_LEGEND_COLOR,
	LST_LEGEND_TITLE,
	LST_LEGEND_AMOUNT,
	LST_LEGEND_RATE,
	NUM_LST_LEGEND
};


/* you should access only the entry and list fields directly */
struct _GtkChart
{
	/*< private >*/
	GtkHBox			hbox;

	GtkWidget		*drawarea;
	GtkAdjustment	*adjustment;
	GtkWidget		*scrollbar;

	GtkWidget		*scrollwin;
	GtkWidget		*treeview;
	GtkTreeModel	*legend;

	GtkWidget		*tooltipwin;
	GtkWidget		*ttlabel;

	/* data storage */
	guint		entries;
	gchar		*title;
	gchar		**titles;
	gdouble		*datas1;
	gdouble		*datas2;

	/* chart properties */
	gint		type;
	gboolean	dual;
	gboolean	show_over;
	gboolean	show_xval;
	gint		decy_xval;
	//guint32		kcur;
	gboolean	minor;
	gdouble		minor_rate;
	gchar		*minor_symbol;

	/* color datas */
	struct rgbcol		*colors;
	gint	nb_cols;
	gint	cs_red, cs_green, cs_blue;


	double		l, t, b, r, w, h;
	/* our drawing rectangle with margin */
	double		legend_w;

	/* zones height */
	double		title_zh;


	double		ox, oy;
	gint		lastx, lasty, lastactive;
	gint		lastpress_x, lastpress_y;
	gint		active;
	guint		timer_tag;

	/* pie specifics */
	gdouble		total;
	gint		rayon, left, top;

	/* bar specifics */
	double	range, min, max, unit, minimum;
	gint	div;
	gint	visible;

	double font_h;

	double graph_x, graph_y, graph_width, graph_height;	//graph dimension
	double barw, blkw, posbarh, negbarh;

	gchar			buffer[CHART_BUFFER_LENGTH];
};

struct _GtkChartClass {
	GtkHBoxClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType gtk_chart_get_type (void) G_GNUC_CONST;

/* public function */
GtkWidget *gtk_chart_new(gint type);

void gtk_chart_set_type(GtkChart *chart, gint type);
void gtk_chart_set_color_scheme(GtkChart * chart, gint colorscheme);

void gtk_chart_set_datas(GtkChart *chart, GtkTreeModel *model, guint column, gchar *title);
void gtk_chart_set_dualdatas(GtkChart *chart, GtkTreeModel *model, guint column1, guint column2, gchar *title);

void gtk_chart_set_minor_prefs(GtkChart * chart, gdouble rate, gchar *symbol);
//void gtk_chart_set_currency(GtkChart * chart, guint32 kcur);

void gtk_chart_set_overdrawn(GtkChart * chart, gdouble minimum);
void gtk_chart_set_decy_xval(GtkChart * chart, gint decay);
void gtk_chart_set_barw(GtkChart * chart, gdouble barw);

void gtk_chart_show_legend(GtkChart * chart, gboolean visible);
void gtk_chart_show_overdrawn(GtkChart * chart, gboolean visible);
void gtk_chart_show_xval(GtkChart * chart, gboolean visible);
void gtk_chart_show_minor(GtkChart * chart, gboolean minor);

G_END_DECLS
#endif /* __GTK_CHART_H__ */
