/*--------------------------------------------------------------------------*/
/*  Includes                                                                */
/*--------------------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <conio.h>
#include <stdint.h>

#include "portmacro.h"
#include "uITRON.h"
#define __GLOBAL_DEFINITION_USER_DEFINITIONS__
#include "UserDefinitions.h"

/* やむなくFreeRTOS I/Fを使う場合、手動で追加 */
void vTaskStartScheduler(void);

/*--------------------------------------------------------------------------*/
/*  Macro and Typedef Definitions                                           */
/*--------------------------------------------------------------------------*/
/*--- Message ---*/
typedef struct {
    T_MSG       msg;
    ID          id;
    uint32_t    params[1];
} MESSAGE_ENTITY;

#define MESSAGE_SIZE    (64)
static MESSAGE_ENTITY   g_Messages[MESSAGE_SIZE];
static uint16_t         g_MessageIndex = 0;

/* メッセージ取得 */
static MESSAGE_ENTITY* GetUserMessage(void) {
    uint16_t index = g_MessageIndex++;
    index %= MESSAGE_SIZE;
    return &(g_Messages[index]);
}
/* 解放はまだ用意していない */

/*--- Event ---*/
#define EVENT_ALL       (0x00FFFFFF)    /* FreeRTOS仕様制限で、最大で24-bit */
#define EVENT_CYCLIC    (0x00000001)
/* Reserved bit1 - bit15 */
#define EVENT_STOP      (0x00010000)
#define EVENT_RESTART   (0x00020000)
#define EVENT_DUMMY     (0x00800000)

/*--- Task ---*/
#define CREATE_TASK(name, act, func) {   \
    T_CTSK ctsk_;   \
    ATR tskatr_ = TA_HLNG;  \
    if (act) tskatr_ |= TA_ACT; \
    ctsk_.tskatr = tskatr_;                 /* タスク属性 */ \
    ctsk_.exinf = NULL;                     /* タスク起動時パラメータ */ \
    ctsk_.task = func;                      /* タスク関数 */ \
    ctsk_.itskpri = TASK_PRI(name);         /* タスクの優先度 */   \
    ctsk_.stksz = sizeof(TASK_STACK(name)); /* タスクスタックサイズ */   \
    ctsk_.stk = TASK_STACK(name);           /* タスクスタック（静的に確保しておく必要あり） */    \
    cre_tsk(TASK_ID(name), &ctsk_); \
}

/*--------------------------------------------------------------------------*/
/*  Prototypes                                                              */
/*--------------------------------------------------------------------------*/
/* Tasks */
static void SendTask(void*);
static void RecvTask(void*);
static void TimerTask(void*);
static void CommandTask(void*);
static void GetsTask(void*);
/* Handlers */
static void UserCyclicHandler(void*);
static void GetsCyclicHandler(void*);

/*--------------------------------------------------------------------------*/
/*  Static Functions                                                        */
/*--------------------------------------------------------------------------*/

static ER CreateOsResources(void)
{
    T_CFLG cflg;
    cflg.flgatr = TA_WSGL;
    cflg.iflgptn = 0;
    cre_flg(FLAG_ID(SEND), &cflg);

    T_CMBX cmbx;
    cmbx.mbxatr = TA_TFIFO;
    cre_mbx(MAILBOX_ID(RECV), &cmbx);
    cre_mbx(MAILBOX_ID(COMMAND), &cmbx);

    T_CMTX cmtx;
    cmtx.mtxatr = TA_TFIFO;
    cre_mtx(MTX_ID(RECV), &cmtx);

    T_CCYC ccyc;
    ccyc.cychdr = UserCyclicHandler;
    ccyc.cyctim = 1000;
    cre_cyc(CYCLIC_ID(USER), &ccyc);
    ccyc.cychdr = GetsCyclicHandler;
    ccyc.cyctim = 10;
    cre_cyc(CYCLIC_ID(GETS), &ccyc);

    return E_OK;
}

