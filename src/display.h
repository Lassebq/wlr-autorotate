#ifndef DISPLAY_H
#define DISPLAY_H

#include "orientation.h"

void apply_orientation(OrientationUp o, char* output);
int init_display();
int destroy_display();

#endif