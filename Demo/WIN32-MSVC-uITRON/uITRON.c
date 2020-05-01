/*--------------------------------------------------------------------------*/
/*  Includes                                                                */
/*--------------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"
#include "timers.h"

#include "uITRON.h"
/* XXX_ID_MAXを参照するためだけにinclude */
#include "UserDefinitions.h"

/*--------------------------------------------------------------------------*/
/*  Macro and Typedef Definitions                                           */
/*--------------------------------------------------------------------------*/
/* Common */
#define CHECK_ID_(name, id)     if ((id < 0) || ( name ## _ID_MAX <= id)) return E_ID;
#define CHECK_ID_TASK_(id)      CHECK_ID_(TASK, id)
#define CHECK_ID_FLAG_(id)      CHECK_ID_(FLAG, id)
#define CHECK_ID_MAILBOX_(id)   CHECK_ID_(MAILBOX, id)
#define CHECK_ID_MTX_(id)       CHECK_ID_(MTX, id)
#define CHECK_ID_CYCLIC_(id)    CHECK_ID_(CYCLIC, id)
#define CHECK_ID_ALARM_(id)     CHECK_ID_(ALARM, id)
/* Mail Box */
#define QUEUE_LENGTH    10
#define ITEM_SIZE       sizeof(void*)

/*--------------------------------------------------------------------------*/
/*  Global Variables                                                        */
/*--------------------------------------------------------------------------*/
/* Task */
static TaskHandle_t         g_TaskHandles[TASK_ID_MAX];
/* Task Controll Block */
static StaticTask_t         g_TCBs[TASK_ID_MAX];
/* Event Flag */
static EventGroupHandle_t   g_FlagHandles[FLAG_ID_MAX];
static StaticEventGroup_t   g_FlagGroups[FLAG_ID_MAX];
/* Mail Box */
static QueueHandle_t        g_MailBoxHandles[MAILBOX_ID_MAX];
static StaticQueue_t        g_MailBoxQueues[MAILBOX_ID_MAX];
static uint8_t              g_MailBoxQueueStorageArea[MAILBOX_ID_MAX][QUEUE_LENGTH * ITEM_SIZE];
/* Mutex */
static SemaphoreHandle_t    g_MutexHandles[MTX_ID_MAX];
static StaticSemaphore_t    g_MutexSemaphores[MTX_ID_MAX];
/* Cyclic Handler */
static TimerHandle_t        g_CyclicHandles[CYCLIC_ID_MAX];
static FP                   g_CyclicFunctions[CYCLIC_ID_MAX];
static StaticTimer_t        g_CyclicBuffers[CYCLIC_ID_MAX];
/* Cyclic Handler */
static TimerHandle_t        g_AlarmHandles[ALARM_ID_MAX];
static FP                   g_AlarmFunctions[ALARM_ID_MAX];
static StaticTimer_t        g_AlarmBuffers[ALARM_ID_MAX];

/*--------------------------------------------------------------------------*/
/*  APIs                                                                    */
/*--------------------------------------------------------------------------*/

ER cre_tsk(ID tskid, T_CTSK* pk_ctsk)
{
    CHECK_ID_TASK_(tskid);

    g_TaskHandles[tskid] = xTaskCreateStatic(
        pk_ctsk->task, "Task",
        /* ここはStackType_t型配列の要素数を期待している */
        /* 対して、stkszはバイト数を指定しているので、sizeof(SizeType_t)で除算する */
        (pk_ctsk->stksz / sizeof(StackType_t)),
        NULL, pk_ctsk->itskpri, pk_ctsk->stk, &(g_TCBs[tskid]));
    /* もしTA_ACTが指定されていなかったらSUSPENDEDとする */
    /* FreeRTOSにはWAITINGの概念がなさそう・・・ */
    if (!((pk_ctsk->tskatr) & TA_ACT)) vTaskSuspend(g_TaskHandles[tskid]);
    return E_OK;
}

ER sta_tsk(ID tskid, VP_INT stacd)
{
    CHECK_ID_TASK_(tskid);
    (void)stacd;
    /* Resumeと同じ */
    rsm_tsk(tskid);
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
    CHECK_ID_FLAG_(flgid);
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
    CHECK_ID_FLAG_(flgid);
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

    xQueueReceive(g_MailBoxHandles[mbxid], &(pxRxedMessage), portMAX_DELAY);
    *ppk_msg = pxRxedMessage;

    return E_OK;
}

ER cre_mtx(ID mtxid, T_CMTX* pk_cmtx)
{
    CHECK_ID_MTX_(mtxid);
    (void)pk_cmtx;

    g_MutexHandles[mtxid] = xSemaphoreCreateMutexStatic(&(g_MutexSemaphores[mtxid]));

    return E_OK;
}

ER loc_mtx(ID mtxid)
{
    CHECK_ID_MTX_(mtxid);

    xSemaphoreTake(g_MutexHandles[mtxid], portMAX_DELAY);

    return E_OK;
}

ER ploc_mtx(ID mtxid)
{
    CHECK_ID_MTX_(mtxid);

    return (xSemaphoreTake(g_MutexHandles[mtxid], 0) ? E_OK : E_TMOUT);
}

ER unl_mtx(ID mtxid)
{
    CHECK_ID_MTX_(mtxid);

    xSemaphoreGive(g_MutexHandles[mtxid]);

    return E_OK;
}

void vTimerCallback(TimerHandle_t xTimer)
{
    /* Cyclic */
    for (int i = 0; i < CYCLIC_ID_MAX; i++) {
        if (xTimer == g_CyclicHandles[i]) {
            g_CyclicFunctions[i](NULL);
            break;
        }
    }
    /* Alarm */
    for (int i = 0; i < ALARM_ID_MAX; i++) {
        if (xTimer == g_AlarmHandles[i]) {
            g_AlarmFunctions[i](NULL);
            break;
        }
    }
}

ER cre_cyc(ID cycid, T_CCYC* pk_ccyc)
{
    ER ercd = E_OK;
    CHECK_ID_CYCLIC_(cycid);
    g_CyclicFunctions[cycid] = pk_ccyc->cychdr;
    g_CyclicHandles[cycid] = xTimerCreateStatic("Cyclic",
        pk_ccyc->cyctim, pdTRUE, (void*)0, vTimerCallback, &(g_CyclicBuffers[cycid]));
    if ((pk_ccyc->cycatr) & TA_STA) {
        ercd = sta_cyc(cycid);
    }

    return ercd;
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

ER cre_alm(ID almid, T_CALM* pk_calm)
{
    CHECK_ID_ALARM_(almid);
    g_AlarmFunctions[almid] = pk_calm->almhdr;
    g_AlarmHandles[almid] = xTimerCreateStatic("Alarm",
        10 /* Temporary */, pdFALSE, (void*)0, vTimerCallback, &(g_AlarmBuffers[almid]));

    return E_OK;
}

ER sta_alm(ID almid, RELTIM almtim)
{
    CHECK_ID_ALARM_(almid);
    xTimerChangePeriod(g_AlarmHandles[almid], almtim, 0);
    xTimerStart(g_AlarmHandles[almid], 0);
    return E_OK;
}

ER stp_alm(ID almid)
{
    CHECK_ID_ALARM_(almid);
    xTimerStop(g_AlarmHandles[almid], 0);
    return E_OK;
}

ER ref_alm(ID almid, T_RALM* pk_ralm)
{
    CHECK_ID_ALARM_(almid);
    pk_ralm->almstat = (STAT)(xTimerIsTimerActive(g_AlarmHandles[almid]));
    return E_OK;
}
