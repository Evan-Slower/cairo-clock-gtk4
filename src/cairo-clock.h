#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define CAIRO_TYPE_CLOCK (cairo_clock_get_type())
G_DECLARE_FINAL_TYPE(CairoClock, cairo_clock, CAIRO, CLOCK, GtkWidget)

G_END_DECLS
