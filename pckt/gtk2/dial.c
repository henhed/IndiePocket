/* Copyright (C) 2016 Henrik Hedelund.

   This file is part of IndiePocket.

   IndiePocket is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   IndiePocket is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with IndiePocket.  If not, see <http://www.gnu.org/licenses/>. */

#include <math.h>
#include <gtk/gtkmain.h>
#include "dial.h"

#ifndef M_PI
# define M_PI 3.14159265358979323846
#endif

#define PCKT_GTK_DIAL_DEFAULT_SIZE 32

#define P_DEFAULT_VALUE_ID 1
#define P_DEFAULT_VALUE_NAME "default-value"
#define P_DEFAULT_VALUE_LABEL "Default Value"
#define P_DEFAULT_VALUE_DESC "The value assigned when the dial is reset"

static void pckt_gtk_dial_size_request (GtkWidget *, GtkRequisition *);
static gboolean pckt_gtk_dial_expose (GtkWidget *, GdkEventExpose *);
static gboolean pckt_gtk_dial_button_press (GtkWidget *, GdkEventButton *);
static gboolean pckt_gtk_dial_button_release (GtkWidget *, GdkEventButton *);
static gboolean pckt_gtk_dial_motion_notify (GtkWidget *, GdkEventMotion *);
static gboolean pckt_gtk_dial_enter_notify (GtkWidget *, GdkEventCrossing *);
static gboolean pckt_gtk_dial_leave_notify (GtkWidget *, GdkEventCrossing *);
static void pckt_gtk_dial_get_property (GObject *, guint, GValue *,
                                        GParamSpec *);
static void pckt_gtk_dial_set_property (GObject *, guint, const GValue *,
                                        GParamSpec *);

G_DEFINE_TYPE (PcktGtkDial, pckt_gtk_dial, GTK_TYPE_RANGE)

static void
pckt_gtk_dial_class_init (PcktGtkDialClass *klass)
{
  GtkWidgetClass *widget_class = GTK_WIDGET_CLASS (klass);
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  widget_class->expose_event = pckt_gtk_dial_expose;
  widget_class->size_request = pckt_gtk_dial_size_request;
  widget_class->button_press_event = pckt_gtk_dial_button_press;
  widget_class->button_release_event = pckt_gtk_dial_button_release;
  widget_class->motion_notify_event = pckt_gtk_dial_motion_notify;
  widget_class->enter_notify_event = pckt_gtk_dial_enter_notify;
  widget_class->leave_notify_event = pckt_gtk_dial_leave_notify;

  gobject_class->get_property = pckt_gtk_dial_get_property;
  gobject_class->set_property = pckt_gtk_dial_set_property;

  g_object_class_install_property (gobject_class,
                                   P_DEFAULT_VALUE_ID,
                                   g_param_spec_double (P_DEFAULT_VALUE_NAME,
                                                        P_DEFAULT_VALUE_LABEL,
                                                        P_DEFAULT_VALUE_DESC,
                                                        -G_MAXDOUBLE,
                                                        G_MAXDOUBLE,
                                                        0,
                                                        G_PARAM_READWRITE));
}

static void
pckt_gtk_dial_init (PcktGtkDial *dial)
{
  GtkWidget *widget = GTK_WIDGET (dial);
  gtk_widget_set_can_focus (GTK_WIDGET (dial), TRUE);
  widget->requisition.height = PCKT_GTK_DIAL_DEFAULT_SIZE;
  widget->requisition.width = PCKT_GTK_DIAL_DEFAULT_SIZE;
}

GtkWidget *
pckt_gtk_dial_new ()
{
  GtkAdjustment *adjustment;
  adjustment = (GtkAdjustment *) gtk_adjustment_new (0, 0, 1, 0.01, 0.5, 0);
  return pckt_gtk_dial_new_with_adjustment (adjustment);
}

static gboolean
pckt_gtk_dial_value_changed (gpointer object)
{
  GtkWidget *widget = (GtkWidget *) object;
  gtk_widget_queue_draw (widget);
  return FALSE;
}

gdouble
pckt_gtk_dial_get_default_value (const PcktGtkDial *dial)
{
  g_return_val_if_fail (PCKT_GTK_IS_DIAL (dial), 0);
  return dial->default_value;
}

void
pckt_gtk_dial_set_default_value (PcktGtkDial *dial, gdouble default_value)
{
  g_return_if_fail (PCKT_GTK_IS_DIAL (dial));
  dial->default_value = default_value;
}