static ER CreateOsTasks(void)
{
    CREATE_TASK(SEND, TRUE, SendTask);
    CREATE_TASK(RECV, TRUE, RecvTask);

    CREATE_TASK(TIMER, FALSE, TimerTask);

    CREATE_TASK(COMMAND, TRUE, CommandTask);
    CREATE_TASK(GETS, TRUE, GetsTask);

    return E_OK;
}

static void PrintUsage()
{
    DEBUG_PRINT("");
    DEBUG_PRINT("uTITRON (based on FreeRTOS) Demo Program");
    DEBUG_PRINT("   1 - 15の数値    :   数値メッセージの送受信");
    DEBUG_PRINT("   stop            :   受信タスクの一時停止");
    DEBUG_PRINT("   start           :   受信タスクの再開");
    DEBUG_PRINT("   lock            :   受信タスクのロック");
    DEBUG_PRINT("   unlock          :   受信タスクのアンロック");
    DEBUG_PRINT("   cyclic          :   周期ハンドラの開始/停止（初期状態は停止）");
    DEBUG_PRINT("   exit            :   デモの終了");
    DEBUG_PRINT("   help            :   このメッセージを表示");
    DEBUG_PRINT("");
    DEBUG_PRINT("");
}

/*--------------------------------------------------------------------------*/
/*  Main and Task Functions                                                 */
/*--------------------------------------------------------------------------*/

int main(void)
{
    CreateOsResources();
    CreateOsTasks();

    /* スケジューリング開始 */
    /* 対応するuITRONサービスコールはない・・・ */
    vTaskStartScheduler();

    /* ここには来ないはず */

    return 0;
}

static void SendTask(void* params)
{
    uint32_t count = 0;

    /* Just to remove compiler warning. */
    (void)params;

    /* タイマタスクは遅延起動 */
    /* sta_tsk()を使いたいがためだけにこうしている */
    sta_tsk(TASK_ID(TIMER), NULL);

    while (1) {
        int32_t num = 0;
        MESSAGE_ENTITY* ent = NULL;

        FLGPTN flgptn;
        wai_flg(FLAG_ID(SEND), EVENT_ALL, TWF_ORW, &flgptn);
        DEBUG_PRINT("[%s]: RECV EVENT (%08X)", __func__, flgptn);

        /* Dummy Event */
        if (flgptn & EVENT_DUMMY) {
            num = -1;
            goto send_message;
        }

        /* Suspend Task */
        if (flgptn & EVENT_STOP) {
            sus_tsk(TASK_ID(RECV));
            continue;
        }
        /* Resume Task */
        if (flgptn & EVENT_RESTART) {
            rsm_tsk(TASK_ID(RECV));
            continue;
        }

        /* Cyclic Event */
        if (flgptn & EVENT_CYCLIC) {
            static int32_t expired = 0;
            DEBUG_PRINT("[%s]: Cyclic Timer Expired (%d)", __func__, ++expired);
            continue;
        }

        /* Numeric Event */
        /* メッセージIDはイベントを（数値に戻して）設定 */
        /* パラメータにカウンタを渡す */
        for (num = 0; !((flgptn >> num) & 1) ; num++) ;
send_message:
        ent = GetUserMessage();
        ent->id = (ID)num;
        ent->params[0] = ++count;

        snd_mbx(MAILBOX_ID(RECV), (T_MSG *)ent);
    }
}

static void RecvTask(void* params)
{
    /* Just to remove compiler warning. */
    (void)params;

    while (1) {
        T_MSG *msg;
        rcv_mbx(MAILBOX_ID(RECV), &msg);
        MESSAGE_ENTITY* ent = (MESSAGE_ENTITY*)(msg);
        /* 有効IDのみ出力対象 */
        if (0 <= ent->id) DEBUG_PRINT("[%s]: RECV MESSAGE (%d,%d)", __func__, ent->id, ent->params[0]);

        /* Lock/Unlock Mutex */
        loc_mtx(MTX_ID(RECV));
        unl_mtx(MTX_ID(RECV));
    }
}

