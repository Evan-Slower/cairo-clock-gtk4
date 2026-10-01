/*******************************************************************************
**3456789 123456789 123456789 123456789 123456789 123456789 123456789 123456789 
**      10        20        30        40        50        60        70        80
**
** program:
**    cairo-clock
**
** author:
**    Mirco "MacSlow" Müller <macslow@bangang.de>, <macslow@gmail.com>
**
** created: .
**    10.1.2006 (or so)
**
** last change:
**    17.8.2007
**
** notes:
**    In my ongoing efforts to do something useful while learning the cairo-API
**    I produced this nifty program. Surprisingly it displays the current system
**    time in the old-fashioned way of an analog clock. I place this program
**    under the "GNU General Public License". If you don't know what that means
**    take a look a here...
**
**        http://www.gnu.org/licenses/licenses.html#GPL
**
** todo:
**    clean up code and make it sane to read/understand
**
** supplied patches:
**    26.3.2006 - received a patch to add a 24h-mode from Darryll "Moppsy"
**    Truchan <moppsy@comcast.net>
**
*******************************************************************************/

/* ============================================================================
 * MACRO DEFINITIONS & CONFIGURATION
 * ============================================================================
 * Define standard math constants and localization paths if not provided by
 * the build system (config.h). Fallbacks ensure compilation on all systems.
 */
 /*
 * cairo-clock-gtk4 — A 20th anniversary GTK4 port of MacSlow's cairo-clock
 *
 * Original: Mirco "MacSlow" Müller, 10.1.2006
 * Port:     Michael "Evan Slower" Savicky, pushed port to GitHub 10.1.2026
 * License:  GPL-3.0+
 */   
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

#ifndef GETTEXT_PACKAGE
#define GETTEXT_PACKAGE "cairo-clock"
#endif

#ifndef CAIROCLOCKLOCALEDIR
#define CAIROCLOCKLOCALEDIR "/usr/share/locale"
#endif

#ifndef PKGDATA_DIR
#define PKGDATA_DIR "/usr/local/share/cairo-clock"
#endif

/* app_globals.h */
#ifndef APP_GLOBALS_H
#define APP_GLOBALS_H

#include <gtk/gtk.h> 

const gchar g_acAppVersion[] = "1.0.0";    
#endif   

#define SECOND_INTERVAL      1000
#define MINUTE_INTERVAL     60000
#define MIN_WIDTH              32
#define MIN_HEIGHT             32
#define MAX_WIDTH            1900
#define MAX_HEIGHT           1000
#define MIN_REFRESH_RATE        1
#define MAX_REFRESH_RATE       60

#include "config.h"

#include <stdlib.h>         // IWYU pragma: keep
#include <string.h>         // IWYU pragma: keep
#include <ctype.h>          // IWYU pragma: keep
#include <dirent.h>         // IWYU pragma: keep
#include <math.h>

#include <gtk/gtk.h>
#include <glib/gi18n.h>
#include <locale.h>
#include <glib/gstdio.h>    // IWYU pragma: keep

#include <librsvg/rsvg.h>

#include <gdk/x11/gdkx.h>
#include <X11/Xlib.h>
#include <X11/Xatom.h>

typedef enum _LayerElement {
    /* Static (0–3) — first loop */
    CLOCK_DROP_SHADOW = 0,
    CLOCK_FACE_SHADOW,
    CLOCK_FACE,
    CLOCK_MARKS,
    /* Dynamic (4–9) — second loop */
    CLOCK_HOUR_HAND_SHADOW,
    CLOCK_HOUR_HAND,
    CLOCK_MINUTE_HAND_SHADOW,
    CLOCK_MINUTE_HAND,
    CLOCK_SECOND_HAND_SHADOW,
    CLOCK_SECOND_HAND,
    /* Glass & Frame (10–11) — drawn last, after both loops */
    CLOCK_GLASS,
    CLOCK_FRAME,
    CLOCK_ELEMENTS
} LayerElement;   

typedef enum _SurfaceKind {
    KIND_BACKGROUND = 0,
    KIND_FOREGROUND
} SurfaceKind;

typedef enum _StartupSizeKind {
    SIZE_SMALL = 0,
    SIZE_MEDIUM,
    SIZE_LARGE,
    SIZE_CUSTOM1,
    SIZE_CUSTOM2,
} StartupSizeKind;

typedef struct _ThemeEntry {
    GString *pName;  
    GString *pPath;  
} ThemeEntry;   

typedef struct {
    double width;
    double height;
} ClockDimensions;


static const gchar *const CLOCK_FILE_NAMES[CLOCK_ELEMENTS] = {
    // Static (0–3)
    "clock-drop-shadow.svg",
    "clock-face-shadow.svg",
    "clock-face.svg",
    "clock-marks.svg",
    // Dynamic (4–9)
    "clock-hour-hand-shadow.svg",
    "clock-hour-hand.svg",
    "clock-minute-hand-shadow.svg",
    "clock-minute-hand.svg",
    "clock-second-hand-shadow.svg",
    "clock-second-hand.svg",
    // Glass & Frame (10–11)
    "clock-glass.svg",
    "clock-frame.svg"
};   

typedef struct {
    GtkWidget   *main_window;
    GtkWidget   *settings_dialog;
    GtkWidget *table_startup_size;   
    GtkWidget *spin_button_width;    
    GtkWidget *spin_button_height;   
    GtkDrawingArea *drawing_area; 
    GtkWidget   *theme_dropdown;
    GtkWidget *combobox_startup_size;   
    GtkWidget   *main_box;
    GtkBuilder  *builder;
    GMenuModel  *popup_model;
    GtkWidget *smoothness_scale;   

    /* --- Graphics Resources --- */
    RsvgHandle  *svg_handles[CLOCK_ELEMENTS];  
    ClockDimensions dimensions;
    guint startup_size;  
    /* --- Animation & Time State --- */
    guint        tick_callback_id;
    double       fAngleSecond;
    double       fAngleMinute;
    double       fAngleHour;
    gboolean     bAnimateMinute;
    gint         iFrames;
    float        fFactor;
    gint         iHour, iMinute, iSecond;

    /* --- Configuration --- */
    gchar *theme;   
    gchar       *theme_path;
    gchar       *config_file;
    gint         refresh_rate;
    gint         iX, iY, iWidth, iHeight;
    gint default_x;
    gint default_y;   
     
    gint custom1_width;
    gint custom1_height;
    gint custom2_width;
    gint custom2_height;   
    gint startup_size_kind;   
    gint svg_width;
    gint svg_height;      
     
    gboolean     show_seconds;
    gboolean     show_date;
    gboolean     keep_on_top;
    gboolean sticky;   
    gboolean     use_24_hour;
    gboolean updating_slider;
    gboolean updating_spin;   

    /* --- Runtime State --- */
    GList       *theme_list;
    gboolean     needs_update;
	GSettings *settings;   
    guint interval_handler_id;
    gint64 last_update_ms;
    GtkApplication *app;
    gboolean allow_close;
      
} ClockState;

static void clock_state_free(ClockState *state);

static void free_theme_entry(ThemeEntry *entry) {
    if (entry) {
        if (entry->pName) g_string_free(entry->pName, TRUE);
        if (entry->pPath) g_string_free(entry->pPath, TRUE);
        g_free(entry);
    }
}

static void
clock_state_free (ClockState *state)
{
    if (!state) return;

    if (state->tick_callback_id != 0) {
        gtk_widget_remove_tick_callback(
            GTK_WIDGET(state->drawing_area), 
            state->tick_callback_id
        );
        state->tick_callback_id = 0;
    }

    if (state->settings) {
        g_object_unref(state->settings);
        state->settings = NULL;
    }

    if (state->theme_list) {
        g_list_free_full(state->theme_list, (GDestroyNotify)free_theme_entry);
        state->theme_list = NULL;
    }

    for (int i = 0; i < CLOCK_ELEMENTS; i++) {
        if (state->svg_handles[i]) {
            g_object_unref(state->svg_handles[i]);
            state->svg_handles[i] = NULL;
        }
    }

    g_free(state->theme);
    g_free(state->theme_path);
    
    if (state->popup_model) {
        g_object_unref(state->popup_model);
        state->popup_model = NULL;
    }

    if (state->builder) {
        g_object_unref(state->builder);
        state->builder = NULL;
    } 

    g_free(state);
}   

