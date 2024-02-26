/*
 * uMTP Responder
 * Copyright (c) 2018 - 2021 Viveris Technologies
 *
 * uMTP Responder is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 3.0 of the License, or (at your option) any later version.
 *
 * uMTP Responder is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License version 3 for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with uMTP Responder; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

/**
 * @file   usb_gadget_fct.h
 * @brief  USB gadget / FunctionFS layer - Public functions declarations.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#ifndef _INC_USB_GADGET_FCT_H_
#define _INC_USB_GADGET_FCT_H_

#define EP_DESCRIPTOR_OUT       0
#define EP_DESCRIPTOR_IN        1
#define EP_DESCRIPTOR_INT_IN    2

int read_usb(void * ctx, unsigned char * buffer, int maxsize);
int write_usb(void * ctx, int channel, unsigned char * buffer, int size);

#endif
