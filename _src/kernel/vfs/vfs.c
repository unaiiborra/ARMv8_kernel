#include <kernel/lib/rbtree.h>
#include <kernel/mm.h>
#include <kernel/vfs.h>
#include <lib/branch.h>
#include <stddef.h>
#include <stdint.h>

#include "lib/lock.h"


typedef struct {
    rb_header_t              __tree_header;
    uint64_t                 fd; // used as key of by the tree
    const file_operations_t* ops;
    void*                    device_data; // ctx passed to ops
} file_descriptor_node_t;

_Static_assert(
    offsetof(file_descriptor_node_t, fd) == 24); // required by rbtree

static file_descriptor_node_t* descriptor_new(
    file_descriptor_t        fd,
    const file_operations_t* ops,
    void*                    device_data)
{
    file_descriptor_node_t* node = kmalloc(sizeof(file_descriptor_node_t));

    *node = (file_descriptor_node_t) {
        // node->__tree_header => initialized by rbt_insert_u64, it is opaque for
        // the caller
        .fd          = fd,
        .ops         = ops,
        .device_data = device_data,
    };

    return node;
}

typedef enum : uint64_t {
    OP_READ  = offsetof(file_operations_t, read) / sizeof(void*),
    OP_WRITE = offsetof(file_operations_t, write) / sizeof(void*),
    OP_OPEN  = offsetof(file_operations_t, open) / sizeof(void*),
    OP_CLOSE = offsetof(file_operations_t, close) / sizeof(void*),
    OP_MMAP  = offsetof(file_operations_t, mmap) / sizeof(void*),
} file_operation_e;

static vfs_result_t validate_descriptor_op(
    fd_table_t*              table,
    file_descriptor_t        fd,
    file_operation_e         op,
    file_descriptor_node_t** descriptor)
{
    if (unlikely(fd < 0 || !table))
        return VFS_ERR_INVAL;

    *descriptor = rbt_find_u64(&table->files, (uint64_t)fd);

    if (unlikely(!*descriptor))
        return VFS_ERR_BADF;

    if (unlikely(!(*descriptor)->ops))
        return VFS_ERR_NODEV;

    const void** operations = (const void**)((*descriptor)->ops);

    if (unlikely(!operations[op]))
        return VFS_ERR_NOSUP;

    return VFS_OK;
}

vfs_result_t vfs_bind(
    fd_table_t*              table,
    file_descriptor_t        fd,
    const file_operations_t* ops,
    void*                    device_data)
{
    if (unlikely(fd < 0))
        return VFS_ERR_INVAL;

    switch (
        rbt_insert_u64(&table->files, descriptor_new(fd, ops, device_data))) {
        case RBT_INSERT_OK:
            return VFS_OK;

        case RBT_INSERT_EXISTS:
            return VFS_ERR_BADF;

        default:
            PANIC();
    }
}

file_descriptor_t vfs_open(fd_table_t* table, const char* path)
{
    (void)table, (void)path;
    PANIC("TODO: implement file system");
}


vfs_result_t vfs_close(fd_table_t* table, file_descriptor_t fd)
{
    file_descriptor_node_t* descriptor;

    vfs_result_t result = validate_descriptor_op(
        table,
        fd,
        OP_CLOSE,
        &descriptor);

    if (unlikely(result != VFS_OK)) {
        if (result != VFS_ERR_NOSUP)
            return result;
    }
    else {
        result = descriptor->ops->close(descriptor->device_data, fd);
        if (result != VFS_OK)
            return result;
    }

    kfree(rbt_remove(&table->files, descriptor));

    return VFS_OK;
}

vfs_result_t vfs_read(
    fd_table_t*       table,
    file_descriptor_t fd,
    uint8_t*          buf,
    uint32_t          count)
{
    if ((int64_t)fd < 0 || fd > INT32_MAX)
        return VFS_ERR_BADF;

    if (count == 0 || count > VFS_MAX_READ_SIZE)
        return VFS_ERR_INVAL;

    file_descriptor_node_t* descriptor;

    vfs_result_t result = validate_descriptor_op(
        table,
        fd,
        OP_READ,
        &descriptor);

    if (unlikely(result != VFS_OK))
        return result;

    return descriptor->ops->read(descriptor->device_data, buf, count);
}

vfs_result_t vfs_write(
    fd_table_t*       table,
    file_descriptor_t fd,
    const uint8_t*    buf,
    uint32_t          count)
{
    if ((int64_t)fd < 0 || fd > INT32_MAX)
        return VFS_ERR_BADF;

    if (count == 0 || count > VFS_MAX_WRITE_SIZE)
        return VFS_ERR_INVAL;

    file_descriptor_node_t* descriptor;

    vfs_result_t result = validate_descriptor_op(
        table,
        fd,
        OP_WRITE,
        &descriptor);

    if (unlikely(result != VFS_OK))
        return result;

    return descriptor->ops->write(descriptor->device_data, buf, count);
}

vfs_result_t vfs_mmap(
    fd_table_t*       table,
    void*             addr,
    uint32_t          len,
    uint32_t          prot,
    uint32_t          flags,
    file_descriptor_t fd,
    uint32_t          offset)
{
    file_descriptor_node_t* descriptor;

    vfs_result_t result = validate_descriptor_op(
        table,
        fd,
        OP_MMAP,
        &descriptor);

    if (unlikely(result != VFS_OK))
        return result;

    return descriptor->ops
        ->mmap(descriptor->device_data, addr, len, prot, flags, offset);
}
