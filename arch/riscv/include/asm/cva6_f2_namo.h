/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * UP software atomics for AWS F2 CVA6: HBM does not complete AMO or LR/SC.
 */
#ifndef _ASM_RISCV_CVA6_F2_NAMO_H
#define _ASM_RISCV_CVA6_F2_NAMO_H

#include <linux/bug.h>
#include <linux/irqflags.h>
#include <linux/types.h>
#include <asm/barrier.h>

#define __atomic_acquire_fence()		barrier()
#define __atomic_release_fence()		barrier()

static __always_inline void __cmpwait_relaxed(volatile void *ptr, unsigned long val)
{
	(void)ptr;
	(void)val;
	barrier();
}

#define __cva6_f2_irq_rmw(flags, stmt)					\
	do {								\
		local_irq_save(flags);					\
		stmt;							\
		local_irq_restore(flags);				\
	} while (0)

#define _arch_xchg(ptr, new, sc_sfx, swap_sfx, prepend, sc_append, swap_append) \
({									\
	__typeof__(ptr) __p = (ptr);					\
	__typeof__(*__p) __n = (new);					\
	__typeof__(*__p) __old;						\
	unsigned long __flags;						\
	__cva6_f2_irq_rmw(__flags, { __old = *__p; *__p = __n; });	\
	__old;								\
})

#define arch_xchg_relaxed(ptr, x)	_arch_xchg(ptr, x, "", "", "", "", "")
#define arch_xchg_acquire(ptr, x)	arch_xchg_relaxed(ptr, x)
#define arch_xchg_release(ptr, x)	arch_xchg_relaxed(ptr, x)
#define arch_xchg(ptr, x)		arch_xchg_relaxed(ptr, x)

#define xchg32(ptr, x) ({ BUILD_BUG_ON(sizeof(*(ptr)) != 4); arch_xchg(ptr, x); })
#define xchg64(ptr, x) ({ BUILD_BUG_ON(sizeof(*(ptr)) != 8); arch_xchg(ptr, x); })

#define _arch_cmpxchg(ptr, old, new, sc_sfx, prepend, append)		\
({									\
	__typeof__(ptr) __p = (ptr);					\
	__typeof__(*__p) __o = (old);					\
	__typeof__(*__p) __n = (new);					\
	__typeof__(*__p) __cur;						\
	unsigned long __flags;						\
	__cva6_f2_irq_rmw(__flags, {					\
		__cur = *__p;						\
		if (__cur == __o)					\
			*__p = __n;					\
	});								\
	__cur;								\
})

#define arch_cmpxchg_relaxed(ptr, o, n)	_arch_cmpxchg(ptr, o, n, "", "", "")
#define arch_cmpxchg_acquire(ptr, o, n)	arch_cmpxchg_relaxed(ptr, o, n)
#define arch_cmpxchg_release(ptr, o, n)	arch_cmpxchg_relaxed(ptr, o, n)
#define arch_cmpxchg(ptr, o, n)		arch_cmpxchg_relaxed(ptr, o, n)
#define arch_cmpxchg_local(ptr, o, n)	arch_cmpxchg_relaxed(ptr, o, n)
#define arch_cmpxchg64(ptr, o, n) ({ BUILD_BUG_ON(sizeof(*(ptr)) != 8); arch_cmpxchg(ptr, o, n); })
#define arch_cmpxchg64_local(ptr, o, n) arch_cmpxchg64(ptr, o, n)
#define arch_cmpxchg64_relaxed(ptr, o, n) arch_cmpxchg64(ptr, o, n)
#define arch_cmpxchg64_acquire(ptr, o, n) arch_cmpxchg64(ptr, o, n)
#define arch_cmpxchg64_release(ptr, o, n) arch_cmpxchg64(ptr, o, n)

static __always_inline int arch_atomic_read(const atomic_t *v)
{
	return READ_ONCE(v->counter);
}

static __always_inline void arch_atomic_set(atomic_t *v, int i)
{
	WRITE_ONCE(v->counter, i);
}

#define ATOMIC_OP(op, c_op, i)						\
static __always_inline void arch_atomic_##op(int i, atomic_t *v)		\
{										\
	unsigned long flags;							\
	__cva6_f2_irq_rmw(flags, { v->counter c_op i; });			\
}

ATOMIC_OP(add, +=, i)
ATOMIC_OP(sub, -=, i)
ATOMIC_OP(and, &=, i)
ATOMIC_OP(or, |=, i)
ATOMIC_OP(xor, ^=, i)

