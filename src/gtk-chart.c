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

#include <math.h>
#include <string.h>

#include <gtk/gtk.h>

#include "gtk-chart.h"

#define DEFAULT_DELAY 500           /* Default delay in ms */

#define MASKCOL 255
#define COLTO16(col8) ( (col8 | col8<<8 ) )
#define COLTOOVER(col8) ( (col8 + MASKCOL) / 2 )

/* new stuff */
#define HELPDRAW 0

#define MARGIN 6

#define OVER_ALPHA .25
#define OVER_COLOR (MASKCOL * OVER_ALPHA)

#define COLTOCAIRO(col8) 	 ( (col8 / 255.0) )

#define COLTOCAIROOVER(col8) ( ((col8 * (1 - OVER_ALPHA)) + OVER_COLOR ) / 255.0 )

/* end */

#define MYDEBUG 0

#if MYDEBUG
#define DB(x) (x);
#else
#define DB(x);
#endif

#define PROP_SHOW_MINOR		6
#define PROP_SHOW_LEGEND	7

static void         gtk_chart_class_init      (GtkChartClass *klass);
static void         gtk_chart_init            (GtkChart      *chart);
static void         gtk_chart_destroy         (GtkObject     *chart);

// for cairo pie
#define PIE_LINE_SLICE 0
#define SOFT_LIGHT 0
#define GRADIENT 0


double GetBase10(double num);
double GetUnit(double value);
gchar *chart_printval(GtkChart *chart, gdouble value);
void chart_clear(GtkChart *chart, gboolean store);
void chart_setup_with_model(GtkChart *chart, GtkTreeModel *list_store, guint column1, guint column2);
void chart_sizeallocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data);
//static gboolean chart_map( GtkWidget *widget, GdkEvent *event, gpointer user_data);
static gboolean chart_expose( GtkWidget *widget, GdkEventExpose *event, gpointer user_data);
static gboolean chart_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data);
static gboolean chart_button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean chart_button_release (GtkWidget *widget, GdkEventButton *event, gpointer user_data);
static gboolean chart_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);
static gboolean chart_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data);

void chart_tooltip_start_delay(GtkChart *chart);


void barchart_calculation(GtkChart *chart);
static void barchart_draw_scale(GtkWidget *widget, gpointer user_data);
static void barchart_draw_bars(GtkWidget *widget, gpointer user_data);
static gint barchart_get_active(GtkWidget *widget, gint x, gint y, gpointer user_data);
static void barchart_first_changed( GtkAdjustment *adj, gpointer user_data);
void barchart_scrollbar_setvalues(GtkChart *chart);
void barchart_compute_range(GtkChart *chart);

void piechart_calculation(GtkChart *chart);
static void piechart_draw_slices(GtkWidget *widget, gpointer user_data);
static gint piechart_get_active(GtkWidget *widget, gint x, gint y, gpointer user_data);

static void chart_tooltip_hide(GtkChart *chart);
static void chart_tooltip_show(GtkChart *chart, gint xpos, gint ypos);
void chart_cleanup_colors(GtkWidget *widget);

gboolean chart_setup_colors(GtkWidget *widget, GtkChart *chart );
static GdkPixbuf *create_color_pixbuf (GdkColor *col);
GtkWidget *legend_list_new(GtkChart *chart);

void chart_recompute(GtkChart *chart);

gchar *chart_print_int(GtkChart *chart, gint value);
gchar *chart_print_double(GtkChart *chart, gdouble value);

static GtkHBoxClass *parent_class = NULL;


struct mycolors
{
	guint8	r, g, b;
};


struct mycolors money_colors[] =
{
	{ 255, 193,  96 },
	{  92, 131, 180 },
	{ 165,  88, 124 },
	{ 108, 124, 101 },
	{ 230, 121,  99 },
	{  91, 160, 154 },
	{ 207,  93,  96 },
	{  70, 136, 106 },

	{ 245, 163,  97 },
	{ 158, 153,  88 },
	{ 255, 140,  90 },
	{ 122, 151, 173 },
	{  84, 142, 128 },
	{ 185, 201, 149 },
	{ 165,  99, 103 },
	{  77, 140, 172 },

	{ 251, 228, 128 },
	{  73,  99, 149 },
	{ 192,  80,  77 },
	{ 139, 180, 103 },
	{ 132, 165, 214 },
	{ 221, 216, 115 },
	{  77, 103, 137 },
	{ 165, 181, 156 },
};


struct mycolors quicken_colors[] =
{
	{ 226,  73,  13 },
	{ 223, 180,   6 },
	{ 124, 179,   0 },
	{  44, 108, 182 },
	{ 184,  81, 186 },
	{ 165, 165, 165 },
	{ 122, 122, 122 },
	{ 137,  42,  40 },
	{  70, 161, 100 },
	{ 220, 106,   0 },
	{ 113, 113, 113 },
};


struct mycolors analytics_colors[] =
{
	{   5, 141, 199 },
	{  80, 180,  50 },
	{ 237,  86,  27 },
	{ 237, 239,   0 },
	{  36, 203, 229 },
	{ 100, 229, 114 },
	{ 255, 150,  85 },
	{ 255, 242,  99 },
	{ 106, 249, 196 },
	{ 178, 222, 255 },
	{ 204, 204, 204 },
};


struct mycolors office2010_colors[] =
{
	{  60, 100, 149 },
	{ 150,  60,  59 },
	{ 120, 147,  68 },
	{  99,  75, 123 },
	{  61, 133, 157 },
	{ 196, 115,  49 },

	{  73, 120, 176 },
	{ 179,  74,  71 },
	{ 144, 178,  84 },
	{ 117,  93, 153 },
	{  73, 161, 185 },
	{ 232, 140,  65 },

	{ 126, 155, 199 },
	{ 202, 126, 126 },
	{ 174, 197, 129 },
	{ 156, 137, 182 },
	{ 123, 185, 206 },
	{ 248, 170, 121 },
};


struct mycolors *colors = money_colors;
gint nb_colors = G_N_ELEMENTS(money_colors);



	
	
	



GType
gtk_chart_get_type ()
{
static GType chart_type = 0;

	if (!chart_type)
    {
		static const GTypeInfo chart_info =
		{
		sizeof (GtkChartClass),
		NULL,		/* base_init */
		NULL,		/* base_finalize */
		(GClassInitFunc) gtk_chart_class_init,
		NULL,		/* class_finalize */
		NULL,		/* class_data */
		sizeof (GtkChart),
		0,		/* n_preallocs */
		(GInstanceInitFunc) gtk_chart_init,

		};

		chart_type = g_type_register_static (GTK_TYPE_HBOX, "GtkChart",
							 &chart_info, 0);

	}
	return chart_type;
}

static void
gtk_chart_class_init (GtkChartClass * class)
{
  //GObjectClass *gobject_class;
  GtkObjectClass *object_class;
  //GtkWidgetClass *widget_class;

  //gobject_class = (GObjectClass*) class;
  object_class = (GtkObjectClass*) class;
  //widget_class = (GtkWidgetClass*) class;

  parent_class = g_type_class_peek_parent (class);

	DB( g_print("\n[gtkchart] class_init\n") );

	object_class->destroy = gtk_chart_destroy;

}