static gboolean print_theme_list = FALSE;
static gboolean print_version    = FALSE;   

static void on_activate(GtkApplication *app, gpointer user_data);
static void read_settings (GSettings *settings,
                           gint *piX, gint *piY,
                           gint *piCustom1Width, gint *piCustom1Height,
                           gint *piCustom2Width, gint *piCustom2Height,
                           gboolean *piShowSeconds, gboolean *piShowDate,
                           gchar **pcTheme,
                           gboolean *piKeepOnTop, gboolean *piSticky,
                           gboolean *pi24, gint *piRefreshRate);

static GtkStringList *load_theme_list(void);   
static void change_theme(const gchar *theme_name, ClockState *state);

static void setup_window_geometry_and_style(ClockState *state); 
static gboolean load_ui_resources_and_widgets(ClockState *state, GError **error);
static void initialize_widget_states(ClockState *state);
static void connect_application_signals(ClockState *state);
static void setup_actions_controllers_and_layout(ClockState *state);

static gboolean on_refresh_timer(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer data);   

static void on_width_value_changed(GtkSpinButton *button, gpointer user_data);
static void on_height_value_changed(GtkSpinButton *button, gpointer user_data);
static void on_value_changed(GtkRange *range, gpointer user_data);
static void on_theme_changed(GtkDropDown *drop, GParamSpec *pspec, gpointer user_data);
static void on_startup_size_changed(GtkDropDown *drop, GParamSpec *pspec, gpointer user_data);
static void on_spin_value_changed (GtkSpinButton *spin, gpointer user_data);   

static void on_seconds_toggled(GtkCheckButton *check, gpointer user_data);
static void on_date_toggled(GtkCheckButton *check, gpointer user_data);
static void on_keep_on_top_toggled(GtkCheckButton *check, gpointer user_data);
static void on_sticky_toggled(GtkCheckButton *check, gpointer data);   
static void on_24h_toggled(GtkCheckButton *check, gpointer user_data);
static gboolean apply_x11_hints(gpointer user_data);   

static gboolean on_settings_close_request(GtkWindow *window, gpointer user_data);   
static void on_window_close(GtkWindow *window, gpointer user_data);
static void on_window_mapped(GtkWindow *window, GParamSpec *pspec, gpointer user_data);
static void on_drawing_area_clicked(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data);

static void clock_draw_func(GtkDrawingArea *pWidget,
                            cairo_t *cr,
                            int width,
                            int height,
                            gpointer data);   

static void save_settings(ClockState *state); 
static gboolean on_close(GtkWidget *window, gpointer user_data);   
static gboolean poll_and_save_position(gpointer user_data);
static void restore_position(GtkWidget *window);   

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"

static void set_window_hints_x11(GtkWindow *window, gboolean keep_above, gboolean sticky)
{
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
    if (!GDK_IS_X11_SURFACE(surface))
    return;

    Window xid = GDK_SURFACE_XID(surface);
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    if (!dpy) return;   

    Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    Atom sticky_atom = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);

    Atom atoms[2];
    int count = 0;
    if (keep_above) atoms[count++] = above;
    if (sticky) atoms[count++] = sticky_atom;

    XChangeProperty(dpy, xid, net_wm_state, XA_ATOM, 32,
                    PropModeReplace, (unsigned char *)atoms, count);
    XFlush(dpy);
}

static void toggle_window_hints_x11(GtkWindow *window, gboolean keep_above, gboolean sticky)
{
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
    if (!GDK_IS_X11_SURFACE(surface))
    return;   
    
    Window xid = GDK_SURFACE_XID(surface);
    Display *dpy = GDK_DISPLAY_XDISPLAY(gdk_display_get_default());
    if (!dpy) return;

    Atom net_wm_state = XInternAtom(dpy, "_NET_WM_STATE", False);
    Atom above = XInternAtom(dpy, "_NET_WM_STATE_ABOVE", False);
    Atom sticky_atom = XInternAtom(dpy, "_NET_WM_STATE_STICKY", False);

    XEvent xev;

    memset(&xev, 0, sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.window = xid;
    xev.xclient.message_type = net_wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = keep_above ? 1 : 0;
    xev.xclient.data.l[1] = above;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;
    XSendEvent(dpy, DefaultRootWindow(dpy), False,
               SubstructureNotifyMask | SubstructureRedirectMask, &xev);

    memset(&xev, 0, sizeof(xev));
    xev.xclient.type = ClientMessage;
    xev.xclient.window = xid;
    xev.xclient.message_type = net_wm_state;
    xev.xclient.format = 32;
    xev.xclient.data.l[0] = sticky ? 1 : 0;
    xev.xclient.data.l[1] = sticky_atom;
    xev.xclient.data.l[2] = 0;
    xev.xclient.data.l[3] = 0;
    xev.xclient.data.l[4] = 0;
    XSendEvent(dpy, DefaultRootWindow(dpy), False,
               SubstructureNotifyMask | SubstructureRedirectMask, &xev);

    XFlush(dpy);
}   

#pragma GCC diagnostic pop

static gboolean
apply_x11_hints_delayed(gpointer user_data)
{
    ClockState *state = user_data;
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(state->main_window));
    if (!surface) return G_SOURCE_REMOVE;
    set_window_hints_x11(GTK_WINDOW(state->main_window), state->keep_on_top, state->sticky);
    toggle_window_hints_x11(GTK_WINDOW(state->main_window), state->keep_on_top, state->sticky);
    return G_SOURCE_REMOVE;
}   

static gboolean
apply_x11_hints(gpointer user_data)
{
    g_timeout_add(200, (GSourceFunc)apply_x11_hints_delayed, user_data);
    return G_SOURCE_REMOVE;
}

static void on_mapped(GtkWidget *widget, gpointer data) {
    apply_x11_hints(data);
    g_timeout_add(300, (GSourceFunc)restore_position, widget);
}   

static gboolean
reassert_desktop(gpointer data)
{
    ClockState *state = data;
    set_window_hints_x11(GTK_WINDOW(state->main_window), state->keep_on_top, state->sticky);   
    return G_SOURCE_REMOVE;
}   

static void
on_action_settings(GSimpleAction *action, GVariant *param, gpointer data)
{
    ClockState *state = data;

    gtk_window_set_default_size(GTK_WINDOW(state->settings_dialog), 300, 500);
    gtk_window_present(GTK_WINDOW(state->settings_dialog));

    // Sync slider to saved value
    GtkWidget *scale = GTK_WIDGET(gtk_builder_get_object(state->builder, "hscaleSmoothness"));
    if (scale) {
        state->updating_slider = TRUE;
        gtk_range_set_value(GTK_RANGE(scale), state->refresh_rate);
        state->updating_slider = FALSE;
    }

    if (g_getenv("WAYLAND_DISPLAY") == NULL) {
        g_timeout_add(500, (GSourceFunc)reassert_desktop, state);
    }
}   

static gboolean
on_settings_close_request(GtkWindow *window, gpointer user_data)
{
    ClockState *state = (ClockState *)user_data;
    save_settings(state);
    gtk_widget_set_visible(GTK_WIDGET(window), FALSE);
    return TRUE;  /* stop the default handler from destroying it */
}   

static void
on_action_about(GSimpleAction *action, GVariant *param, gpointer data)
{
    ClockState *state = data;

    GtkAboutDialog *about = GTK_ABOUT_DIALOG(gtk_about_dialog_new());   
    gtk_about_dialog_set_program_name(about, "Cairo-Clock-Gtk4");
    gtk_about_dialog_set_version(about, "1.0");
    gtk_about_dialog_set_comments(about, _("A 20th anniversary GTK4 port by Evan Slower\nof MacSlow's super fine Cairo-based analog clock"));   
    gtk_about_dialog_set_license_type(about, GTK_LICENSE_GPL_3_0);   

    GdkTexture *tex = gdk_texture_new_from_filename("/usr/local/share/icons/hicolor/256x256/apps/cairo-clock-gtk4-logo.png", NULL);
    gtk_about_dialog_set_logo(about, GDK_PAINTABLE(tex));
    g_object_unref(tex);
    gtk_window_set_transient_for(GTK_WINDOW(about), GTK_WINDOW(state->main_window));
    gtk_widget_set_name(GTK_WIDGET(about), "aboutDialog");   
    gtk_window_present(GTK_WINDOW(about));
}      

