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

#ifndef PCKT_GTK2_DIAL_H
#define PCKT_GTK2_DIAL_H 1

#include <gtk/gtkwidget.h>
#include <gtk/gtkrange.h>
#include "../pckt.h"

__BEGIN_DECLS

#define PCKT_GTK_TYPE_DIAL \
  (pckt_gtk_dial_get_type ())
#define PCKT_GTK_DIAL(obj) \
  GTK_CHECK_CAST (obj, PCKT_GTK_TYPE_DIAL, PcktGtkDial)
#define PCKT_GTK_DIAL_CLASS(klass) \
  GTK_CHECK_CLASS_CAST (klass, PCKT_GTK_TYPE_DIAL, PcktGtkDialClass)
#define PCKT_GTK_IS_DIAL(obj) \
  GTK_CHECK_TYPE (obj, PCKT_GTK_TYPE_DIAL)

typedef struct PcktGtkDialImpl PcktGtkDial;
typedef struct PcktGtkDialClassImpl PcktGtkDialClass;

struct PcktGtkDialImpl
{
  GtkRange range;
  double default_value;
};

struct PcktGtkDialClassImpl
{
  GtkRangeClass parent_class;
};

extern GType pckt_gtk_dial_get_type ();
extern GtkWidget *pckt_gtk_dial_new ();
extern GtkWidget *pckt_gtk_dial_new_with_adjustment (GtkAdjustment *);

__END_DECLS

#endif /* ! PCKT_GTK2_DIAL_H */