static void
gtk_chart_init (GtkChart * chart)
{
GtkWidget *widget, *vbox, *frame, *scrollwin, *treeview;

	DB( g_print("\n[gtkchart] init\n") );

 	chart->entries = 0;
 	chart->model = NULL;
 	chart->title = NULL;
 	chart->id = NULL;
 	chart->titles = NULL;
 	chart->datas1 = NULL;
 	chart->datas2 = NULL;
	chart->dual = FALSE;
	chart->barw = GTK_CHART_BARW;

	chart->tooltipwin = NULL;
	chart->active = -1;
	chart->lastactive = -1;

 	chart->minor_rate = 1.0;
	chart->timer_tag = 0;

	widget=GTK_WIDGET(chart);

	GTK_BOX(widget)->homogeneous = FALSE;

	vbox = gtk_vbox_new (FALSE, 0);
    gtk_box_pack_start (GTK_BOX (widget), vbox, TRUE, TRUE, 0);

	/* drawing area */
	frame = gtk_frame_new(NULL);
    gtk_box_pack_start (GTK_BOX (vbox), frame, TRUE, TRUE, 0);
    gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_ETCHED_IN);

	chart->drawarea = gtk_drawing_area_new();
	gtk_container_add( GTK_CONTAINER(frame), chart->drawarea );
	gtk_widget_set_size_request(chart->drawarea, 150, 150 );
	gtk_widget_show(chart->drawarea);

	/* scrollbar */
    chart->adjustment = GTK_ADJUSTMENT(gtk_adjustment_new (0.0, 0.0, 1.0, 1.0, 1.0, 1.0));
    chart->scrollbar = gtk_hscrollbar_new (GTK_ADJUSTMENT (chart->adjustment));
    gtk_box_pack_start (GTK_BOX (vbox), chart->scrollbar, FALSE, TRUE, 0);


	/* legend treeview */
	scrollwin = gtk_scrolled_window_new(NULL,NULL);
	chart->scrollwin = scrollwin;
    gtk_scrolled_window_set_shadow_type (GTK_SCROLLED_WINDOW (scrollwin), GTK_SHADOW_ETCHED_IN);
	//gtk_container_set_border_width (GTK_CONTAINER(scrollwin), 5);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrollwin), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
	treeview = legend_list_new(chart);
	chart->treeview = treeview;
	chart->legend = gtk_tree_view_get_model(GTK_TREE_VIEW(treeview));
	gtk_container_add(GTK_CONTAINER(scrollwin), treeview);
	gtk_box_pack_start (GTK_BOX (widget), scrollwin, FALSE, FALSE, 0);

	chart_setup_colors(widget, chart);

	gtk_widget_set_events(GTK_WIDGET(chart->drawarea),
		GDK_EXPOSURE_MASK |
		GDK_POINTER_MOTION_MASK |
		GDK_POINTER_MOTION_HINT_MASK |
		GDK_ENTER_NOTIFY_MASK |
		GDK_LEAVE_NOTIFY_MASK |
		GDK_BUTTON_PRESS_MASK |
		GDK_BUTTON_RELEASE_MASK
		);

	g_signal_connect( G_OBJECT(chart->drawarea), "size-allocate", G_CALLBACK(chart_sizeallocate), chart ) ;
	//g_signal_connect( G_OBJECT(chart->drawarea), "map-event", G_CALLBACK(chart_map), chart ) ;
	g_signal_connect( G_OBJECT(chart->drawarea), "expose-event", G_CALLBACK(chart_expose), chart ) ;

	g_signal_connect( G_OBJECT(chart->drawarea), "motion-notify-event", G_CALLBACK(chart_motion), chart );
	g_signal_connect( G_OBJECT(chart->drawarea), "leave-notify-event", G_CALLBACK(chart_leave), chart );
	g_signal_connect( G_OBJECT(chart->drawarea), "enter-notify-event", G_CALLBACK(chart_enter), chart );
	g_signal_connect( G_OBJECT(chart->drawarea), "button-press-event", G_CALLBACK(chart_button_press), chart );
	g_signal_connect( G_OBJECT(chart->drawarea), "button-release-event", G_CALLBACK(chart_button_release), chart );

	g_signal_connect (G_OBJECT(chart->adjustment), "value_changed", G_CALLBACK (barchart_first_changed), chart);

}

/* --- */

GtkWidget *
gtk_chart_new (gint type)
{
GtkChart *chart;

	DB( g_print("\n[gtkchart] new\n") );

	chart = g_object_new (GTK_TYPE_CHART, NULL);
	chart->type = type;

	return GTK_WIDGET(chart);
}

static void
gtk_chart_destroy (GtkObject * object)
{
GtkChart *chart;

	DB( g_print("\n[gtkchart] destroy\n") );

  g_return_if_fail (GTK_IS_CHART (object));

	chart = GTK_CHART (object);

	chart_clear(chart, FALSE);

	chart_tooltip_hide(chart);

  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */
/* public functions */
/* = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = = */

/*
** change the model and/or column
*/
void gtk_chart_set_datas(GtkChart *chart, GtkTreeModel *model, guint column)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	if( GTK_IS_TREE_MODEL(model) )
	{
		chart_setup_with_model(chart, model, column, -1);
		chart_recompute(chart);
	}
	else
	{
		chart_clear(chart, TRUE);
	}
}

/*
** change the model and/or column
*/
void gtk_chart_set_dualdatas(GtkChart *chart, GtkTreeModel *model, guint column1, guint column2)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	if( GTK_IS_TREE_MODEL(model) )
	{
		chart_setup_with_model(chart, model, column1, column2 );
		chart_recompute(chart);
	}
	else
	{
		chart_clear(chart, TRUE);
	}
}

/*
** change the tooltip title
*/
void gtk_chart_set_title(GtkChart * chart, gchar *title)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->title = g_strdup(title);

	DB( g_print("\n[gtkchart] set title = %s\n", chart->title) );

}

/*
** change the type dynamically
*/
void gtk_chart_set_type(GtkChart * chart, gint type)
{
	g_return_if_fail (GTK_IS_CHART (chart));
	//g_return_if_fail (type < CHART_TYPE_MAX);

	DB( g_print("\n[gtkchart] set type %d\n", type) );

	chart->type = type;
	chart->dual = FALSE;
	chart_recompute(chart);
}

/* = = = = = = = = = = parameters = = = = = = = = = = */

void gtk_chart_set_color_scheme(GtkChart * chart, gint colorscheme)
{
	switch(colorscheme)
	{
		default:
		case CHART_COLMAP_MSMONEY:
			colors = money_colors;
			nb_colors = G_N_ELEMENTS(money_colors);
			break;
		case CHART_COLMAP_QUICKEN:
			colors = quicken_colors;
			nb_colors = G_N_ELEMENTS(quicken_colors);
			break;
		case CHART_COLMAP_ANALYTICS:
			colors = analytics_colors;
			nb_colors = G_N_ELEMENTS(analytics_colors);
			break;
		case CHART_COLMAP_OFFICE2010:
			colors = office2010_colors;
			nb_colors = G_N_ELEMENTS(office2010_colors);
			break;
	}
}



/*
** set the minor parameters
*/
void gtk_chart_set_minor_prefs(GtkChart * chart, gdouble rate, gchar *symbol)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->minor_rate   = rate;
	chart->minor_symbol = symbol;
}


/*
void gtk_chart_set_currency(GtkChart * chart, guint32 kcur)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->kcur = kcur;
}
*/

/*
** set the overdrawn minimum
*/
void gtk_chart_set_overdrawn(GtkChart * chart, gdouble minimum)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->minimum = minimum;

	if(chart->type == CHART_LINE_TYPE)
		chart_recompute(chart);
}

/*
** set the decy_xval
*/
void gtk_chart_set_decy_xval(GtkChart * chart, gint decay)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->decy_xval = decay;

	if(chart->type != CHART_PIE_TYPE)
		chart_recompute(chart);
}
/*
** set the barw
*/
void gtk_chart_set_barw(GtkChart * chart, gdouble barw)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->barw = barw;

	if(chart->type != CHART_PIE_TYPE)
		chart_recompute(chart);
}


/* = = = = = = = = = = visibility = = = = = = = = = = */

/*
** change the legend visibility
*/
void gtk_chart_show_legend(GtkChart * chart, gboolean visible)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	if(visible == TRUE)
		gtk_widget_show(chart->scrollwin);
	else
		gtk_widget_hide(chart->scrollwin);
}

/*
** change the x-value visibility
*/
void gtk_chart_show_xval(GtkChart * chart, gboolean visible)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->show_xval = visible;

	if(chart->type != CHART_PIE_TYPE)
		chart_recompute(chart);
}

/*
** chnage the overdrawn visibility
*/
void gtk_chart_show_overdrawn(GtkChart * chart, gboolean visible)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->show_over = visible;

	if(chart->type == CHART_LINE_TYPE)
		chart_recompute(chart);
}


/*
** change the minor visibility
*/
void gtk_chart_show_minor(GtkChart * chart, gboolean minor)
{
	g_return_if_fail (GTK_IS_CHART (chart));

	chart->minor = minor;

	if(chart->type != CHART_PIE_TYPE)
		chart_recompute(chart);

	gtk_tree_view_columns_autosize (GTK_TREE_VIEW(chart->treeview));
}



// ---------------------------------------------
/*
**
*/
double GetBase10(double num)
{
double result;
double cnt;

	for(cnt=0;;cnt++)
	{
		if(floor(num) == 0) break;
		num = num / 10;
	}
	result = pow(10.0, cnt-1);
	return(result);
}

/*
**
*/
double GetUnit(double value)
{
double truc, base10, unit;

	value = ABS(value);

	base10 = GetBase10(value);
	truc = value / base10;
	if(truc > 5) unit = base10;
	else
		if(truc > 2) unit = 0.5*base10;
		else
			if(truc > 1) unit = 0.2*base10;
			else
				unit = 0.1*base10;

	return(unit);
}

/*
** print a integer number
*/
gchar *chart_print_int(GtkChart *chart, gint value)
{

	//mystrfmon(chart->buffer, CHART_BUFFER_LENGTH-1, (gdouble)value, chart->minor);
	mystrfmon_int(chart->buffer, CHART_BUFFER_LENGTH-1, (gdouble)value, chart->minor);

	/*
	if(chart->minor)
	{
		value *= chart->minor_rate;
		strfmon(chart->buffer, CHART_BUFFER_LENGTH-1, "%!.0n ", (gdouble)value);
		strcat(chart->buffer, chart->minor_symbol);
	}
	else
		strfmon(chart->buffer, CHART_BUFFER_LENGTH-1, "%.0n", (gdouble)value);
	*/

	return chart->buffer;
}