static void
on_action_quit(GSimpleAction *action, GVariant *param, gpointer data)
{
    ClockState *state = data;
    g_application_quit(G_APPLICATION(state->app));
} 

static void
on_activate (GtkApplication *app, gpointer user_data)
{
    GError *error = NULL;
    ClockState *state = (ClockState *)user_data;
    state->app = app;
    const gchar *saved_theme = g_settings_get_string(state->settings, "theme");
    if (saved_theme && saved_theme[0] != '\0') {
        state->theme = g_strdup(saved_theme);
    } else if (state->theme_path) {
        state->theme = g_strdup(state->theme_path);
    } else {
        state->theme = g_strdup("zen_MacSlow");
    }   
    g_free((gchar *)saved_theme);
    g_free(state->theme_path);
    state->theme_path = g_build_filename(DATA_DIR, "themes", state->theme, NULL);  
    // 1. Load ALL 12 SVG Resources into the array
    for (int i = 0; i < CLOCK_ELEMENTS; i++) {
        g_autofree gchar *svg_path = g_build_filename(state->theme_path, CLOCK_FILE_NAMES[i], NULL);
        state->svg_handles[i] = rsvg_handle_new_from_file(svg_path, &error);
                if (error) {
            g_clear_error(&error);
        }   
    }

    // 2. Verify the first layer (Face) loaded successfully
    if (!state->svg_handles[0]) {
        g_critical("FATAL: Clock face failed to load. Aborting.");
        return;
    }

    // 3. Register app-level actions (before menu model references them)
    GSimpleAction *settings = g_simple_action_new("settings", NULL);
    g_signal_connect(settings, "activate", G_CALLBACK(on_action_settings), state);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(settings));

    GSimpleAction *about = g_simple_action_new("about", NULL);
    g_signal_connect(about, "activate", G_CALLBACK(on_action_about), state);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(about));

    GSimpleAction *quit = g_simple_action_new("quit", NULL);
    g_signal_connect(quit, "activate", G_CALLBACK(on_action_quit), state);
    g_action_map_add_action(G_ACTION_MAP(app), G_ACTION(quit));
    // ---------------------------------------------------------------

    // 4. UI setup
    if (!load_ui_resources_and_widgets(state, &error)) {
        g_critical("UI Loading Failed: %s", error->message);
        g_error_free(error);
        for (int i = 0; i < CLOCK_ELEMENTS; i++) {
            if (state->svg_handles[i]) {
                g_object_unref(state->svg_handles[i]);
            }
        }
        return;
    }

    setup_window_geometry_and_style(state);
    initialize_widget_states(state);
    connect_application_signals(state);
    setup_actions_controllers_and_layout(state);

		// 5. Finalize and Present
	if (g_getenv("WAYLAND_DISPLAY") == NULL) {
		g_signal_connect_after(state->main_window, "map",
			G_CALLBACK(on_mapped), state);
	}
	
	gtk_window_present(GTK_WINDOW(state->main_window)); 
}

/* --- Helper 1: UI Loading and Widget Retrieval --- */
static gboolean
load_ui_resources_and_widgets(ClockState *state, GError **error)
{
    const gchar *resource_path = "/org/gnome/cairo-clock/data/cairo-clock-gtk4.ui";

    state->builder = gtk_builder_new();
    if (!gtk_builder_add_from_resource(state->builder, resource_path, error)) {
        g_object_unref(state->builder);
        return FALSE;
    }

    GMenu *menu = g_menu_new();
    g_menu_append(menu, _("Settings"), "app.settings");
    g_menu_append(menu, _("About"), "app.about");
    g_menu_append(menu, _("Quit"), "app.quit");   
	state->popup_model = G_MENU_MODEL(g_object_ref_sink(menu));
    
    state->main_window      = GTK_WIDGET(gtk_builder_get_object(state->builder, "mainWindow"));
    state->settings_dialog  = GTK_WIDGET(gtk_builder_get_object(state->builder, "settingsDialog"));
    gtk_widget_set_name(GTK_WIDGET(state->settings_dialog), "settingsDialog");   
	state->table_startup_size = GTK_WIDGET(gtk_builder_get_object(state->builder, "tableStartupSize"));   
    state->drawing_area     = GTK_DRAWING_AREA(gtk_drawing_area_new());
    state->combobox_startup_size = GTK_WIDGET(gtk_builder_get_object(state->builder, "comboboxStartupSize"));
    g_signal_connect(state->combobox_startup_size, "notify::selected",
                 G_CALLBACK(on_startup_size_changed), state);   

    if (!state->main_window || !state->settings_dialog || !state->drawing_area) {
        g_set_error(error, G_IO_ERROR, G_IO_ERROR_FAILED, "Critical UI widgets missing in XML");
        g_object_unref(state->builder);
        return FALSE;
    }

    gtk_window_set_application(GTK_WINDOW(state->main_window), GTK_APPLICATION(state->app));
    gtk_window_set_application(GTK_WINDOW(state->settings_dialog), GTK_APPLICATION(state->app));
    
       return TRUE;
}   

/* --- Helper 2: Window Geometry and CSS --- */
static void
setup_window_geometry_and_style(ClockState *state) 
{
    
    if (!state->main_window) {
        g_critical("FATAL: main_window is NULL — builder lookup failed!");
            return;
    }
        
    GdkDisplay *display = gtk_widget_get_display(state->main_window);
   
    if (!gdk_display_is_composited(display)) {
        g_warning("WARNING: Compositor is not running. Transparency may appear black.");
    }

   	if (g_getenv("WAYLAND_DISPLAY") != NULL) {
		GtkWidget *empty_titlebar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
		gtk_widget_set_size_request(empty_titlebar, 0, 0);
		gtk_window_set_titlebar(GTK_WINDOW(state->main_window), empty_titlebar);
	}
	gtk_window_set_decorated(GTK_WINDOW(state->main_window), FALSE);
	gtk_window_set_resizable(GTK_WINDOW(state->main_window), TRUE);
	gtk_window_set_title(GTK_WINDOW(state->main_window), _("MacSlow's Cairo-Clock"));   
    
    GtkCssProvider *provider = gtk_css_provider_new();

    const char *css =
	"window { background-color: rgba(0,0,0,0); border-radius: 0; box-shadow: none; } "
	".window-frame { border-radius: 0; box-shadow: none; margin: 0; } "
	"drawingarea { background-color: rgba(0,0,0,0); } "
	"#settingsDialog { background-image: none; background-color: rgba(0,0,0,0.75); } "
	"#aboutDialog { background-image: none; background-color: rgba(0,0,0,0.75); }";   

    // Use load_from_string (GTK 4.10+) or load_from_data (older GTK 4)
    #if GTK_CHECK_VERSION(4, 10, 0)
        gtk_css_provider_load_from_string(provider, css);
    #else
        gtk_css_provider_load_from_data(provider, css, -1, NULL);
    #endif

    gtk_style_context_add_provider_for_display(display, 
                                               GTK_STYLE_PROVIDER(provider), 
                                               GTK_STYLE_PROVIDER_PRIORITY_USER);
    
    g_object_unref(provider);
}   