#define ATOMIC_FETCH_OP(op, c_op, i)						\
static __always_inline int arch_atomic_fetch_##op##_relaxed(int i, atomic_t *v) \
{										\
	int old; unsigned long flags;						\
	__cva6_f2_irq_rmw(flags, { old = v->counter; v->counter = old c_op i; }); \
	return old;								\
}										\
static __always_inline int arch_atomic_fetch_##op(int i, atomic_t *v)		\
{										\
	return arch_atomic_fetch_##op##_relaxed(i, v);				\
}										\
static __always_inline int arch_atomic_##op##_return_relaxed(int i, atomic_t *v) \
{										\
	int old = arch_atomic_fetch_##op##_relaxed(i, v);			\
	return old c_op i;							\
}										\
static __always_inline int arch_atomic_##op##_return(int i, atomic_t *v)	\
{										\
	return arch_atomic_##op##_return_relaxed(i, v);				\
}

ATOMIC_FETCH_OP(add, +, i)

static __always_inline int arch_atomic_fetch_sub_relaxed(int i, atomic_t *v)
{
	int old;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, { old = v->counter; v->counter -= i; });
	return old;
}

static __always_inline int arch_atomic_fetch_sub(int i, atomic_t *v)
{
	return arch_atomic_fetch_sub_relaxed(i, v);
}

static __always_inline int arch_atomic_sub_return_relaxed(int i, atomic_t *v)
{
	return arch_atomic_fetch_sub_relaxed(i, v) - i;
}

static __always_inline int arch_atomic_sub_return(int i, atomic_t *v)
{
	return arch_atomic_sub_return_relaxed(i, v);
}

#define arch_atomic_add_return_relaxed arch_atomic_add_return_relaxed
#define arch_atomic_sub_return_relaxed arch_atomic_sub_return_relaxed
#define arch_atomic_add_return arch_atomic_add_return
#define arch_atomic_sub_return arch_atomic_sub_return
#define arch_atomic_fetch_add_relaxed arch_atomic_fetch_add_relaxed
#define arch_atomic_fetch_sub_relaxed arch_atomic_fetch_sub_relaxed
#define arch_atomic_fetch_add arch_atomic_fetch_add
#define arch_atomic_fetch_sub arch_atomic_fetch_sub

ATOMIC_FETCH_OP(and, &, i)
ATOMIC_FETCH_OP(or, |, i)
ATOMIC_FETCH_OP(xor, ^, i)

#define arch_atomic_fetch_and_relaxed arch_atomic_fetch_and_relaxed
#define arch_atomic_fetch_or_relaxed arch_atomic_fetch_or_relaxed
#define arch_atomic_fetch_xor_relaxed arch_atomic_fetch_xor_relaxed
#define arch_atomic_fetch_and arch_atomic_fetch_and
#define arch_atomic_fetch_or arch_atomic_fetch_or
#define arch_atomic_fetch_xor arch_atomic_fetch_xor

static __always_inline int arch_atomic_fetch_add_unless(atomic_t *v, int a, int u)
{
	int prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev != u)
			v->counter = prev + a;
	});
	return prev;
}

static __always_inline bool arch_atomic_inc_unless_negative(atomic_t *v)
{
	int prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev >= 0)
			v->counter = prev + 1;
	});
	return prev >= 0;
}

static __always_inline bool arch_atomic_dec_unless_positive(atomic_t *v)
{
	int prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev <= 0)
			;
		else
			v->counter = prev - 1;
	});
	return prev <= 0;
}

static __always_inline int arch_atomic_dec_if_positive(atomic_t *v)
{
	int prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev > 0)
			v->counter = prev - 1;
	});
	return prev > 0 ? prev - 1 : prev;
}

#define ATOMIC64_INIT(i) { (i) }

static __always_inline s64 arch_atomic64_read(const atomic64_t *v)
{
	return READ_ONCE(v->counter);
}

static __always_inline void arch_atomic64_set(atomic64_t *v, s64 i)
{
	WRITE_ONCE(v->counter, i);
}

#define ATOMIC64_OP(op, c_op, i)						\
static __always_inline void arch_atomic64_##op(s64 i, atomic64_t *v)		\
{										\
	unsigned long flags;							\
	__cva6_f2_irq_rmw(flags, { v->counter c_op i; });			\
}