/*
** print a double number
*/
gchar *chart_print_double(GtkChart *chart, gdouble value)
{

	mystrfmon(chart->buffer, CHART_BUFFER_LENGTH-1, value, chart->minor);

	/*
	if(chart->minor)
	{
		value *= chart->minor_rate;
		strfmon(chart->buffer, CHART_BUFFER_LENGTH-1, "%!n ", (gdouble)value);
		strcat(chart->buffer, chart->minor_symbol);
	}
	else
		strfmon(chart->buffer, CHART_BUFFER_LENGTH-1, "%n", (gdouble)value);
	*/

	return chart->buffer;
}


/*
** clear any allocated memory
*/
void chart_clear(GtkChart *chart, gboolean store)
{
guint i;

	DB( g_print("\n[gtkchart] clear\n") );

	//free & clear any previous allocated datas
	if(chart->title != NULL)
	{
		g_free(chart->title);
		chart->title = NULL;
	}

	if(chart->titles != NULL)
	{
		for(i=0;i<chart->entries;i++)
		{
			g_free(chart->titles[i]);
		}
		g_free(chart->titles);
		chart->titles = NULL;
	}

	if(chart->datas1 != NULL)
	{
		g_free(chart->datas1);
		chart->datas1 = NULL;
	}

	if(chart->datas2 != NULL)
	{
		g_free(chart->datas1);
		chart->datas2 = NULL;
	}

	if(chart->id != NULL)
	{
		g_free(chart->id);
		chart->id = NULL;
	}

	if(store == TRUE)
	{
		gtk_list_store_clear (GTK_LIST_STORE(chart->legend));

	}

	chart->entries = 0;

	chart->total = 0;
	chart->range = 0;
	chart->min = 0;
	chart->max = 0;
	chart->decy_xval = 7;

}

/*
** setup our chart with a model and column
*/
void chart_setup_with_model(GtkChart *chart, GtkTreeModel *list_store, guint column1, guint column2)
{
guint i;
gboolean valid;
GtkTreeIter iter, l_iter;
gint color;
GdkColor colour;

	DB( g_print("\n[gtkchart] setup with model\n") );

	chart_clear(chart, TRUE);

	chart->entries = gtk_tree_model_iter_n_children(GTK_TREE_MODEL(list_store), NULL);

	DB( g_print(" nb=%d\n", chart->entries) );


	chart->id     = g_malloc0(chart->entries * sizeof(gint));	//todo:not used ?
	chart->titles = g_malloc0(chart->entries * sizeof(gchar *));
	chart->datas1 = g_malloc0(chart->entries * sizeof(gdouble));
	chart->datas2 = g_malloc0(chart->entries * sizeof(gdouble));

	/* dual mode ? */
	if( chart->type == CHART_BAR_TYPE )
		chart->dual = column2 != -1 ? TRUE : FALSE;
	else
		chart->dual = FALSE;

	/* Get the first iter in the list */
	valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(list_store), &iter);
	i = 0;
	while (valid)
    {
	gint id;
	gchar *title;
	gdouble	value1, value2;

		/* column 0: pos (gint) */
		/* column 1: key (gint) */
		/* column 2: name (gchar) */
		/* column x: values (double) */

		if( !chart->dual )
		{
			gtk_tree_model_get (GTK_TREE_MODEL(list_store), &iter,
				0, &id,
				2, &title,
				column1, &value1,
				-1);
		}
		else
		{
			gtk_tree_model_get (GTK_TREE_MODEL(list_store), &iter,
				0, &id,
				2, &title,
				column1, &value1,
				column2, &value2,
				-1);
		}

		/* ignore negative value: total, partial total */
		//todo: remove this (was a test)
		if(id < 0)
		{
			chart->entries--;
	     	 //DB( g_print ("ignoring Row %d: (%s, %2.f)\n", id, title, value) );
		}
		else
		{
			chart->id[i] = id;
			chart->titles[i] = title;

			/* data1 value storage & min, max compute */
			chart->datas1[i] = (chart->dual) ? ABS(value1) : value1;
			chart->min = MIN(chart->min, chart->datas1[i]);
			chart->max = MAX(chart->max, chart->datas1[i]);

			if( chart->dual )
			{
				/* data2 value storage & min, max compute */
				chart->datas2[i] = (chart->dual) ? ABS(value2) : value2;
				chart->min = MIN(chart->min, chart->datas2[i]);
				chart->max = MAX(chart->max, chart->datas2[i]);
			}

			/* pie chart total sum */
			chart->total += ABS(chart->datas1[i]);

	     	 /* Do something with the data */

		//populate our legend list
			color = i % chart->nb_colors;
			//color = id % chart->nb_colors;

			//DB( g_print ("Row %d: (%s, %2.f) color %d\n", id, title, value, color) );

			colour.red   = COLTO16(colors[color].r);
			colour.green = COLTO16(colors[color].g);
			colour.blue  = COLTO16(colors[color].b);

	        gtk_list_store_append (GTK_LIST_STORE(chart->legend), &l_iter);
	        gtk_list_store_set (GTK_LIST_STORE(chart->legend), &l_iter,
	                            LST_LEGEND_COLOR, create_color_pixbuf (&colour),
	                            LST_LEGEND_TITLE, title,
	                            LST_LEGEND_AMOUNT, value1,
	                            -1);

			i++;
		}
		valid = gtk_tree_model_iter_next (list_store, &iter);
	}

	// compute rate for legend for bar/pie
	if( chart->type != CHART_LINE_TYPE )
	{

		valid = gtk_tree_model_get_iter_first (GTK_TREE_MODEL(chart->legend), &iter);
		while (valid)
		{
		gdouble amount, ramount;
		gint rate;
		
			gtk_tree_model_get(GTK_TREE_MODEL(chart->legend), &iter,
				LST_LEGEND_AMOUNT, &amount,
			-1);

			ramount = floor((amount * 100) + 0.5) / 100;
			if(ramount == 0.0)
				rate = 0;
			else
				rate = (gint)((ABS(amount)*100)/ABS(chart->total)) + 0.5;


			gtk_list_store_set(GTK_LIST_STORE(chart->legend), &iter,
				LST_LEGEND_RATE, rate,
			-1);

	
			valid = gtk_tree_model_iter_next (GTK_TREE_MODEL(chart->legend), &iter);
		}

	}

	


	//g_print("total is %.2f\n", total);
	//ensure the widget is mapped
	//gtk_widget_map(chart);

}

/*
** recompute according to type
*/
void chart_recompute(GtkChart *chart)
{

	DB( g_print("\n[gtkchart] recompute\n") );

	switch(chart->type)
	{
		case CHART_LINE_TYPE:
			//gtk_widget_show(chart->scrollwin);

		case CHART_BAR_TYPE:
			barchart_compute_range(chart);

			barchart_calculation(chart);
			chart->adjustment->value = 0;
			barchart_scrollbar_setvalues(chart);
			gtk_widget_show(chart->scrollbar);
			break;
		case CHART_PIE_TYPE:
			piechart_calculation(chart);
			gtk_widget_hide(chart->scrollbar);
			break;
	}
	gtk_widget_queue_draw( chart->drawarea );
}




/* bar section */

void barchart_compute_range(GtkChart *chart)
{
gint div;

	DB( g_print("\n[gtkchart] bar compute range\n") );

	chart->range = chart->max - chart->min;

	DB(g_print(" initial: min=%.2f, max=%.2f, range=%.2f\n", chart->min, chart->max, chart->range) );

	chart->unit  = GetUnit(chart->range);

	div = ceil(chart->max / chart->unit);
	chart->max   = div * chart->unit;
	chart->div   = div;

	div = ceil(-chart->min / chart->unit);
	chart->min   = -div * chart->unit;
	chart->div  += div;

	chart->range = chart->max - chart->min;

	/*
	chart->unit  = GetUnit(chart->range);
	chart->div   = (ceil(chart->range / chart->unit));
	chart->min   = -chart->unit*ceil(-chart->min/chart->unit);
	chart->max   = chart->unit*ceil(chart->max/chart->unit);
	chart->range = chart->unit*chart->div;
	*/

	DB(g_print(" unit=%.2f, div=%d => %d\n", chart->unit, chart->div, (gint)chart->unit*chart->div) );
	DB(g_print(" min=%.2f, max=%.2f, range=%.2f\n", chart->min, chart->max, chart->range) );


}