/* --- Helper 3: Initialize Widget Values --- */
static void
initialize_widget_states(ClockState *state)
{
    GtkWidget *spin_w       = GTK_WIDGET(gtk_builder_get_object(state->builder, "spinbuttonWidth"));
    GtkWidget *spin_h       = GTK_WIDGET(gtk_builder_get_object(state->builder, "spinbuttonHeight"));
    GtkWidget *scale        = GTK_WIDGET(gtk_builder_get_object(state->builder, "hscaleSmoothness"));
    state->smoothness_scale = scale;
    GtkWidget *chk_sec      = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonSeconds"));
    GtkWidget *chk_date     = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonDate"));
    GtkWidget *chk_top      = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonKeepOnTop"));
    GtkWidget *chk_sticky = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonSticky"));   
    GtkWidget *chk_24h    = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbutton24h"));   
    
    state->spin_button_width  = spin_w;
    state->spin_button_height = spin_h;
    
    
    if(spin_w) gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_w), (gdouble)state->iWidth); 
    if(spin_h) gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin_h), (gdouble)state->iHeight);
    if(scale)  gtk_range_set_value(GTK_RANGE(scale), (gdouble)state->refresh_rate);

    if(chk_sec)  gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_sec), state->show_seconds);
    if(chk_date) gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_date), state->show_date);
    if(chk_top)  gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_top), state->keep_on_top);
    if(chk_sticky) gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_sticky), state->sticky);   
    if(chk_24h) gtk_check_button_set_active(GTK_CHECK_BUTTON(chk_24h), state->use_24_hour);

    guint startup_size = g_settings_get_int(state->settings, "startup-size-kind");

    if (state->combobox_startup_size) {
        gtk_drop_down_set_selected(GTK_DROP_DOWN(state->combobox_startup_size), startup_size);
        gtk_widget_set_sensitive(state->table_startup_size,
            (startup_size == SIZE_CUSTOM1 || startup_size == SIZE_CUSTOM2));
    }   
    GtkStringList *themes = load_theme_list();
    state->theme_dropdown = GTK_WIDGET(gtk_builder_get_object(state->builder, "comboboxTheme"));
    gtk_drop_down_set_model(GTK_DROP_DOWN(state->theme_dropdown), G_LIST_MODEL(themes));   
    // Select the current theme from state
    if (state->theme_path) {
           gtk_drop_down_set_selected(GTK_DROP_DOWN(state->theme_dropdown), 0); 
    } else {
        gtk_drop_down_set_selected(GTK_DROP_DOWN(state->theme_dropdown), 0);
    }

    guint n = g_list_model_get_n_items(G_LIST_MODEL(themes));   
    for (int i = 0; i < n; i++) {
        if (g_strcmp0(gtk_string_list_get_string(themes, i), state->theme) == 0) {
            gtk_drop_down_set_selected(GTK_DROP_DOWN(state->theme_dropdown), i);
            break;
        }
    }   
    
    g_object_unref(themes); 
}   

/* --- Helper 4: Connect Signals --- */
static void
connect_application_signals(ClockState *state)
{
    g_signal_connect(state->main_window, "notify::mapped", G_CALLBACK(on_window_mapped), state);
    g_signal_connect(state->main_window, "destroy", G_CALLBACK(on_window_close), state);
    g_signal_connect(state->settings_dialog, "close-request", G_CALLBACK(on_settings_close_request), state);   

    GtkWidget *spin_w = GTK_WIDGET(gtk_builder_get_object(state->builder, "spinbuttonWidth"));
    GtkWidget *spin_h = GTK_WIDGET(gtk_builder_get_object(state->builder, "spinbuttonHeight"));
    GtkWidget *scale  = GTK_WIDGET(gtk_builder_get_object(state->builder, "hscaleSmoothness"));
    GtkWidget *chk_sec  = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonSeconds"));
    GtkWidget *chk_date = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonDate"));
    GtkWidget *chk_sticky = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonSticky"));
    GtkWidget *chk_24h      = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbutton24h"));

    if(spin_w) g_signal_connect(spin_w, "value-changed", G_CALLBACK(on_width_value_changed), state);
    if(spin_h) g_signal_connect(spin_h, "value-changed", G_CALLBACK(on_height_value_changed), state);
    g_signal_connect(state->spin_button_width, "value-changed", G_CALLBACK(on_spin_value_changed), state);
    g_signal_connect(state->spin_button_height, "value-changed", G_CALLBACK(on_spin_value_changed), state);
    if(scale)  g_signal_connect(scale, "value-changed", G_CALLBACK(on_value_changed), state);
    if(chk_sec) g_signal_connect(chk_sec, "toggled", G_CALLBACK(on_seconds_toggled), state);
    if(chk_date) g_signal_connect(chk_date, "toggled", G_CALLBACK(on_date_toggled), state);
    if(chk_sticky) g_signal_connect(chk_sticky, "toggled", G_CALLBACK(on_sticky_toggled), state);   
    if(chk_24h)     g_signal_connect(chk_24h,     "toggled", G_CALLBACK(on_24h_toggled),             state);

    g_signal_connect(state->theme_dropdown, "notify::selected-item", G_CALLBACK(on_theme_changed), state);
    g_timeout_add_seconds(1, poll_and_save_position, state->main_window);   
   
    GtkGesture *gesture = gtk_gesture_click_new();
    gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
    g_signal_connect(gesture, "pressed", G_CALLBACK(on_drawing_area_clicked), state);
    gtk_widget_add_controller(GTK_WIDGET(state->drawing_area), GTK_EVENT_CONTROLLER(gesture));
       
    // 1. Retrieve the widget (Must happen BEFORE the connection)
    GtkWidget *chk_top = GTK_WIDGET(gtk_builder_get_object(state->builder, "checkbuttonKeepOnTop"));

    // 2. Connect the signal (Must check if chk_top is not NULL)
    if (chk_top) {
        g_signal_connect(chk_top, "toggled", G_CALLBACK(on_keep_on_top_toggled), state);
    }   
}

/* --- Helper 5: Actions, Layout, and Animation --- */
static void
setup_actions_controllers_and_layout(ClockState *state)
{
    if (!state->main_window)
    return;   
    
   const GActionEntry actions[] = {
        { "info", on_action_about, NULL, NULL, NULL }
    };   
    g_action_map_add_action_entries(G_ACTION_MAP(state->main_window), actions, G_N_ELEMENTS(actions), NULL);

    g_signal_connect(state->main_window, "close-request", G_CALLBACK(on_close), state);

    state->main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    
    gtk_drawing_area_set_draw_func(GTK_DRAWING_AREA(state->drawing_area), clock_draw_func, state, NULL);   

    guint kind = g_settings_get_int(state->settings, "startup-size-kind");
    int w, h;
    switch (kind) {
        case SIZE_SMALL:   w = 150; h = 150; break;
        case SIZE_MEDIUM:  w = 250; h = 250; break;
        case SIZE_LARGE:   w = 350; h = 350; break;
        case SIZE_CUSTOM1: w = g_settings_get_int(state->settings, "custom1-width");
                        h = g_settings_get_int(state->settings, "custom1-height"); break;
        case SIZE_CUSTOM2: w = g_settings_get_int(state->settings, "custom2-width");
                        h = g_settings_get_int(state->settings, "custom2-height"); break;
        default:           w = 150; h = 150; break;
    }

    if (w < 50) w = 350;
    if (h < 50) h = 350;
   
    state->updating_spin = TRUE;
    if (state->spin_button_width)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_width), w);
    if (state->spin_button_height)
        gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_height), h);   
    state->updating_spin = FALSE;

    gtk_widget_set_size_request(state->main_box, -1, -1);   

    gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(state->drawing_area), w);
    gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(state->drawing_area), h);
    gtk_widget_set_hexpand(GTK_WIDGET(state->drawing_area), TRUE);
    gtk_widget_set_vexpand(GTK_WIDGET(state->drawing_area), TRUE);

    // Add to layout
    gtk_box_append(GTK_BOX(state->main_box), GTK_WIDGET(state->drawing_area));
    gtk_window_set_child(GTK_WINDOW(state->main_window), state->main_box);
    gtk_window_set_default_size(GTK_WINDOW(state->main_window), w, h);

    // Start animation
    state->needs_update = TRUE;
    state->tick_callback_id = gtk_widget_add_tick_callback(
        GTK_WIDGET(state->drawing_area),
        (GtkTickCallback)on_refresh_timer,
        state, NULL);   
}

