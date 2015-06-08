/* Copyright 2015 Alexander Koch <lynix47@gmail.com>
 *
 * This file is part of 'libtemper1'. 'libtemper1' is distributed under the
 * terms of the MIT License, see file LICENSE.
 */


/* Reads temperature value from TEMPer1 USB device. Handles device discovery and
 * initialization transparently. Keeps device open for re-use.
 */
double 	temper1_read(char **err);

/* Performs proper shutdown of libusb environment, closes open file handle.
 * Always remember to call this function when device is not required anymore.
 */
void 	temper1_exit();
