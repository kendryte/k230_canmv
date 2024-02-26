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
 * @file   msgqueue.c
 * @brief  Message queue handler.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <pthread.h>

#include "mtp.h"
#include "mtp_cfg.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_datasets.h"
#include "mtp_ops_helpers.h"

#include "usb_gadget_fct.h"
#include "fs_handles_db.h"
#include "logs_out.h"

int send_message_queue( char * message )
{
	return -1;
}

int msgqueue_handler_init( mtp_ctx * ctx )
{
	return 0;
}

int msgqueue_handler_deinit( mtp_ctx * ctx )
{
	return 0;
}
