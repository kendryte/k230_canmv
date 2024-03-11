/*
 * Copyright (c) 2006-2018, RT-Thread Development Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2018-02-11     Bernard      Ignore O_CREAT flag in open.
 */
#include <rthw.h>
#include <rtthread.h>
#include <rtdevice.h>

#include <dfs.h>
#include <dfs_fs.h>
#include <dfs_file.h>

#include "devfs.h"

struct device_dirent
{
    rt_device_t *devices;
    rt_uint16_t read_index;
    rt_uint16_t device_count;
};

int dfs_device_fs_mount(struct dfs_filesystem *fs, unsigned long rwflag, const void *data)
{
    return RT_EOK;
}

int dfs_device_fs_ioctl(struct dfs_fd *file, int cmd, void *args)
{
    rt_err_t result;
    rt_device_t dev_id;

    RT_ASSERT(file != RT_NULL);

    /* get device handler */
    dev_id = (rt_device_t)file->fnode->data;
    RT_ASSERT(dev_id != RT_NULL);

    if ((file->fnode->path[0] == '/') && (file->fnode->path[1] == '\0'))
        return -RT_ENOSYS; 

    /* close device handler */
    result = rt_device_control(dev_id, cmd, args);
    if (result == RT_EOK)
        return RT_EOK;

    return result;
}

int dfs_device_fs_read(struct dfs_fd *file, void *buf, size_t count)
{
    int result;
    rt_device_t dev_id;

    RT_ASSERT(file != RT_NULL);

    /* get device handler */
    dev_id = (rt_device_t)file->fnode->data;
    RT_ASSERT(dev_id != RT_NULL);

    if ((file->fnode->path[0] == '/') && (file->fnode->path[1] == '\0'))
        return -RT_ENOSYS;
    /* read device data */
    result = rt_device_read(dev_id, file->pos, buf, count);
    file->pos += result;

    return result;
}

int dfs_device_fs_write(struct dfs_fd *file, const void *buf, size_t count)
{
    int result;
    rt_device_t dev_id;

    RT_ASSERT(file != RT_NULL);

    /* get device handler */
    dev_id = (rt_device_t)file->fnode->data;
    RT_ASSERT(dev_id != RT_NULL);

    if ((file->fnode->path[0] == '/') && (file->fnode->path[1] == '\0'))
        return -RT_ENOSYS;

    /* read device data */
    result = rt_device_write(dev_id, file->pos, buf, count);
    file->pos += result;

    return result;
}

int dfs_device_fs_close(struct dfs_fd *file)
{
    rt_err_t result;
    rt_device_t dev_id;

    RT_ASSERT(file != RT_NULL);
    RT_ASSERT(file->fnode->ref_count > 0);

    if (file->fnode->ref_count > 1)
    {
        return 0;
    }

    if (file->fnode->type == FT_DIRECTORY && (file->fnode->path[0] == '/') && (file->fnode->path[1] == '\0'))
    {
        struct device_dirent *root_dirent;

        root_dirent = (struct device_dirent *)file->fnode->data;
        RT_ASSERT(root_dirent != RT_NULL);

        /* release dirent */
        rt_free(root_dirent);
        return RT_EOK;
    }

    /* get device handler */
    dev_id = (rt_device_t)file->fnode->data;
    RT_ASSERT(dev_id != RT_NULL);

    /* close device handler */
    result = rt_device_close(dev_id);
    if (result == RT_EOK)
    {
        file->fnode->data = RT_NULL;

        return RT_EOK;
    }

    return -EIO;
}

int dfs_device_fs_open(struct dfs_fd *file)
{
    rt_err_t result;
    rt_device_t device;
    rt_base_t level;

    RT_ASSERT(file->fnode->ref_count > 0);
    if (file->fnode->ref_count > 1)
    {
        file->pos = 0;
        return 0;
    }
    /* open root directory */
    if ((file->fnode->path[0] == '/') && (file->fnode->path[1] == '\0') &&
        (file->flags & O_DIRECTORY))
    {
        struct rt_object *object;
        struct rt_list_node *node;
        struct rt_object_information *information;
        struct device_dirent *root_dirent;
        rt_uint32_t count = 0;

        /* disable interrupt */
        level = rt_hw_interrupt_disable();

        /* traverse device object */
        information = rt_object_get_information(RT_Object_Class_Device);
        RT_ASSERT(information != RT_NULL);
        for (node = information->object_list.next; node != &(information->object_list); node = node->next)
        {
            count ++;
        }

        root_dirent = (struct device_dirent *)rt_malloc(sizeof(struct device_dirent) +
                      count * sizeof(rt_device_t));
        if (root_dirent != RT_NULL)
        {
            root_dirent->devices = (rt_device_t *)(root_dirent + 1);
            root_dirent->read_index = 0;
            root_dirent->device_count = count;
            count = 0;
            /* get all device node */
            for (node = information->object_list.next; node != &(information->object_list); node = node->next)
            {
                object = rt_list_entry(node, struct rt_object, list);
                root_dirent->devices[count] = (rt_device_t)object;
                count ++;
            }
        }
        rt_hw_interrupt_enable(level);

        /* set data */
        file->fnode->data = root_dirent;

        return RT_EOK;
    }
#ifdef RT_USING_DEV_BUS
    else if (file->flags & O_CREAT)
    {
        if (!(file->flags & O_DIRECTORY))
        {
            return -ENOSYS;
        }
        /* regester bus device */
        if (rt_device_bus_create(&file->fnode->path[1], 0) == RT_NULL)
        {
            return -EEXIST;
        }
    }
#endif

    device = rt_device_find(&file->fnode->path[1]);
    if (device == RT_NULL)
    {
        return -ENODEV;
    }

#ifdef RT_USING_POSIX
    if (device->fops)
    {
        /* use device fops */
        file->fnode->fops = device->fops;
        file->fnode->data = (void *)device;

        /* use fops */
        if (file->fnode->fops->open)
        {
            result = file->fnode->fops->open(file);
            if (result == RT_EOK || result == -RT_ENOSYS)
            {
                file->fnode->type = FT_DEVICE;
                return 0;
            }
        }
    }
    else
#endif
    {
        result = rt_device_open(device, RT_DEVICE_OFLAG_RDWR);
        if (result == RT_EOK || result == -RT_ENOSYS)
        {
            file->fnode->data = device;
            file->fnode->type = FT_DEVICE;
            return RT_EOK;
        }
    }

    file->fnode->data = RT_NULL;
    /* open device failed. */
    return -EIO;
}

