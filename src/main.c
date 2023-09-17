#include <locale.h>
#include <stdbool.h>
#include <gio/gio.h>
#include <orientation.h>
#include <wayland-client.h>
#include "wlr-output-management-unstable-v1-protocol.h"

static GMainLoop *loop;
static guint watch_id;
static GDBusProxy *iio_proxy;
static char* monitor = NULL;

struct randr_state;
struct randr_head;

struct randr_head {
	struct randr_state *state;
	struct zwlr_output_head_v1 *wlr_head;
	struct wl_list link;
	char *name;

	bool enabled;
	enum wl_output_transform transform;
};

struct randr_state {
	struct zwlr_output_manager_v1 *output_manager;

	struct wl_list heads;
	uint32_t serial;
};

static struct wl_display *display = NULL; 
static struct randr_state state = {};

static void head_handle_name(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *name) {
	struct randr_head *head = data;
	head->name = strdup(name);
}

static void head_handle_description(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *description) {
}

static void head_handle_physical_size(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t width, int32_t height) {
}

static void head_handle_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
}

static void head_handle_enabled(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t enabled) {
	struct randr_head *head = data;
	head->enabled = !!enabled;
}

static void head_handle_current_mode(void *data,
		struct zwlr_output_head_v1 *wlr_head,
		struct zwlr_output_mode_v1 *wlr_mode) {
}

static void head_handle_position(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t x, int32_t y) {
}

static void head_handle_transform(void *data,
		struct zwlr_output_head_v1 *wlr_head, int32_t transform) {
	struct randr_head *head = data;
	head->transform = transform;
}

static void head_handle_scale(void *data,
		struct zwlr_output_head_v1 *wlr_head, wl_fixed_t scale) {
}

static void head_handle_finished(void *data,
		struct zwlr_output_head_v1 *wlr_head) {
	struct randr_head *head = data;
	wl_list_remove(&head->link);
	zwlr_output_head_v1_destroy(head->wlr_head);
	free(head->name);
	free(head);
}

static const struct zwlr_output_head_v1_listener head_listener = {
	.name = head_handle_name,
	.description = head_handle_description,
	.physical_size = head_handle_physical_size,
	.mode = head_handle_mode,
	.enabled = head_handle_enabled,
	.current_mode = head_handle_current_mode,
	.position = head_handle_position,
	.transform = head_handle_transform,
	.scale = head_handle_scale,
	.finished = head_handle_finished,
};

static void output_manager_handle_head(void *data,
		struct zwlr_output_manager_v1 *manager,
		struct zwlr_output_head_v1 *wlr_head) {
	struct randr_state *state = data;

	struct randr_head *head = calloc(1, sizeof(*head));
	head->state = state;
	head->wlr_head = wlr_head;
	wl_list_insert(state->heads.prev, &head->link);

	zwlr_output_head_v1_add_listener(wlr_head, &head_listener, head);
}

static void output_manager_handle_finished(void *data,
		struct zwlr_output_manager_v1 *manager) {
	// This space is intentionally left blank
}

static void output_manager_handle_done(void *data,
		struct zwlr_output_manager_v1 *manager, uint32_t serial) {
	struct randr_state *state = data;
	state->serial = serial;
}

static const struct zwlr_output_manager_v1_listener output_manager_listener = {
	.head = output_manager_handle_head,
	.finished = output_manager_handle_finished,
	.done = output_manager_handle_done,
};

static void registry_handle_global(void *data, struct wl_registry *registry,
		uint32_t name, const char *interface, uint32_t version) {
	struct randr_state *state = data;

	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		state->output_manager = wl_registry_bind(registry, name,
			&zwlr_output_manager_v1_interface, 1);
		zwlr_output_manager_v1_add_listener(state->output_manager,
			&output_manager_listener, state);
	}
}

static void registry_handle_global_remove(void *data,
		struct wl_registry *registry, uint32_t name) {
	// This space is intentionally left blank
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

static void apply_orientation(OrientationUp o, char* output) {
	struct zwlr_output_configuration_v1 *config =
		zwlr_output_manager_v1_create_configuration(state.output_manager,
		state.serial);
	struct randr_head *head = NULL;
	wl_list_for_each(head, &state.heads, link) {
		if (!head->enabled) {
			zwlr_output_configuration_v1_disable_head(config, head->wlr_head);
			continue;
		}
		if (strcmp(head->name, output) == 0) {
			struct zwlr_output_configuration_head_v1 *config_head = zwlr_output_configuration_v1_enable_head(config, head->wlr_head);
			zwlr_output_configuration_head_v1_set_transform(config_head, orientation_transform(o));
			break;
		}
	}
	zwlr_output_configuration_v1_apply(config);
	wl_display_dispatch(display);
}

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
	wl_list_init(&state.heads);

	display = wl_display_connect(NULL);
	if (display == NULL) {
		g_printerr("failed to connect to display\n");
		return EXIT_FAILURE;
	}

	struct wl_registry *registry = wl_display_get_registry(display);
	wl_registry_add_listener(registry, &registry_listener, &state);
	wl_display_dispatch(display);
	wl_display_roundtrip(display);

	if (state.output_manager == NULL) {
		g_printerr("compositor doesn't support "
			"wlr-output-management-unstable-v1\n");
		return EXIT_FAILURE;
	}

	g_autoptr(GOptionContext) option_context = NULL;
	g_autoptr(GError) error = NULL;
	const GOptionEntry options[] = {
		{ "monitor", 'm', 0, G_OPTION_ARG_STRING, &monitor, "Which monitor to rotate", NULL },
		{ NULL}
	};
	int ret = 0;

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
	g_main_loop_run (loop);

	while (wl_display_dispatch(display) != -1) {
		// This space intentionally left blank
	}

	struct randr_head *head, *tmp_head;
	wl_list_for_each_safe(head, tmp_head, &state.heads, link) {
		zwlr_output_head_v1_destroy(head->wlr_head);
		free(head);
	}

	zwlr_output_manager_v1_destroy(state.output_manager);
	wl_registry_destroy(registry);
	wl_display_disconnect(display);
	return EXIT_SUCCESS;
}
