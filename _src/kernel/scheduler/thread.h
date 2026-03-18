#pragma once

#include <kernel/mm.h>
#include <kernel/panic.h>
#include <kernel/scheduler.h>


void thread_assign_stack(thread* th);