static void
clock_draw_func(GtkDrawingArea *area, cairo_t *cr,
                int width, int height, gpointer user_data)
{
	if (width == 0 || height == 0) return;  // skip if not yet allocated
    ClockState *state = (ClockState *)user_data;

    if (width == 0 || height == 0) return;
    if (!state->svg_handles[0]) return;

    RsvgRectangle face_viewport = { -0.5, -0.5, 1.0, 1.0 };
    RsvgRectangle hand_viewport = { 0.0, 0.0, 1.0, 1.0 };
    double shadowX = -0.0075, shadowY = 0.0075;
    GError *error = NULL;

    cairo_save(cr);
    cairo_scale(cr, width, height);
    cairo_translate(cr, 0.5, 0.5);
    cairo_scale(cr, 0.9, 0.9);

    for (int i = 0; i < CLOCK_HOUR_HAND_SHADOW; i++) {
        if (!state->svg_handles[i]) continue;
        rsvg_handle_render_document(state->svg_handles[i], cr, &face_viewport, &error);
        g_clear_error(&error);
    }

    cairo_save(cr);
    cairo_rotate(cr, -G_PI / 2.0);

    if (state->show_date) {
        cairo_save(cr);
        cairo_identity_matrix(cr);

        GDateTime *now = g_date_time_new_now_local();
        char *date_str = g_date_time_format(now, "%b %e");
        g_date_time_unref(now);

        PangoLayout *layout = pango_cairo_create_layout(cr);
        PangoFontDescription *desc = pango_font_description_from_string("Sans Bold");
        pango_font_description_set_absolute_size(desc, height * 0.06 * PANGO_SCALE);
        pango_layout_set_font_description(layout, desc);
        pango_font_description_free(desc);
        pango_layout_set_text(layout, date_str, -1);

        int tw, th;
        pango_layout_get_pixel_size(layout, &tw, &th);

        cairo_move_to(cr, width / 2.0 - tw / 2.0, height * 0.58);
        cairo_set_source_rgb(cr, 144/255.0, 84/255.0, 0.0);
        pango_cairo_show_layout(cr, layout);

        g_object_unref(layout);
        g_free(date_str);

        cairo_restore(cr);
    }

    for (int i = CLOCK_HOUR_HAND_SHADOW; i < CLOCK_GLASS; i++) {
        if ((i == CLOCK_SECOND_HAND_SHADOW || i == CLOCK_SECOND_HAND) && !state->show_seconds)
            continue;
        if (!state->svg_handles[i]) continue;

        cairo_save(cr);

        if (i == CLOCK_HOUR_HAND_SHADOW || i == CLOCK_MINUTE_HAND_SHADOW || i == CLOCK_SECOND_HAND_SHADOW)
            cairo_translate(cr, shadowX, shadowY);

        double angle;
        switch (i) {
        case CLOCK_HOUR_HAND_SHADOW:
        case CLOCK_HOUR_HAND:
            angle = state->fAngleHour;
            break;
        case CLOCK_MINUTE_HAND_SHADOW:
        case CLOCK_MINUTE_HAND:
            angle = state->bAnimateMinute
                  ? (state->fAngleMinute + state->fFactor * 6.0)
                  : state->fAngleMinute;
            break;
        case CLOCK_SECOND_HAND_SHADOW:
        case CLOCK_SECOND_HAND:
            angle = state->fAngleSecond + state->fFactor * 6.0;
            break;
        default:
            angle = 0.0;
            break;
        }
        cairo_rotate(cr, angle * G_PI / 180.0);

        rsvg_handle_render_document(state->svg_handles[i], cr, &hand_viewport, &error);
        g_clear_error(&error);
        cairo_restore(cr);
    }

    cairo_restore(cr);

    rsvg_handle_render_document(state->svg_handles[CLOCK_GLASS], cr, &face_viewport, &error);
    g_clear_error(&error);
    rsvg_handle_render_document(state->svg_handles[CLOCK_FRAME], cr, &face_viewport, &error);
    g_clear_error(&error);

    cairo_restore(cr);
}   

static void
on_drawing_area_clicked(GtkGestureClick *gesture,
                        gint            n_press,
                        gdouble         x,
                        gdouble         y,
                        gpointer        user_data)
{
    ClockState *state = (ClockState *)user_data;
    guint button = gtk_gesture_single_get_current_button(GTK_GESTURE_SINGLE(gesture));

    if (button == GDK_BUTTON_SECONDARY) {
        // Right-click: show popup menu
        GtkPopover *popover = GTK_POPOVER(
            gtk_popover_menu_new_from_model(state->popup_model));
        gtk_popover_set_has_arrow(popover, FALSE);

        GdkRectangle rect = { (gint)x, (gint)y, 1, 1 };
        gtk_popover_set_pointing_to(popover, &rect);
        gtk_widget_set_parent(GTK_WIDGET(popover),
                              GTK_WIDGET(state->drawing_area));
        gtk_popover_popup(popover);
    }
    else if (button == GDK_BUTTON_PRIMARY) {
        // Left-click: drag window
        GtkWidget *widget = gtk_event_controller_get_widget(
            GTK_EVENT_CONTROLLER(gesture));
        GdkEvent *event = gtk_event_controller_get_current_event(
            GTK_EVENT_CONTROLLER(gesture));

        if (event) {
            GdkSurface *surface = gtk_native_get_surface(
                gtk_widget_get_native(widget));
            if (surface && GDK_IS_TOPLEVEL(surface)) {
                GdkToplevel *toplevel = GDK_TOPLEVEL(surface);
                GdkDevice *device = gdk_event_get_device(event);
                guint32 timestamp = gdk_event_get_time(event);
                double move_x, move_y;
                gdk_event_get_position(event, &move_x, &move_y);
                gdk_toplevel_begin_move(toplevel, device,
                    GDK_BUTTON_PRIMARY, move_x, move_y, timestamp);
            }
        }
    }
}   

static void on_window_close(GtkWindow *window, gpointer user_data) {
    ClockState *state = (ClockState *)user_data;

    save_settings(state);
}   

static void on_window_mapped(GtkWindow *window, GParamSpec *pspec, gpointer user_data) {
        
    ClockState *state = (ClockState *)user_data;

    // Start the tick timer if not already running
    if (state->tick_callback_id == 0) {
        state->tick_callback_id = gtk_widget_add_tick_callback(GTK_WIDGET(window), on_refresh_timer, state, NULL);
    }   
}   

static void
on_startup_size_changed (GtkDropDown *pDropDown,
                         GParamSpec *pspec,
                         gpointer user_data) // Changed from 'gpointer window'
{
    ClockState *state = (ClockState *)user_data; // Cast to state
    guint selected_id = gtk_drop_down_get_selected(pDropDown);
    int width, height;

    if (!state->spin_button_width || !state->spin_button_height)
    return;   

    switch (selected_id)
    {
        case SIZE_SMALL:
            state->startup_size_kind = SIZE_SMALL;
            gtk_widget_set_sensitive(state->table_startup_size, FALSE);
            state->updating_spin = TRUE;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_width), 150);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_height), 150);
            state->updating_spin = FALSE;
            width = 150; height = 150;
            break;
        case SIZE_MEDIUM:
            state->startup_size_kind = SIZE_MEDIUM;
            gtk_widget_set_sensitive(state->table_startup_size, FALSE);
            state->updating_spin = TRUE;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_width), 250);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_height), 250);
            state->updating_spin = FALSE;
            width = 250; height = 250;
            break;
        case SIZE_LARGE:
            state->startup_size_kind = SIZE_LARGE;
            gtk_widget_set_sensitive(state->table_startup_size, FALSE);
            state->updating_spin = TRUE;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_width), 350);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_height), 350);
            state->updating_spin = FALSE;
            width = 350; height = 350;
            break;   
        case SIZE_CUSTOM1:
            state->startup_size_kind = SIZE_CUSTOM1;
            gtk_widget_set_sensitive(state->table_startup_size, TRUE);
            state->updating_spin = TRUE;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_width), state->custom1_width);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_height), state->custom1_height);
            state->updating_spin = FALSE;
            width = state->custom1_width;
            height = state->custom1_height;
            break;   
        case SIZE_CUSTOM2:
            state->startup_size_kind = SIZE_CUSTOM2;
            gtk_widget_set_sensitive(state->table_startup_size, TRUE);
            state->updating_spin = TRUE;
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_width), state->custom2_width);
            gtk_spin_button_set_value(GTK_SPIN_BUTTON(state->spin_button_height), state->custom2_height);
            state->updating_spin = FALSE;
            width = state->custom2_width;
            height = state->custom2_height;
            break;   
        default:
            return;
    }

    if (state->startup_size_kind == SIZE_CUSTOM1) {
        state->custom1_width = width;
        state->custom1_height = height;
    } else if (state->startup_size_kind == SIZE_CUSTOM2) {
        state->custom2_width = width;
        state->custom2_height = height;
    }   

        gtk_drawing_area_set_content_width(GTK_DRAWING_AREA(state->drawing_area), width);
        gtk_drawing_area_set_content_height(GTK_DRAWING_AREA(state->drawing_area), height);
        gtk_window_set_default_size(GTK_WINDOW(state->main_window), width, height);
        gtk_widget_queue_draw(GTK_WIDGET(state->drawing_area));
}   

