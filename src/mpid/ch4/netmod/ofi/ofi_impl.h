/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil ; -*- */
/*
 *  (C) 2006 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 *
 *  Portions of this code were written by Intel Corporation.
 *  Copyright (C) 2011-2016 Intel Corporation.  Intel provides this material
 *  to Argonne National Laboratory subject to Software Grant and Corporate
 *  Contributor License Agreement dated February 8, 2012.
 */
#ifndef OFI_IMPL_H_INCLUDED
#define OFI_IMPL_H_INCLUDED

#include <mpidimpl.h>
#include "ofi_types.h"
#include "mpidch4r.h"
#include "mpidig.h"
#include "ch4_impl.h"
#include "ofi_iovec_util.h"

#define MPIDI_OFI_DT(dt)         ((dt)->dev.netmod.ofi)
#define MPIDI_OFI_OP(op)         ((op)->dev.netmod.ofi)
#define MPIDI_OFI_COMM(comm)     ((comm)->dev.ch4.netmod.ofi)
#define MPIDI_OFI_COMM_TO_INDEX(comm,rank) \
    MPIDIU_comm_rank_to_pid(comm, rank, NULL, NULL)
#define MPIDI_OFI_AV_TO_PHYS(av) (MPIDI_OFI_AV(av).dest)
#define MPIDI_OFI_COMM_TO_PHYS(comm,rank)                       \
    MPIDI_OFI_AV(MPIDIU_comm_rank_to_av((comm), (rank))).dest
#define MPIDI_OFI_TO_PHYS(avtid, lpid)                                 \
    MPIDI_OFI_AV(&MPIDIU_get_av((avtid), (lpid))).dest

#define MPIDI_OFI_WIN(win)     ((win)->dev.netmod.ofi)
/*
 * Helper routines and macros for request completion
 */
#define MPIDI_OFI_ssendack_request_t_tls_alloc(req)             \
    do {                                                                \
        (req) = (MPIDI_OFI_ssendack_request_t*)                 \
            MPIR_Request_create(MPIR_REQUEST_KIND__SEND);               \
        if (req == NULL)                                                \
            MPID_Abort(NULL, MPI_ERR_NO_SPACE, -1,                      \
                       "Cannot allocate Ssendack Request");             \
    } while (0)

#define MPIDI_OFI_ssendack_request_t_tls_free(req) \
  MPIR_Handle_obj_free(&MPIR_Request_mem, (req))

#define MPIDI_OFI_ssendack_request_t_alloc_and_init(req)        \
    do {                                                                \
        MPIDI_OFI_ssendack_request_t_tls_alloc(req);            \
        MPIR_Assert(req != NULL);                                       \
        MPIR_Assert(HANDLE_GET_MPI_KIND(req->handle)                    \
                    == MPID_SSENDACK_REQUEST);                          \
    } while (0)

#define MPIDI_OFI_request_create_null_rreq(rreq_, mpi_errno_, FAIL_) \
  do {                                                                  \
    (rreq_) = MPIR_Request_create(MPIR_REQUEST_KIND__RECV);             \
    if ((rreq_) != NULL) {                                              \
      MPIR_cc_set(&(rreq_)->cc, 0);                                     \
      (rreq_)->kind = MPIR_REQUEST_KIND__RECV;                                \
      MPIR_Status_set_procnull(&(rreq_)->status);                       \
    }                                                                   \
    else {                                                              \
      MPIR_ERR_SETANDJUMP(mpi_errno_,MPI_ERR_OTHER,"**nomemreq");       \
    }                                                                   \
  } while (0)


#define MPIDI_OFI_PROGRESS()                                      \
    do {                                                          \
        mpi_errno = MPID_Progress_test();                        \
        if (mpi_errno!=MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);      \
    } while (0)

#define MPIDI_OFI_PROGRESS_NONINLINE()                            \
    do {                                                          \
        mpi_errno = MPIDI_OFI_progress_test_no_inline();          \
        if (mpi_errno!=MPI_SUCCESS) MPIR_ERR_POP(mpi_errno);      \
    } while (0)

#define MPIDI_OFI_PROGRESS_WHILE(cond)                 \
    while (cond) MPIDI_OFI_PROGRESS()

