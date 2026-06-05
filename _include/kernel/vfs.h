#pragma once

#include <kernel/lib/rbtree.h>
#include <stdint.h>

typedef int32_t file_descriptor_t;

typedef enum : file_descriptor_t {
    VFS_OK        = 0,
    VFS_ERR_BADF  = -1, // Invalid fd
    VFS_ERR_NODEV = -2, // Device not registered
    VFS_ERR_NOSUP = -3, // Op not supported
    VFS_ERR_INVAL = -4, // Invalid argument
} vfs_result_t;

typedef struct {
    vfs_result_t (*read)(void* ctx, uint8_t* buf, uint32_t len);
    vfs_result_t (*write)(void* ctx, const uint8_t* buf, uint32_t len);
    vfs_result_t (*open)(void* ctx);
    vfs_result_t (*close)(void* ctx);
    vfs_result_t (*mmap)(
        void*    ctx,
        void*    addr,
        uint32_t len,
        uint32_t prot,
        uint32_t flags,
        uint32_t offset);
} file_operations_t;

typedef struct {
    rbtree_t files;
} fd_table_t;

static constexpr fd_table_t FD_TABLE_INIT = {.files = (void*)0};

/// used by the kernel to select an specific file descriptor number and assign
/// it to a table.
vfs_result_t vfs_bind(
    fd_table_t*              table,
    file_descriptor_t        fd,
    const file_operations_t* ops,
    void*                    device_data);

file_descriptor_t vfs_open(fd_table_t* table, const char* path);

vfs_result_t vfs_close(fd_table_t* table, file_descriptor_t fd);

vfs_result_t vfs_read(
    fd_table_t*       table,
    file_descriptor_t fd,
    uint8_t*          buf,
    uint32_t          len);

vfs_result_t vfs_write(
    fd_table_t*       table,
    file_descriptor_t fd,
    const uint8_t*    buf,
    uint32_t          len);

vfs_result_t vfs_mmap(
    fd_table_t*       table,
    void*             addr,
    uint32_t          len,
    uint32_t          prot,
    uint32_t          flags,
    file_descriptor_t fd,
    uint32_t          offset);
