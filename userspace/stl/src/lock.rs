use core::cell::UnsafeCell;
use core::ops::{Deref, DerefMut};
use core::sync::atomic::{AtomicBool, Ordering};

use crate::syscall::syscall_yield;

pub struct Spinlock<T> {
    data: UnsafeCell<T>,
    locked: AtomicBool,
}

pub struct SpinlockGuard<'a, T> {
    guard: &'a Spinlock<T>,
}

unsafe impl<T: Send> Send for Spinlock<T> {}
unsafe impl<T: Send> Sync for Spinlock<T> {}

impl<T> Spinlock<T> {
    pub const fn new(data: T) -> Self {
        Self {
            data: UnsafeCell::new(data),
            locked: AtomicBool::new(false),
        }
    }

    pub fn lock(&'_ self) -> SpinlockGuard<'_, T> {
        loop {
            if self
                .locked
                .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
                .is_ok()
            {
                return SpinlockGuard { guard: self };
            }

            while self.locked.load(Ordering::Relaxed) {
                syscall_yield();
            }
        }
    }

    pub fn try_lock(&'_ self) -> Option<SpinlockGuard<'_, T>> {
        self.locked
            .compare_exchange(false, true, Ordering::Acquire, Ordering::Relaxed)
            .ok()
            .map(|_| SpinlockGuard { guard: self })
    }

    pub fn locked<R, F: FnOnce(&mut T) -> R>(&self, f: F) -> R {
        let mut guard = self.lock();
        f(&mut *guard)
    }
}

impl<T> Deref for SpinlockGuard<'_, T> {
    type Target = T;
    fn deref(&self) -> &T {
        unsafe { &*self.guard.data.get() }
    }
}

impl<T> DerefMut for SpinlockGuard<'_, T> {
    fn deref_mut(&mut self) -> &mut T {
        unsafe { &mut *self.guard.data.get() }
    }
}

impl<T> Drop for SpinlockGuard<'_, T> {
    fn drop(&mut self) {
        self.guard.locked.store(false, Ordering::Release);
    }
}