#define MPIDI_OFI_ERR  MPIR_ERR_CHKANDJUMP4
#define MPIDI_OFI_CALL(FUNC,STR)                                     \
    do {                                                    \
        MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);   \
        ssize_t _ret = FUNC;                                \
        MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
        MPIDI_OFI_ERR(_ret<0,                       \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              FCNAME,                       \
                              fi_strerror(-_ret));          \
    } while (0)

#define MPIDI_OFI_CALL_NOLOCK(FUNC,STR)                              \
    do {                                                    \
        ssize_t _ret = FUNC;                                \
        MPIDI_OFI_ERR(_ret<0,                       \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              FCNAME,                       \
                              fi_strerror(-_ret));          \
    } while (0)

#define MPIDI_OFI_CALL_LOCK 1
#define MPIDI_OFI_CALL_NO_LOCK 0
#define MPIDI_OFI_CALL_RETRY(FUNC,STR,LOCK)                               \
    do {                                                    \
    ssize_t _ret;                                           \
    do {                                                    \
        if (LOCK == MPIDI_OFI_CALL_LOCK)                    \
            MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);   \
        _ret = FUNC;                                        \
        if (LOCK == MPIDI_OFI_CALL_LOCK)                    \
            MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
        if (likely(_ret==0)) break;                          \
        MPIDI_OFI_ERR(_ret!=-FI_EAGAIN,             \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              FCNAME,                       \
                              fi_strerror(-_ret));          \
        if (LOCK == MPIDI_OFI_CALL_NO_LOCK)                 \
            MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);     \
        MPIDI_OFI_PROGRESS_NONINLINE();                              \
        if (LOCK == MPIDI_OFI_CALL_NO_LOCK)                 \
            MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

#define MPIDI_OFI_CALL_RETRY2(FUNC1,FUNC2,STR)                       \
    do {                                                    \
    ssize_t _ret;                                           \
    MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);       \
    FUNC1;                                                  \
    do {                                                    \
        _ret = FUNC2;                                       \
        MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);    \
        if (likely(_ret==0)) break;                          \
        MPIDI_OFI_ERR(_ret!=-FI_EAGAIN,             \
                              mpi_errno,                    \
                              MPI_ERR_OTHER,                \
                              "**ofid_"#STR,                \
                              "**ofid_"#STR" %s %d %s %s",  \
                              __SHORT_FILE__,               \
                              __LINE__,                     \
                              FCNAME,                       \
                              fi_strerror(-_ret));          \
        MPIDI_OFI_PROGRESS_NONINLINE();                         \
        MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);   \
    } while (_ret == -FI_EAGAIN);                           \
    } while (0)

#define MPIDI_OFI_CALL_RETURN(FUNC, _ret)                               \
        do {                                                            \
            MPID_THREAD_CS_ENTER(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);       \
            (_ret) = FUNC;                                              \
            MPID_THREAD_CS_EXIT(POBJ,MPIDI_OFI_THREAD_FI_MUTEX);        \
        } while (0)

