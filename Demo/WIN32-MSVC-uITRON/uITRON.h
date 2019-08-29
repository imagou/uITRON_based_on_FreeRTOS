/*--------------------------------------------------------------------------*/
/*  Common                                                                  */
/*--------------------------------------------------------------------------*/
typedef int     ID;
typedef enum {
    E_OK = 0,
    E_ID,
} ER;

/*--------------------------------------------------------------------------*/
/*  Task                                                                    */
/*--------------------------------------------------------------------------*/
/* Common */
#define TASK_ID(x)          TASK_ID_ ## x
#define TASK_PRI(x)         TASK_PRI_ ## x
#define TASK_STACK(x)       G_TaskStack_ ## x
#define TASK_STACK_DEPTH(x) (sizeof(TASK_STACK(x)) / sizeof(TASK_STACK(x)[0]))
typedef StackType_t     STACK_TYPE;
/* Create */
typedef const char*     ATR;
typedef void (*FP)(void*);
typedef int             PRI;
typedef StackType_t*    VP;
typedef struct {
    ATR         tskatr;
    FP          task;
    PRI         itskpri;
    VP          stk;
/* “ÆŽ©‚Ì’l */
    uint32_t    depth;
} T_CTSK;
ER cre_tsk(ID, T_CTSK*);
/* Delay */
typedef int RELTIM;
ER dly_tsk(RELTIM);
/* Suspend/Resume */
ER sus_tsk(ID);
ER rsm_tsk(ID);

/*--------------------------------------------------------------------------*/
/*  Event Flag                                                              */
/*--------------------------------------------------------------------------*/
/* Common */
#define FLAG_ID(x)      FLAG_ID_ ## x
/* Create */
typedef uint32_t    FLGPTN;
typedef struct {
    FLGPTN  iflgptn;
} T_CFLG;
ER cre_flg(ID, T_CFLG*);
/* Set */
ER set_flg(ID, FLGPTN);
typedef enum {
    TWF_ANDW = 0,
    TWF_ORW = 1,
} MODE;
/* Wait */
ER wai_flg(ID, FLGPTN, MODE, FLGPTN*);

/*--------------------------------------------------------------------------*/
/*  Mail Box                                                                */
/*--------------------------------------------------------------------------*/
/* Common */
#define MAILBOX_ID(x)       MAILBOX_ID_ ## x
/* Create */
typedef struct {
    ATR mbxatr;
} T_CMBX;
ER cre_mbx(ID, T_CMBX*);
/* Send */
typedef struct {
    void* msgqueue;   /* Message Queue Address */
} T_MSG;
ER snd_mbx(ID, T_MSG*);
ER rcv_mbx(ID, T_MSG**);
