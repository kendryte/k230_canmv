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
 * @file   inotify.c
 * @brief  inotify file system events handler.
 * @author Jean-François DEL NERO <Jean-Francois.DELNERO@viveris.fr>
 */

#include "buildconf.h"

#include <inttypes.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>

#include "mtp.h"
#include "mtp_helpers.h"
#include "mtp_constant.h"
#include "mtp_datasets.h"

#include "usbd_core.h"
#include "rtthread.h"

#include "usb_gadget_fct.h"
#include "fs_handles_db.h"
#include "inotify.h"
#include "logs_out.h"

typedef struct {
	int type;
	char *path;
} file_chg_msg_t;

static rt_mq_t mtp_inty_mq;

extern int mtp_fs_db_valid(void);

void inotify_thread(void* arg)
{
	mtp_ctx *ctx = arg;
	file_chg_msg_t msg;
	fs_handles_db *db;
	uint32_t storage_id;
	char send_event_flag;
	uint32_t handle;
	char *seg, *seg_next;
	uint32_t parent;
	filefoundinfo find_finfo;
	fs_entry *entry, *entry_parent;

	while (1) {
		if (RT_EOK != rt_mq_recv(mtp_inty_mq, &msg, sizeof(file_chg_msg_t), RT_WAITING_FOREVER))
			continue;
		if (mtp_fs_db_valid() == 0)
			goto msg_free;
		send_event_flag = 0;
		if (msg.type == NTY_FILE_ADD) {
			filefoundinfo new_finfo;
			fs_entry_stat(msg.path, &new_finfo);
			seg_next = msg.path;
			pthread_mutex_lock(&ctx->inotify_mutex);
			if (mtp_fs_db_valid() == 0)
				goto mtx_free;
			seg = strtok_r(NULL, "/", &seg_next);
			if (strncmp(seg, "sdcard", sizeof("sdcard")) != 0)
				goto mtx_free;
			db = ctx->fs_db;
			storage_id = ctx->storages[0].storage_id;
			seg = "/";
			parent = 0;
			entry_parent = NULL;
			do {
				strncpy(find_finfo.filename, seg, FS_HANDLE_MAX_FILENAME_SIZE);
				entry = search_entry(db, &find_finfo, parent, storage_id);
				if (entry) {
					if (entry->watch_descriptor == -1)
						break;
					parent = entry->handle;
					entry_parent = entry;
					continue;
				}
				if (entry_parent == NULL)
					break;
				// If the entry is not in the db, add it and trigger an MTP_EVENT_OBJECT_ADDED event
				entry = add_entry(db, &new_finfo, parent, storage_id );
				if (entry) {
					// Send an "ObjectAdded" (0x4002) MTP event message with the entry handle.
					handle = entry->handle;
					send_event_flag = 1;
				}
				break;
			} while (seg = strtok_r(NULL, "/", &seg_next));
			pthread_mutex_unlock(&ctx->inotify_mutex);
			if (send_event_flag)
				mtp_push_event(ctx, MTP_EVENT_OBJECT_ADDED, 1, (uint32_t *)&handle);
		} else if (msg.type == NTY_FILE_CHG) {
			seg_next = msg.path;
			pthread_mutex_lock(&ctx->inotify_mutex);
			if (mtp_fs_db_valid() == 0)
				goto mtx_free;
			seg = strtok_r(NULL, "/", &seg_next);
			if (strncmp(seg, "sdcard", sizeof("sdcard")) != 0)
				goto mtx_free;
			db = ctx->fs_db;
			storage_id = ctx->storages[0].storage_id;
			seg = "/";
			parent = 0;
			entry_parent = NULL;
			do {
				strncpy(find_finfo.filename, seg, FS_HANDLE_MAX_FILENAME_SIZE);
				entry = search_entry(db, &find_finfo, parent, storage_id);
				if (entry) {
					if (entry_parent && (entry_parent->watch_descriptor == -1))
						goto mtx_free;
					parent = entry->handle;
					entry_parent = entry;
					continue;
				}
				goto mtx_free;
			} while (seg = strtok_r(NULL, "/", &seg_next));
			// Send an "ObjectInfoChanged" (0x4007) MTP event message with the entry handle.
			handle = entry->handle;
			send_event_flag = 1;
			pthread_mutex_unlock(&ctx->inotify_mutex);
			if (send_event_flag)
				mtp_push_event(ctx, MTP_EVENT_OBJECT_INFO_CHANGED, 1, (uint32_t *)&handle);
		} else if (msg.type == NTY_FILE_RM) {
			seg_next = msg.path;
			pthread_mutex_lock(&ctx->inotify_mutex);
			if (mtp_fs_db_valid() == 0)
				goto mtx_free;
			seg = strtok_r(NULL, "/", &seg_next);
			if (strncmp(seg, "sdcard", sizeof("sdcard")) != 0)
				goto mtx_free;
			db = ctx->fs_db;
			storage_id = ctx->storages[0].storage_id;
			seg = "/";
			parent = 0;
			entry_parent = NULL;
			do {
				strncpy(find_finfo.filename, seg, FS_HANDLE_MAX_FILENAME_SIZE);
				entry = search_entry(db, &find_finfo, parent, storage_id);
				if (entry) {
					if (entry_parent && (entry_parent->watch_descriptor == -1))
						goto mtx_free;
					parent = entry->handle;
					entry_parent = entry;
					continue;
				}
				goto mtx_free;
			} while (seg = strtok_r(NULL, "/", &seg_next));
			// Send an "ObjectRemoved" (0x4003) MTP event message with the entry handle.
			entry->flags |= ENTRY_IS_DELETED;
			entry->watch_descriptor = -1;
			if (entry->name) {
				free(entry->name);
				entry->name = NULL;
			}
			handle = entry->handle;
			send_event_flag = 1;
			pthread_mutex_unlock(&ctx->inotify_mutex);
			if (send_event_flag)
				mtp_push_event(ctx, MTP_EVENT_OBJECT_REMOVED, 1, (uint32_t *)&handle);
		}
msg_free:
		if (msg.path)
			free(msg.path);
		continue;
mtx_free:
		pthread_mutex_unlock(&ctx->inotify_mutex);
		goto msg_free;
	}
}

int inotify_handler_init( mtp_ctx * ctx )
{
	rt_thread_t tid;
	mtp_inty_mq = rt_mq_create("mtp_inty", sizeof(file_chg_msg_t), 16, RT_IPC_FLAG_FIFO);
	tid = rt_thread_create("mtp_inty", inotify_thread, ctx, CONFIG_USBDEV_MTP_STACKSIZE, CONFIG_USBDEV_MTP_PRIO, 10);
	rt_thread_startup(tid);

	return 0;
}

int inotify_handler_deinit( mtp_ctx * ctx )
{
	return 0;
}

int inotify_handler_addwatch( mtp_ctx * ctx, char * path )
{
	return 1;
}

int inotify_handler_rmwatch( mtp_ctx * ctx, int wd )
{
	return -1;
}

int inotify_handler_filechange(int type, char *path)
{
	if (mtp_fs_db_valid() == 0)
		return 0;
	file_chg_msg_t msg = {};
	msg.type = type;
	if (path)
		msg.path = rt_strdup(path);
	rt_mq_send_wait(mtp_inty_mq, &msg, sizeof(file_chg_msg_t), RT_WAITING_FOREVER);
	return 0;
}