#define MPIDI_OFI_PMI_CALL_POP(FUNC,STR)                    \
  do                                                          \
    {                                                         \
      pmi_errno  = FUNC;                                      \
      MPIDI_OFI_ERR(pmi_errno!=PMI_SUCCESS,           \
                            mpi_errno,                        \
                            MPI_ERR_OTHER,                    \
                            "**ofid_"#STR,                    \
                            "**ofid_"#STR" %s %d %s %s",      \
                            __SHORT_FILE__,                   \
                            __LINE__,                         \
                            FCNAME,                           \
                            #STR);                            \
    } while (0)

#define MPIDI_OFI_MPI_CALL_POP(FUNC)                               \
  do                                                                 \
    {                                                                \
      mpi_errno = FUNC;                                              \
      if (unlikely(mpi_errno!=MPI_SUCCESS)) MPIR_ERR_POP(mpi_errno); \
    } while (0)

#define MPIDI_OFI_STR_CALL(FUNC,STR)                                   \
  do                                                            \
    {                                                           \
      str_errno = FUNC;                                         \
      MPIDI_OFI_ERR(str_errno!=MPL_STR_SUCCESS,        \
                            mpi_errno,                          \
                            MPI_ERR_OTHER,                      \
                            "**"#STR,                           \
                            "**"#STR" %s %d %s %s",             \
                            __SHORT_FILE__,                     \
                            __LINE__,                           \
                            FCNAME,                             \
                            #STR);                              \
    } while (0)

#define MPIDI_OFI_REQUEST_CREATE(req, kind)                 \
    do {                                                      \
        (req) = MPIR_Request_create(kind);  \
        MPIR_Request_add_ref((req));                                \
    } while (0)

#ifndef HAVE_DEBUGGER_SUPPORT
#define MPIDI_OFI_SEND_REQUEST_CREATE_LW(req)                   \
    do {                                                                \
        (req) = MPIDI_Global.lw_send_req;                               \
        MPIR_Request_add_ref((req));                                    \
    } while (0)
#else
#define MPIDI_OFI_SEND_REQUEST_CREATE_LW(req)                   \
    do {                                                                \
        (req) = MPIR_Request_create(MPIR_REQUEST_KIND__SEND);           \
        MPIR_cc_set(&(req)->cc, 0);                                     \
    } while (0)
#endif

#define MPIDI_OFI_SSEND_ACKREQUEST_CREATE(req)            \
    do {                                                          \
        MPIDI_OFI_ssendack_request_t_tls_alloc(req);      \
    } while (0)

#define WINFO(w,rank) MPIDI_CH4U_WINFO(w,rank)

MPL_STATIC_INLINE_PREFIX uintptr_t MPIDI_OFI_winfo_base(MPIR_Win * w, int rank)
{
    if (MPIDI_OFI_ENABLE_MR_SCALABLE)
        return 0;
    else
        return MPIDI_OFI_WIN(w).winfo[rank].base;
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_winfo_mr_key(MPIR_Win * w, int rank)
{
    if (MPIDI_OFI_ENABLE_MR_SCALABLE)
        return MPIDI_OFI_WIN(w).mr_key;
    else
        return MPIDI_OFI_WIN(w).winfo[rank].mr_key;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_cntr_incr(MPIR_Win * win)
{
    (*MPIDI_OFI_WIN(win).issued_cntr)++;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_cntr_incr()
{
    MPIDI_Global.rma_issued_cntr++;
}

/* Externs:  see util.c for definition */
int MPIDI_OFI_handle_cq_error_util(int ep_idx, ssize_t ret);
int MPIDI_OFI_progress_test_no_inline(void);
int MPIDI_OFI_control_handler(int handler_id, void *am_hdr,
                              void **data, size_t * data_sz, int *is_contig,
                              MPIDIG_am_target_cmpl_cb * target_cmpl_cb, MPIR_Request ** req);
void MPIDI_OFI_map_create(void **map);
void MPIDI_OFI_map_destroy(void *map);
void MPIDI_OFI_map_set(void *_map, uint64_t id, void *val);
void MPIDI_OFI_map_erase(void *_map, uint64_t id);
void *MPIDI_OFI_map_lookup(void *_map, uint64_t id);
int MPIDI_OFI_control_dispatch(void *buf);
void MPIDI_OFI_index_datatypes(void);
void MPIDI_OFI_index_allocator_create(void **_indexmap, int start);
int MPIDI_OFI_index_allocator_alloc(void *_indexmap);
void MPIDI_OFI_index_allocator_free(void *_indexmap, int index);
void MPIDI_OFI_index_allocator_destroy(void *_indexmap);

/* Common Utility functions used by the
 * C and C++ components
 */
MPL_STATIC_INLINE_PREFIX MPIDI_OFI_win_request_t *MPIDI_OFI_win_request_alloc_and_init(int extra)
{
    MPIDI_OFI_win_request_t *req;
    req = (MPIDI_OFI_win_request_t *) MPIR_Request_create(MPIR_REQUEST_KIND__RMA);
    memset((char *) req + MPIDI_REQUEST_HDR_SIZE, 0,
           sizeof(MPIDI_OFI_win_request_t) - MPIDI_REQUEST_HDR_SIZE);
    req->noncontig =
        (MPIDI_OFI_win_noncontig_t *) MPL_calloc(1, (extra) + sizeof(*(req->noncontig)));
    return req;
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_datatype_unmap(MPIDI_OFI_win_datatype_t * dt)
{
    if (dt && dt->map && (dt->map != &dt->__map))
        MPL_free(dt->map);
}

MPL_STATIC_INLINE_PREFIX void MPIDI_OFI_win_request_complete(MPIDI_OFI_win_request_t * req)
{
    int in_use;
    MPIR_Assert(HANDLE_GET_MPI_KIND(req->handle) == MPIR_REQUEST);
    MPIR_Object_release_ref(req, &in_use);
    MPIR_Assert(in_use >= 0);
    if (!in_use) {
        MPIDI_OFI_win_datatype_unmap(&req->noncontig->target_dt);
        MPIDI_OFI_win_datatype_unmap(&req->noncontig->origin_dt);
        MPIDI_OFI_win_datatype_unmap(&req->noncontig->result_dt);
        MPL_free(req->noncontig);
        MPIR_Handle_obj_free(&MPIR_Request_mem, (req));
    }
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_comm_to_phys(MPIR_Comm * comm, int rank)
{
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        MPIDI_OFI_addr_t *av = &MPIDI_OFI_AV(MPIDIU_comm_rank_to_av(comm, rank));
        int ep_num = MPIDI_OFI_av_to_ep(av);
        int rx_idx = ep_num;
        return fi_rx_addr(av->dest, rx_idx, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_COMM_TO_PHYS(comm, rank);
    }
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_av_to_phys(MPIDI_av_entry_t *av)
{
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        int ep_num = MPIDI_OFI_av_to_ep(&MPIDI_OFI_AV(av));
        return fi_rx_addr(MPIDI_OFI_AV_TO_PHYS(av), ep_num, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    }
    else {
        return MPIDI_OFI_AV_TO_PHYS(av);
    }
}

MPL_STATIC_INLINE_PREFIX fi_addr_t MPIDI_OFI_to_phys(int rank)
{
    if (MPIDI_OFI_ENABLE_SCALABLE_ENDPOINTS) {
        int ep_num = 0;
        int rx_idx = ep_num;
        return fi_rx_addr(MPIDI_OFI_TO_PHYS(0, rank), rx_idx, MPIDI_OFI_MAX_ENDPOINTS_BITS);
    } else {
        return MPIDI_OFI_TO_PHYS(0, rank);
    }
}

MPL_STATIC_INLINE_PREFIX bool MPIDI_OFI_is_tag_sync(uint64_t match_bits)
{
    return (0 != (MPIDI_OFI_SYNC_SEND & match_bits));
}

MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_init_sendtag(MPIR_Context_id_t contextid,
                                                  int source, int tag, uint64_t type)
{
    uint64_t match_bits;
    match_bits = contextid;

    if (!MPIDI_OFI_ENABLE_DATA) {
        match_bits = (match_bits << MPIDI_OFI_SOURCE_BITS);
        match_bits |= source;
    }

    match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
    match_bits |= (MPIDI_OFI_TAG_MASK & tag) | type;
    return match_bits;
}

/* receive posting */
MPL_STATIC_INLINE_PREFIX uint64_t MPIDI_OFI_init_recvtag(uint64_t * mask_bits,
                                                  MPIR_Context_id_t contextid,
                                                  int source, int tag)
{
    uint64_t match_bits = 0;
    *mask_bits = MPIDI_OFI_PROTOCOL_MASK;
    match_bits = contextid;

    if (!MPIDI_OFI_ENABLE_DATA) {
        match_bits = (match_bits << MPIDI_OFI_SOURCE_BITS);

        if (MPI_ANY_SOURCE == source) {
            match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
            *mask_bits |= MPIDI_OFI_SOURCE_MASK;
        }
        else {
            match_bits |= source;
            match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
        }
    }
    else {
        match_bits = (match_bits << MPIDI_OFI_TAG_BITS);
    }

    if (MPI_ANY_TAG == tag)
        *mask_bits |= MPIDI_OFI_TAG_MASK;
    else
        match_bits |= (MPIDI_OFI_TAG_MASK & tag);

    return match_bits;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_init_get_tag(uint64_t match_bits)
{
    return ((int) (match_bits & MPIDI_OFI_TAG_MASK));
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_init_get_source(uint64_t match_bits)
{
    return ((int) ((match_bits & MPIDI_OFI_SOURCE_MASK) >> MPIDI_OFI_TAG_BITS));
}

MPL_STATIC_INLINE_PREFIX MPIR_Request *MPIDI_OFI_context_to_request(void *context)
{
    char *base = (char *) context;
    return (MPIR_Request *) MPL_container_of(base, MPIR_Request, dev.ch4.netmod);
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_send_handler
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_send_handler(struct fid_ep *ep, const void *buf, size_t len,
                                             void *desc, uint32_t dest, fi_addr_t dest_addr,
                                             uint64_t tag, void *context, int is_inject,
                                             int do_lock)
{
    int mpi_errno = MPI_SUCCESS;

    if (is_inject) {
        if (MPIDI_OFI_ENABLE_DATA)
            MPIDI_OFI_CALL_RETRY(fi_tinjectdata(ep, buf, len, dest, dest_addr, tag), tinjectdata,
                                 do_lock);
        else
            MPIDI_OFI_CALL_RETRY(fi_tinject(ep, buf, len, dest_addr, tag), tinject, do_lock);
    }
    else {
        if (MPIDI_OFI_ENABLE_DATA)
            MPIDI_OFI_CALL_RETRY(fi_tsenddata(ep, buf, len, desc, dest, dest_addr, tag, context),
                                 tsenddata, do_lock);
        else
            MPIDI_OFI_CALL_RETRY(fi_tsend(ep, buf, len, desc, dest_addr, tag, context), tsend,
                                 do_lock);
    }

  fn_exit:
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_conn_manager_insert_conn(fi_addr_t conn,
                                                                int rank,
                                                                int state)
{
    int conn_id = -1;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INSERT_CONN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INSERT_CONN);

    /* We've run out of space in the connection table. Allocate more. */
    if (MPIDI_Global.conn_mgr.next_conn_id == -1) {
        int old_max, new_max, i;
        old_max = MPIDI_Global.conn_mgr.max_n_conn;
        new_max = old_max + 1;
        MPIDI_Global.conn_mgr.free_conn_id =
            (int *) MPL_realloc(MPIDI_Global.conn_mgr.free_conn_id, new_max * sizeof(int));
        for (i = old_max; i < new_max - 1; ++i) {
            MPIDI_Global.conn_mgr.free_conn_id[i] = i + 1;
        }
        MPIDI_Global.conn_mgr.free_conn_id[new_max - 1] = -1;
        MPIDI_Global.conn_mgr.max_n_conn = new_max;
        MPIDI_Global.conn_mgr.next_conn_id = old_max;
    }

    conn_id = MPIDI_Global.conn_mgr.next_conn_id;
    MPIDI_Global.conn_mgr.next_conn_id = MPIDI_Global.conn_mgr.free_conn_id[conn_id];
    MPIDI_Global.conn_mgr.free_conn_id[conn_id] = -1;
    MPIDI_Global.conn_mgr.n_conn++;

    MPIR_Assert(MPIDI_Global.conn_mgr.n_conn <= MPIDI_Global.conn_mgr.max_n_conn);

    MPIDI_Global.conn_mgr.conn_list[conn_id].dest = conn;
    MPIDI_Global.conn_mgr.conn_list[conn_id].rank = rank;
    MPIDI_Global.conn_mgr.conn_list[conn_id].state = state;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " new_conn_id=%d for conn=%lu rank=%d state=%d",
                     conn_id, conn, rank,
                     MPIDI_Global.conn_mgr.conn_list[conn_id].state));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONN_MANAGER_INSERT_CONN);
    return conn_id;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_conn_manager_remove_conn(int conn_id)
{
    int mpi_errno = MPI_SUCCESS;
    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONN_MANAGER_REMOVE_CONN);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONN_MANAGER_REMOVE_CONN);

    MPIR_Assert(MPIDI_Global.conn_mgr.n_conn > 0);
    MPIDI_Global.conn_mgr.free_conn_id[conn_id] = MPIDI_Global.conn_mgr.next_conn_id;
    MPIDI_Global.conn_mgr.next_conn_id = conn_id;
    MPIDI_Global.conn_mgr.n_conn--;

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " free_conn_id=%d", conn_id));

    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONN_MANAGER_REMOVE_CONN);
    return mpi_errno;
}

MPL_STATIC_INLINE_PREFIX int MPIDI_OFI_dynproc_send_disconnect(int conn_id)
{
    int mpi_errno = MPI_SUCCESS;

    MPIR_Context_id_t context_id = 0xF000;
    MPIDI_OFI_dynamic_process_request_t req;
    uint64_t match_bits = 0;
    int close_msg = 0xcccccccc;
    int rank = MPIDI_Global.conn_mgr.conn_list[conn_id].rank;
    struct fi_msg_tagged msg;
    struct iovec msg_iov;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_DYNPROC_SEND_DISCONNECT);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_DYNPROC_SEND_DISCONNECT);

    if (MPIDI_Global.conn_mgr.conn_list[conn_id].state == MPIDI_OFI_DYNPROC_CONNECTED_CHILD) {
        MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                        (MPL_DBG_FDEST, " send disconnect msg conn_id=%d from child side",
                         conn_id));
        match_bits = MPIDI_OFI_init_sendtag(context_id,
                                            rank,
                                            1, MPIDI_OFI_DYNPROC_SEND);

        /* fi_av_map here is not quite right for some providers */
        /* we need to get this connection from the sockname     */
        req.done = 0;
        req.event_id = MPIDI_OFI_EVENT_DYNPROC_DONE;
        msg_iov.iov_base = &close_msg;
        msg_iov.iov_len = sizeof(close_msg);
        msg.msg_iov = &msg_iov;
        msg.desc = NULL;
        msg.iov_count = 0;
        msg.addr = MPIDI_Global.conn_mgr.conn_list[conn_id].dest;
        msg.tag = match_bits;
        msg.ignore = context_id;
        msg.context = (void *) &req.context;
        msg.data = 0;
        MPIDI_OFI_CALL_RETRY(fi_tsendmsg(MPIDI_Global.ctx[0].tx, &msg,
                                         FI_COMPLETION | FI_TRANSMIT_COMPLETE | (MPIDI_OFI_ENABLE_DATA ? FI_REMOTE_CQ_DATA : 0)),
                             tsendmsg, MPIDI_OFI_CALL_LOCK);
        MPIDI_OFI_PROGRESS_WHILE(!req.done);
    }

    switch (MPIDI_Global.conn_mgr.conn_list[conn_id].state) {
    case MPIDI_OFI_DYNPROC_CONNECTED_CHILD:
        MPIDI_Global.conn_mgr.conn_list[conn_id].state = MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_CHILD;
        break;
    case MPIDI_OFI_DYNPROC_CONNECTED_PARENT:
        MPIDI_Global.conn_mgr.conn_list[conn_id].state = MPIDI_OFI_DYNPROC_LOCAL_DISCONNECTED_PARENT;
        break;
    default:
        break;
    }

    MPL_DBG_MSG_FMT(MPIDI_CH4_DBG_GENERAL, VERBOSE,
                    (MPL_DBG_FDEST, " local_disconnected conn_id=%d state=%d",
                     conn_id, MPIDI_Global.conn_mgr.conn_list[conn_id].state));

  fn_exit:
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_DYNPROC_SEND_DISCONNECT);
    return mpi_errno;
  fn_fail:
    goto fn_exit;
}

