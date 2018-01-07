/***********************************************/
/*       Async Message Passing Library         */
/*       Copyright  2005                       */
/*                                             */
/*  Author:                                    */ 
/*          Rongfeng Tang                      */
/***********************************************/
#ifndef __AMP_THREAD_H_
#define __AMP_THREAD_H_

#include <amp_sys.h>
#include <amp_types.h>

#ifndef AMP_MAX_THREAD_NUM
#define AMP_MAX_THREAD_NUM       (128)  /*for each type of thread*/
#endif


/*the number created when init module*/
#define AMP_SRVIN_THR_INIT_NUM      AMP_MAX_THREAD_NUM
#define AMP_SRVOUT_THR_INIT_NUM     AMP_MAX_THREAD_NUM

//#define AMP_RECONN_THR_INIT_NUM  (1)
#define AMP_BH_THR_NUM           (8)

#ifdef __AMP_RECONFIRM_MSG
#define AMP_RECONFIRM_MSG_RECHECK_INTERVAL  (120)
#endif

/*tell service to work sem*/
extern amp_sem_t      amp_process_out_sem;
extern amp_sem_t      amp_process_in_sem;
extern amp_sem_t      amp_netmorn_sem;
extern amp_sem_t      amp_reconn_sem;
extern amp_s32_t      amp_listen_sockfd;

/*current number of service thread*/
/*
 * increased by specific creation function
 */ 
extern amp_u32_t amp_srvin_thread_num;
extern amp_u32_t amp_srvout_thread_num;
extern amp_u32_t amp_reconn_thread_num;

/*thread structure*/
extern amp_thread_t  *amp_srvin_threads;
extern amp_thread_t  *amp_srvout_threads;
extern amp_thread_t  *amp_reconn_threads;

#define AMP_NET_MORNITOR_INTVL  (50)  /*seconds*/

extern amp_sem_t     amp_reconn_finalize_sem;

/*lock for pretect changing thread structure or thread number*/
extern amp_lock_t     amp_threads_lock;

/*some kernel thread callbacks*/
void* __amp_serve_out_thread (void *argv);
void* __amp_serve_in_thread (void *argv);
void* __amp_reconn_thread (void *argv);
void* __amp_listen_thread (void *argv);
void* __amp_wakeup_reconn_thread(void *argv);
void* __amp_netmorn_thread(void *argv);
void* __amp_netmorn_thread2(void *argv);
/*start and stop threads functions*/
int __amp_start_srvin_thread (amp_comp_context_t *ctxt, amp_u32_t seqno);
int __amp_start_srvout_thread (amp_comp_context_t *ctxt, amp_u32_t seqno);
amp_thread_t*  __amp_start_listen_thread(amp_comp_context_t *ctxt);
amp_thread_t*  __amp_start_netmorn_thread(amp_comp_context_t *);

int __amp_start_srvin_threads (amp_comp_context_t *ctxt);
int __amp_start_srvout_threads (amp_comp_context_t *ctxt);
int __amp_start_reconn_threads (void);
int __amp_stop_srvin_threads (void);
int __amp_stop_srvout_threads (void);
int __amp_stop_reconn_threads (void);

int  __amp_start_reconn_thread (amp_u32_t seqno);
int  __amp_stop_reconn_thread (amp_u32_t seqno);
int  __amp_start_wakeup_thread(amp_u32_t seqno);
int __amp_stop_wakeup_thread (amp_u32_t seqno);


int __amp_stop_listen_thread(amp_comp_context_t *);
int __amp_stop_netmorn_thread(amp_comp_context_t *);


/*create daemon for the kernel thread*/
void __amp_kdaemonize (char *str);


/*blocking signal function*/
void __amp_blockallsigs (void);

	
/*init and finalize*/
int __amp_threads_init (amp_comp_context_t *ctxt);     /*tell them up*/
int __amp_threads_finalize(void);  /*tell them down*/

int __amp_recv_msg (amp_connection_t *conn, amp_message_t **retmsgp);
#endif

/*end of file*/
