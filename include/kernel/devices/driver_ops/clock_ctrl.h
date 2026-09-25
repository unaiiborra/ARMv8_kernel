#pragma once

#include <stdint.h>

#include "../device.h"

// DEVICE_CLASS_CLOCK_CTRL

// A clock is identified by the controller that provides it plus a local id
typedef struct {
	const device_t *const dev;
	uint32_t id;
} clk_ref_t;

#define CLK_REF_NONE ((clk_ref_t){.dev = NULL, .id = 0})

typedef struct {
	int32_t (*init)(driver_handle_t handle);
	int32_t (*exit)(driver_handle_t handle);

	// Number of clocks exposed by this controller (ids are 0..count-1)
	uint32_t (*get_count)(driver_handle_t handle);
	const char *(*get_name)(driver_handle_t handle, uint32_t id);

	// Current parent, CLK_REF_NONE for root
	clk_ref_t (*get_parent)(driver_handle_t handle, uint32_t id);
	// NULL if the clock has a fixed parent
	int32_t (*set_parent)(driver_handle_t handle, uint32_t id, clk_ref_t parent);

	// Only touch hardware, no refcounting (the subsystem handles it)
	// NULL if the clock cannot be gated
	int32_t (*enable)(driver_handle_t handle, uint32_t id);
	int32_t (*disable)(driver_handle_t handle, uint32_t id);

	// 1 enabled, 0 disabled, < 0 error
	int32_t (*is_enabled)(driver_handle_t handle, uint32_t id);

	// Rate of this clock given its parent's rate
	uint64_t (*recalc_rate)(driver_handle_t handle, uint32_t id, uint64_t parent_hz);

	// Closest achievable rate. Does not apply it
	uint64_t (*round_rate)(
		driver_handle_t handle,
		uint32_t id,
		uint64_t hz,
		uint64_t parent_hz
	);
	int32_t (*set_rate)(driver_handle_t handle, uint32_t id, uint64_t hz, uint64_t parent_hz);
} clock_ctrl_ops_t;

#define get_clock_ctrl_ops(dev)                                                                    \
	_Generic((dev), const device_t *: (const clock_ctrl_ops_t *)((dev)->driver_ops))