struct MPIDI_OFI_contig_blocks_params {
    size_t max_pipe;
    DLOOP_Count count;
    DLOOP_Offset last_loc;
    DLOOP_Offset start_loc;
    size_t       last_chunk;
};

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_contig_count_block
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    int MPIDI_OFI_contig_count_block(DLOOP_Offset * blocks_p,
                                     DLOOP_Type el_type,
                                     DLOOP_Offset rel_off, DLOOP_Buffer bufp, void *v_paramp)
{
    DLOOP_Offset size, el_size;
    size_t rem, num;
    struct MPIDI_OFI_contig_blocks_params *paramp = v_paramp;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_CONTIG_COUNT_BLOCK);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_CONTIG_COUNT_BLOCK);

    DLOOP_Assert(*blocks_p > 0);

    DLOOP_Handle_get_size_macro(el_type, el_size);
    size = *blocks_p * el_size;
    if (paramp->count > 0 && rel_off == paramp->last_loc) {
        /* this region is adjacent to the last */
        paramp->last_loc += size;
        /* If necessary, recalculate the number of chunks in this block */
        if(paramp->last_loc - paramp->start_loc > paramp->max_pipe) {
            paramp->count -= paramp->last_chunk;
            num = (paramp->last_loc - paramp->start_loc) / paramp->max_pipe;
            rem = (paramp->last_loc - paramp->start_loc) % paramp->max_pipe;
            if(rem) num++;
            paramp->last_chunk = num;
            paramp->count += num;
        }
    }
    else {
         /* new region */
        num  = size / paramp->max_pipe;
        rem  = size % paramp->max_pipe;
        if(rem) num++;

        paramp->last_chunk = num;
        paramp->last_loc   = rel_off + size;
        paramp->start_loc  = rel_off;
        paramp->count     += num;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_CONTIG_COUNT_BLOCK);
    return 0;
}

