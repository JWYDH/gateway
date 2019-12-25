#define atomic_memory_barrier() __sync_synchronize()

#define atomic_sync_fetch_and_add(p, v) __sync_fetch_and_add(p, v)
#define atomic_sync_fetch_and_sub(p, v) __sync_fetch_and_sub(p, v)
#define atomic_sync_fetch_and_and(p, v) __sync_fetch_and_and(p, v)
#define atomic_sync_fetch_and_or(p, v) __sync_fetch_and_or(p, v)
#define atomic_sync_fetch_and_nand(p, v) __sync_fetch_and_nand(p, v)
#define atomic_sync_fetch_and_xor(p, v) __sync_fetch_and_xor(p, v)

#define atomic_sync_add_and_fetch(p, v) __sync_add_and_fetch(p, v)
#define atomic_sync_sub_and_fetch(p, v) __sync_sub_and_fetch(p, v)
#define atomic_sync_and_and_fetch(p, v) __sync_and_and_fetch(p, v)
#define atomic_sync_or_and_fetch(p, v) __sync_or_and_fetch(p, v)
#define atomic_sync_nand_and_fetch(p, v) __sync_nand_and_fetch(p, v)
#define atomic_sync_and_xor_fetch(p, v) __sync_and_xor_fetch(p, v)

#define atomic_sync_bool_compare_and_swap(ptr, old_val, new_val) __sync_bool_compare_and_swap(ptr, old_val, new_val)


#define atomic_sync_lock_test_and_set(p, v) __sync_lock_test_and_set(p, v)
#define atomic___sync_lock_release(p) __sync_lock_test_and_set(p)