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
    E_TMOUT,
} ER;
#define TA_NULL         (0x00)
#define TA_HLNG         (0x00)
#define TA_ACT          (0x02)
#define TA_TFIFO        (0x00)
#define TA_TPRI         (0x01)
#define TA_WSGL         (0x00)
#define TA_WMUL         (0x02)
#define TA_CLR          (0x04)
#define TA_MFIFO        (0x00)
#define TA_MPRI         (0x02)

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
    PRI maxmpri;
    VP  mprihd;
} T_CMBX;
ER cre_mbx(ID, T_CMBX*);
/* Send/Receive */
typedef struct {
    void* msgqueue;   /* Message Queue Address */
} T_MSG;
ER snd_mbx(ID, T_MSG*);
ER rcv_mbx(ID, T_MSG**);

/*--------------------------------------------------------------------------*/
/*  Mutex                                                                   */
/*--------------------------------------------------------------------------*/
/* Create */
typedef struct {
    ATR mtxatr;
    PRI ceilpri;
} T_CMTX;
ER cre_mtx(ID, T_CMTX*);
/* Lock/Unlock */
ER loc_mtx(ID);
ER ploc_mtx(ID);
ER unl_mtx(ID);

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
/* Start/Stop */
ER sta_cyc(ID);
ER stp_cyc(ID);
/* Refer */
typedef int     STAT;
typedef struct {
    STAT    cycstat;
} T_RCYC;
ER ref_cyc(ID, T_RCYC*);

/*--------------------------------------------------------------------------*/
/*  Alarm Handler                                                           */
/*--------------------------------------------------------------------------*/
/* Create */
typedef struct {
    ATR     almatr;
    VP_INT  exinf;
    FP      almhdr;
} T_CALM;
ER cre_alm(ID, T_CALM*);
/* Start/Stop */
ER sta_alm(ID, RELTIM);
ER stp_alm(ID);