GtkWidget *
pckt_gtk_dial_new_with_adjustment (GtkAdjustment *adjustment)
{
  GtkWidget *widget = GTK_WIDGET (gtk_type_new (PCKT_GTK_TYPE_DIAL));

  if (widget)
    {
      pckt_gtk_dial_set_default_value (PCKT_GTK_DIAL (widget),
                                       adjustment->value);
      gtk_range_set_adjustment (GTK_RANGE (widget), adjustment);
      g_signal_connect (GTK_OBJECT (widget),
                        "value-changed",
                        G_CALLBACK (pckt_gtk_dial_value_changed),
                        widget);
    }

  return widget;
}

static void
pckt_gtk_dial_size_request (GtkWidget *widget, GtkRequisition *requisition)
{
  (void) widget;
  (void) requisition;
  requisition->width = PCKT_GTK_DIAL_DEFAULT_SIZE;
  requisition->height = PCKT_GTK_DIAL_DEFAULT_SIZE;
}

#define SET_CAIRO_COLOR(cr, gdk_color)                                  \
  cairo_set_source_rgb ((cr),                                           \
                        (double) (gdk_color).red / UINT16_MAX,          \
                        (double) (gdk_color).green / UINT16_MAX,        \
                        (double) (gdk_color).blue / UINT16_MAX)

static gboolean
pckt_gtk_dial_expose (GtkWidget *widget, GdkEventExpose *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (PCKT_GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);
  g_return_val_if_fail (event->count <= 0, FALSE);

  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (widget));
  if (adj->upper == adj->lower)
    return FALSE;

  cairo_t *cr = gdk_cairo_create (GDK_DRAWABLE (widget->window));

  double wx = widget->allocation.x;
  double wy = widget->allocation.y;
  double ww = widget->allocation.width;
  double wh = widget->allocation.height;
  double ox = wx + (ww / 2);
  double oy = wy + (wh / 2);
  double weight = fmin (ww, wh) / 16;
  double radius = (fmin (ww, wh) / 2) - weight;
  double val = (adj->value - adj->lower) / (adj->upper - adj->lower);
  double ang_min = M_PI * 0.75;
  double ang_max = M_PI * 2.25;
  double ang_val = ((ang_max - ang_min) * val) + ang_min;
  double fill_from = ang_min;
  double fill_to = ang_val;

  /* Set drawing bounds.  */
  cairo_rectangle (cr, wx, wy, ww, wh);
  cairo_clip (cr);

  /* Draw track.  */
  SET_CAIRO_COLOR (cr, widget->style->fg[GTK_STATE_NORMAL]);
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, weight);
  cairo_arc (cr, ox, oy, radius, 0, M_PI * 2);
  cairo_stroke (cr);

  /* Draw ticks.  */
  cairo_set_line_cap (cr, CAIRO_LINE_CAP_SQUARE);
  cairo_set_line_width (cr, weight / 2);
  cairo_move_to (cr,
                 ox + (cos (ang_min) * (radius + (weight * 1.5))),
                 oy + (sin (ang_min) * (radius + (weight * 1.5))));
  cairo_line_to (cr,
                 ox + (cos (ang_min) * (sqrt (radius * radius * 2))),
                 oy + (sin (ang_min) * (sqrt (radius * radius * 2))));
  cairo_stroke (cr);
  cairo_move_to (cr,
                 ox + (cos (ang_max) * (radius + (weight * 1.5))),
                 oy + (sin (ang_max) * (radius + (weight * 1.5))));
  cairo_line_to (cr,
                 ox + (cos (ang_max) * (sqrt (radius * radius * 2))),
                 oy + (sin (ang_max) * (sqrt (radius * radius * 2))));
  cairo_stroke (cr);

  /* Draw handle.  */
  cairo_set_line_width (cr, 1);
  cairo_arc (cr,
             ox + (cos (ang_val) * (radius - (weight * 3))),
             oy + (sin (ang_val) * (radius - (weight * 3))),
             weight,
             0,
             M_PI * 2);
  cairo_fill (cr);

  /* Draw fill level.  */
  if ((adj->lower < 0.) && (adj->upper > 0.))
    {
      fill_from = ang_min + ((-adj->lower / (adj->upper - adj->lower))
                             * (ang_max - ang_min));
      if (fill_to < fill_from)
        {
          fill_to = fill_from;
          fill_from = ang_val;
        }
    }

  if (gtk_widget_has_grab (widget))
    SET_CAIRO_COLOR (cr, widget->style->fg[GTK_STATE_ACTIVE]);
  else if (gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT)
    SET_CAIRO_COLOR (cr, widget->style->fg[GTK_STATE_PRELIGHT]);
  else
    SET_CAIRO_COLOR (cr, widget->style->fg[GTK_STATE_SELECTED]);

  cairo_set_line_cap (cr, CAIRO_LINE_CAP_ROUND);
  cairo_set_line_width (cr, weight);
  cairo_arc (cr, ox, oy, radius, fill_from, fill_to);
  cairo_stroke (cr);

  cairo_destroy (cr);

  return TRUE;
}