/*
**
** here we will compute every value which is size dependent
*/
void barchart_calculation(GtkChart *chart)
{
GtkWidget *drawarea = chart->drawarea;
cairo_surface_t *s;
cairo_t *cr;
cairo_text_extents_t extents;
gint blkw;

	DB( g_print("\n[gtkchart] bar calculation\n") );

	chart->l = MARGIN;
	chart->t = MARGIN;
	chart->w = drawarea->allocation.width - (MARGIN*2);
	chart->h = drawarea->allocation.height - (MARGIN*2);
	chart->r = drawarea->allocation.width - MARGIN;
	chart->b = drawarea->allocation.height - MARGIN;

	//debug
	//chart->legend_w = 100;

	//todo: seems not working well...
	s = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, 100, 100);

	cr = cairo_create (s);
	cairo_text_extents(cr, "ajpq12", &extents);

	cairo_destroy(cr);
	cairo_surface_destroy(s);

	DB( g_print(" + text w=%f h=%f\n", extents.width, extents.height) );

	chart->font_h = extents.height;
	chart->graph_width  = chart->w; // - chart->legend_w;
	chart->graph_height = chart->h - extents.height - 4;

	//if expand : we compute available space
	//chart->barw = MAX(32, (chart->graph_width)/chart->entries);
	//chart->barw = 32; // usr setted or defaut to BARW

	blkw = chart->barw;

	if( chart->dual )
		blkw = blkw * 2;

	chart->visible = chart->graph_width / blkw;
	chart->visible = MIN(chart->visible, chart->entries);

	chart->ox = chart->l;
	chart->oy = floor(chart->t + (chart->max/chart->range) * chart->graph_height);

	DB( g_print(" + ox=%f oy=%f\n", chart->ox, chart->oy) );


}


/*
** draw the scale
*/
static void barchart_draw_scale(GtkWidget *widget, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint i;
double x, y;
gdouble curxval;
gint first;

	DB( g_print("----------------------\n(gtkline) draw scale\n") );

cairo_t *cr;
static const double dashed3[] = {2.0};

	cr = gdk_cairo_create (widget->window);

	cairo_set_line_width(cr, 1);

	/* clip */
	cairo_rectangle(cr, MARGIN, 0, chart->w, chart->h + MARGIN);
	cairo_clip(cr);

	/* draw vertical lines + legend */
	if(chart->show_xval)
	{
	gint blkw = chart->barw;

		if( chart->dual )
			blkw = blkw * 2;

		x = chart->ox + 1 + (chart->barw/2);
		y = chart->oy;
		first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

		for(i=first; i<(first+chart->visible) ;i++)
		{
			if( !(i % chart->decy_xval) )
			{

				cairo_set_source_rgb(cr, COLTOCAIRO(238), COLTOCAIRO(238), COLTOCAIRO(238));

				cairo_move_to(cr, x, chart->t);
				cairo_line_to(cr, x, chart->b);
				cairo_stroke(cr);

			}

			x += blkw;
		}
	}

	/* horizontal lines */

	curxval = chart->max;
	for(i=0;i<=chart->div;i++)
	{

		if(i == 0 || i == chart->div) 	/* top/bottom line */
		{
			cairo_set_dash(cr, 0, 0, 0);
			cairo_set_source_rgb(cr, COLTOCAIRO(238), COLTOCAIRO(238), COLTOCAIRO(238));
		}
		else /* intermediate line (dotted) */
		{
			cairo_set_dash(cr, dashed3, 1, 0);

			cairo_set_source_rgb(cr, COLTOCAIRO(204), COLTOCAIRO(204), COLTOCAIRO(204));
		}

		/* x axis ? */
		if( curxval == 0.0 )
		{
			cairo_set_dash(cr, 0, 0, 0);
			cairo_set_source_rgb(cr, COLTOCAIRO(102), COLTOCAIRO(102), COLTOCAIRO(102));
		}


		y = 0.5 + floor(chart->t + ((i * chart->unit) / chart->range) * chart->graph_height);

		DB( g_print(" + i=%d :: y=%f (%f / %f) * %f\n", i, y, i*chart->unit, chart->range, chart->graph_height) );

		cairo_move_to(cr, chart->l, y);
		cairo_line_to(cr, chart->l + chart->w, y);
		cairo_stroke(cr);

		curxval -= chart->unit;
	}


	cairo_destroy(cr);

}

static void barchart_draw_scale_text(GtkWidget *widget, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint i;
double x, y;
gdouble curxval;
gchar *valstr;
gint first;

	DB( g_print("----------------------\n(gtkline) draw scale text\n") );

cairo_t *cr;
cairo_text_extents_t extents;


	cr = gdk_cairo_create (widget->window);

	cairo_set_line_width(cr, 1);

	/* clip */
	cairo_rectangle(cr, MARGIN, 0, chart->w, chart->h + MARGIN);
	cairo_clip(cr);

	//cairo_set_operator(cr, CAIRO_OPERATOR_SATURATE);

	/* draw y-legend (amount) */
	if(chart->show_xval)
	{
	gint blkw = chart->barw;

		if( chart->dual )
			blkw = blkw * 2;


		x = chart->ox + 1 + (chart->barw/2);
		y = chart->oy;
		first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

		for(i=first; i<(first+chart->visible) ;i++)
		{
			if( !(i % chart->decy_xval) )
			{
				valstr = chart->titles[i];
				cairo_text_extents(cr, valstr, &extents);

				DB( g_print("%s w=%f h=%f\n", valstr, extents.width, extents.height) );

				cairo_set_source_rgb(cr, 0, 0, 0);
				cairo_move_to(cr, x + 2, y + 2 + chart->font_h);
				cairo_show_text(cr, valstr);

				cairo_set_source_rgb(cr, COLTOCAIRO(153), COLTOCAIRO(153), COLTOCAIRO(153));
				cairo_move_to(cr, x, y);
				cairo_line_to(cr, x, y + 4 + extents.height);
				cairo_stroke(cr);
			}

			x += blkw;
		}
	}

	/* draw x-legend (amount) */

	curxval = chart->max;
	for(i=0;i<=chart->div;i++)
	{
		y = 0.5 + floor(chart->t + ((i * chart->unit) / chart->range) * chart->graph_height);

		DB( g_print(" + i=%d :: y=%f (%f / %f) * %f\n", i, y, i*chart->unit, chart->range, chart->graph_height) );

		if( curxval != 0.0 )
		{

			valstr = chart_print_int(chart, (gint)curxval);
			cairo_text_extents(cr, valstr, &extents);

			//DB( g_print("'%s', %f %f %f %f %f %f\n", valstr, extents.x_bearing, extents.y_bearing, extents.width, extents.height, extents.x_advance, extents.y_advance) );

			// draw white background
			cairo_set_line_width (cr, 2);
			cairo_set_source_rgba(cr, COLTOCAIRO(255), COLTOCAIRO(255), COLTOCAIRO(255), 0.66);
			cairo_move_to(cr, chart->l, y + 2 + extents.height + 0.5);
			cairo_text_path (cr, valstr);
			cairo_stroke (cr);

			cairo_move_to(cr, chart->r - extents.width, y + 2 + extents.height + 0.5);
			cairo_text_path (cr, valstr);
			cairo_stroke (cr);
			
			// draw texts
			cairo_move_to(cr, chart->l, y + 2 + extents.height);
			cairo_set_source_rgb(cr, COLTOCAIRO(102), COLTOCAIRO(102), COLTOCAIRO(102));
			cairo_show_text(cr, valstr);
			
			cairo_move_to(cr, chart->r - extents.width, y + 2 + extents.height);
			cairo_show_text(cr, valstr);
		}

		curxval -= chart->unit;
	}

	cairo_destroy(cr);
}

