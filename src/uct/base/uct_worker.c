/**
 * Copyright (C) Mellanox Technologies Ltd. 2001-2017.  ALL RIGHTS RESERVED.
 * Copyright (C) UT-Battelle, LLC. 2017. ALL RIGHTS RESERVED.
 * Copyright (C) ARM Ltd. 2016-2017. ALL RIGHTS RESERVED.
 *
 * See file LICENSE for terms.
 */

#include "uct_worker.h"

#include <ucs/type/class.h>
#include <ucs/async/async.h>


static UCS_CLASS_INIT_FUNC(uct_worker_t, ucs_async_context_t *async,
                           ucs_thread_mode_t thread_mode)
{
    self->async       = async;
    self->thread_mode = thread_mode;
    ucs_callbackq_init(&self->progress_q);
    ucs_list_head_init(&self->tl_data);
    return UCS_OK;
}

static UCS_CLASS_CLEANUP_FUNC(uct_worker_t)
{
    ucs_callbackq_cleanup(&self->progress_q);
}

UCS_CLASS_DEFINE(uct_worker_t, void);
UCS_CLASS_DEFINE_NAMED_NEW_FUNC(uct_worker_create, uct_worker_t, uct_worker_t,
                                ucs_async_context_t*, ucs_thread_mode_t)
UCS_CLASS_DEFINE_NAMED_DELETE_FUNC(uct_worker_destroy, uct_worker_t, uct_worker_t)

void uct_worker_progress(uct_worker_h worker)
{
    ucs_callbackq_dispatch(&worker->progress_q);
}

void uct_worker_progress_init(uct_worker_progress_t *prog)
{
    prog->id       = UCS_CALLBACKQ_ID_NULL;
    prog->refcount = 0;
}

void uct_worker_progress_register(uct_worker_h worker, ucs_callback_t cb,
                                  void *arg, uct_worker_progress_t *prog)
{
    UCS_ASYNC_BLOCK(worker->async);
    if (prog->refcount++ == 0) {
        prog->id = ucs_callbackq_add(&worker->progress_q, cb, arg,
                                     UCS_CALLBACKQ_FLAG_FAST);
    }
    UCS_ASYNC_UNBLOCK(worker->async);
}

void uct_worker_progress_unregister(uct_worker_h worker,
                                    uct_worker_progress_t *prog)
{
    UCS_ASYNC_BLOCK(worker->async);
    ucs_assert(prog->refcount > 0);
    if (--prog->refcount == 0) {
        ucs_callbackq_remove(&worker->progress_q, prog->id);
        prog->id = UCS_CALLBACKQ_ID_NULL;
    }
    UCS_ASYNC_UNBLOCK(worker->async);
}

void uct_worker_progress_register_safe(uct_worker_h worker, ucs_callback_t func,
                                       void *arg, unsigned flags,
                                       uct_worker_cb_id_t *id_p)
{
    if (*id_p == UCS_CALLBACKQ_ID_NULL) {
        UCS_ASYNC_BLOCK(worker->async);
        *id_p = ucs_callbackq_add_safe(&worker->progress_q, func, arg, flags);
        UCS_ASYNC_UNBLOCK(worker->async);
        ucs_assert(*id_p != UCS_CALLBACKQ_ID_NULL);
    }
}

void uct_worker_progress_unregister_safe(uct_worker_h worker,
                                         uct_worker_cb_id_t *id_p)
{
    if (*id_p != UCS_CALLBACKQ_ID_NULL) {
        UCS_ASYNC_BLOCK(worker->async);
        ucs_callbackq_remove_safe(&worker->progress_q, *id_p);
        UCS_ASYNC_UNBLOCK(worker->async);
        *id_p = UCS_CALLBACKQ_ID_NULL;
    }
}