static void
on_height_value_changed (GtkSpinButton *pSpinButton,
                         gpointer       user_data)
{
    ClockState *state = (ClockState *)user_data;
    gint iWidth, iOldHeight, iNewHeight;

    state->needs_update = TRUE;
    GtkWidget *window = GTK_WIDGET(state->main_window);

    gtk_window_get_default_size (GTK_WINDOW(window), &iWidth, &iOldHeight);
    iNewHeight = (gint)gtk_spin_button_get_value (pSpinButton);

       if (iOldHeight != iNewHeight)
    {
        if (state->startup_size_kind == SIZE_CUSTOM1)
            state->custom1_height = iNewHeight;
        else if (state->startup_size_kind == SIZE_CUSTOM2)
            state->custom2_height = iNewHeight;   
        gtk_drawing_area_set_content_height (GTK_DRAWING_AREA(state->drawing_area), iNewHeight);
        gtk_window_set_default_size (GTK_WINDOW(window), iWidth, iNewHeight);
        gtk_widget_queue_draw (GTK_WIDGET(state->drawing_area));
    }   
}   

static void
on_width_value_changed (GtkSpinButton *pSpinButton,
                        gpointer       user_data)
{
    ClockState *state = (ClockState *)user_data;
    gint iWidth, iHeight, iNewWidth;

    state->needs_update = TRUE;
    GtkWidget *window = GTK_WIDGET(state->main_window);

    gtk_window_get_default_size (GTK_WINDOW(window), &iWidth, &iHeight);
    iNewWidth = (gint)gtk_spin_button_get_value (pSpinButton);

        if (iWidth != iNewWidth)
    {
        if (state->startup_size_kind == SIZE_CUSTOM1)
            state->custom1_width = iNewWidth;
        else if (state->startup_size_kind == SIZE_CUSTOM2)
            state->custom2_width = iNewWidth;   
        gtk_drawing_area_set_content_width (GTK_DRAWING_AREA(state->drawing_area), iNewWidth);
        gtk_window_set_default_size (GTK_WINDOW(window), iNewWidth, iHeight);
        gtk_widget_queue_draw (GTK_WIDGET(state->drawing_area));
    }   
}   

static void
on_spin_value_changed (GtkSpinButton *spin, gpointer user_data)
{
    ClockState *state = (ClockState *)user_data;
    if (state->updating_spin) return;
    int w = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(state->spin_button_width));
    int h = (int)gtk_spin_button_get_value(GTK_SPIN_BUTTON(state->spin_button_height));

    if (state->startup_size_kind == 1) {
        state->custom1_width = w;
        state->custom1_height = h;
    } else if (state->startup_size_kind == 2) {
        state->custom2_width = w;
        state->custom2_height = h;
    }
}   

static void
on_value_changed (GtkRange *pRange,
                  gpointer  user_data)
{
   ClockState *state = (ClockState *)user_data;
    if (state->updating_slider) return;
    state->refresh_rate = (gint)gtk_range_get_value(pRange);
}   

static void
save_position(GtkWidget *window)
{
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
    if (!GDK_IS_X11_SURFACE(surface))
        return;

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    XID xid = GDK_SURFACE_XID(surface);
    Display *display = GDK_SURFACE_XDISPLAY(surface);
#pragma GCC diagnostic pop   

    Window root, parent, *children;
    unsigned int n;
    XQueryTree(display, xid, &root, &parent, &children, &n);
    if (children) XFree(children);

    int x, y;
    Window child;
    XTranslateCoordinates(display, parent, root, 0, 0, &x, &y, &child);

    char *path = g_build_filename(g_get_user_config_dir(),
                                  "cairo-clock-gtk4", "position.ini", NULL);
    GKeyFile *kf = g_key_file_new();
    g_key_file_set_integer(kf, "window", "x", x);
    g_key_file_set_integer(kf, "window", "y", y);
    g_mkdir_with_parents(g_path_get_dirname(path), 0755);

    gsize length = 0;
    char *data = g_key_file_to_data(kf, &length, NULL);
    g_file_set_contents(path, data, length, NULL);
    g_free(data);

    g_key_file_free(kf);
    g_free(path);
}

static void
restore_position(GtkWidget *window)
{
    char *path = g_build_filename(g_get_user_config_dir(),
                                  "cairo-clock-gtk4", "position.ini", NULL);
    GKeyFile *kf = g_key_file_new();
    if (!g_key_file_load_from_file(kf, path, G_KEY_FILE_NONE, NULL)) {
        g_key_file_free(kf);
        g_free(path);
        return;
    }
    int x = g_key_file_get_integer(kf, "window", "x", NULL);
    int y = g_key_file_get_integer(kf, "window", "y", NULL);
    g_key_file_free(kf);
    g_free(path);

    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
    if (!GDK_IS_X11_SURFACE(surface))
        return;

    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    XID xid = GDK_SURFACE_XID(surface);
    Display *display = GDK_SURFACE_XDISPLAY(surface);
    XMoveWindow(display, xid, x, y);
    XFlush(display);
    #pragma GCC diagnostic pop
}   

static gboolean
on_close(GtkWidget *window, gpointer user_data)
{
    ClockState *state = (ClockState *)user_data;

    save_settings(state);
    save_position(window);
    g_application_quit(G_APPLICATION(state->app));
    return TRUE;
}   

static gboolean
poll_and_save_position(gpointer user_data)
{
    GtkWidget *window = GTK_WIDGET(user_data);
    GdkSurface *surface = gtk_native_get_surface(GTK_NATIVE(window));
    
    if (!GDK_IS_X11_SURFACE(surface))
        return G_SOURCE_CONTINUE; 

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
    XID xid = GDK_SURFACE_XID(surface);
    Display *display = GDK_SURFACE_XDISPLAY(surface);
#pragma GCC diagnostic pop

    int x, y;
    Window child;
    XTranslateCoordinates(display, xid, DefaultRootWindow(display), 0, 0, &x, &y, &child);   

    char *path = g_build_filename(g_get_user_config_dir(), "cairo-clock-gtk4", "position.ini", NULL);
                                  
    GKeyFile *kf = g_key_file_new();
    g_key_file_set_integer(kf, "window", "x", x);
    g_key_file_set_integer(kf, "window", "y", y);
    g_mkdir_with_parents(g_path_get_dirname(path), 0755);

    gsize length = 0;
    char *data = g_key_file_to_data(kf, &length, NULL);
    g_file_set_contents(path, data, length, NULL);
    g_free(data);

    g_key_file_free(kf);
    g_free(path);

    return G_SOURCE_CONTINUE; // Keep timer running
}   

static void
on_seconds_toggled (GtkCheckButton *pTogglebutton,
                    gpointer        user_data)
{
    ClockState *state = (ClockState *)user_data;

    if (gtk_check_button_get_active (pTogglebutton))
        state->show_seconds = 1;
    else
        state->show_seconds = 0;

    gtk_widget_queue_draw (GTK_WIDGET (state->drawing_area));
}   

static void
on_date_toggled (GtkCheckButton *pTogglebutton,
                 gpointer        user_data)
{
    ClockState *state = (ClockState *)user_data;

    if (gtk_check_button_get_active (pTogglebutton))
        state->show_date = 1;
    else
        state->show_date = 0;

    gtk_widget_queue_draw (GTK_WIDGET (state->drawing_area));
}   

static void
on_keep_on_top_toggled(GtkCheckButton *pTogglebutton, gpointer user_data)
{
    ClockState *state = (ClockState *)user_data;
    state->keep_on_top = gtk_check_button_get_active(pTogglebutton) ? 1 : 0;

    g_settings_set_boolean(state->settings, "keep-on-top", state->keep_on_top);
    toggle_window_hints_x11(GTK_WINDOW(state->main_window), state->keep_on_top, state->sticky);   
}   

static void
on_sticky_toggled(GtkCheckButton *pTogglebutton, gpointer user_data)
{
    ClockState *state = (ClockState *)user_data;
    state->sticky = gtk_check_button_get_active(pTogglebutton) ? 1 : 0;

    g_settings_set_boolean(state->settings, "sticky", state->sticky);
    toggle_window_hints_x11(GTK_WINDOW(state->main_window), state->keep_on_top, state->sticky);   
}   