static gboolean
pckt_gtk_dial_button_press (GtkWidget *widget, GdkEventButton *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (PCKT_GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (event->type == GDK_2BUTTON_PRESS)
    gtk_range_set_value (GTK_RANGE (widget),
                         PCKT_GTK_DIAL (widget)->default_value);

  if (!gtk_widget_has_focus (widget))
    gtk_widget_grab_focus (widget);

  gtk_grab_add (widget);
  gtk_widget_set_state (widget, GTK_STATE_ACTIVE);
  gtk_widget_queue_draw (widget);

  return FALSE;
}

static gboolean
pckt_gtk_dial_button_release (GtkWidget *widget, GdkEventButton *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (PCKT_GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (gtk_widget_has_grab (widget))
    gtk_grab_remove (widget);

  gtk_widget_set_state (widget, GTK_STATE_NORMAL);
  gtk_widget_queue_draw (widget);

  return FALSE;
}

static gboolean
pckt_gtk_dial_motion_notify (GtkWidget *widget, GdkEventMotion *event)
{
  g_return_val_if_fail (widget != NULL, FALSE);
  g_return_val_if_fail (PCKT_GTK_IS_DIAL (widget), FALSE);
  g_return_val_if_fail (event != NULL, FALSE);

  if (!gtk_widget_has_grab (widget))
    return FALSE;

  GtkAdjustment *adj = gtk_range_get_adjustment (GTK_RANGE (widget));
  double ox = widget->allocation.x + (widget->allocation.width / 2);
  double oy = widget->allocation.y + (widget->allocation.height / 2);
  double rx = widget->allocation.x + event->x - ox;
  double ry = widget->allocation.y + event->y - oy;
  double ang_min = M_PI * 0.25;
  double ang_max = M_PI * 1.75;
  double ang_val = atan2 (ry, rx);
  if (ang_val < 0.)
    ang_val = (2 * M_PI) + ang_val;
  ang_val = fmod (ang_val + (M_PI * 1.5), M_PI * 2);
  if (ang_val < ang_min)
    ang_val = ang_min;
  else if (ang_val > ang_max)
    ang_val = ang_max;

  double val = adj->lower + (((ang_val - ang_min) / (ang_max - ang_min))
                             * (adj->upper - adj->lower));
  if (val != adj->value)
    gtk_range_set_value (GTK_RANGE (widget), val);

  return TRUE;
}

static gboolean
pckt_gtk_dial_enter_notify (GtkWidget *widget, GdkEventCrossing *event)
{
  (void) event;
  if (gtk_widget_get_state (widget) == GTK_STATE_NORMAL)
    {
      gtk_widget_set_state (widget, GTK_STATE_PRELIGHT);
      gtk_widget_queue_draw (widget);
    }
  return TRUE;
}

static gboolean
pckt_gtk_dial_leave_notify (GtkWidget *widget, GdkEventCrossing *event)
{
  (void) event;
  if (gtk_widget_get_state (widget) == GTK_STATE_PRELIGHT)
    {
      gtk_widget_set_state (widget, GTK_STATE_NORMAL);
      gtk_widget_queue_draw (widget);
    }
  return TRUE;
}

static void
pckt_gtk_dial_get_property (GObject *object, guint prop_id, GValue *value,
                            GParamSpec *pspec)
{
  PcktGtkDial *dial = PCKT_GTK_DIAL (object);
  if (prop_id == P_DEFAULT_VALUE_ID)
    g_value_set_double (value, pckt_gtk_dial_get_default_value (dial));
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}

static void
pckt_gtk_dial_set_property (GObject *object, guint prop_id, const GValue *value,
                            GParamSpec *pspec)
{
  PcktGtkDial *dial = PCKT_GTK_DIAL (object);
  if (prop_id == P_DEFAULT_VALUE_ID)
    pckt_gtk_dial_set_default_value (dial, g_value_get_double (value));
  else
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
}
