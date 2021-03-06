/*
 * Copyright(c) 2012-2018 Intel Corporation
 * SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#ifndef SOURCE_KERNEL_INTERNAL_CONFIG_H
#define SOURCE_KERNEL_INTERNAL_CONFIG_H

#include <linux/bio.h>
#include <linux/blk_types.h>
#include <linux/blkdev.h>
#include <linux/fcntl.h>
#include <linux/kallsyms.h>
#include <linux/version.h>
#include <trace/events/block.h>

/* ************************************************************************** */
/* Common declarations */
/* ************************************************************************** */

/*
 * BIO completion trace function
 */
typedef void (*iotrace_bio_complete_fn)(void *ignore,
                                        struct request_queue *q,
                                        struct bio *bio,
                                        int error);

#ifndef SECTOR_SHIFT
#define SECTOR_SHIFT 9ULL
#endif
#ifndef SECTOR_SIZE
#define SECTOR_SIZE (1ULL << SECTOR_SHIFT)
#endif

/* BIO operation macros (read/write/discard) */
#define IOTRACE_BIO_IS_WRITE(bio) (bio_data_dir(bio) == WRITE)
/* BIO flags macros (flush, fua, ...) */
#define IOTRACE_BIO_IS_FUA(bio) ((IOTRACE_BIO_OP_FLAGS(bio)) & REQ_FUA)
/* Gets BIO vector  */
#define IOTRACE_BIO_BVEC(vec) (vec)

/* ************************************************************************** */
/* Defines for CentOS 7.6 (3.10 kernel) */
/* ************************************************************************** */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 14, 0)

#define IOTRACE_BIO_OP_FLAGS(bio) (bio)->bi_rw
/* BIO operation macros (read/write/discard) */
#define IOTRACE_BIO_IS_DISCARD(bio) ((IOTRACE_BIO_OP_FLAGS(bio)) & REQ_DISCARD)
/* BIO attributes macros (address, size ...) */
#define IOTRACE_BIO_BISIZE(bio) (bio)->bi_size
#define IOTRACE_BIO_BISECTOR(bio) (bio)->bi_sector
/* BIO flags macros (flush, fua, ...) */
#define IOTRACE_BIO_IS_FLUSH(bio) ((IOTRACE_BIO_OP_FLAGS(bio)) & REQ_FLUSH)

static inline int iotrace_register_trace_block_bio_queue(
        void (*fn)(void *ignore, struct request_queue *, struct bio *)) {
    return register_trace_block_bio_queue(fn, NULL);
}

static inline int iotrace_unregister_trace_block_bio_queue(
        void (*fn)(void *ignore, struct request_queue *, struct bio *)) {
    return unregister_trace_block_bio_queue(fn, NULL);
}

void iotrace_block_rq_complete(void *data,
                               struct request_queue *q,
                               struct request *rq,
                               unsigned int nr_bytes);

static inline int iotrace_register_trace_block_bio_complete(
        iotrace_bio_complete_fn fn) {
    int result;

    result = register_trace_block_bio_complete(fn, NULL);
    WARN_ON(result);
    if (result) {
        goto REG_BIO_COMPLETE_ERROR;
    }

    result = register_trace_block_rq_complete(iotrace_block_rq_complete, fn);
    WARN_ON(result);
    if (result) {
        goto REG_RQ_COMPLETE_ERROR;
    }

    return 0;

REG_RQ_COMPLETE_ERROR:
    unregister_trace_block_bio_complete(fn, NULL);

REG_BIO_COMPLETE_ERROR:
    return result;
}

static inline int iotrace_unregister_trace_block_bio_complete(
        iotrace_bio_complete_fn fn) {
    int result = 0;

    result |= unregister_trace_block_bio_complete(fn, NULL);
    WARN_ON(result);

    result |= unregister_trace_block_rq_complete(iotrace_block_rq_complete, fn);
    WARN_ON(result);

    return result;
}

/* ************************************************************************** */
/* Defines for Ubuntu 18.04 (4.15 kernel) */
/* ************************************************************************** */
#elif LINUX_VERSION_CODE >= KERNEL_VERSION(4, 14, 0)

#define IOTRACE_BIO_OP_FLAGS(bio) (bio)->bi_opf
/* BIO operation macros (read/write/discard) */
#define IOTRACE_BIO_IS_DISCARD(bio) (bio_op(bio) == REQ_OP_DISCARD)
/* BIO attributes macros (address, size ...) */
#define IOTRACE_BIO_BISIZE(bio) (bio)->bi_iter.bi_size
#define IOTRACE_BIO_BISECTOR(bio) (bio)->bi_iter.bi_sector
/* BIO flags macros (flush, fua, ...) */
#define IOTRACE_BIO_IS_FLUSH(bio) ((IOTRACE_BIO_OP_FLAGS(bio)) & REQ_OP_FLUSH)