/*
** draw all visible bars
*/
static void barchart_draw_bars(GtkWidget *widget, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
cairo_t *cr;
double x, x2, y2, h;
gint first;
gint i;

	DB( g_print("\n[gtkchart] bar draw bars\n") );

	x = chart->ox + 1;
	//y = chart->oy;
	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

	cr = gdk_cairo_create (widget->window);

	/* clip */
	cairo_rectangle(cr, MARGIN, 0, chart->w, chart->h + MARGIN);
	cairo_clip(cr);

	#if HELPDRAW == 1
	x2 = x;
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, .0, 1.0, .0, .5);
	for(i=first; i<=(first+chart->visible) ;i++)
	{
		cairo_move_to(cr, x2, chart->t);
		cairo_line_to(cr, x2, chart->b);
		cairo_stroke(cr);
		x2 += chart->dual ? chart->barw * 2 : chart->barw;
	}
	#endif

	for(i=first; i<=(first+chart->visible) ;i++)
	{
	gint color;
	gint barw = chart->barw;
	gint blkw = chart->barw;

		if( chart->dual )
			blkw = blkw * 2;

		//if(!chart->datas1[i]) goto nextbar;

		color = i % chart->nb_colors;
		//color = chart->id[i] % chart->nb_colors;

		if( i == chart->active )
			cairo_set_source_rgb(cr, COLTOCAIROOVER(colors[color].r), COLTOCAIROOVER(colors[color].g), COLTOCAIROOVER(colors[color].b));
		else
			cairo_set_source_rgb(cr, COLTOCAIRO(colors[color].r), COLTOCAIRO(colors[color].g), COLTOCAIRO(colors[color].b));

		if(chart->datas1[i])
		{
			x2 = x + 2;
			h = floor((chart->datas1[i] / chart->range) * chart->graph_height);
			y2 = chart->oy - h;
			if(chart->datas1[i] < 0.0)
				y2 += 1;

			//DB( g_print(" + i=%d :: y2=%f h=%f (%f / %f) * %f\n", i, y2, h, chart->datas1[i], chart->range, chart->graph_height ) );


			cairo_rectangle(cr, x2, y2, barw-3, h);
			cairo_fill(cr);

		}

		if( chart->dual && chart->datas2[i])
		{

			x2 = x + barw + 1;
			h = floor((chart->datas2[i] / chart->range) * chart->graph_height);
			y2 = chart->oy - h;

			cairo_rectangle(cr, x2, y2, barw-3, h);
			cairo_fill(cr);

		}

		x += blkw;

		//debug
		//gdk_draw_line (widget->window, widget->style->fg_gc[widget->state], x, chart->oy-chart->posbarh, x, chart->oy+chart->negbarh);

	}

	cairo_destroy(cr);
	
}

/*
** get the bar under the mouse pointer
*/
static gint barchart_get_active(GtkWidget *widget, gint x, gint y, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint retval, first, index, px;
gint blkw = chart->barw;

	if( chart->dual )
		blkw = blkw * 2;

	retval = -1;

	if( x <= chart->r && x >= chart->l )
	{
		px = (x - chart->ox);
		//py = (y - chart->oy);
		first = gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
		index = first + (px / blkw);

		if(index < chart->entries)
			retval = index;
	}

	return(retval);
}

static void barchart_first_changed( GtkAdjustment *adj, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
//gint first;

	DB( g_print("\n[gtkchart] bar first changed\n") );

	//first = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

	//DB( g_print(" first=%d\n", first) );

/*
	DB( g_print("scrollbar\n adj=%8x, low=%.2f upp=%.2f val=%.2f step=%.2f page=%.2f size=%.2f\n", adj,
		adj->lower, adj->upper, adj->value, adj->step_increment, adj->page_increment, adj->page_size) );
 */
    /* Set the number of decimal places to which adj->value is rounded */
    //gtk_scale_set_digits (GTK_SCALE (hscale), (gint) adj->value);
    //gtk_scale_set_digits (GTK_SCALE (vscale), (gint) adj->value);
	gtk_widget_queue_draw(chart->drawarea);

}

/*
** scrollbar set values for upper, page size, and also show/hide
*/
void barchart_scrollbar_setvalues(GtkChart *chart)
{
GtkAdjustment *adj = chart->adjustment;
gint first;

	g_return_if_fail (GTK_IS_ADJUSTMENT (adj));

	DB( g_print("\n[gtkchart] sb_set_values\n") );

	//if(visible < entries)
	//{
		first = gtk_adjustment_get_value(GTK_ADJUSTMENT(adj));

		DB( g_print(" entries=%d, visible=%d\n", chart->entries, chart->visible) );
		DB( g_print(" first=%d, upper=%d, pagesize=%d\n", first, chart->entries, chart->visible) );

		adj->upper = (gdouble)chart->entries;
		adj->page_size = (gdouble)chart->visible;
		adj->page_increment = (gdouble)chart->visible;

		if(first+chart->visible > chart->entries)
			adj->value = (gdouble)chart->entries - chart->visible;
			//g_object_set(adj1, "value", (gdouble)entries-visible);

		gtk_adjustment_changed (adj);

		//gtk_widget_show(GTK_WIDGET(scrollbar));
	//}
	//else
		//gtk_widget_hide(GTK_WIDGET(scrollbar));


}

/* line section */

/*
** draw all visible lines
*/
static void draw_plot(cairo_t *cr, double x, double y, double r)
{
	cairo_set_line_width(cr, r / 2);

	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_arc(cr, x, y, r, 0, 2*M_PI);
	cairo_stroke_preserve(cr);

	cairo_set_source_rgb(cr, COLTOCAIRO(0), COLTOCAIRO(119), COLTOCAIRO(204));
	cairo_fill(cr);

}

static void linechart_draw_lines(GtkWidget *widget, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
cairo_t *cr;
double x, y, x2, y2, firstx, lastx, linew;
gint first;
gint i;

	DB( g_print("\n(gtkline) line draw lines\n") );

	x = chart->ox + 1;
	y = chart->oy;
	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

	cr = gdk_cairo_create (widget->window);

	/* clip */
	cairo_rectangle(cr, MARGIN, 0, chart->w, chart->h + MARGIN);
	cairo_clip(cr);


	#if HELPDRAW == 1
	x2 = x;
	cairo_set_line_width(cr, 1.0);
	cairo_set_source_rgba(cr, .0, 1.0, .0, .5);
	for(i=first; i<=(first+chart->visible) ;i++)
	{
		cairo_move_to(cr, x2, chart->t);
		cairo_line_to(cr, x2, chart->b);
		cairo_stroke(cr);
		x2 += chart->barw;
	}

	#endif

	//todo: it should be possible to draw line & plot together using surface and composite fill, or sub path ??
	lastx = x;
	firstx = x;
	linew = 4.0;
	if(chart->barw < 24)
	{
		linew = 1 + (chart->barw / 8.0);
	}

	cairo_set_line_join(cr, CAIRO_LINE_JOIN_BEVEL);
	cairo_set_line_width(cr, linew);

	for(i=first; i<=(first+chart->visible) ;i++)
	{
		x2 = x + (chart->barw)/2;
		y2 = chart->oy - (chart->datas1[i] / chart->range) * chart->graph_height;
		if( i == first)
		{
			firstx = x2;
			cairo_move_to(cr, x2, y2);
		}
		else
		{
			if( i < (chart->entries) )
			{
				cairo_line_to(cr, x2, y2);
				lastx = x2;
			}
			else
				lastx = x2 - chart->barw;
		}

		x += chart->barw;
	}

	cairo_set_source_rgb(cr, COLTOCAIRO(0), COLTOCAIRO(119), COLTOCAIRO(204));
	cairo_stroke_preserve(cr);

	cairo_line_to(cr, lastx, y);
	cairo_line_to(cr, firstx, y);
	cairo_close_path(cr);

	cairo_set_source_rgba(cr, COLTOCAIRO(0), COLTOCAIRO(119), COLTOCAIRO(204), .15);
	cairo_fill(cr);

	x = chart->ox + 1;
	y = chart->oy;
	first = (gint)gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));

	// draw plots

	for(i=first; i<(first+chart->visible) ;i++)
	{
		x2 = x + (chart->barw)/2;
		y2 = chart->oy - (chart->datas1[i] / chart->range) * chart->graph_height;
		draw_plot(cr,  x2, y2, i == chart->active ? linew+1 : linew);
		x += chart->barw;
	}

/* overdrawn */
	if( chart->show_over )
	{
		if(chart->minimum != 0 && chart->minimum >= chart->min)
		{
			y = chart->oy - (chart->minimum/chart->range) * chart->graph_height;
			cairo_set_source_rgba(cr, COLTOCAIRO(255), COLTOCAIRO(0), COLTOCAIRO(0), .15);

			DB( g_print(" draw over: %f, %f, %f, %f\n", chart->l, y, chart->w, chart->b - y) );

			cairo_rectangle(cr, chart->l, y, chart->w, chart->b - y);
			cairo_fill(cr);
		}
	}

	cairo_destroy(cr);


}




/*
** get the point under the mouse pointer
*/
static gint linechart_get_active(GtkWidget *widget, gint x, gint y, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint first, index, px, retval;

	retval = -1;

	if( x <= chart->r && x >= chart->l )
	{
		px = (x - chart->ox);
		//py = (y - chart->oy);
		first = gtk_adjustment_get_value(GTK_ADJUSTMENT(chart->adjustment));
		index = first + (px / (chart->barw));

		if(index < chart->entries)
			retval = index;
	}

	return(retval);
}



/* pie section */

void piechart_calculation(GtkChart *chart)
{
GtkWidget *drawarea = chart->drawarea;
gint w, h;

	w = drawarea->allocation.width - (MARGIN*2);
	h = drawarea->allocation.height - (MARGIN*2);

	chart->rayon = MIN(w, h);

	chart->ox    = drawarea->allocation.width / 2;
	chart->oy    = MARGIN + (chart->rayon / 2) + 1;	//drawarea->allocation.height / 2;

	//chart->left  = (w - chart->rayon) / 2;
	//chart->top   = (h - chart->rayon) / 2;
}

