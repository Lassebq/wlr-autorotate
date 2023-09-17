#include <glib.h>
#include <wayland-client.h>
#include "orientation.h"
#include "wlr-output-management-unstable-v1-protocol.h"

struct Orientation
{
    const char *name;
    enum wl_output_transform transform;
};

static const struct Orientation orientations[] = {
    [ORIENTATION_UNDEFINED] = {"undefined", -1},
    [ORIENTATION_NORMAL] = {"normal", WL_OUTPUT_TRANSFORM_NORMAL},
    [ORIENTATION_BOTTOM_UP] = {"bottom-up", WL_OUTPUT_TRANSFORM_180},
    [ORIENTATION_LEFT_UP] = {"left-up", WL_OUTPUT_TRANSFORM_90},
    [ORIENTATION_RIGHT_UP] = {"right-up", WL_OUTPUT_TRANSFORM_270}};

const char *orientation_to_string(OrientationUp o)
{
    return orientations[o].name;
}

OrientationUp string_to_orientation(const char *orientation)
{
    unsigned int i;

    if (orientation == NULL)
        return ORIENTATION_UNDEFINED;
    for (i = 0; i < sizeof(orientations); i++)
    {
        if (g_str_equal(orientation, orientations[i].name))
            return i;
    }
    return ORIENTATION_UNDEFINED;
}

enum wl_output_transform orientation_transform(OrientationUp o)
{
    return orientations[o].transform;
}