ATOMIC64_OP(add, +=, i)
ATOMIC64_OP(sub, -=, i)
ATOMIC64_OP(and, &=, i)
ATOMIC64_OP(or, |=, i)
ATOMIC64_OP(xor, ^=, i)

#define ATOMIC64_FETCH_OP(op, c_op, i)						\
static __always_inline s64 arch_atomic64_fetch_##op##_relaxed(s64 i, atomic64_t *v) \
{										\
	s64 old; unsigned long flags;						\
	__cva6_f2_irq_rmw(flags, { old = v->counter; v->counter = old c_op i; }); \
	return old;								\
}										\
static __always_inline s64 arch_atomic64_fetch_##op(s64 i, atomic64_t *v)	\
{										\
	return arch_atomic64_fetch_##op##_relaxed(i, v);			\
}										\
static __always_inline s64 arch_atomic64_##op##_return_relaxed(s64 i, atomic64_t *v) \
{										\
	s64 old = arch_atomic64_fetch_##op##_relaxed(i, v);			\
	return old c_op i;							\
}										\
static __always_inline s64 arch_atomic64_##op##_return(s64 i, atomic64_t *v)	\
{										\
	return arch_atomic64_##op##_return_relaxed(i, v);			\
}

ATOMIC64_FETCH_OP(add, +, i)

static __always_inline s64 arch_atomic64_fetch_sub_relaxed(s64 i, atomic64_t *v)
{
	s64 old;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, { old = v->counter; v->counter -= i; });
	return old;
}

static __always_inline s64 arch_atomic64_fetch_sub(s64 i, atomic64_t *v)
{
	return arch_atomic64_fetch_sub_relaxed(i, v);
}

static __always_inline s64 arch_atomic64_sub_return_relaxed(s64 i, atomic64_t *v)
{
	return arch_atomic64_fetch_sub_relaxed(i, v) - i;
}

static __always_inline s64 arch_atomic64_sub_return(s64 i, atomic64_t *v)
{
	return arch_atomic64_sub_return_relaxed(i, v);
}

#define arch_atomic64_add_return_relaxed arch_atomic64_add_return_relaxed
#define arch_atomic64_sub_return_relaxed arch_atomic64_sub_return_relaxed
#define arch_atomic64_add_return arch_atomic64_add_return
#define arch_atomic64_sub_return arch_atomic64_sub_return
#define arch_atomic64_fetch_add_relaxed arch_atomic64_fetch_add_relaxed
#define arch_atomic64_fetch_sub_relaxed arch_atomic64_fetch_sub_relaxed
#define arch_atomic64_fetch_add arch_atomic64_fetch_add
#define arch_atomic64_fetch_sub arch_atomic64_fetch_sub

ATOMIC64_FETCH_OP(and, &, i)
ATOMIC64_FETCH_OP(or, |, i)
ATOMIC64_FETCH_OP(xor, ^, i)

#define arch_atomic64_fetch_and_relaxed arch_atomic64_fetch_and_relaxed
#define arch_atomic64_fetch_or_relaxed arch_atomic64_fetch_or_relaxed
#define arch_atomic64_fetch_xor_relaxed arch_atomic64_fetch_xor_relaxed
#define arch_atomic64_fetch_and arch_atomic64_fetch_and
#define arch_atomic64_fetch_or arch_atomic64_fetch_or
#define arch_atomic64_fetch_xor arch_atomic64_fetch_xor

static __always_inline s64 arch_atomic64_fetch_add_unless(atomic64_t *v, s64 a, s64 u)
{
	s64 prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev != u)
			v->counter = prev + a;
	});
	return prev;
}

static __always_inline bool arch_atomic64_inc_unless_negative(atomic64_t *v)
{
	s64 prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev >= 0)
			v->counter = prev + 1;
	});
	return prev >= 0;
}

static __always_inline bool arch_atomic64_dec_unless_positive(atomic64_t *v)
{
	s64 prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev > 0)
			v->counter = prev - 1;
	});
	return prev <= 0;
}

static __always_inline s64 arch_atomic64_dec_if_positive(atomic64_t *v)
{
	s64 prev;
	unsigned long flags;

	__cva6_f2_irq_rmw(flags, {
		prev = v->counter;
		if (prev > 0)
			v->counter = prev - 1;
	});
	return prev > 0 ? prev - 1 : prev;
}

#endif /* _ASM_RISCV_CVA6_F2_NAMO_H */