static void
on_24h_toggled (GtkCheckButton *pTogglebutton, 
                gpointer         user_data)
{
    ClockState *state = (ClockState *)user_data;
    state->use_24_hour = gtk_check_button_get_active (pTogglebutton) ? 1 : 0;
} 
  
static void
read_settings (GSettings *settings,
               gint *piX, gint *piY,
               gint *piCustom1Width, gint *piCustom1Height,
               gint *piCustom2Width, gint *piCustom2Height,
               gboolean *piShowSeconds, gboolean *piShowDate,
               gchar **pcTheme,
               gboolean *piKeepOnTop, gboolean *piSticky,
               gboolean *pi24, gint *piRefreshRate)
{
    *piX            = g_settings_get_int (settings, "default-x");
    *piY            = g_settings_get_int (settings, "default-y");
    *piCustom1Width  = g_settings_get_int (settings, "custom1-width");
    *piCustom1Height = g_settings_get_int (settings, "custom1-height");
    *piCustom2Width  = g_settings_get_int (settings, "custom2-width");
    *piCustom2Height = g_settings_get_int (settings, "custom2-height");
    *piShowSeconds  = g_settings_get_boolean (settings, "show-seconds");
    *piShowDate     = g_settings_get_boolean (settings, "show-date");
    *pcTheme        = g_settings_get_string (settings, "theme");
    *piKeepOnTop    = g_settings_get_boolean (settings, "keep-on-top");
    *piSticky       = g_settings_get_boolean (settings, "sticky");
    *pi24           = g_settings_get_boolean (settings, "use-24h");
    *piRefreshRate  = g_settings_get_int (settings, "refresh-rate");
}   

static void
save_settings (ClockState *state)
{
    g_settings_set_int (state->settings, "default-x", state->default_x);
    g_settings_set_int (state->settings, "default-y", state->default_y);
    g_settings_set_int (state->settings, "startup-size-kind", state->startup_size_kind);   
    g_settings_set_int (state->settings, "custom1-width", state->custom1_width);
    g_settings_set_int (state->settings, "custom1-height", state->custom1_height);
    g_settings_set_int (state->settings, "custom2-width", state->custom2_width);
    g_settings_set_int (state->settings, "custom2-height", state->custom2_height);
    g_settings_set_int (state->settings, "refresh-rate", state->refresh_rate);

    g_settings_set_boolean (state->settings, "show-seconds", state->show_seconds);
    g_settings_set_boolean (state->settings, "show-date", state->show_date);
    g_settings_set_boolean (state->settings, "keep-on-top", state->keep_on_top);
    g_settings_set_boolean (state->settings, "sticky", state->sticky);
    g_settings_set_boolean (state->settings, "use-24h", state->use_24_hour);

    g_settings_set_string (state->settings, "theme", state->theme);
}   

static gchar*
get_system_theme_path (void)
{
	return PKGDATA_DIR "/themes";
}

static gchar*
get_user_theme_path (void)
{
    return g_build_filename (g_get_home_dir (), ".cairo-clock-gtk4", "themes", NULL);
} 

static gboolean
is_valid_theme_folder (const gchar *path)
{
    gchar *check_file = g_build_filename (path, "clock-face.svg", NULL);
    gboolean exists = g_file_test (check_file, G_FILE_TEST_EXISTS);
    g_free (check_file);
    return exists;
}

static gint
compare_theme_strings(gconstpointer a, gconstpointer b)
{
    return g_ascii_strcasecmp(*(const gchar *const *)a,
                              *(const gchar *const *)b);
}   

static gint
compare_theme_entries(gconstpointer a, gconstpointer b)
{
    const ThemeEntry *ta = (const ThemeEntry *)a;
    const ThemeEntry *tb = (const ThemeEntry *)b;

    const gchar *sa = ta->pName ? ta->pName->str : NULL;
    const gchar *sb = tb->pName ? tb->pName->str : NULL;

    return g_strcmp0(sa, sb);
}   

static GList*
get_theme_list (GString* pSystemPath,
                GString* pUserPath)
{
    GDir*       pThemeDir   = NULL;
    const gchar* name       = NULL; 
    GList*      pThemeList  = NULL;

        
    /* 1. Process System Path */
    pThemeDir = g_dir_open (pSystemPath->str, 0, NULL);
    if (pThemeDir)
    {
        while ((name = g_dir_read_name (pThemeDir)) != NULL)
	{
	    gchar *full_path = g_build_filename (pSystemPath->str, name, NULL);
	    if (!g_file_test (full_path, G_FILE_TEST_IS_DIR))
	    {
		g_free (full_path);
		continue;
	    }
	    g_free (full_path);

	    ThemeEntry* pThemeEntry = g_new0 (ThemeEntry, 1);
	    pThemeEntry->pPath = g_string_new (pSystemPath->str);
	    pThemeEntry->pName = g_string_new (name);
	    
	    pThemeList = g_list_append (pThemeList, pThemeEntry);
	}   
        g_dir_close (pThemeDir);
    }

    /* 2. Process User Path */
    pThemeDir = g_dir_open (pUserPath->str, 0, NULL);
    if (pThemeDir)
    {
        while ((name = g_dir_read_name (pThemeDir)) != NULL)
	{
	    gchar *full_path = g_build_filename (pUserPath->str, name, NULL);
	    if (!g_file_test (full_path, G_FILE_TEST_IS_DIR))
	    {
		g_free (full_path);
		continue;
	    }
	    g_free (full_path);

	    ThemeEntry* pThemeEntry = g_new0 (ThemeEntry, 1);
	    pThemeEntry->pPath = g_string_new (pUserPath->str);
	    pThemeEntry->pName = g_string_new (name);
	    
	    pThemeList = g_list_append (pThemeList, pThemeEntry);
	}   
        g_dir_close (pThemeDir);
    }
    
	GList *sorted = NULL;
    for (GList *l = pThemeList; l; l = l->next) {
        sorted = g_list_insert_sorted(sorted, l->data, compare_theme_entries);
    }
    g_list_free(pThemeList);
    pThemeList = sorted;   
    return pThemeList;
}   

static GtkStringList *
load_theme_list (void)
{
    GPtrArray *names = g_ptr_array_new_with_free_func(g_free);

    gchar *dir_path = g_build_filename(DATA_DIR, "themes", NULL);

    DIR *dir = opendir(dir_path);

    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            gchar *full_path = g_build_filename(dir_path, entry->d_name, NULL);

            if (g_file_test(full_path, G_FILE_TEST_IS_DIR) && is_valid_theme_folder(full_path)) {
                g_ptr_array_add(names, g_strdup(entry->d_name));
            }

            g_free(full_path);
        }
        closedir(dir);
    }

    g_free(dir_path);

    g_ptr_array_sort(names, compare_theme_strings);   

    g_ptr_array_add(names, NULL);
    GtkStringList *store = gtk_string_list_new((const gchar *const *)names->pdata);
   
    g_ptr_array_free(names, TRUE);   
    return store;
}   

// Helper: Get dimensions safely across all librsvg versions
static gboolean
get_svg_dimensions (RsvgHandle *handle, gdouble *w, gdouble *h)
{
    #if LIBRSVG_CHECK_VERSION(2, 52, 0)
        return rsvg_handle_get_intrinsic_size_in_pixels(handle, w, h);
    #else
        RsvgDimensionData dims;
        rsvg_handle_get_dimensions(handle, &dims);
        *w = (gdouble)dims.width;
        *h = (gdouble)dims.height;
        return TRUE; 
    #endif
}