int dfs_device_fs_lseek(struct dfs_fd *file, off_t offset)
{
    RT_ASSERT(file != RT_NULL);
    file->pos = offset;
    return offset;
}

int dfs_device_fs_unlink(struct dfs_filesystem *fs, const char *path)
{
#ifdef RT_USING_DEV_BUS
    rt_device_t dev_id;

    dev_id = rt_device_find(&path[1]);
    if (dev_id == RT_NULL)
    {
        return -1;
    }
    if (dev_id->type != RT_Device_Class_Bus)
    {
        return -1;
    }
    rt_device_bus_destroy(dev_id);
#endif
    return RT_EOK;
}

int dfs_device_fs_stat(struct dfs_filesystem *fs, const char *path, struct stat *st)
{
    /* stat root directory */
    if ((path[0] == '/') && (path[1] == '\0'))
    {
        st->st_dev = 0;

        st->st_mode = S_IFREG | S_IRUSR | S_IRGRP | S_IROTH |
                      S_IWUSR | S_IWGRP | S_IWOTH;
        st->st_mode &= ~S_IFREG;
        st->st_mode |= S_IFDIR | S_IXUSR | S_IXGRP | S_IXOTH;

        st->st_size  = 0;
        st->st_mtime = 0;

        return RT_EOK;
    }
    else
    {
        rt_device_t dev_id;

        dev_id = rt_device_find(&path[1]);
        if (dev_id != RT_NULL)
        {
            st->st_dev = 0;

            st->st_mode = S_IRUSR | S_IRGRP | S_IROTH |
                          S_IWUSR | S_IWGRP | S_IWOTH;

            if (dev_id->type == RT_Device_Class_Char)
                st->st_mode |= S_IFCHR;
            else if (dev_id->type == RT_Device_Class_Block)
                st->st_mode |= S_IFBLK;
            else if (dev_id->type == RT_Device_Class_Pipe)
                st->st_mode |= S_IFIFO;
            else if (dev_id->type == RT_Device_Class_Bus)
                st->st_mode |= S_IFDIR;
            else
                st->st_mode |= S_IFREG;

            st->st_size  = 0;
            st->st_mtime = 0;

            return RT_EOK;
        }
    }

    return -ENOENT;
}

int dfs_device_fs_getdents(struct dfs_fd *file, struct dirent *dirp, uint32_t count)
{
    rt_uint32_t index;
    rt_object_t object;
    struct dirent *d;
    struct device_dirent *root_dirent;

    root_dirent = (struct device_dirent *)file->fnode->data;
    RT_ASSERT(root_dirent != RT_NULL);

    /* make integer count */
    count = (count / sizeof(struct dirent));
    if (count == 0)
        return -EINVAL;

    for (index = 0; index < count && index + root_dirent->read_index < root_dirent->device_count; 
        index ++)
    {
        object = (rt_object_t)root_dirent->devices[root_dirent->read_index + index];

        d = dirp + index;
        if ((((rt_device_t)object)->type) == RT_Device_Class_Bus)
        {
            d->d_type = DT_DIR;
        }
        else
        {
            d->d_type = DT_REG;
        }
        d->d_namlen = RT_NAME_MAX;
        d->d_reclen = (rt_uint16_t)sizeof(struct dirent);
        rt_strncpy(d->d_name, object->name, RT_NAME_MAX);
    }

    root_dirent->read_index += index;

    return index * sizeof(struct dirent);
}

static int dfs_device_fs_poll(struct dfs_fd *fd, struct rt_pollreq *req)
{
    int mask = 0;

    return mask;
}

static const struct dfs_file_ops _device_fops =
{
    dfs_device_fs_open,
    dfs_device_fs_close,
    dfs_device_fs_ioctl,
    dfs_device_fs_read,
    dfs_device_fs_write,
    RT_NULL,                    /* flush */
    dfs_device_fs_lseek,
    dfs_device_fs_getdents,
    dfs_device_fs_poll,
};

static const struct dfs_filesystem_ops _device_fs =
{
    "devfs",
    DFS_FS_FLAG_DEFAULT,
    &_device_fops,
    dfs_device_fs_mount,
    RT_NULL, /*unmount*/
    RT_NULL, /*mkfs*/
    RT_NULL, /*statfs*/
    dfs_device_fs_unlink,
    dfs_device_fs_stat,
    RT_NULL, /*rename*/
};

int devfs_init(void)
{
    /* register device file system */
    dfs_register(&_device_fs);

    return 0;
}
