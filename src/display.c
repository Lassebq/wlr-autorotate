#include <glib.h>
#include <wayland-client.h>
#include <stdbool.h>
#include "orientation.h"
#include "wlr-output-management-unstable-v1-protocol.h"

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

struct DisplayInfo
{
    struct wl_display *display;
    struct randr_head head;
    struct randr_state state;
};

struct DisplayInfo display_info = {};
struct wl_registry *registry;

static void head_handle_name(void *data,
		struct zwlr_output_head_v1 *wlr_head, const char *name) {
	struct randr_head *head = data;
	head->name = strdup(name);
}

static void head_handle_description(void *data, struct zwlr_output_head_v1 *wlr_head, const char *description) {
}

static void head_handle_physical_size(void *data, struct zwlr_output_head_v1 *wlr_head, int32_t width, int32_t height) {
}

static void head_handle_mode(void *data, struct zwlr_output_head_v1 *wlr_head, struct zwlr_output_mode_v1 *wlr_mode) {
}

static void head_handle_enabled(void *data, struct zwlr_output_head_v1 *wlr_head, int32_t enabled) {
	struct randr_head *head = data;
	head->enabled = !!enabled;
}

static void head_handle_current_mode(void *data, struct zwlr_output_head_v1 *wlr_head, struct zwlr_output_mode_v1 *wlr_mode) {
}

static void head_handle_position(void *data, struct zwlr_output_head_v1 *wlr_head, int32_t x, int32_t y) {
}

static void head_handle_transform(void *data, struct zwlr_output_head_v1 *wlr_head, int32_t transform) {
	struct randr_head *head = data;
	head->transform = transform;
}

static void head_handle_scale(void *data, struct zwlr_output_head_v1 *wlr_head, wl_fixed_t scale) {
}

static void head_handle_finished(void *data, struct zwlr_output_head_v1 *wlr_head) {
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

static void output_manager_handle_head(void *data, struct zwlr_output_manager_v1 *manager, struct zwlr_output_head_v1 *wlr_head) {
	struct randr_state *state = data;
	struct randr_head *head = calloc(1, sizeof(*head));
	head->state = state;
	head->wlr_head = wlr_head;
	wl_list_insert(state->heads.prev, &head->link);

	zwlr_output_head_v1_add_listener(wlr_head, &head_listener, head);
}

static void output_manager_handle_finished(void *data, struct zwlr_output_manager_v1 *manager) {
}

static void output_manager_handle_done(void *data, struct zwlr_output_manager_v1 *manager, uint32_t serial) {
	struct randr_state *state = data;
	state->serial = serial;
}

static const struct zwlr_output_manager_v1_listener output_manager_listener = {
	.head = output_manager_handle_head,
	.finished = output_manager_handle_finished,
	.done = output_manager_handle_done,
};

static void registry_handle_global(void *data, struct wl_registry *registry, uint32_t name, const char *interface, uint32_t version) {
	struct randr_state *state = data;

	if (strcmp(interface, zwlr_output_manager_v1_interface.name) == 0) {
		state->output_manager = wl_registry_bind(registry, name,
			&zwlr_output_manager_v1_interface, 1);
		zwlr_output_manager_v1_add_listener(state->output_manager,
			&output_manager_listener, state);
	}
}

static void registry_handle_global_remove(void *data, struct wl_registry *registry, uint32_t name) {
}

static const struct wl_registry_listener registry_listener = {
	.global = registry_handle_global,
	.global_remove = registry_handle_global_remove,
};

void apply_orientation(OrientationUp o, char* output) {
	struct wl_display* display = display_info.display;
	struct randr_state state = display_info.state;

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

int init_display() {
	wl_list_init(&display_info.state.heads);

	display_info.display = wl_display_connect(NULL);
	if (display_info.display == NULL) {
		g_printerr("failed to connect to display\n");
		return 0;
	}

	registry = wl_display_get_registry(display_info.display);
	wl_registry_add_listener(registry, &registry_listener, &display_info.state);
	wl_display_dispatch(display_info.display);
	wl_display_roundtrip(display_info.display);

	if (display_info.state.output_manager == NULL) {
		g_printerr("compositor doesn't support "
			"wlr-output-management-unstable-v1\n");
		return 0;
	}
    return 1;
}

void destroy_display() {
	struct wl_display* display = display_info.display;
	while (wl_display_dispatch(display) != -1);
    
	struct randr_head *head, *tmp_head;
	wl_list_for_each_safe(head, tmp_head, &display_info.state.heads, link) {
		zwlr_output_head_v1_destroy(head->wlr_head);
		free(head);
	}

	zwlr_output_manager_v1_destroy(display_info.state.output_manager);
	wl_registry_destroy(registry);
	wl_display_disconnect(display);
}