static void
change_theme(const gchar *theme_name, ClockState *state)
{
    gint        iElement       = 0;
    GError*     pError         = NULL;
    gchar      *theme_path     = NULL;
    RsvgHandle *new_handles[CLOCK_ELEMENTS] = { NULL };
    
    if (!theme_name) return;

    theme_path = g_build_filename(DATA_DIR, "themes", theme_name, NULL);  // install

    if (!g_file_test(theme_path, G_FILE_TEST_IS_DIR)) {
        g_critical("Theme path not found: %s", theme_path);
        g_free(theme_path);
        return;
    }   

        for (iElement = 0; iElement < CLOCK_ELEMENTS; iElement++) {
        gchar *pcFullFilename = g_build_filename(theme_path, CLOCK_FILE_NAMES[iElement], NULL);
        GFile *file = g_file_new_for_path(pcFullFilename);

        new_handles[iElement] = rsvg_handle_new_from_gfile_sync(
            file, RSVG_HANDLE_FLAGS_NONE, NULL, &pError);

        if (!new_handles[iElement] || pError) {
            if (pError) { g_error_free(pError); pError = NULL; }
            new_handles[iElement] = NULL;
        }

        g_object_unref(file);
        g_free(pcFullFilename);
    }   

    for (iElement = 0; iElement < CLOCK_ELEMENTS; iElement++) {
        if (state->svg_handles[iElement]) {
            g_object_unref(state->svg_handles[iElement]);
        }
        state->svg_handles[iElement] = new_handles[iElement];
    }

    if (state->svg_handles[CLOCK_DROP_SHADOW]) {
        gdouble w, h;
        if (get_svg_dimensions(state->svg_handles[CLOCK_DROP_SHADOW], &w, &h)) {
            state->svg_width = (gint)w;
            state->svg_height = (gint)h;
        }
    }

    if (state->main_window)
        gtk_widget_queue_draw(state->main_window);

    g_free(theme_path);
}   

static void
on_theme_changed (GtkDropDown *pDropDown, 
                  GParamSpec *pspec,
                  gpointer data)
{
    GObject*      pSelectedItem = NULL;
    const gchar*  pThemeName    = NULL;
    
   ClockState*   state = (ClockState*)data; 

    state->needs_update = TRUE;

    pSelectedItem = gtk_drop_down_get_selected_item(pDropDown);

    if (pSelectedItem && GTK_IS_STRING_OBJECT(pSelectedItem)) {
        pThemeName = gtk_string_object_get_string(GTK_STRING_OBJECT(pSelectedItem));
        
        if (pThemeName) {
            g_free(state->theme);
            state->theme = g_strdup(pThemeName);
            g_settings_set_string(state->settings, "theme", pThemeName);
            change_theme(pThemeName, state);
            return;
        }   
    }
}   

static gboolean 
on_refresh_timer(GtkWidget *widget, GdkFrameClock *frame_clock, gpointer data) 
{
    ClockState *state = (ClockState *)data;

    if (gtk_widget_get_width(state->main_box) == 0)   
        return G_SOURCE_CONTINUE;

    gint64 now_ms = g_get_monotonic_time () / 1000;
    if (now_ms - state->last_update_ms < 1000 / state->refresh_rate)
        return G_SOURCE_CONTINUE;
    state->last_update_ms = now_ms;   
   
    GDateTime *now = g_date_time_new_now_local();
    gint new_second = g_date_time_get_second(now);
    gint new_minute = g_date_time_get_minute(now);
    gint new_hour   = g_date_time_get_hour(now);
    gint new_frames = state->refresh_rate * g_date_time_get_microsecond(now) / 1000000;
    g_date_time_unref(now);

    gdouble new_angle_second = new_second * 6.0;
    gdouble new_angle_minute = new_minute * 6.0; 
    gdouble new_angle_hour;
    if (state->use_24_hour)
        new_angle_hour = new_hour * 15.0 + (new_minute * 0.25);
    else
        new_angle_hour = (new_hour % 12) * 30.0 + (new_minute * 0.5);   
    
    if (new_angle_second >= 360.0) new_angle_second = 0.0;
    if (new_angle_minute >= 360.0) new_angle_minute = 0.0;
    if (new_angle_hour   >= 360.0) new_angle_hour   -= 360.0;

    // Calculate overshoot factor
    gdouble new_factor = powf((float)new_frames / (float)state->refresh_rate * 0.875f * 3.4f, 3.0f) *
                         sinf((float)new_frames / (float)state->refresh_rate * 0.875f * G_PI) * 0.1f;
    
    gboolean changed = FALSE;

    if (state->show_seconds) {
        changed = TRUE;
    } else {
        if (new_minute != state->iMinute || new_hour != state->iHour) {
            changed = TRUE;
        }
    }

    if (changed) {
        state->iSecond = new_second;
        state->iMinute = new_minute;
        state->iHour   = new_hour;
        state->iFrames = new_frames;
        
        state->fAngleSecond = new_angle_second;
        state->fAngleMinute = new_angle_minute;
        state->fAngleHour   = new_angle_hour;
        state->fFactor      = new_factor;
        
        state->bAnimateMinute = (new_angle_second >= 354.0);   
    }

    gtk_widget_queue_draw(GTK_WIDGET(state->drawing_area));   
    return G_SOURCE_CONTINUE;
}   

int
main(int argc, char **argv)
{
    setlocale(LC_ALL, "");
	g_setenv("GSK_RENDERER", "cairo", FALSE); 
    bindtextdomain(GETTEXT_PACKAGE, CAIROCLOCKLOCALEDIR);
    bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
    textdomain(GETTEXT_PACKAGE);

    ClockState *state = g_new0(ClockState, 1);

    state->custom1_width = 200;
    state->custom1_height = 200;
    state->custom2_width = 300;
    state->custom2_height = 300;   
   
    state->iX = -1;
    state->iY = -1;
    state->iWidth = -1;
    state->iHeight = -1;
    state->refresh_rate = 30;

    gchar *user_path = get_user_theme_path();
    GString *sys_path = g_string_new(get_system_theme_path());
    GString *usr_path = g_string_new(user_path);
    g_free(user_path);
    state->theme_list = get_theme_list(sys_path, usr_path);
    g_string_free(sys_path, TRUE);
    g_string_free(usr_path, TRUE);   
    
    state->settings = g_settings_new("org.gnome.cairo-clock");

    read_settings (state->settings,
               &state->iX, &state->iY,
               &state->custom1_width, &state->custom1_height,
               &state->custom2_width, &state->custom2_height,
               &state->show_seconds, &state->show_date,
               &state->theme_path,
               &state->keep_on_top,
               &state->sticky,
               &state->use_24_hour,
               &state->refresh_rate);   

    GtkApplication *app = gtk_application_new(
        "org.gnome.cairo-clock",
        G_APPLICATION_NON_UNIQUE
    );

    const GOptionEntry options[] = {
        { "xposition", 'x', 0,
          G_OPTION_ARG_INT, &state->iX,
          N_("x-position of the top-left window-corner"), "X" },
        { "yposition", 'y', 0,
          G_OPTION_ARG_INT, &state->iY,
          N_("y-position of the top-left window-corner"), "Y" },
        { "width", 'w', 0,
          G_OPTION_ARG_INT, &state->iWidth,
          N_("open window with this width"), "WIDTH" },
        { "height", 'h', 0,
          G_OPTION_ARG_INT, &state->iHeight,
          N_("open window with this height"), "HEIGHT" },
        { "seconds", 's', 0,
          G_OPTION_ARG_NONE, &state->show_seconds,
          N_("draw seconds hand"), NULL },
        { "date", 'd', 0,
          G_OPTION_ARG_NONE, &state->show_date,
          N_("draw date-display"), NULL },
        { "list", 'l', 0,
          G_OPTION_ARG_NONE, &print_theme_list,
          N_("list installed themes and exit"), NULL },
        { "theme", 't', 0,
          G_OPTION_ARG_STRING, &state->theme_path,
          N_("theme to draw the clock with"), "NAME" },
        { "ontop", 'o', 0,
          G_OPTION_ARG_NONE, &state->keep_on_top,
          N_("clock-window stays on top of all windows"), NULL },
        { "twentyfour", 'f', 0,
          G_OPTION_ARG_NONE, &state->use_24_hour,
          N_("hands work in 24 hour mode"), NULL },
        { "refresh", 'r', 0,
          G_OPTION_ARG_INT, &state->refresh_rate,
          N_("render at RATE (default: 30 Hz)"), "RATE" },
        { "version", 'v', 0,
          G_OPTION_ARG_NONE, &print_version,
          N_("print version of program and exit"), NULL },
        { NULL }
    };

    g_application_add_main_option_entries(G_APPLICATION(app), options);

    g_signal_connect(app, "activate", G_CALLBACK(on_activate), state);

    int status = g_application_run(G_APPLICATION(app), argc, argv);

    clock_state_free(state);
    g_object_unref(app);

    return status;   
}   




   


	