static void TimerTask(void *params)
{
    /* Just to remove compiler warning. */
    (void)params;

    while (1) {
        RELTIM tim = 3000;
        DEBUG_PRINT("[%s]: DELAY %dms", __func__, tim);
        dly_tsk(tim);
    }
}

static void CommandTask(void* params)
{
    char str[256];

    /* Just to remove compiler warning. */
    (void)params;

    while (1) {
        T_MSG* msg;
        rcv_mbx(MAILBOX_ID(COMMAND), &msg);
        MESSAGE_ENTITY* ent = (MESSAGE_ENTITY*)(msg);
        /* Timeout */
        if (!(ent->params[0])) continue;
        /* Get String */
        strcpy_s(str, sizeof(str), (const char *)(ent->params[0]));

        /* Command Parser */
        int32_t num = atol(str);
        /* Numeric Event (1 To 15) */
        if ((0 < num) && (num < 16)) {
            set_flg(FLAG_ID(SEND), (1 << num));
        }
#define EQUALS_(x) (0 == strcmp(str, #x))
        /* Suspend */
        else if (EQUALS_(stop)) {
            set_flg(FLAG_ID(SEND), EVENT_STOP);
        }
        /* Resume */
        else if (EQUALS_(start)) {
            set_flg(FLAG_ID(SEND), EVENT_RESTART);
        }
        /* Lock */
        else if (EQUALS_(lock)) {
            /* 多重ロックを避けるためポーリングとする。 */
            if (ploc_mtx(MTX_ID(RECV)) == E_OK) {
                /* ダミーメッセージを投げ、ロック待ちに遷移させる。 */
                set_flg(FLAG_ID(SEND), EVENT_DUMMY);
            }
        }
        /* Unlock */
        else if (EQUALS_(unlock)) {
            unl_mtx(MTX_ID(RECV));
        }
        /* Cyclic */
        else if (EQUALS_(cyclic)) {
            T_RCYC rcyc;
            ref_cyc(CYCLIC_ID(USER), &rcyc);
            if (!(rcyc.cycstat))    sta_cyc(CYCLIC_ID(USER));
            else                    stp_cyc(CYCLIC_ID(USER));
        }
        /* Print Usage */
        else if (EQUALS_(help)) {
            sus_tsk(TASK_ID(TIMER));
            PrintUsage();
            rsm_tsk(TASK_ID(TIMER));
        }
        /* Exit */
        else if (EQUALS_(exit)) {
            /* 強制終了 */
            exit(1);
        }
#undef EQUALS_
    }
}

static void GetsTask(void* params)
{
    static char str[256];

    /* Just to remove compiler warning. */
    (void)params;

    while (1) {
        stp_cyc(CYCLIC_ID(GETS));
        sta_cyc(CYCLIC_ID(GETS));

        /* gets_s()で改行入力までブロックしてしまうが、これはRTOSのお作法に反している。 */
        /* このため、周期ハンドラを用い、タイムアウトを強引に検知している。 */
        gets_s(str, sizeof(str));

        MESSAGE_ENTITY* ent = GetUserMessage();
        ent->params[0] = (uint32_t)str;
        snd_mbx(MAILBOX_ID(COMMAND), (T_MSG*)ent);
    }
}

/*--------------------------------------------------------------------------*/
/*  Handlers                                                                */
/*--------------------------------------------------------------------------*/

static void UserCyclicHandler(void* params)
{
    /* Just to remove compiler warning. */
    (void)params;

    iset_flg(FLAG_ID(SEND), EVENT_CYCLIC);
}

static void GetsCyclicHandler(void* params)
{
    /* Just to remove compiler warning. */
    (void)params;

    MESSAGE_ENTITY* ent = GetUserMessage();
    ent->params[0] = (uint32_t)NULL;
    snd_mbx(MAILBOX_ID(COMMAND), (T_MSG*)ent);
}
