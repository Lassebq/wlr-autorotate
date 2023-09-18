#ifndef ORIENTATION_H
#define ORIENTATION_H

typedef enum
{
        ORIENTATION_UNDEFINED,
        ORIENTATION_NORMAL,
        ORIENTATION_BOTTOM_UP,
        ORIENTATION_LEFT_UP,
        ORIENTATION_RIGHT_UP
} OrientationUp;

const char *orientation_to_string(OrientationUp o);
OrientationUp string_to_orientation(const char *orientation);
enum wl_output_transform orientation_transform(OrientationUp o);

#endif