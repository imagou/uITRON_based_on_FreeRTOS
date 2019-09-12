/*--------------------------------------------------------------------------*/
/*  Common                                                                  */
/*--------------------------------------------------------------------------*/
/* Other */
typedef int             ID;
typedef unsigned int    ATR;
typedef void*           VP_INT;
typedef void            (*FP)(void*);
typedef int             RELTIM;
typedef enum {
    E_OK = 0,
    E_ID,
} ER;
#define TA_NULL         (0x00)
#define TA_HLNG         (0x00)
#define TA_ACT          (0x02)
#define TA_TFIFO        (0x00)
#define TA_TPRI         (0x01)
#define TA_WSGL         (0x00)
#define TA_WMUL         (0x02)
#define TA_CLR          (0x04)

/*--------------------------------------------------------------------------*/
/*  Task                                                                    */
/*--------------------------------------------------------------------------*/
/* Create */
typedef int     PRI;
#undef SIZE
#define SIZE    unsigned int
typedef void*   VP;
typedef struct {
    ATR         tskatr;
    VP_INT      exinf;
    FP          task;
    PRI         itskpri;
    SIZE        stksz;
    VP          stk;
} T_CTSK;
ER cre_tsk(ID, T_CTSK*);
/* Start */
ER sta_tsk(ID, VP_INT);
/* Delay */
ER dly_tsk(RELTIM);
/* Suspend/Resume */
ER sus_tsk(ID);
ER rsm_tsk(ID);

/*--------------------------------------------------------------------------*/
/*  Event Flag                                                              */
/*--------------------------------------------------------------------------*/
/* Create */
typedef unsigned int    FLGPTN;
typedef struct {
    ATR     flgatr;
    FLGPTN  iflgptn;
} T_CFLG;
ER cre_flg(ID, T_CFLG*);
/* Set */
ER set_flg(ID, FLGPTN);
ER iset_flg(ID, FLGPTN);
typedef enum {
    TWF_ANDW = 0,
    TWF_ORW = 1,
} MODE;
/* Wait */
ER wai_flg(ID, FLGPTN, MODE, FLGPTN*);

/*--------------------------------------------------------------------------*/
/*  Mail Box                                                                */
/*--------------------------------------------------------------------------*/
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

/*--------------------------------------------------------------------------*/
/*  Cyclic Handler                                                          */
/*--------------------------------------------------------------------------*/
/* Create */
typedef struct {
    ATR     cycatr;
    FP      cychdr;
    RELTIM  cyctim;
} T_CCYC;
ER cre_cyc(ID, T_CCYC*);
/* Start */
ER sta_cyc(ID);
/* Stop */
ER stp_cyc(ID);
/* Refer */
typedef int     STAT;
typedef struct {
    STAT    cycstat;
} T_RCYC;
ER ref_cyc(ID, T_RCYC*);
