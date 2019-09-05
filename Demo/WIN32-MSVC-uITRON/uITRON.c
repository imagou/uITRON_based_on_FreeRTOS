/*--------------------------------------------------------------------------*/
/*  Includes                                                                */
/*--------------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "timers.h"

#include "uITRON.h"

#define __GLOBAL_DEFINITION_USER_DEFINITIONS__
#include "UserDefinitions.h"

/*--------------------------------------------------------------------------*/
/*  Macro and Typedef Definitions                                           */
/*--------------------------------------------------------------------------*/
/* Common */
#define CHECK_ID_(name, id)     if ((id < 0) || ( name ## _ID_MAX <= id)) return E_ID;
#define CHECK_ID_TASK_(id)      CHECK_ID_(TASK, id)
#define CHECK_ID_FLAG_(id)      CHECK_ID_(FLAG, id)
#define CHECK_ID_MAILBOX_(id)   CHECK_ID_(MAILBOX, id)
#define CHECK_ID_CYCLIC_(id)    CHECK_ID_(CYCLIC, id)
/* Mail Box */
#define QUEUE_LENGTH    10
#define ITEM_SIZE       sizeof(void*)

/*--------------------------------------------------------------------------*/
/*  Global Variables                                                        */
/*--------------------------------------------------------------------------*/
/* Task */
static TaskHandle_t g_TaskHandles[TASK_ID_MAX];
/* Task Controll Block */
static StaticTask_t g_TCBs[TASK_ID_MAX];
/* Event Flag */
static EventGroupHandle_t   g_FlagHandles[FLAG_ID_MAX];
static StaticEventGroup_t   g_FlagGroups[FLAG_ID_MAX];
/* Mail Box */
static QueueHandle_t        g_MailBoxHandles[MAILBOX_ID_MAX];
static StaticQueue_t        g_MailBoxQueues[MAILBOX_ID_MAX];
static uint8_t              g_MailBoxQueueStorageArea[MAILBOX_ID_MAX][QUEUE_LENGTH * ITEM_SIZE];
/* Cyclic Handler */
static TimerHandle_t        g_CyclicHandles[CYCLIC_ID_MAX];
static FP                   g_CyclicFunctions[CYCLIC_ID_MAX];
static StaticTimer_t        g_CyclicBuffers[CYCLIC_ID_MAX];

/*--------------------------------------------------------------------------*/
/*  APIs                                                                    */
/*--------------------------------------------------------------------------*/

ER cre_tsk(ID tskid, T_CTSK* pk_ctsk)
{
    if ((tskid < 0) || (TASK_ID_MAX <= tskid)) return E_ID;

    g_TaskHandles[tskid] = xTaskCreateStatic(
        pk_ctsk->task, pk_ctsk->tskatr, pk_ctsk->depth, NULL, pk_ctsk->itskpri, pk_ctsk->stk, &(g_TCBs[tskid]));
    return E_OK;
}

ER dly_tsk(RELTIM dlytim)
{
    TickType_t xCycleFrequency = pdMS_TO_TICKS((unsigned long int)dlytim);
    vTaskDelay(xCycleFrequency);
    return E_OK;
}

ER sus_tsk(ID tskid)
{
    CHECK_ID_TASK_(tskid);
    vTaskSuspend(g_TaskHandles[tskid]);
    return E_OK;
}

ER rsm_tsk(ID tskid)
{
    CHECK_ID_TASK_(tskid);
    vTaskResume(g_TaskHandles[tskid]);
    return E_OK;
}

ER cre_flg(ID flgid, T_CFLG* pk_cflg)
{
    if ((flgid < 0) || (FLAG_ID_MAX <= flgid)) return E_ID;

    (void)pk_cflg;
    g_FlagHandles[flgid] = xEventGroupCreateStatic(&(g_FlagGroups[flgid]));
    return E_OK;
}

ER set_flg(ID flgid, FLGPTN setptn)
{
    CHECK_ID_FLAG_(flgid);
    xEventGroupSetBits(g_FlagHandles[flgid], setptn);
    return E_OK;
}

ER iset_flg(ID flgid, FLGPTN setptn)
{
    CHECK_ID_FLAG_(flgid);
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    xEventGroupSetBitsFromISR(g_FlagHandles[flgid], setptn, &xHigherPriorityTaskWoken);
    return E_OK;
}

ER wai_flg(ID flgid, FLGPTN waiptn, MODE wfmode, FLGPTN* p_flgptn)
{
    if ((flgid < 0) || (FLAG_ID_MAX <= flgid)) return E_ID;

    *p_flgptn = xEventGroupWaitBits(g_FlagHandles[flgid], waiptn,
        pdTRUE,     /* Clear Bits */
        (const BaseType_t)(wfmode == TWF_ANDW), portMAX_DELAY);
    return E_OK;
}

ER cre_mbx(ID mbxid, T_CMBX* pk_cmbx)
{
    CHECK_ID_MAILBOX_(mbxid);
    (void)pk_cmbx;

    /* void*型をQUEUE_LENGTHだけキューイング可能 */
    g_MailBoxHandles[mbxid] = xQueueCreateStatic(QUEUE_LENGTH,
        ITEM_SIZE,
        g_MailBoxQueueStorageArea[mbxid],
        &(g_MailBoxQueues[mbxid]));

    return E_OK;
}

ER snd_mbx(ID mbxid, T_MSG* pk_msg)
{
    CHECK_ID_MAILBOX_(mbxid);

    pk_msg->msgqueue = (void*)pk_msg;
    xQueueSend(g_MailBoxHandles[mbxid], (void*)pk_msg, (TickType_t)0);

    return E_OK;
}

ER rcv_mbx(ID mbxid, T_MSG** ppk_msg)
{
    CHECK_ID_MAILBOX_(mbxid);

    T_MSG* pxRxedMessage;

    xQueueReceive(g_MailBoxHandles[0], &(pxRxedMessage), portMAX_DELAY);
    *ppk_msg = pxRxedMessage;

    return E_OK;
}

void vTimerCallback(TimerHandle_t xTimer)
{
    for (int i = 0; i < CYCLIC_ID_MAX; i++) {
        if (xTimer == g_CyclicHandles[i]) {
            g_CyclicFunctions[i](NULL);
            break;
        }
    }
}

ER cre_cyc(ID cycid, T_CCYC* pk_ccyc)
{
    CHECK_ID_CYCLIC_(cycid);
    g_CyclicFunctions[cycid] = pk_ccyc->cychdr;
    g_CyclicHandles[cycid] = xTimerCreateStatic(pk_ccyc->cycatr,
        pk_ccyc->cyctim, pdTRUE, (void*)0, vTimerCallback, &(g_CyclicBuffers[cycid]));

    return E_OK;
}

ER sta_cyc(ID cycid)
{
    CHECK_ID_CYCLIC_(cycid);
    xTimerStart(g_CyclicHandles[cycid], 0);
    return E_OK;
}

ER stp_cyc(ID cycid)
{
    CHECK_ID_CYCLIC_(cycid);
    xTimerStop(g_CyclicHandles[cycid], 0);
    return E_OK;
}

ER ref_cyc(ID cycid, T_RCYC* pk_rcyc)
{
    CHECK_ID_CYCLIC_(cycid);
    pk_rcyc->cycstat = (STAT)(xTimerIsTimerActive(g_CyclicHandles[cycid]));
    return E_OK;
}
