#ifndef __RSPQ_INTERNAL
#define __RSPQ_INTERNAL

#define RSPQ_DEBUG                     1
#define RSPQ_PROFILE                   0

#define RSPQ_DRAM_LOWPRI_BUFFER_SIZE   0x200   ///< Size of each RSPQ RDRAM buffer for lowpri queue (in 32-bit words)
#define RSPQ_DRAM_HIGHPRI_BUFFER_SIZE  0x80    ///< Size of each RSPQ RDRAM buffer for highpri queue (in 32-bit words)

#define RSPQ_DMEM_BUFFER_SIZE          0x100   ///< Size of the RSPQ DMEM buffer (in bytes)
#define RSPQ_OVERLAY_TABLE_SIZE        0x10    ///< Number of overlay IDs (0-F)
#define RSPQ_OVERLAY_DESC_SIZE         0x10    ///< Size of a single overlay descriptor

/** Maximum number of overlays that can be registered (affects DMEM table size) */
#define RSPQ_MAX_OVERLAY_COUNT         8
#define RSPQ_OVERLAY_ID_COUNT          16
#define RSPQ_MAX_OVERLAY_COMMAND_COUNT ((RSPQ_MAX_OVERLAY_COUNT - 1) * 16)

/** Minimum / maximum size of a block's chunk (contiguous memory buffer) */
#define RSPQ_BLOCK_MIN_SIZE            64
#define RSPQ_BLOCK_MAX_SIZE            4192

/** Maximum number of nested block calls */
#define RSPQ_MAX_BLOCK_NESTING_LEVEL   8
#define RSPQ_LOWPRI_CALL_SLOT          (RSPQ_MAX_BLOCK_NESTING_LEVEL+0)  ///< Special slot used to store the current lowpri pointer
#define RSPQ_HIGHPRI_CALL_SLOT         (RSPQ_MAX_BLOCK_NESTING_LEVEL+1)  ///< Special slot used to store the current highpri pointer

/** Signal used by RDP SYNC_FULL command to notify that an interrupt is pending */
#define SP_STATUS_SIG_RDPSYNCFULL              SP_STATUS_SIG1
#define SP_WSTATUS_SET_SIG_RDPSYNCFULL         SP_WSTATUS_SET_SIG1
#define SP_WSTATUS_CLEAR_SIG_RDPSYNCFULL       SP_WSTATUS_CLEAR_SIG1

/** Signal used by RSP to notify that a syncpoint was reached */
#define SP_STATUS_SIG_SYNCPOINT                SP_STATUS_SIG2
#define SP_WSTATUS_SET_SIG_SYNCPOINT           SP_WSTATUS_SET_SIG2
#define SP_WSTATUS_CLEAR_SIG_SYNCPOINT         SP_WSTATUS_CLEAR_SIG2

/** Signal used to notify that RSP is executing the highpri queue */
#define SP_STATUS_SIG_HIGHPRI_RUNNING          SP_STATUS_SIG3
#define SP_WSTATUS_SET_SIG_HIGHPRI_RUNNING     SP_WSTATUS_SET_SIG3
#define SP_WSTATUS_CLEAR_SIG_HIGHPRI_RUNNING   SP_WSTATUS_CLEAR_SIG3

/** Signal used to notify that the CPU has requested that the RSP switches to the highpri queue */
#define SP_STATUS_SIG_HIGHPRI_REQUESTED        SP_STATUS_SIG4
#define SP_WSTATUS_SET_SIG_HIGHPRI_REQUESTED   SP_WSTATUS_SET_SIG4
#define SP_WSTATUS_CLEAR_SIG_HIGHPRI_REQUESTED SP_WSTATUS_CLEAR_SIG4

/** Signal used by RSP to notify that has finished one of the two buffers of the highpri queue */
#define SP_STATUS_SIG_BUFDONE_HIGH             SP_STATUS_SIG5
#define SP_WSTATUS_SET_SIG_BUFDONE_HIGH        SP_WSTATUS_SET_SIG5
#define SP_WSTATUS_CLEAR_SIG_BUFDONE_HIGH      SP_WSTATUS_CLEAR_SIG5

/** Signal used by RSP to notify that has finished one of the two buffers of the lowpri queue */
#define SP_STATUS_SIG_BUFDONE_LOW              SP_STATUS_SIG6
#define SP_WSTATUS_SET_SIG_BUFDONE_LOW         SP_WSTATUS_SET_SIG6
#define SP_WSTATUS_CLEAR_SIG_BUFDONE_LOW       SP_WSTATUS_CLEAR_SIG6

/** Signal used by the CPU to notify the RSP that more data has been written in the current queue */
#define SP_STATUS_SIG_MORE                     SP_STATUS_SIG7
#define SP_WSTATUS_SET_SIG_MORE                SP_WSTATUS_SET_SIG7
#define SP_WSTATUS_CLEAR_SIG_MORE              SP_WSTATUS_CLEAR_SIG7

// RSP assert codes (for assers generated by rsp_queue.S)
#define ASSERT_INVALID_OVERLAY       0xFF01    ///< A command is referencing an overlay that is not registered
#define ASSERT_INVALID_COMMAND       0xFF02    ///< The requested command is not defined in the overlay

/** Debug marker in DMEM to check that C and Assembly have the same DMEM layout */
#define RSPQ_DEBUG_MARKER            0xABCD0123

#define RSPQ_PROFILE_SLOT_SIZE     8
#define RSPQ_PROFILE_SLOT_COUNT    (RSPQ_MAX_OVERLAY_COUNT + 1)
#define RSPQ_PROFILE_IDLE_SLOT     RSPQ_MAX_OVERLAY_COUNT

#endif