#undef FUNCNAME
#define FUNCNAME MPIDI_OFI_count_iov
#undef FCNAME
#define FCNAME MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX
    size_t MPIDI_OFI_count_iov(int dt_count, MPI_Datatype dt_datatype, size_t max_pipe)
{
    struct MPIDI_OFI_contig_blocks_params params;
    MPIR_Segment dt_seg;
    ssize_t dt_size, num, rem;
    size_t dtc, count, count1, count2;;

    MPIR_FUNC_VERBOSE_STATE_DECL(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    MPIR_FUNC_VERBOSE_ENTER(MPID_STATE_MPIDI_OFI_COUNT_IOV);

    count = 0;
    dtc = MPL_MIN(2, dt_count);
    params.max_pipe = max_pipe;
    MPIDI_Datatype_check_size(dt_datatype, dtc, dt_size);
    if (dtc) {
        params.count = 0;
        params.last_loc = 0;
        params.start_loc = 0;
        params.last_chunk = 0;
        MPIR_Segment_init(NULL, 1, dt_datatype, &dt_seg, 0);
        MPIR_Segment_manipulate(&dt_seg, 0, &dt_size,
                                 MPIDI_OFI_contig_count_block,
                                 NULL, NULL, NULL, NULL, (void *) &params);
        count1 = params.count;
        params.count = 0;
        params.last_loc = 0;
        params.start_loc = 0;
        params.last_chunk = 0;
        MPIR_Segment_init(NULL, dtc, dt_datatype, &dt_seg, 0);
        MPIR_Segment_manipulate(&dt_seg, 0, &dt_size,
                                 MPIDI_OFI_contig_count_block,
                                 NULL, NULL, NULL, NULL, (void *) &params);
        count2 = params.count;
        if (count2 == 1) {      /* Contiguous */
            num = (dt_size * dt_count) / max_pipe;
            rem = (dt_size * dt_count) % max_pipe;
            if (rem)
                num++;
            count += num;
        }
        else if (count2 < count1 * 2)
            /* The commented calculation assumes merged blocks  */
            /* The iov processor will not merge adjacent blocks */
            /* When the iov state machine adds this capability  */
            /* we should switch to use the optimized calcultion */
            /* count += (count1*dt_count) - (dt_count-1);       */
             count += count1 * dt_count;
        else
            count += count1 * dt_count;
    }
    MPIR_FUNC_VERBOSE_EXIT(MPID_STATE_MPIDI_OFI_COUNT_IOV);
    return count;
}

/* Find the nearest length of iov that meets alignment requirement */
#undef  FUNCNAME
#define FUNCNAME MPIDI_OFI_align_iov_len
#undef  FCNAME
#define FCNAME   MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX size_t MPIDI_OFI_align_iov_len(size_t len)
{
    size_t pad  = MPIDI_OFI_IOVEC_ALIGN - 1;
    size_t mask = ~pad;

    return (len + pad) & mask;
}

/* Find the minimum address that is >= ptr && meets alignment requirement */
#undef  FUNCNAME
#define FUNCNAME MPIDI_OFI_aligned_next_iov
#undef  FCNAME
#define FCNAME   MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX void* MPIDI_OFI_aligned_next_iov(void *ptr)
{
    return (void *) MPIDI_OFI_align_iov_len((size_t) ptr);
}

#undef  FUNCNAME
#define FUNCNAME MPIDI_OFI_request_util_iov
#undef  FCNAME
#define FCNAME   MPL_QUOTE(FUNCNAME)
MPL_STATIC_INLINE_PREFIX struct iovec *MPIDI_OFI_request_util_iov(MPIR_Request *req)
{
#if defined (MPL_HAVE_VAR_ATTRIBUTE_ALIGNED)
    return &MPIDI_OFI_REQUEST(req, util.iov);
#else
    return (struct iovec *) MPIDI_OFI_aligned_next_iov(&MPIDI_OFI_REQUEST(req, util.iov_store));
#endif
}

/* Make sure `p` is properly aligned */
#define MPIDI_OFI_ASSERT_IOVEC_ALIGN(p)                                 \
    do {                                                                \
        MPIR_Assert((((uintptr_t)p) & (MPIDI_OFI_IOVEC_ALIGN - 1)) == 0); \
    } while (0)

#endif /* OFI_IMPL_H_INCLUDED */
