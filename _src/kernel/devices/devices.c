#include <kernel/devices/device.h>
#include <lib/string.h>
#include <stdatomic.h>
#include <stddef.h>

#include "kernel/mm.h"
#include "kernel/panic.h"
#include "lib/lock.h"

typedef struct device_node_t {
    device_t              device;
    struct device_node_t *prev, *next;
} device_node_t;


static spinlock_t     locks[DEVICE_CLASS_COUNT];
static device_node_t* device_lists[DEVICE_CLASS_COUNT];
static atomic_ulong   uid_counter;


void device_ctrl_init()
{
    atomic_init(&uid_counter, 1);

    for (size_t i = 0; i < DEVICE_CLASS_COUNT; i++) {
        locks[i]        = SPINLOCK_INIT;
        device_lists[i] = NULL;
    }
}



void device_register(
    const char*    name,
    device_class_t class_id,
    uint8_t        rank,
    puintptr_t     base,
    const void*    driver_ops)
{
    DEBUG_ASSERT(class_id >= 0 && class_id < DEVICE_CLASS_COUNT);

    device_node_t* node = kmalloc(sizeof(device_node_t));

    node->device = (device_t) {
        .uid          = atomic_fetch_add(&uid_counter, 1),
        .name         = name,
        .rank         = rank,
        .class_id     = class_id,
        .base_pa      = base,
        .driver_state = NULL,
        .driver_ops   = driver_ops,
    };
    node->prev = NULL;
    node->next = NULL;


    irq_spinlocked(&locks[class_id])
    {
        device_node_t** head = &device_lists[class_id];

        if (*head == NULL) {
            *head = node;
            return;
        }

        device_node_t* cur = *head;
        while (cur != NULL && cur->device.rank >= node->device.rank) {
            cur = cur->next;
        }

        if (cur == NULL) {
            device_node_t* tail = *head;
            while (tail->next != NULL)
                tail = tail->next;

            tail->next = node;
            node->prev = tail;
        }
        else if (cur->prev == NULL) {
            node->next    = *head;
            (*head)->prev = node;
            *head         = node;
        }
        else {
            device_node_t* prev = cur->prev;
            prev->next          = node;
            node->prev          = prev;
            node->next          = cur;
            cur->prev           = node;
        }
    }
}

const device_t* device_get_by_name(device_class_t class_id, const char* name)
{
    DEBUG_ASSERT(class_id >= 0 && class_id < DEVICE_CLASS_COUNT);

    const device_t* device = NULL;

    irq_spinlocked(&locks[class_id])
    {
        device_node_t* cur = device_lists[class_id];

        while (cur != NULL) {
            if (streq(cur->device.name, name)) {
                device = &cur->device;
                break;
            }

            cur = cur->next;
        }
    }

    return device;
}


const device_t* device_get_primary(device_class_t class_id)
{
    DEBUG_ASSERT(class_id >= 0 && class_id < DEVICE_CLASS_COUNT);

    const device_t* device = NULL;

    irq_spinlocked(&locks[class_id])
    {
        if (device_lists[class_id] == NULL)
            return NULL;

        device = &device_lists[class_id]->device;
    }

    return device;
}