#include <locale.h>
#include <gio/gio.h>
#include <orientation.h>
#include <wayland-client.h>
#include "display.h"
#include "wlr-output-management-unstable-v1-protocol.h"

static GMainLoop *loop;
static guint watch_id;
static GDBusProxy *iio_proxy;
char* monitor;

static void properties_changed (GDBusProxy *proxy,
		    GVariant   *changed_properties,
		    GStrv       invalidated_properties,
		    gpointer    user_data)
{
	GVariant *v;
	GVariantDict dict;

	g_variant_dict_init (&dict, changed_properties);

	if (g_variant_dict_contains (&dict, "HasAccelerometer")) {
		v = g_dbus_proxy_get_cached_property (iio_proxy, "HasAccelerometer");
		if (g_variant_get_boolean (v))
			g_print ("Accelerometer appeared\n");
		else
			g_print ("Accelerometer disappeared\n");
		g_variant_unref (v);
	}
	if (g_variant_dict_contains (&dict, "AccelerometerOrientation")) {
		v = g_dbus_proxy_get_cached_property (iio_proxy, "AccelerometerOrientation");
		const char* o_string = g_variant_get_string (v, NULL);
		g_print("Orientation changed: %s\n", o_string);
		OrientationUp o = string_to_orientation(o_string);
		apply_orientation(o, monitor);
		g_variant_unref(v);
	}

	g_variant_dict_clear (&dict);
}

static void
appeared_cb (GDBusConnection *connection,
	     const gchar     *name,
	     const gchar     *name_owner,
	     gpointer         user_data)
{
	GError *error = NULL;
	GVariant *ret = NULL;

	g_print ("iio-sensor-proxy appeared\n");

	iio_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
						   G_DBUS_PROXY_FLAGS_NONE,
						   NULL,
						   "net.hadess.SensorProxy",
						   "/net/hadess/SensorProxy",
						   "net.hadess.SensorProxy",
						   NULL, NULL);

	g_signal_connect (G_OBJECT (iio_proxy), "g-properties-changed",
			  G_CALLBACK (properties_changed), NULL);

    ret = g_dbus_proxy_call_sync (iio_proxy,
                        "ClaimAccelerometer",
                        NULL,
                        G_DBUS_CALL_FLAGS_NONE,
                        -1,
                        NULL, &error);
    if (!ret) {
        if (!g_error_matches (error, G_IO_ERROR, G_IO_ERROR_CANCELLED))
            g_warning ("Failed to claim accelerometer: %s", error->message);
        g_main_loop_quit (loop);
        return;
    }
    g_clear_pointer (&ret, g_variant_unref);

	GVariant *v;
    v = g_dbus_proxy_get_cached_property (iio_proxy, "HasAccelerometer");
    if (g_variant_get_boolean (v)) {
        g_variant_unref (v);
        v = g_dbus_proxy_get_cached_property (iio_proxy, "AccelerometerOrientation");
        g_print ("Has accelerometer (orientation: %s)\n",
            g_variant_get_string (v, NULL));
    } else {
        g_print ("No accelerometer\n");
    }
    g_variant_unref (v);
}

static void
vanished_cb (GDBusConnection *connection,
	     const gchar *name,
	     gpointer user_data)
{
	if (iio_proxy) {
		g_clear_object (&iio_proxy);
		g_print ("iio-sensor-proxy vanished, waiting for it to appear\n");
	}
}

int main (int argc, char **argv) {
	g_autoptr(GOptionContext) option_context = NULL;
	g_autoptr(GError) error = NULL;
	const GOptionEntry options[] = {
		{ "monitor", 'm', 0, G_OPTION_ARG_STRING, &monitor, "Which monitor to rotate", NULL },
		{ NULL}
	};
	int ret = 0;
	ret = init_display();

	if(!ret) {
		return EXIT_FAILURE;
	}

	setlocale (LC_ALL, "");
	option_context = g_option_context_new ("");
	g_option_context_add_main_entries (option_context, options, NULL);

	ret = g_option_context_parse (option_context, &argc, &argv, &error);
	if(monitor == NULL) {
		g_printerr("Please specify monitor\n");
		return EXIT_FAILURE;
	}
	if (!ret) {
		g_printerr("Failed to parse arguments: %s\n", error->message);
		return EXIT_FAILURE;
	}

	watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
				     "net.hadess.SensorProxy",
				     G_BUS_NAME_WATCHER_FLAGS_NONE,
				     appeared_cb,
				     vanished_cb,
				     NULL, NULL);

	g_print ("Waiting for iio-sensor-proxy...\n");
	loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run(loop);
	destroy_display();
	return EXIT_SUCCESS;
}