static void piechart_draw_slices(GtkWidget *widget, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
cairo_t *cr;
		
	if(chart->entries)
	{
		//cairo drawing

		double a1 = 0 * (M_PI / 180);
		double a2 = 360 * (M_PI / 180);

		//g_print("angle1=%.2f angle2=%.2f\n", a1, a2);

		double cx = chart->ox;
		double cy = chart->oy;
		double radius = chart->rayon/2;
		int i;
		double dx, dy;
		double sum = 0.0;
		double alpha;
		gint color;

		cr = gdk_cairo_create (widget->window);

		for(i=0; i< chart->entries ;i++)
		{
			a1 = ((360 * (sum / chart->total)) - 90) * (M_PI / 180);
			sum += ABS(chart->datas1[i]);
			a2 = ((360 * (sum / chart->total)) - 90) * (M_PI / 180);

			dx = cx;
			dy = cy;

			// move slice away if active
			/*
			if(i == chart->active)
			{
			gchar *txt;
			double ac = a1 + (a2 - a1) / 2;

				txt = "test";
				cairo_text_extents(cr, txt, &extents);

				dx = cx + (cos(ac)*(radius + extents.width));
				dy = cy + (sin(ac)*(radius + extents.height));

				cairo_set_source_rgb(cr, .0, .0, .0);
				cairo_move_to(cr, dx, dy);
				cairo_show_text(cr, txt);

				dx = cx + (cos(ac)*8);
				dy = cy + (sin(ac)*8);

			}*/

			cairo_move_to(cr, dx, dy);
			cairo_arc(cr, dx, dy, radius, a1, a2);

#if PIE_LINE_SLICE == 1
			cairo_set_line_width(cr, 1.0);
			cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
			cairo_line_to(cr, cx, cy);
			cairo_stroke_preserve(cr);
#endif

			DB( g_print("%d: %.2f%% %.2f %.2f\n", i, sum / chart->total, a1, a2) );

			//g_print("color : %f %f %f\n", COLTOCAIRO(colors[i].r), COLTOCAIRO(colors[i].g), COLTOCAIRO(colors[i].b));

			color = i % chart->nb_colors;
			//color = chart->id[i] % chart->nb_colors;

			alpha = (i == chart->active) ? (1.0 - OVER_ALPHA) : 1.0;
			cairo_set_source_rgba(cr, COLTOCAIRO(colors[color].r), COLTOCAIRO(colors[color].g), COLTOCAIRO(colors[color].b), alpha);

			cairo_fill(cr);

		}


#if SOFT_LIGHT == 1
	cairo_pattern_t *pat1;

	a1 = 0;
	a2 = 2 * M_PI;

	pat1 = cairo_pattern_create_radial( cx, cy, 0,
		cx, cy, radius);

	cairo_pattern_add_color_stop_rgba(pat1, 0.0, 1.0, 1.0, 1.0, .50);
	cairo_pattern_add_color_stop_rgba(pat1, 0.9, 1.0, 1.0, 1.0, 0.1);

	cairo_arc(cr, cx, cy,
			radius, a1, a2);
	 cairo_set_source(cr, pat1);
	cairo_fill(cr);
#endif

#if GRADIENT == 1
	cairo_pattern_t *pat1;

	a1 = 0;
	a2 = 2 * M_PI;
	double gradius = radius - 8;

	// start point, end point
	pat1 = cairo_pattern_create_linear(cx, cy-gradius, cx, cy+gradius);

	cairo_pattern_add_color_stop_rgba(pat1, 0.0, 1.0, 1.0, 1.0, .15);
	cairo_pattern_add_color_stop_rgba(pat1, 1.0, 1.0, 1.0, 1.0, 0.0);

	//debug
	//cairo_rectangle(cr, 	cx-radius, cy-radius, radius*2, radius*2);

	cairo_arc(cr, cx, cy, gradius, a1, a2);


	cairo_set_source(cr, pat1);
	cairo_fill(cr);

#endif

		cairo_destroy(cr);
	}

}


static gint piechart_get_active(GtkWidget *widget, gint x, gint y, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint retval, index, px, py;
double h;

	px = x - chart->ox;
	py = y - chart->oy;
	h  = sqrt( pow(px,2) + pow(py,2) );
	retval = -1;

	if(h < (chart->rayon/2))
	{
	double angle, b;

		b     = (acos(px / h) * 180) / M_PI;
		angle = py > 0 ? b : 360 - b;
		angle += 90;
		if(angle > 360) angle -= 360;
		//angle = 360 - angle;

		//todo optimize
		gdouble cumul = 0;
		for(index=0; index< chart->entries ;index++)
		{
			cumul += (ABS(chart->datas1[index])/chart->total)*360;
			if( cumul > angle )
			{
				retval = index;
				break;
			}
		}

		//DB( g_printf(" inside: x=%d, y=%d\n", x, y) );
		//DB( g_printf(" inside: b=%f angle=%f, slice is %d\n", b, angle, index) );
	}
	return(retval);
}






void chart_sizeallocate(GtkWidget *widget, GtkAllocation *allocation, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);

	if(chart->entries == 0)
		return;

	DB( g_print("\n[gtkchart] sizeallocate\n") );
	DB( g_print("w=%d h=%d\n", widget->allocation.width, widget->allocation.height) );

	//here we will compute any widget size dependend values


	switch(chart->type)
	{
		case CHART_BAR_TYPE:
		case CHART_LINE_TYPE:
			barchart_calculation(chart);
			barchart_scrollbar_setvalues(chart);
			break;
		case CHART_PIE_TYPE:
			piechart_calculation(chart);
			break;
	}

}


/* global widget function */

/*
static gboolean chart_map( GtkWidget *widget, GdkEvent *event, gpointer user_data)
{
//GtkChart *chart = GTK_CHART(user_data);

	DB( g_print("\n[gtkchart] map\n") );

	// our colors
	//chart_setup_colors(widget, chart);

	DB( g_print("** end map\n") );

	return TRUE;
}
*/

static gboolean chart_expose( GtkWidget *widget, GdkEventExpose *event, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);


	DB( g_print("\n[gtkchart] expose %d\n", chart->type) );

cairo_t *cr;
	cr = gdk_cairo_create (widget->window);

	/* fillin the back in whithe */
	cairo_set_source_rgb(cr, 1.0, 1.0, 1.0);
	cairo_paint(cr);

	/*debug help draws */
#if HELPDRAW == 1
		cairo_set_source_rgba(cr, 1.0, .0, .0, .5);
		cairo_rectangle(cr, chart->l, chart->t, chart->w, chart->h);
		cairo_fill(cr);

		cairo_set_source_rgba(cr, 1.0, .0, 1.0, .5);
		cairo_rectangle(cr, chart->l, chart->t, chart->graph_width, chart->graph_height);
		cairo_fill(cr);
#endif

	cairo_destroy(cr);

  if (event->count > 0)
    return FALSE;

	if(chart->entries == 0)
		return FALSE;


/*
	gdk_draw_rectangle (widget->window,
		      //widget->style->white_gc,
				widget->style->base_gc[widget->state],
		      TRUE,
		      0, 0,
		      widget->allocation.width,
		      widget->allocation.height);
*/

	switch(chart->type)
	{
		case CHART_BAR_TYPE:
			barchart_draw_scale(widget, chart);
			barchart_draw_bars(widget, chart);
			barchart_draw_scale_text(widget, chart);
			break;
		case CHART_LINE_TYPE:
			barchart_draw_scale(widget, chart);
			linechart_draw_lines(widget, chart);
			barchart_draw_scale_text(widget, chart);
			break;
		case CHART_PIE_TYPE:
			piechart_draw_slices(widget, chart);
			break;
	}

	return TRUE;
}

static gboolean chart_motion(GtkWidget *widget, GdkEventMotion *event, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint x, y;
GdkModifierType state;

	if(chart->entries == 0)
		return FALSE;

	//DB( g_print("\n[gtkchart] motion\n") );

	//todo see this
	if(event->is_hint)
	{
		//DB( g_print("is hint\n") );

		gdk_window_get_pointer(widget->window, &x, &y, &state);
	}
	else
	{
		x = event->x;
		y = event->y;
		state = event->state;
	}

	//DB( g_print(" x=%d, y=%d, time=%d\n", x, y, event->time) );

	switch(chart->type)
	{
		case CHART_BAR_TYPE:
			chart->active = barchart_get_active(widget, x, y, chart);
			break;
		case CHART_LINE_TYPE:
			chart->active = linechart_get_active(widget, x, y, chart);
			break;
		case CHART_PIE_TYPE:
			chart->active = piechart_get_active(widget, x, y, chart);
			break;
	}

	// rollover redraw ?
	DB( g_print(" active: last=%d, curr=%d\n", chart->lastactive, chart->active) );

	if(chart->lastactive != chart->active)
	{
		DB( g_print(" motion rollover redraw :: active=%d\n", chart->active) );

		//set_info(active);
		gtk_widget_queue_draw( widget );
//		chart_tooltip_start_delay(chart);
	}

	chart->lastactive = chart->active;

	// hide the tooltip ?
	if( (x != chart->lastpress_x || y != chart->lastpress_y) )
	{
		//DB( g_print("hide tooltip\n") );
		chart_tooltip_hide(chart);

	}

	if (chart->timer_tag == 0)
	{
		chart_tooltip_start_delay(chart);
	}

	return TRUE;
}


