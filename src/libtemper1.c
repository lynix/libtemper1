/* Copyright 2014 Alexander Koch <lynix47@gmail.com>
 *
 * This file is part of 'temper2csv'.
 *
 * 'temper2csv' is distributed under the MIT License, see LICENSE file.
 */

#include <libusb-1.0/libusb.h>

#include <fcntl.h>
#include <sys/types.h>
#include <string.h>

#define USB_VID   0x0c45		// USB Vendor ID
#define USB_PID   0x7401		// USB Product ID
#define USB_EPA   0x82			// USB Endpoint Address
#define USB_IFN1  0x00			// USB Interface No.
#define USB_IFN2  0x01			// USB Interface No.
#define USB_CNF   0x01			// USB Configuration No.
#define USB_LEN   8				// USB Transfer Data Length
#define USB_TMO   5000			// USB Transfer Timeout

static const unsigned char data_init[] = { 0x01, 0x01 };
static const unsigned char data_query[] = { 0x01, 0x80, 0x33, 0x01, 0x00, 0x00,
		0x00, 0x00 };

static libusb_context *usb_ctxt = NULL;
static libusb_device *usb_dev = NULL;
static libusb_device_handle *usb_handle = NULL;


static int fail(char **errp, const char *msg)
{
	if (usb_handle != NULL) {
		libusb_release_interface(usb_handle, USB_IFN1);
		libusb_release_interface(usb_handle, USB_IFN2);
		libusb_close(usb_handle);
	}
	if (usb_dev != NULL)
		libusb_unref_device(usb_dev);
	libusb_exit(NULL);

	if (*errp == NULL)
		*errp = (char *)msg;

	return -1;
}


static int temper_open(char **err)
{
	libusb_init(&usb_ctxt);

	libusb_device **dev_list;
	ssize_t num_devs;
	if ((num_devs = libusb_get_device_list(usb_ctxt, &dev_list)) < 0)
		return fail(err, "failed to fetch USB device list");

	libusb_device *dev = NULL;
	char found = 0;
	for (ssize_t i=0; i<num_devs; i++) {
		dev = dev_list[i];
		struct libusb_device_descriptor desc;
		if (libusb_get_device_descriptor(dev, &desc) < 0) {
			*err = "failed to read USB device descriptor";
			break;
		}
		if (desc.idVendor == USB_VID && desc.idProduct == USB_PID) {
			libusb_ref_device(dev);
			found = 1;
			break;
		}
	}
	libusb_free_device_list(dev_list, 1);

	if (!found)
		return fail(err, "no TEMPer1 device found");

	usb_dev = dev;
	if (libusb_open(dev, &usb_handle) != LIBUSB_SUCCESS)
		return fail(err, "failed to open USB device");

	if (libusb_kernel_driver_active(usb_handle, USB_IFN1)) {
		int r = libusb_detach_kernel_driver(usb_handle, USB_IFN1);
		if (r != 0 && r != LIBUSB_ERROR_NOT_FOUND)
			return fail(err, "failed to detach USB kernel driver");
	}
	if (libusb_kernel_driver_active(usb_handle, USB_IFN2)) {
		int r = libusb_detach_kernel_driver(usb_handle, USB_IFN2);
		if (r != 0 && r != LIBUSB_ERROR_NOT_FOUND)
			return fail(err, "failed to detach USB kernel driver");
	}

	if (libusb_set_configuration(usb_handle, USB_CNF) != LIBUSB_SUCCESS)
		return fail(err, "failed to set USB device configuration");

	return 0;
}


static int temper_query(uint16_t value, uint16_t index,	unsigned char *data,
		uint16_t length, char **err)
{
	if (libusb_claim_interface(usb_handle, USB_IFN1) != LIBUSB_SUCCESS ||
			libusb_claim_interface(usb_handle, USB_IFN2) != LIBUSB_SUCCESS)
		return fail(err, "failed to claim interface(s)");

	if (libusb_control_transfer(usb_handle, 0x21, 0x09, value, index, data,
			length, USB_TMO) != length)
		fail(err, "USB control transfer failed");

	libusb_release_interface(usb_handle, USB_IFN1);
	libusb_release_interface(usb_handle, USB_IFN2);

	return 0;
}


static int temper_fetch_data(unsigned char *buffer, int length, char **err)
{
	if (libusb_claim_interface(usb_handle, USB_IFN1) != LIBUSB_SUCCESS ||
			libusb_claim_interface(usb_handle, USB_IFN2) != LIBUSB_SUCCESS)
		return fail(err, "failed to claim interface(s)");

	int ntrans;
	if (libusb_interrupt_transfer(usb_handle, USB_EPA, buffer, length, &ntrans,
			USB_TMO) != 0)
		return fail(err, "USB interrupt transfer failed");

	libusb_release_interface(usb_handle, USB_IFN1);
	libusb_release_interface(usb_handle, USB_IFN2);

	return ntrans;
}


double temper1_read(char **err)
{
	*err = NULL;

	/* device initialization */
	if (usb_handle == NULL) {
		if (temper_open(err) != 0)
			return -1;
		if (temper_query(0x0201, 0x00, (unsigned char *)data_init, 2, err) != 0)
			return -1;
	}

	/* data query */
	if (temper_query(0x0200, 0x01, (unsigned char *)data_query,	USB_LEN, err)
			!= 0)
		return -1;

	/* data retrieval */
	unsigned char data[USB_LEN];
	bzero(data, USB_LEN);
	if (temper_fetch_data(data, USB_LEN, err) < 0)
		return -1;

	double temp = (data[3] & 0xFF) + (data[2] << 8);
	temp *= 125.0 / 32000.0;

	return temp;
}
