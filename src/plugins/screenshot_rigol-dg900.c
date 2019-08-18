/*
 * Copyright (c) 2017  Martin Lund
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the copyright holders nor contributors may be
 *    used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT
 * HOLDERS OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <lxi.h>
#include "error.h"
#include "screenshot.h"

#define IMAGE_SIZE_MAX 0x400000 // 4 MB
#define HEADER_LENGTH 2

int rigol_dg900_screenshot(char *address, int timeout)
{
    char response[IMAGE_SIZE_MAX];
    char *command;
    int device, l, length, n;
    
    // Connect to LXI instrument
    device = lxi_connect(address, 0, NULL, timeout, VXI11);
    if (device == LXI_ERROR)
    {
        lxi_disconnect(device);
	error_printf("Failed to connect\n");
        return 1;
    }

    // Send SCPI command to grab BMP image
    command = "HCOPy:SDUMp:DATA:FORMat BMP";
    lxi_send(device, command, strlen(command), timeout);
    command = ":HCOPy:SDUMp:DATA?";
    lxi_send(device, command, strlen(command), timeout);
   
    /*read length of the header*/
    length = lxi_receive(device, response, HEADER_LENGTH, timeout);
    if (length < 0)
    {
        lxi_disconnect(device);
        error_printf("Failed to receive header\n");
        return 1;
    }
    response[length] = '\0';
    n = atoi(&response[1]);

    /* read length of data */
    length = lxi_receive(device, response, n, timeout);
    if (length < 0 || length != n)
    {
        lxi_disconnect(device);
        error_printf("Failed to receive data length\n");
        return 1;
    }
    response[length] = '\0';
    n = atoi(response);

    /* read data */
    length = 0;
    while(n > length)
    {
        l = lxi_receive \
	    (device, \
	     response + length, \
	     n - length > IMAGE_SIZE_MAX? IMAGE_SIZE_MAX : n - length, \
	     timeout);
        if (l < 0)
        {
            lxi_disconnect(device);
            error_printf("Failed to receive data\n");
            return 1;
        }
        length += l;
    }

    // Dump remaining BMP image data to file
    screenshot_file_dump(response, length, "bmp");

    // Disconnect
    lxi_disconnect(device);

    return 0;
}

// Screenshot plugin configuration
struct screenshot_plugin rigol_dg900 =
{
    .name = "rigol-dg900",
    .description = "Rigol DG 900 series function generator",
    .regex = "RIGOL TECHNOLOGIES Rigol Technologies DG9.. DG8..",
    .screenshot = rigol_dg900_screenshot
};