static gboolean chart_button_press (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint x, y;

	if(chart->entries == 0)
		return FALSE;

	DB( g_print("\n[gtkchart] button press\n") );
   	DB( g_print(" active is %d\n", chart->active) );

	if(chart->active < 0) return TRUE;

	gdk_window_get_origin (GDK_WINDOW (event->window), &x, &y);

	chart_tooltip_show(chart, x + (gint)event->x, y + (gint)event->y);

	DB( g_print("x=%d, y=%d\n", (gint)event->x, (gint)event->y) );

	chart->lastpress_x = event->x;
	chart->lastpress_y = event->y;

	return TRUE;
}




static gboolean chart_button_release (GtkWidget *widget, GdkEventButton *event, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);

	if(chart->entries == 0)
		return FALSE;

	DB( g_print("\n[gtkchart] release\n") );

	chart_tooltip_hide(chart);

	return TRUE;
}


static gboolean chart_enter(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);
gint x, y;

	if(chart->entries == 0)
		return FALSE;

	DB( g_print("++++++++++++++++\n[gtkchart] enter\n") );

	x = event->x;
	y = event->y;

	switch(chart->type)
	{
		case CHART_BAR_TYPE:
			chart->active = barchart_get_active(widget, x, y, chart);
			break;
		case CHART_LINE_TYPE:
			chart->active = linechart_get_active(widget, x, y, chart);
			break;
		case CHART_PIE_TYPE:
			chart->active = piechart_get_active(widget, x, y, chart);
			break;
	}

   	DB( g_print(" active is %d\n", chart->active) );

	return TRUE;
}

static gboolean chart_leave(GtkWidget *widget, GdkEventCrossing *event, gpointer user_data)
{
GtkChart *chart = GTK_CHART(user_data);

	if(chart->entries == 0)
		return FALSE;

	chart->active = -1;
	chart->lastactive = -1;

	DB( g_print("\n[gtkchart] leave\n") );
   	DB( g_print(" active is %d\n", chart->active) );

	if (chart->timer_tag)
	{
		g_source_remove (chart->timer_tag);
		chart->timer_tag = 0;
	}

	chart_tooltip_hide(chart);

	gtk_widget_queue_draw(chart->drawarea);

	DB( g_print("----------------\n") );

	return TRUE;
}

static gboolean
chart_tooltip_paint_window (GtkWidget *window)
{
  gtk_paint_flat_box (window->style,
		      window->window,
		      GTK_STATE_NORMAL,
		      GTK_SHADOW_OUT,
		      NULL,
		      window,
		      "tooltip",
		      0, 0,
		      window->allocation.width,
		      window->allocation.height);

  return FALSE;
}

static gint chart_tooltip_timeout (gpointer data)
{
GtkChart *chart = GTK_CHART(data);
gint x, y, wx, wy;
GdkModifierType state;
GtkWidget *widget = GTK_WIDGET(chart);

	GDK_THREADS_ENTER ();

	DB( g_print("\n[gtkchart] SHOULD TOOLTIP %x\n", (int)chart) );


 	//debug
 	gdk_window_get_origin (widget->window, &wx, &wy);

	gdk_window_get_pointer(widget->window, &x, &y, &state);
 	chart_tooltip_show(chart, wx+x, wy+y);
	chart->timer_tag = 0;

	GDK_THREADS_LEAVE ();

	return FALSE;
}


void chart_tooltip_start_delay(GtkChart *chart)
{

	DB( g_print("\n[gtkchart] start delay %x\n", (int)chart) );

	if(chart->timer_tag == 0 )
		chart->timer_tag = g_timeout_add( DEFAULT_DELAY, chart_tooltip_timeout, (gpointer)chart);

}

static void chart_tooltip_hide(GtkChart *chart)
{
	DB( g_print("\n[gtkchart] tooltip hide\n") );

	DB( g_print(" - timertag=%d\n", chart->timer_tag) );

	if(chart->tooltipwin != NULL)
		gtk_widget_hide(chart->tooltipwin);
}


static void chart_tooltip_show(GtkChart *chart, gint xpos, gint ypos)
{
GtkWidget *window, *vbox, *label;
GtkRequisition req;
gint active;
gchar *strval, *buffer;
GtkWidget *alignment;

	DB( g_print("\n[gtkchart] tooltip show\n") );
   	DB( g_print(" x=%d, y=%d\n", xpos, ypos) );

	if(!chart->tooltipwin)
	{
		window = gtk_window_new (GTK_WINDOW_POPUP);
		gtk_window_set_type_hint (GTK_WINDOW (window),
			GDK_WINDOW_TYPE_HINT_TOOLTIP);
		gtk_widget_set_app_paintable (window, TRUE);
		gtk_window_set_resizable (GTK_WINDOW (window), FALSE);
		gtk_widget_set_name (window, "gtk-tooltip");

		alignment = gtk_alignment_new (0.5, 0.5, 1.0, 1.0);
		gtk_alignment_set_padding (GTK_ALIGNMENT (alignment),
			     window->style->ythickness,
			     window->style->ythickness,
			     window->style->xthickness,
			     window->style->xthickness);
		gtk_container_add (GTK_CONTAINER (window), alignment);
		gtk_widget_show (alignment);

		g_signal_connect_swapped (window, "expose_event",
			G_CALLBACK (chart_tooltip_paint_window), window);

		//frame = gtk_frame_new(NULL);
	    //gtk_frame_set_shadow_type (GTK_FRAME(frame), GTK_SHADOW_OUT);
		//gtk_container_add (GTK_CONTAINER (window), frame);
		//gtk_container_set_border_width (GTK_CONTAINER (frame), 4);
		//gtk_widget_show(frame);

		vbox = gtk_vbox_new (FALSE, window->style->xthickness);
		//gtk_container_add( GTK_CONTAINER(frame), vbox);
		gtk_container_add (GTK_CONTAINER (alignment), vbox);
		gtk_widget_show(vbox);

		label = gtk_label_new (NULL);
		gtk_widget_show(label);
		chart->tttitle= label;
		//gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		gtk_misc_set_alignment (GTK_MISC (label), 0.0, 0.5);
		//gtk_misc_set_padding(GTK_MISC(label), 2, 2);
		//gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_LEFT);
		gtk_box_pack_start (GTK_BOX (vbox), label, FALSE, FALSE, 0);

		label = gtk_label_new ("tooltip");
		gtk_widget_show(label);
		chart->ttlabel= label;
		gtk_label_set_line_wrap (GTK_LABEL (label), TRUE);
		//gtk_misc_set_alignment (GTK_MISC (label), 1.0, 0.5);
		gtk_misc_set_padding(GTK_MISC(label), 2, 2);
		gtk_label_set_justify (GTK_LABEL (label), GTK_JUSTIFY_RIGHT);
		gtk_box_pack_start (GTK_BOX (vbox), label, TRUE, TRUE, 0);

		//gtk_widget_reset_rc_styles (tooltipwin);

		chart->tooltipwin = window;

	}

	/* create tooltip text */
	active = chart->active;

	if( active >= 0 )
	{

		DB( g_print("title = %s\n", chart->title) );


		if( chart->title != NULL )
		{
			buffer = g_markup_printf_escaped("<b>%s</b>", chart->title);
			gtk_label_set_markup(GTK_LABEL(chart->tttitle), buffer);
			g_free(buffer);
			gtk_widget_show(chart->tttitle);
		}
		else
		{
			gtk_widget_hide(chart->tttitle);
		}

		if( !chart->dual )
		{
			strval = chart_print_double(chart, chart->datas1[active]);

			if( chart->type == CHART_PIE_TYPE )
				buffer = g_markup_printf_escaped("%s\n%s\n%.2f%%", chart->titles[active], strval, ABS(chart->datas1[active])*100/chart->total);
			else
				buffer = g_markup_printf_escaped("%s\n%s", chart->titles[active], strval);

			gtk_label_set_markup(GTK_LABEL(chart->ttlabel), buffer);
			g_free(buffer);
		}
		else
		{
		gchar *buf1;

			strval = chart_print_double(chart, chart->datas1[active]);
			buf1 = g_strdup(strval);
			strval = chart_print_double(chart, chart->datas2[active]);

			buffer = g_markup_printf_escaped("%s\n-%s\n+%s",
				chart->titles[active],
				buf1,
				strval
				);

			gtk_label_set_markup(GTK_LABEL(chart->ttlabel), buffer);
			g_free(buf1);
			g_free(buffer);
		}


		/*
		}
		else
		{
			gtk_label_set_text(GTK_LABEL(chart->ttlabel), "Please move to:\na bar,\na slice\nor a point.");

		}
		*/

		/* position and show our tooltip */
		//gtk_widget_queue_resize(chart->tooltipwin);
		gtk_widget_size_request (chart->tooltipwin, &req);

		//DB( g_printf("size is: w%d h%d :: xpos=%d ypos=%d\n", req.width, req.height, xpos,ypos) );

		gtk_window_move(GTK_WINDOW(chart->tooltipwin), xpos - (req.width/2), ypos - req.height);

		gtk_widget_show(chart->tooltipwin);

	}

}


