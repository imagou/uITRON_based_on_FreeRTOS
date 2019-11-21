/*--------------------------------------------------------------------------*/
/*  Preprocessors                                                           */
/*--------------------------------------------------------------------------*/
#undef EXTERN
#ifdef __GLOBAL_DEFINITION_USER_DEFINITIONS__
#define EXTERN
#else
#define EXTERN extern
#endif

/*--------------------------------------------------------------------------*/
/*  Tasks                                                                   */
/*--------------------------------------------------------------------------*/
/* Task ID */
#define TASK_ID(x)      TASK_ID_ ## x
enum {
    TASK_ID(SEND),
    TASK_ID(RECV),
    TASK_ID(TIMER),
    TASK_ID(COMMAND),

/* ここまでユーザー定義 */
/* インデックスとしても使用するので、0からの連続値として定義すること */
/* また、以下は消さないこと */
    TASK_ID(MAX),
};

/* Task Priority */
#define TASK_PRI(x)     TASK_PRI_ ## x
#define TASK_PRI_HIGH   (7)     // See configMAX_PRIORITIES
#define TASK_PRI_MIDDLE (5)
#define TASK_PRI_LOW    (3)
enum {
    TASK_PRI(SEND)      = TASK_PRI_HIGH,
    TASK_PRI(RECV)      = TASK_PRI_MIDDLE,
    TASK_PRI(TIMER)     = TASK_PRI_LOW,
    TASK_PRI(COMMAND)   = TASK_PRI_LOW,
};

/* Task Stack */
#define TASK_STACK(x)   G_TaskStack_ ## x
EXTERN unsigned char    TASK_STACK(SEND)[1024];
EXTERN unsigned char    TASK_STACK(RECV)[1024];
EXTERN unsigned char    TASK_STACK(TIMER)[512];
EXTERN unsigned char    TASK_STACK(COMMAND)[1024];

/*--------------------------------------------------------------------------*/
/*  Resources                                                               */
/*--------------------------------------------------------------------------*/
/*--- Event Flag ---*/
/* Flag ID */
#define FLAG_ID(x)      FLAG_ID_ ## x
enum {
    FLAG_ID(SEND),

/* Task IDと同様 */
    FLAG_ID(MAX),
};

/*--- Mail Box ---*/
/* Common */
#define MAILBOX_ID(x)   MAILBOX_ID_ ## x
/* Mail Box ID */
enum {
    MAILBOX_ID(RECV),

    /* Task IDと同様 */
    MAILBOX_ID(MAX),
};

/*--- Mutex ---*/
/* Common */
#define MTX_ID(x)   MTX_ID_ ## x
/* Mutex ID */
enum {
    MTX_ID(RECV),

    /* Task IDと同様 */
    MTX_ID(MAX),
};

/*--- Cyclic Handler ---*/
/* Common */
#define CYCLIC_ID(x)    CYCLIC_ID_ ## x
/* Cyclic ID */
enum {
    CYCLIC_ID(USER),

    /* Task IDと同様 */
    CYCLIC_ID(MAX),
};

/*--------------------------------------------------------------------------*/
/*  For Debug                                                               */
/*--------------------------------------------------------------------------*/
#define DEBUG_PRINT(fmt, ...)    printf(fmt "\r\n", __VA_ARGS__)