static inline int iotrace_register_trace_block_bio_queue(
        void (*fn)(void *ignore, struct request_queue *, struct bio *)) {
    char *sym_name = "__tracepoint_block_bio_queue";
    typeof(&__tracepoint_block_bio_queue) tracepoint =
            (void *) kallsyms_lookup_name(sym_name);
    return tracepoint_probe_register((void *) tracepoint, fn, NULL);
}

static inline int iotrace_unregister_trace_block_bio_queue(
        void (*fn)(void *ignore, struct request_queue *, struct bio *)) {
    char *sym_name = "__tracepoint_block_bio_queue";
    typeof(&__tracepoint_block_bio_queue) tracepoint =
            (void *) kallsyms_lookup_name(sym_name);
    return tracepoint_probe_unregister((void *) tracepoint, fn, NULL);
}

void iotrace_block_rq_complete(void *data,
                               struct request *rq,
                               int error,
                               unsigned int nr_bytes);

static inline int iotrace_register_trace_block_bio_complete(
        iotrace_bio_complete_fn fn) {
    int result;
    char *sym_name = "__tracepoint_block_rq_complete";
    typeof(&__tracepoint_block_rq_complete) tracepoint =
            (void *) kallsyms_lookup_name(sym_name);

    result = register_trace_block_bio_complete(fn, NULL);
    WARN_ON(result);
    if (result) {
        goto REG_BIO_COMPLETE_ERROR;
    }

    result = tracepoint_probe_register((void *) tracepoint,
                                       iotrace_block_rq_complete, fn);
    WARN_ON(result);
    if (result) {
        goto REG_RQ_COMPLETE_ERROR;
    }

    return 0;

REG_RQ_COMPLETE_ERROR:
    unregister_trace_block_bio_complete(fn, NULL);

REG_BIO_COMPLETE_ERROR:
    return result;
}

static inline int iotrace_unregister_trace_block_bio_complete(
        iotrace_bio_complete_fn fn) {
    int result = 0;
    char *sym_name = "__tracepoint_block_rq_complete";
    typeof(&__tracepoint_block_rq_complete) tracepoint =
            (void *) kallsyms_lookup_name(sym_name);

    result |= unregister_trace_block_bio_complete(fn, NULL);
    WARN_ON(result);

    result |= tracepoint_probe_unregister((void *) tracepoint,
                                          iotrace_block_rq_complete, fn);
    WARN_ON(result);

    return result;
}

#endif  // Ubuntu 18.04

/* fsnotify macros */
#define FSNOTIFY_FUN(fun_name) "fsnotify_" #fun_name

#if defined(RHEL_MAJOR) && defined(RHEL_MINOR)
#if RHEL_MAJOR == 7 && RHEL_MINOR >= 7
#define IOTRACE_FSNOTIFY_VERSION_5
#endif
#endif

#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 18, 0) && \
        !defined(IOTRACE_FSNOTIFY_VERSION_5)
#define IOTRACE_FSNOTIFY_ADD_MARK(mark, inode) \
    (fsnotify_ops.add_mark(mark, inode, NULL, 0));

#elif LINUX_VERSION_CODE < KERNEL_VERSION(5, 1, 0)
#define IOTRACE_FSNOTIFY_ADD_MARK(mark, inode)             \
    (fsnotify_ops.add_mark(mark, &inode->i_fsnotify_marks, \
                           FSNOTIFY_OBJ_TYPE_INODE, 0));

#else
#define IOTRACE_FSNOTIFY_ADD_MARK(mark, inode)             \
    (fsnotify_ops.add_mark(mark, &inode->i_fsnotify_marks, \
                           FSNOTIFY_OBJ_TYPE_INODE, 0, NULL));

#endif

#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 13, 0)
#define IOTRACE_GET_WRITE_HINT(bio) (bio->bi_write_hint)
#else
#define IOTRACE_GET_WRITE_HINT(bio) (0)
#endif

/* Memory access OK */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(5, 0, 0) || \
        (defined(RHEL_MAJOR) && RHEL_MAJOR >= 8)
#define IOTRACE_ACCESS_OK(type, addr, size) access_ok(addr, size)
#else
#define IOTRACE_ACCESS_OK(type, addr, size) access_ok(type, addr, size)
#endif

/* typedef of page fault result */
#if LINUX_VERSION_CODE >= KERNEL_VERSION(4, 17, 0)
typedef vm_fault_t iotrace_vm_fault_t;
#else
typedef int iotrace_vm_fault_t;
#endif

/* Block device lookup */
#if LINUX_VERSION_CODE < KERNEL_VERSION(4, 15, 0)
#define IOTRACE_LOOKUP_BDEV(path) lookup_bdev(path)
#elif LINUX_VERSION_CODE < KERNEL_VERSION(4, 16, 0)
#define IOTRACE_LOOKUP_BDEV(path) lookup_bdev(path, 0)
#else
#define IOTRACE_LOOKUP_BDEV(path) lookup_bdev(path)
#endif

#endif  // SOURCE_KERNEL_INTERNAL_CONFIG_H