void chart_cleanup_colors(GtkWidget *widget)
{
//GdkColormap *colormap;

	//colormap = gtk_widget_get_colormap(widget);

	//todo freeclormap ?

}

gboolean chart_setup_colors(GtkWidget *widget, GtkChart *chart )
{
GdkColormap *colormap;
guint col;
GdkColor *normal_colors = chart->normal_colors;
GdkColor *over_colors = chart->over_colors;

	colormap = gtk_widget_get_colormap(widget);

	if(!colormap) return FALSE;
	
	chart->nb_colors = nb_colors;

	DB( g_print("nb colors %d\n", chart->nb_colors) );
	
	for(col=0; col<NUM_COLORMAP_MAX; col++)
	{
		normal_colors[col].red   = COLTO16(colors[col].r);
		normal_colors[col].green = COLTO16(colors[col].g);
		normal_colors[col].blue  = COLTO16(colors[col].b);
		gdk_colormap_alloc_color( colormap, &normal_colors[col], FALSE, TRUE);

		over_colors[col].red   = COLTO16(COLTOOVER(colors[col].r));
		over_colors[col].green = COLTO16(COLTOOVER(colors[col].g));
		over_colors[col].blue  = COLTO16(COLTOOVER(colors[col].b));
		gdk_colormap_alloc_color( colormap, &over_colors[col], FALSE, TRUE);

	}
	return TRUE;
}







/* legend list */

static GdkPixbuf *
//create_color_pixbuf (const char *color)
create_color_pixbuf (GdkColor *col)
{
        GdkPixbuf *pixbuf;
        //GdkColor col = color;

        int x;
        int num;
        guchar *pixels, *p;

/*
        if (!gdk_color_parse (color, &col))
                return NULL;
                */

#define squaredim 12

        pixbuf = gdk_pixbuf_new (GDK_COLORSPACE_RGB,
                                 FALSE, 8, //bits
                                 squaredim, squaredim);		//width,height

        //rowstride = gdk_pixbuf_get_rowstride (pixbuf);
        p = pixels = gdk_pixbuf_get_pixels (pixbuf);

        num = gdk_pixbuf_get_width (pixbuf) *
                gdk_pixbuf_get_height (pixbuf);

        for (x = 0; x < num; x++) {

                p[0] = col->red;
                p[1] = col->green;
                p[2] = col->blue;

				/*
                p[0] = col->red / 65535 * 255;
                p[1] = col->green / 65535 * 255;
                p[2] = col->blue / 65535 * 255;
                */
                p += 3;
        }

        return pixbuf;
}

static void legend_list_cell_data_function(GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
GdkPixbuf *pixbuf;
gchar *title;

	gtk_tree_model_get(model, iter, 
		LST_LEGEND_COLOR, &pixbuf,
		LST_LEGEND_TITLE, &title,
		-1);

	switch(GPOINTER_TO_INT(user_data))
	{
		case LST_LEGEND_COLOR:
			g_object_set(renderer, "pixbuf", pixbuf, NULL);
			break;
		case LST_LEGEND_TITLE:
			g_object_set(renderer, "text", title, NULL);
			break;
	}

}

static void
legend_list_float_cell_data_function (GtkTreeViewColumn *col, GtkCellRenderer *renderer, GtkTreeModel *model, GtkTreeIter *iter, gpointer user_data)
{
GtkChart *chart = user_data;
gchar buf[G_ASCII_DTOSTR_BUF_SIZE];
gdouble amount;

	gtk_tree_model_get(model, iter,
		LST_LEGEND_AMOUNT, &amount,
		-1);

	//hb_strfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, value, kcur);
	//todo: manage GLOBALS->minor eq
	mystrfmon(buf, G_ASCII_DTOSTR_BUF_SIZE-1, amount, chart->minor);

	g_object_set(renderer, 
		"text", buf,
		NULL);

}

static void legend_list_rate_cell_data_function (GtkTreeViewColumn *col,
                           GtkCellRenderer   *renderer,
                           GtkTreeModel      *model,
                           GtkTreeIter       *iter,
                           gpointer           user_data)
{
gint rate;
gchar buf[8];

	gtk_tree_model_get(model, iter,
		LST_LEGEND_RATE, &rate,
		-1);

	g_snprintf(buf, sizeof(buf), "%d %%", rate);
	g_object_set(renderer, "text", buf, NULL);

}


GtkWidget *legend_list_new(GtkChart *chart)
{
GtkListStore *store;
GtkWidget *view;
GtkCellRenderer    *renderer;
GtkTreeViewColumn  *column;

	store = gtk_list_store_new(NUM_LST_LEGEND,
		G_TYPE_POINTER,
		GDK_TYPE_PIXBUF,
		G_TYPE_STRING,
		G_TYPE_DOUBLE,
		G_TYPE_INT
		);

	//treeview
	view = gtk_tree_view_new_with_model(GTK_TREE_MODEL(store));
	g_object_unref(store);

#if MYDEBUG == 1
	GtkStyle *style;
	PangoFontDescription *font_desc;

	style = gtk_widget_get_style(GTK_WIDGET(view));
	font_desc = style->font_desc;

	g_print("family: %s\n", pango_font_description_get_family(font_desc) );
	g_print("size: %d (%d)\n", pango_font_description_get_size (font_desc), pango_font_description_get_size (font_desc )/PANGO_SCALE );
#endif

	// change the font size to a smaller one
	PangoFontDescription *font = pango_font_description_new();
	pango_font_description_set_size (font, 8 * PANGO_SCALE);
	gtk_widget_modify_font(GTK_WIDGET(view), font);
	pango_font_description_free( font );

	// column 1
	column = gtk_tree_view_column_new();
	renderer = gtk_cell_renderer_pixbuf_new ();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, legend_list_cell_data_function, GINT_TO_POINTER(LST_LEGEND_COLOR), NULL);

	renderer = gtk_cell_renderer_text_new ();
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
    gtk_tree_view_column_set_cell_data_func(column, renderer, legend_list_cell_data_function, GINT_TO_POINTER(LST_LEGEND_TITLE), NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);

	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	// column 2
	column = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(column, name);

	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, FALSE);
	gtk_tree_view_column_set_cell_data_func(column, renderer, legend_list_float_cell_data_function, chart, NULL);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
	//gtk_tree_view_column_set_resizable(column, TRUE);
	//gtk_tree_view_column_set_alignment (column, 0.5);
	//gtk_tree_view_column_set_spacing( column, 16 );

	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);

	// column 3
	column = gtk_tree_view_column_new();
	//gtk_tree_view_column_set_title(column, "%");
	renderer = gtk_cell_renderer_text_new ();
	g_object_set(renderer, "xalign", 1.0, NULL);
	gtk_tree_view_column_pack_start(column, renderer, TRUE);
	//gtk_tree_view_column_add_attribute(column, renderer, "text", id);
	gtk_tree_view_column_set_cell_data_func(column, renderer, legend_list_rate_cell_data_function, GINT_TO_POINTER(3), NULL);
	//gtk_tree_view_column_set_alignment (column, 0.5);
	gtk_tree_view_append_column (GTK_TREE_VIEW(view), column);


	gtk_tree_selection_set_mode(gtk_tree_view_get_selection(GTK_TREE_VIEW(view)), GTK_SELECTION_NONE);
	gtk_tree_view_set_headers_visible (GTK_TREE_VIEW(view), FALSE);
	//gtk_tree_view_set_reorderable (GTK_TREE_VIEW(view), TRUE);

	//g_object_set(view, "vertical-separator", 1);

//	g_object_set(view, "vertical_separator", 0, NULL);

/*
	GValue value = { 0, };
	g_value_init (&value, G_TYPE_INT);
	g_value_set_int (&value, 20);
	g_object_set_property(view, "vertical-separator", &value);
	g_value_unset (&value);
*/

	return(view);
}



