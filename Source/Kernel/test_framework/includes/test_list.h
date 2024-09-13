/******************************************************************************
 * @file test_list.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 10/05/2023
 *
 * @version 1.0
 *
 * @brief Testing framework functions and list.
 *
 * @details Testing framework functions and list. This file gathers the enable
 * flags for unit testing as well as the testing functions declarations.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

#ifndef __TEST_FRAMEWORK_TEST_LIST_H_
#define __TEST_FRAMEWORK_TEST_LIST_H_

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/
#ifdef _TESTING_FRAMEWORK_ENABLED

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/*************************************************
 * TESTING ENABLE FLAGS
 ************************************************/
#define TEST_KHEAP_ENABLED                        1
#define TEST_MEMMGR_ENABLED                       0
#define TEST_EXCEPTION_ENABLED                    0
#define TEST_INTERRUPT_ENABLED                    0
#define TEST_PANIC_ENABLED                        0
#define TEST_OS_KQUEUE_ENABLED                    0
#define TEST_OS_QUEUE_ENABLED                     0
#define TEST_OS_VECTOR_ENABLED                    0
#define TEST_OS_UHASHTABLE_ENABLED                0
#define TEST_DEVTREE_ENABLED                      0
#define TEST_FUTEX_ENABLED                        0
#define TEST_SCHEDULER_ENABLED                    0
#define TEST_SEMAPHORE_ENABLED                    0
#define TEST_MUTEX_ENABLED                        0
#define TEST_DEF_INTERRUPT_ENABLED                0
#define TEST_SIGNAL_ENABLED                       0
#define TEST_CRITICAL_ENABLED                     0
#define TEST_ATOMICS_ENABLED                      0

/*************************************************
 * TEST IDENTIFIERS
 ************************************************/

#define TEST_PANIC_SUCCESS_ID                           1000

#define TEST_INTERRUPT_SW_LOCK_REG_HANDLER0_ID          2000
#define TEST_INTERRUPT_SW_LOCK_REG_HANDLER1_ID          2001
#define TEST_INTERRUPT_SW_LOCK_CHECK0_ID                2002
#define TEST_INTERRUPT_SW_LOCK_CHECK1_ID                2003
#define TEST_INTERRUPT_SW_LOCK_CHECK2_ID                2004
#define TEST_INTERRUPT_SW_LOCK_CHECK3_ID                2005
#define TEST_INTERRUPT_SW_LOCK_CHECK4_ID                2006
#define TEST_INTERRUPT_SW_LOCK_CHECK5_ID                2007
#define TEST_INTERRUPT_SW_LOCK_CHECK6_ID                2008
#define TEST_INTERRUPT_SW_LOCK_CHECK7_ID                2009
#define TEST_INTERRUPT_SW_LOCK_REM_HANDLER0_ID          2010
#define TEST_INTERRUPT_SW_LOCK_REM_HANDLER1_ID          2011
#define TEST_INTERRUPT_SW_REG_BAD_HANDLER0_ID           2012
#define TEST_INTERRUPT_SW_REG_BAD_HANDLER1_ID           2013
#define TEST_INTERRUPT_SW_REM_BAD_HANDLER0_ID           2014
#define TEST_INTERRUPT_SW_REM_BAD_HANDLER1_ID           2015
#define TEST_INTERRUPT_SW_REG_BAD_HANDLER2_ID           2016
#define TEST_INTERRUPT_SW_REM_BAD_HANDLER2_ID           2017
#define TEST_INTERRUPT_SW_REG_HANDLER0_ID               2018
#define TEST_INTERRUPT_SW_REG_ALREADY_REG_HANDLER0_ID   2019
#define TEST_INTERRUPT_SW_REM_HANDLER0_ID               2020
#define TEST_INTERRUPT_SW_COUNTER_CHECK0_ID             2021
#define TEST_INTERRUPT_SW_COUNTER_CHECK1_ID             2022
#define TEST_INTERRUPT_SW_REG0_SWINT_HANDLER(IDVAL)     (2023 + IDVAL)
#define TEST_INTERRUPT_SW_REM0_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REG0_SWINT_HANDLER(500) + IDVAL)
#define TEST_INTERRUPT_SW_REG1_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REM0_SWINT_HANDLER(500) + IDVAL)
#define TEST_INTERRUPT_SW_REM1_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REG1_SWINT_HANDLER(500) + IDVAL)

#define TEST_KHEAP_START_ID 3000
#define TEST_KHEAP_ALLOC0_ID(IDVAL) (TEST_KHEAP_START_ID + IDVAL + 1)
#define TEST_KHEAP_ALLOC1_ID(IDVAL) (TEST_KHEAP_ALLOC0_ID(201) + IDVAL)
#define TEST_KHEAP_MEM_FREE0_ID(IDVAL) (TEST_KHEAP_ALLOC1_ID(201) + IDVAL)
#define TEST_KHEAP_MEM_FREE1_ID(IDVAL) (TEST_KHEAP_MEM_FREE0_ID(201) + IDVAL)
#define TEST_KHEAP_CREATE_THREADS(X) ((X) + 1 + TEST_KHEAP_MEM_FREE1_ID(201))
#define TEST_KHEAP_VALUE_ALLOC (1 + TEST_KHEAP_CREATE_THREADS(10))
#define TEST_KHEAP_JOIN_THREADS(X) ((X) + 1 + TEST_KHEAP_VALUE_ALLOC)
#define TEST_KHEAP_VALUE_END (1 + TEST_KHEAP_JOIN_THREADS(10))
#define TEST_KHEAP_VALUE_MID (1 + TEST_KHEAP_VALUE_END)
#define TEST_KHEAP_CREATE_TEST (1 + TEST_KHEAP_VALUE_MID)


#define TEST_QUEUE_CREATE_NODE0_ID  4000
#define TEST_QUEUE_CREATE_NODE1_ID  4001
#define TEST_QUEUE_CREATE_NODE2_ID  4002
#define TEST_QUEUE_CREATE_NODE3_ID  4003
#define TEST_QUEUE_DELETE_NODE0_ID  4004
#define TEST_QUEUE_DELETE_NODE1_ID  4005
#define TEST_QUEUE_DELETE_NODE2_ID  4006
#define TEST_QUEUE_DELETE_NODE3_ID  4007
#define TEST_QUEUE_CREATE0_ID 4008
#define TEST_QUEUE_CREATE1_ID 4009
#define TEST_QUEUE_CREATE2_ID 4010
#define TEST_QUEUE_CREATE3_ID 4011
#define TEST_QUEUE_CREATE4_ID 4012
#define TEST_QUEUE_DELETE0_ID 4013
#define TEST_QUEUE_DELETE1_ID 4014
#define TEST_QUEUE_DELETE2_ID 4015
#define TEST_QUEUE_DELETE3_ID 4016
#define TEST_QUEUE_DELETE4_ID 4017
#define TEST_QUEUE_DELETE5_ID 4018
#define TEST_QUEUE_PUSH0_ID         4019
#define TEST_QUEUE_PUSH1_ID         4020
#define TEST_QUEUE_PUSH2_ID         4021
#define TEST_QUEUE_POP0_ID          4022
#define TEST_QUEUE_POP1_ID          4023
#define TEST_QUEUE_POP2_ID          4024
#define TEST_QUEUE_POP3_ID          4025
#define TEST_QUEUE_POP4_ID          4026
#define TEST_QUEUE_POP5_ID          4027
#define TEST_QUEUE_SIZE0_ID         4028
#define TEST_QUEUE_SIZE1_ID         4029
#define TEST_QUEUE_CREATE_FIND0_ID  4030
#define TEST_QUEUE_CREATE_FIND1_ID  4031
#define TEST_QUEUE_CREATE_FIND2_ID  4032
#define TEST_QUEUE_CREATE_FIND3_ID  4033
#define TEST_QUEUE_CREATE_FIND4_ID  4034
#define TEST_QUEUE_CREATE_NODEBURST0_ID(IDVAL) (4035 + IDVAL)
#define TEST_QUEUE_CREATE_NODEBURST1_ID(IDVAL) \
    (TEST_QUEUE_CREATE_NODEBURST0_ID(80) + IDVAL)
#define TEST_QUEUE_PUSHPRIOBURST0_ID(IDVAL)\
    (TEST_QUEUE_CREATE_NODEBURST1_ID(80) + IDVAL)
#define TEST_QUEUE_PUSHBURST0_ID(IDVAL)\
    (TEST_QUEUE_PUSHPRIOBURST0_ID(40) + IDVAL)
#define TEST_QUEUE_POPBURST0_ID(IDVAL)\
    (TEST_QUEUE_PUSHBURST0_ID(40) + IDVAL)
#define TEST_QUEUE_POPBURST1_ID(IDVAL)\
    (TEST_QUEUE_POPBURST0_ID(120) + IDVAL)
#define TEST_QUEUE_DELETENODEBURST0_ID(IDVAL)\
    (TEST_QUEUE_POPBURST1_ID(120) + IDVAL)
#define TEST_QUEUE_DELETENODEBURST1_ID(IDVAL)\
    (TEST_QUEUE_DELETENODEBURST0_ID(80) + IDVAL)

#define TEST_KQUEUE_CREATE_NODE0_ID             5000
#define TEST_KQUEUE_CREATE_NODE1_ID             5001
#define TEST_KQUEUE_DELETENode0_ID             5002
#define TEST_KQUEUE_CREATE0_ID            5003
#define TEST_KQUEUE_CREATE1_ID            5004
#define TEST_KQUEUE_DELETE0_ID            5005
#define TEST_KQUEUE_DELETE1_ID            5006
#define TEST_KQUEUE_PUSH0_ID                    5007
#define TEST_KQUEUE_POP0_ID                     5008
#define TEST_KQUEUE_POP1_ID                     5009
#define TEST_KQUEUE_CREATE_FIND0_ID             5010
#define TEST_KQUEUE_CREATE_FIND1_ID             5011
#define TEST_KQUEUE_CREATE_FIND2_ID             5012
#define TEST_KQUEUE_SIZE0_ID                    5013
#define TEST_KQUEUE_SIZE1_ID                    5014
#define TEST_KQUEUE_CREATE_NODEBURST0_ID(IDVAL) (5015 + IDVAL)
#define TEST_KQUEUE_CREATE_NODEBURST1_ID(IDVAL) \
    (TEST_KQUEUE_CREATE_NODEBURST0_ID(40) + IDVAL)
#define TEST_KQUEUE_PUSHPRIOBURST0_ID(IDVAL) \
    (TEST_KQUEUE_CREATE_NODEBURST1_ID(40) + IDVAL)
#define TEST_KQUEUE_PUSHBURST0_ID(IDVAL) \
    (TEST_KQUEUE_PUSHPRIOBURST0_ID(40) + IDVAL)
#define TEST_KQUEUE_POPBURST0_ID(IDVAL) \
    (TEST_KQUEUE_PUSHBURST0_ID(40) + IDVAL)
#define TEST_KQUEUE_POPBURST1_ID(IDVAL) \
    (TEST_KQUEUE_POPBURST0_ID(120) + IDVAL)
#define TEST_KQUEUE_DELETENODEBURST0_ID(IDVAL) \
    (TEST_KQUEUE_POPBURST1_ID(120) + IDVAL)
#define TEST_KQUEUE_DELETENODEBURST1_ID(IDVAL) \
    (TEST_KQUEUE_DELETENODEBURST0_ID(40) + IDVAL)

#define TEST_UHASHTABLE_CREATE0_ID      6000
#define TEST_UHASHTABLE_CREATE1_ID      6001
#define TEST_UHASHTABLE_CREATE2_ID      6002
#define TEST_UHASHTABLE_CREATE3_ID      6003
#define TEST_UHASHTABLE_CREATE4_ID      6004
#define TEST_UHASHTABLE_CREATE5_ID      6005
#define TEST_UHASHTABLE_CREATE6_ID      6006
#define TEST_UHASHTABLE_CREATE7_ID      6007
#define TEST_UHASHTABLE_SET0_ID         6008
#define TEST_UHASHTABLE_SET1_ID         6009
#define TEST_UHASHTABLE_SET2_ID         6010
#define TEST_UHASHTABLE_SET3_ID         6011
#define TEST_UHASHTABLE_SET4_ID         6012
#define TEST_UHASHTABLE_SET5_ID         6013
#define TEST_UHASHTABLE_SET6_ID         6014
#define TEST_UHASHTABLE_SET7_ID         6015
#define TEST_UHASHTABLE_GET0_ID         6016
#define TEST_UHASHTABLE_GET1_ID         6017
#define TEST_UHASHTABLE_GET2_ID         6018
#define TEST_UHASHTABLE_GET3_ID         6019
#define TEST_UHASHTABLE_REMOVE0_ID      6020
#define TEST_UHASHTABLE_REMOVE1_ID      6021
#define TEST_UHASHTABLE_REMOVE2_ID      6022
#define TEST_UHASHTABLE_REMOVE3_ID      6023
#define TEST_UHASHTABLE_DESTROY0_ID     6024
#define TEST_UHASHTABLE_DESTROY1_ID     6025
#define TEST_UHASHTABLE_DESTROY2_ID     6026
#define TEST_UHASHTABLE_DESTROY3_ID     6027
#define TEST_UHASHTABLE_DESTROY4_ID     6028
#define TEST_UHASHTABLE_DESTROY5_ID     6029
#define TEST_UHASHTABLE_ALLOC0_ID       6030
#define TEST_UHASHTABLE_SETBURST0_ID(IDVAL) (6031 + IDVAL)
#define TEST_UHASHTABLE_SETBURST1_ID(IDVAL) \
    (TEST_UHASHTABLE_SETBURST0_ID(26) + IDVAL)
#define TEST_UHASHTABLE_SETBURST2_ID(IDVAL) \
    (TEST_UHASHTABLE_SETBURST1_ID(26) + IDVAL)
#define TEST_UHASHTABLE_SETBURST3_ID(IDVAL) \
    (TEST_UHASHTABLE_SETBURST2_ID(26) + IDVAL)
#define TEST_UHASHTABLE_GETBURST0_ID(IDVAL) \
    (TEST_UHASHTABLE_SETBURST3_ID(200) + IDVAL)
#define TEST_UHASHTABLE_GETBURST1_ID(IDVAL) \
    (TEST_UHASHTABLE_GETBURST0_ID(52) + IDVAL)
#define TEST_UHASHTABLE_GETBURST2_ID(IDVAL) \
    (TEST_UHASHTABLE_GETBURST1_ID(52) + IDVAL)
#define TEST_UHASHTABLE_GETBURST3_ID(IDVAL) \
    (TEST_UHASHTABLE_GETBURST2_ID(60) + IDVAL)
#define TEST_UHASHTABLE_GETBURST4_ID(IDVAL) \
    (TEST_UHASHTABLE_GETBURST3_ID(30) + IDVAL)
#define TEST_UHASHTABLE_REMOVEBURST0_ID(IDVAL) \
    (TEST_UHASHTABLE_GETBURST4_ID(400) + IDVAL)

#define TEST_VECTOR_CREATE0_ID          7000
#define TEST_VECTOR_CREATE1_ID          7001
#define TEST_VECTOR_CREATE2_ID          7002
#define TEST_VECTOR_CREATE3_ID          7003
#define TEST_VECTOR_GET0_ID             7004
#define TEST_VECTOR_GET1_ID             7005
#define TEST_VECTOR_GET2_ID             7006
#define TEST_VECTOR_GET3_ID             7007
#define TEST_VECTOR_GET4_ID             7008
#define TEST_VECTOR_GET5_ID             7009
#define TEST_VECTOR_GET6_ID             7010
#define TEST_VECTOR_GET7_ID             7011
#define TEST_VECTOR_INSERT0_ID          7012
#define TEST_VECTOR_INSERT1_ID          7013
#define TEST_VECTOR_POP0_ID             7014
#define TEST_VECTOR_POP1_ID             7015
#define TEST_VECTOR_RESIZE0_ID          7016
#define TEST_VECTOR_RESIZE1_ID          7017
#define TEST_VECTOR_RESIZE2_ID          7018
#define TEST_VECTOR_RESIZE3_ID          7019
#define TEST_VECTOR_RESIZE4_ID          7020
#define TEST_VECTOR_RESIZE5_ID          7021
#define TEST_VECTOR_RESIZE6_ID          7022
#define TEST_VECTOR_RESIZE7_ID          7023
#define TEST_VECTOR_RESIZE8_ID          7024
#define TEST_VECTOR_SHRINK0_ID          7025
#define TEST_VECTOR_SHRINK1_ID          7026
#define TEST_VECTOR_SHRINK2_ID          7027
#define TEST_VECTOR_COPY0_ID            7028
#define TEST_VECTOR_COPY1_ID            7029
#define TEST_VECTOR_COPY2_ID            7030
#define TEST_VECTOR_COPY3_ID            7031
#define TEST_VECTOR_CLEAR0_ID           7032
#define TEST_VECTOR_CLEAR1_ID           7033
#define TEST_VECTOR_CLEAR2_ID           7034
#define TEST_VECTOR_DESTROY0_ID         7035
#define TEST_VECTOR_DESTROY1_ID         7036
#define TEST_VECTOR_DESTROY2_ID         7037
#define TEST_VECTOR_PUSHBURST0_ID(IDVAL) (7038 + IDVAL)
#define TEST_VECTOR_GETBURST0_ID(IDVAL) TEST_VECTOR_PUSHBURST0_ID(40) + IDVAL
#define TEST_VECTOR_GETBURST1_ID(IDVAL) TEST_VECTOR_GETBURST0_ID(40) + IDVAL
#define TEST_VECTOR_GETBURST2_ID(IDVAL) TEST_VECTOR_GETBURST1_ID(105) + IDVAL
#define TEST_VECTOR_GETBURST3_ID(IDVAL) TEST_VECTOR_GETBURST2_ID(58) + IDVAL
#define TEST_VECTOR_GETBURST4_ID(IDVAL) TEST_VECTOR_GETBURST3_ID(58) + IDVAL
#define TEST_VECTOR_GETBURST5_ID(IDVAL) TEST_VECTOR_GETBURST4_ID(40) + IDVAL
#define TEST_VECTOR_GETBURST6_ID(IDVAL) TEST_VECTOR_GETBURST5_ID(40) + IDVAL
#define TEST_VECTOR_GETBURST7_ID(IDVAL) TEST_VECTOR_GETBURST6_ID(40) + IDVAL
#define TEST_VECTOR_SETBURST0_ID(IDVAL) TEST_VECTOR_GETBURST7_ID(80) + IDVAL
#define TEST_VECTOR_INSERTBURST0_ID(IDVAL) TEST_VECTOR_SETBURST0_ID(58) + IDVAL
#define TEST_VECTOR_POPBURST0_ID(IDVAL) TEST_VECTOR_INSERTBURST0_ID(60) + IDVAL

#define TEST_EXCEPTION_DIV_HANDLER0_ID          8000
#define TEST_EXCEPTION_DIV_HANDLER1_ID          8001
#define TEST_EXCEPTION_REGISTER_MIN_ID          8002
#define TEST_EXCEPTION_REGISTER_MAX_ID          8003
#define TEST_EXCEPTION_REMOVE_MIN_ID            8004
#define TEST_EXCEPTION_REMOVE_MAX_ID            8005
#define TEST_EXCEPTION_REGISTER_NULL_ID         8006
#define TEST_EXCEPTION_REMOVE_REGISTERED_ID     8007
#define TEST_EXCEPTION_REMOVE_NONREGISTERED_ID  8008
#define TEST_EXCEPTION_REGISTER_ID              8009
#define TEST_EXCEPTION_ALREADY_REGISTERED_ID    8010
#define TEST_EXCEPTION_NOT_CAUGHT_ID            8011

#define TEST_DEVTREE_PARSE          9000
#define TEST_DEVTREE_GETPROP0       9001
#define TEST_DEVTREE_GETPROP1       9002
#define TEST_DEVTREE_GETFIRSTPROP0  9003
#define TEST_DEVTREE_GETFIRSTPROP1  9004
#define TEST_DEVTREE_GETNEXTPROP0   9005
#define TEST_DEVTREE_GETNEXTPROP1   9006
#define TEST_DEVTREE_GETNEXTPROP2   9007
#define TEST_DEVTREE_GETNEXTPROP3   9008
#define TEST_DEVTREE_GETNEXTPROP4   9009
#define TEST_DEVTREE_GETCHILD0      9010
#define TEST_DEVTREE_GETCHILD1      9011
#define TEST_DEVTREE_GETCHILD2      9012
#define TEST_DEVTREE_GETCHILD3      9013
#define TEST_DEVTREE_GETCHILD4      9014
#define TEST_DEVTREE_GETNEXTNODE0   9015
#define TEST_DEVTREE_GETNEXTNODE1   9016
#define TEST_DEVTREE_GETNEXTNODE2   9017
#define TEST_DEVTREE_GETNEXTNODE3   9018
#define TEST_DEVTREE_GETNEXTNODE4   9019
#define TEST_DEVTREE_GETNEXTNODE5   9020
#define TEST_DEVTREE_GETNEXTNODE6   9021
#define TEST_DEVTREE_GETNODEBYNAME0 9022
#define TEST_DEVTREE_GETNODEBYNAME1 9023
#define TEST_DEVTREE_GETHANDLE0     9024
#define TEST_DEVTREE_GETHANDLE1     9025
#define TEST_DEVTREE_GETHANDLE2     9026
#define TEST_DEVTREE_GETMEMORY0     9027
#define TEST_DEVTREE_GETMEMORY1     9028
#define TEST_DEVTREE_GETMEMORY2     9029
#define TEST_DEVTREE_GETMEMORY3     9030
#define TEST_DEVTREE_GETRESMEMORY0  9031
#define TEST_DEVTREE_GETRESMEMORY1  9032
#define TEST_DEVTREE_GETRESMEMORY2  9033
#define TEST_DEVTREE_GETRESMEMORY3  9034
#define TEST_DEVTREE_GETNODEBYNAME  9035

#define TEST_MEMMGR_INIT_0 10000
#define TEST_MEMMGR_INIT_1 10001
#define TEST_MEMMGR_INIT_2 10002
#define TEST_MEMMGR_INIT_3 10003
#define TEST_MEMMGR_INIT_4 10004
#define TEST_MEMMGR_INIT_5 10005
#define TEST_MEMMGR_ADDBLOCK(X) ((X) + TEST_MEMMGR_INIT_5 + 1)

#define TEST_FUTEX_CREATE_THREADS(X) ((X) + 11000)
#define TEST_FUTEX_JOIN_THREADS(X)   ((X) + 11011)
#define TEST_FUTEX_ORDER_WAIT_SLEEP(X) ((X) + 11021)
#define TEST_FUTEX_ORDER_WAIT_WAIT(X)  ((X) + 11031)
#define TEST_FUTEX_ORDER_WAIT_WAITVAL(X)  ((X) + 11041)
#define TEST_FUTEX_ORDER_WAIT_SLEEP_WAKE(X) ((X) + 11051)
#define TEST_FUTEX_ORDER_WAIT_WAKE(X) ((X) + 11061)
#define TEST_FUTEX_ORDER_WAIT_WAITREASON(X) ((X) + 11071)
#define TEST_FUTEX_CANCEL_SLEEP0 11081
#define TEST_FUTEX_CANCEL_SLEEP1 11082
#define TEST_FUTEX_CANCEL_SLEEP2 11083
#define TEST_FUTEX_CANCEL_WAIT0 11084
#define TEST_FUTEX_CANCEL_WAIT1 11085
#define TEST_FUTEX_CANCEL_WAIT2 11086
#define TEST_FUTEX_CANCEL_CREATE_THREADS0 11087
#define TEST_FUTEX_CANCEL_CREATE_THREADS1 11088
#define TEST_FUTEX_CANCEL_CREATE_THREADS2 11089
#define TEST_FUTEX_CANCEL_JOIN_THREADS0 11090
#define TEST_FUTEX_CANCEL_JOIN_THREADS1 11091
#define TEST_FUTEX_CANCEL_JOIN_THREADS2 11092
#define TEST_FUTEX_CANCEL_WAKE0 11093
#define TEST_FUTEX_CANCEL_WAKE1 11094
#define TEST_FUTEX_MULTIPLE_CREATE_THREADS(X) ((X) + 11095)
#define TEST_FUTEX_MULTIPLE_SLEEP0 11105
#define TEST_FUTEX_MULTIPLE_SLEEP1 11106
#define TEST_FUTEX_MULTIPLE_WAKE 11107
#define TEST_FUTEX_MULTIPLE_VALUE_RET 11108
#define TEST_FUTEX_MULTIPLE_WAIT(X) ((X) + 11109)
#define TEST_FUTEX_MULTIPLE_WAKE1 11119
#define TEST_FUTEX_MULTIPLE_SLEEP2 11120
#define TEST_FUTEX_MULTIPLE_VALUE_RET1 11121
#define TEST_FUTEX_SAMEHANDLE_CREATE_THREADS(X) ((X) + 11122)
#define TEST_FUTEX_SAMEHANDLE_SLEEP0 11222
#define TEST_FUTEX_SAMEHANDLE_SLEEP1 11223
#define TEST_FUTEX_SAMEHANDLE_WAKE 11224
#define TEST_FUTEX_SAMEHANDLE_VALUE_RET 11225
#define TEST_FUTEX_SAMEHANDLE_WAIT(X) ((X) + 11226)
#define TEST_FUTEX_SAMEHANDLE_WAKE1 11326
#define TEST_FUTEX_SAMEHANDLE_SLEEP2 11327
#define TEST_FUTEX_SAMEHANDLE_VALUE_RET1 11328
#define TEST_FUTEX_RELEASE_GET_ID 11329
#define TEST_FUTEX_RELEASE_CREATE_THREADS(X) ((X) + 11330)
#define TEST_FUTEX_RELEASE_SLEEP0 11340
#define TEST_FUTEX_RELEASE_WAKE 11341
#define TEST_FUTEX_RELEASE_SLEEP1 11342
#define TEST_FUTEX_RELEASE_VALUE_RET0 11343
#define TEST_FUTEX_RELEASE_GET_TABLE0 11344
#define TEST_FUTEX_RELEASE_CREATE_THREADS0 11345
#define TEST_FUTEX_RELEASE_SLEEP2 11346
#define TEST_FUTEX_RELEASE_WAKE1 11347
#define TEST_FUTEX_RELEASE_SLEEP3 11348
#define TEST_FUTEX_RELEASE_VALUE_RET1 11349
#define TEST_FUTEX_RELEASE_GET_TABLE1 11350
#define TEST_FUTEX_RELEASE_WAIT(X) ((X) + 11351)
#define TEST_FUTEX_CREATE_THREADS0(X) ((X) + 11362)
#define TEST_FUTEX_JOIN_THREADS0(X)((X) + 11372)


#define TEST_SCHEDULER_TODO 12000

#define TEST_SEMAPHORE_WAIT_SEM_MUTEX1(X) ((X) + 13000)
#define TEST_SEMAPHORE_POST_SEM_MUTEX1(X) ((X) + 13100)
#define TEST_SEMAPHORE_CREATE_SEMAPHORE1  13200
#define TEST_SEMAPHORE_CREATE_THREADS1(X) ((X) + 13201)
#define TEST_SEMAPHORE_POST_SEM_MUTEX0 13301
#define TEST_SEMAPHORE_JOIN_THREADS1(X) ((X) + 13302)
#define TEST_SEMAPHORE_MUTEX_VALUE 13402
#define TEST_SEMAPHORE_CREATE_THREAD0 13403
#define TEST_SEMAPHORE_WAIT_SEM_ORDER1(X) ((X) + 13404)
#define TEST_SEMAPHORE_POST_SEM_ORDER1(X) ((X) + TEST_SEMAPHORE_WAIT_SEM_ORDER1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_SEMAPHORE_ORDER_TEST(X) ((X) + TEST_SEMAPHORE_POST_SEM_ORDER1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_SEMAPHORE_CREATE_SEMAPHORE2 (TEST_SEMAPHORE_ORDER_TEST(KERNEL_LOWEST_PRIORITY + 2) + 1)
#define TEST_SEMAPHORE_CREATE_THREADS2(X) ((X) + TEST_SEMAPHORE_CREATE_SEMAPHORE2 + 1)
#define TEST_SEMAPHORE_POST_SEM_ORDER0 (TEST_SEMAPHORE_CREATE_THREADS2(KERNEL_LOWEST_PRIORITY + 2) + 1)
#define TEST_SEMAPHORE_JOIN_THREADS2(X) ((X) + TEST_SEMAPHORE_POST_SEM_ORDER0 + 1)
#define TEST_SEMAPHORE_WAIT_SEM_FIFO1(X) ((X) + TEST_SEMAPHORE_JOIN_THREADS2(KERNEL_LOWEST_PRIORITY + 2) + 1)
#define TEST_SEMAPHORE_POST_SEM_FIFO1(X) ((X) + TEST_SEMAPHORE_WAIT_SEM_FIFO1(KERNEL_LOWEST_PRIORITY + 2) + 1)
#define TEST_SEMAPHORE_CREATE_SEMAPHORE3 (TEST_SEMAPHORE_POST_SEM_FIFO1(KERNEL_LOWEST_PRIORITY + 2) + 1)
#define TEST_SEMAPHORE_CREATE_THREADS3(X) ((X) + TEST_SEMAPHORE_CREATE_SEMAPHORE3 + 1)
#define TEST_SEMAPHORE_POST_SEM_FIFO0 (TEST_SEMAPHORE_CREATE_THREADS3(KERNEL_LOWEST_PRIORITY + 2) + 1)
#define TEST_SEMAPHORE_JOIN_THREADS3(X) ((X) + TEST_SEMAPHORE_POST_SEM_FIFO0 + 1)
#define TEST_SEMAPHORE_FIFO_VALUE (TEST_SEMAPHORE_JOIN_THREADS3(KERNEL_LOWEST_PRIORITY + 2) + 1)
#define TEST_SEMAPHORE_WAIT_SEM_CANCEL(X) ((X) + 1 + TEST_SEMAPHORE_FIFO_VALUE)
#define TEST_SEMAPHORE_CREATE_SEMAPHORE4 (TEST_SEMAPHORE_WAIT_SEM_CANCEL(100) + 1)
#define TEST_SEMAPHORE_CREATE_THREADS4(X) ((X) + 1 + TEST_SEMAPHORE_CREATE_SEMAPHORE4)
#define TEST_SEMAPHORE_POST_SEM_CANCEL0 (1 + TEST_SEMAPHORE_CREATE_THREADS4(100))
#define TEST_SEMAPHORE_JOIN_THREADS4(X) ((X) + 1 + TEST_SEMAPHORE_POST_SEM_CANCEL0)
#define TEST_SEMAPHORE_WAIT_SEM_TRYPEND1(X) ((X) + 1 + TEST_SEMAPHORE_JOIN_THREADS4(100))
#define TEST_SEMAPHORE_POST_TRYPEND1(X) ((X) + 1 + TEST_SEMAPHORE_WAIT_SEM_TRYPEND1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_SEMAPHORE_TRYWAIT_TRYPEND1(X) ((X) + 1 + TEST_SEMAPHORE_POST_TRYPEND1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_SEMAPHORE_TRYPEND_TEST(X) ((X) + 1 + TEST_SEMAPHORE_TRYWAIT_TRYPEND1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_SEMAPHORE_CREATE_SEMAPHORE5 (1 + TEST_SEMAPHORE_TRYPEND_TEST(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_SEMAPHORE_CREATE_SEMAPHORE6 (1 + TEST_SEMAPHORE_CREATE_SEMAPHORE5)
#define TEST_SEMAPHORE_CREATE_THREADS5(X) ((X) + 1 + TEST_SEMAPHORE_CREATE_SEMAPHORE6)
#define TEST_SEMAPHORE_POST_SEM_TRYPEND0 (1 + TEST_SEMAPHORE_CREATE_THREADS5(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_SEMAPHORE_JOIN_THREADS5(X) ((X) + 1 + TEST_SEMAPHORE_POST_SEM_TRYPEND0)

#define TEST_MUTEX_CREATE_THREAD0 14000
#define TEST_MUTEX_VALUE 14001
#define TEST_MUTEX_JOIN_THREADS_EXC0(X) ((X) + TEST_MUTEX_VALUE + 1)
#define TEST_MUTEX_UNLOCK_EXC0 (TEST_MUTEX_JOIN_THREADS_EXC0(100))
#define TEST_MUTEX_CREATE_THREADS_EXC0(X) ((X) + TEST_MUTEX_UNLOCK_EXC0 + 1)
#define TEST_MUTEX_LOCK_EXC0 (TEST_MUTEX_CREATE_THREADS_EXC0(100) + 1)
#define TEST_MUTEX_CREATE_MUTEX_EXC0 (TEST_MUTEX_LOCK_EXC0 + 1)
#define TEST_MUTEX_UNLOCK_EXC1(X) ((X) + TEST_MUTEX_CREATE_MUTEX_EXC0 + 1)
#define TEST_MUTEX_LOCK_EXC1(X) ((X) + 1 + TEST_MUTEX_UNLOCK_EXC1(100))
// 14408
#define TEST_MUTEX_LOCK_ORDER1(X) ((X) + 1 + TEST_MUTEX_LOCK_EXC1(100))
#define TEST_MUTEX_POST_SEM_ORDER1(X) ((X) + 1 + TEST_MUTEX_LOCK_ORDER1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_ORDER_TEST(X) ((X) + 1 + TEST_MUTEX_POST_SEM_ORDER1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_CREATE_ORDER_MUTEX (1 + TEST_MUTEX_ORDER_TEST(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_LOCK_MUTEX_ORDER0 (1 + TEST_MUTEX_CREATE_ORDER_MUTEX)
#define TEST_MUTEX_CREATE_ORDER_THREAD(X) ((X) + 1 + TEST_MUTEX_LOCK_MUTEX_ORDER0)
#define TEST_MUTEX_UNLOCK_MUTEX_ORDER0 (1 + TEST_MUTEX_CREATE_ORDER_THREAD(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_JOIN_ORDER_THREADS(X) ((X) + 1 + TEST_MUTEX_UNLOCK_MUTEX_ORDER0)
#define TEST_MUTEX_LOCK_FIFO1(X) ((X) + 1 + TEST_MUTEX_JOIN_ORDER_THREADS(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_UNLOCK_FIFO1(X) ((X) + 1 + TEST_MUTEX_LOCK_FIFO1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_CREATE_FIFO_MUTEX (1 + TEST_MUTEX_UNLOCK_FIFO1(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_LOCK_MUTEX_FIFO0 (1 + TEST_MUTEX_CREATE_FIFO_MUTEX)
#define TEST_MUTEX_CREATE_FIFO_THREADS(X) ((X) + 1 + TEST_MUTEX_LOCK_MUTEX_FIFO0)
#define TEST_MUTEX_UNLOCK_FIFO0 (1 + TEST_MUTEX_CREATE_FIFO_THREADS(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_JOIN_FIFO_THREADS(X) ((X) + 1 + TEST_MUTEX_UNLOCK_FIFO0)
#define TEST_MUTEX_FIFO_VALUE (1 + TEST_MUTEX_JOIN_FIFO_THREADS(KERNEL_LOWEST_PRIORITY + 2))
#define TEST_MUTEX_LOCK_RECUR1(X) ((X) + 1 + TEST_MUTEX_FIFO_VALUE)
#define TEST_MUTEX_UNLOCK_RECUR1(X) ((X) + 1 + TEST_MUTEX_LOCK_RECUR1(10))
#define TEST_MUTEX_CREATE_MUTEX_RECUR0 (1 + TEST_MUTEX_UNLOCK_RECUR1(10))
#define TEST_MUTEX_CREATE_THREADS_RECUR(X) ((X) + 1 + TEST_MUTEX_CREATE_MUTEX_RECUR0)
#define TEST_MUTEX_JOIN_THREADS_RECUR(X) ((X) + 1 + TEST_MUTEX_CREATE_THREADS_RECUR(10))
#define TEST_MUTEX_UNLOCK_ORDER1(X) ((X) + 1 + TEST_MUTEX_JOIN_THREADS_RECUR(10))
#define TEST_MUTEX_LOCK_CANCEL1(X) ((X) + 1 + TEST_MUTEX_UNLOCK_ORDER1(100))
#define TEST_MUTEX_CREATE_MUTEX_CANCEL (1 + TEST_MUTEX_LOCK_CANCEL1(100))
#define TEST_MUTEX_LOCK_MUTEX_CANCEL0 (1 + TEST_MUTEX_CREATE_MUTEX_CANCEL)
#define TEST_MUTEX_CREATE_THREADS_CANCEL(X) ((X) + 1 + TEST_MUTEX_LOCK_MUTEX_CANCEL0)
#define TEST_MUTEX_DESTROY_MUTEX (1 + TEST_MUTEX_CREATE_THREADS_CANCEL(100))
#define TEST_MUTEX_JOIN_THREADS_CANCEL(X) ((X) + 1 + TEST_MUTEX_DESTROY_MUTEX)
#define TEST_MUTEX_JOIN_THREADS_TRYLOCK(X) ((X) + 1 + TEST_MUTEX_JOIN_THREADS_CANCEL(100))
#define TEST_MUTEX_SYNC_MUTEX_UNLOCK (1 + TEST_MUTEX_JOIN_THREADS_TRYLOCK(100))
#define TEST_MUTEX_CREATE_THREADS_TRYLOCK(X) ((X) + 1 + TEST_MUTEX_SYNC_MUTEX_UNLOCK)
#define TEST_MUTEX_CREATE_MUTEX_SYNC_TRYLOCK (1 + TEST_MUTEX_CREATE_THREADS_TRYLOCK(100))
#define TEST_MUTEX_LOCK_MUTEX_TRYLOCK_SYNC (1 + TEST_MUTEX_CREATE_MUTEX_SYNC_TRYLOCK)
#define TEST_MUTEX_CREATE_MUTEX_TRYLOCK (1 + TEST_MUTEX_LOCK_MUTEX_TRYLOCK_SYNC)
#define TEST_MUTEX_TRYLOCK_TEST(X) ((X) + 1 + TEST_MUTEX_CREATE_MUTEX_TRYLOCK)
#define TEST_MUTEX_TRYLOCK_TRYLOCK1(X) ((X) + 1 + TEST_MUTEX_TRYLOCK_TEST(100))
#define TEST_MUTEX_UNLOCK_TRYLOCK1(X) ((X) + 1 + TEST_MUTEX_TRYLOCK_TRYLOCK1(100))
#define TEST_MUTEX_LOCK_TRYLOCK1(X) ((X) + 1 + TEST_MUTEX_UNLOCK_TRYLOCK1(100))
#define TEST_MUTEX_LOCK_MUTEX_ELEVATION(X) ((X) + 1 + TEST_MUTEX_LOCK_TRYLOCK1(100))
#define TEST_MUTEX_ELEVATION_PRIO(X) ((X) + 1 + TEST_MUTEX_LOCK_MUTEX_ELEVATION(10))
#define TEST_MUTEX_UNLOCK_MUTEX_ELEVATION(X) ((X) + 1 + TEST_MUTEX_ELEVATION_PRIO(10))
#define TEST_MUTEX_CREATE_MUTEX_ELEVATION (1 + TEST_MUTEX_UNLOCK_MUTEX_ELEVATION(10))
#define TEST_MUTEX_CREATE_THREADS_ELEVATION(X) ((X) + 1 + TEST_MUTEX_CREATE_MUTEX_ELEVATION)
#define TEST_MUTEX_JOIN_THREADS_ELEVATION(X) ((X) + 1 + TEST_MUTEX_CREATE_THREADS_ELEVATION(10))

#define TEST_DEF_INT_CREATE_MAIN_THREAD 15000
#define TEST_DEF_INT_DEFER_INT_ARGS 15001
#define TEST_DEF_INT_DEFER_INT_NULL 15002
#define TEST_DEF_INT_DEFER_INT 15003
#define TEST_DEF_INT_VALUE 15004
#define TEST_DEF_TID_VALUE 15005
#define TEST_DEF_PRIORITY 15006

#define TEST_SIGNAL_DIV_ZERO_THREAD 16000
#define TEST_SIGNAL_SEGFAULT_THREAD 16001
#define TEST_SIGNAL_ILLEGAL_INST_THREAD 16002
#define TEST_SIGNAL_SELF_SIGNAL_THREAD 16003
#define TEST_SIGNAL_REGULAR_THREAD 16004
#define TEST_SIGNAL_SIGNAL_SELF_THREAD 16006
#define TEST_SIGNAL_CREATE_DIV_ZERO_THREAD 16007
#define TEST_SIGNAL_JOIN_DIV_ZERO_THREAD 16008
#define TEST_SIGNAL_RETVAL_DIV_ZERO 16009
#define TEST_SIGNAL_CREATE_SEGFAULT_THREAD 16010
#define TEST_SIGNAL_JOIN_SEGFAULT_THREAD 16011
#define TEST_SIGNAL_RETVAL_SEGFAULT 16012
#define TEST_SIGNAL_CREATE_ILLEGAL_INST_THREAD 16013
#define TEST_SIGNAL_JOIN_ILLEGAL_INST_THREAD 16014
#define TEST_SIGNAL_RETVAL_ILLEGAL_INST 16015
#define TEST_SIGNAL_CREATE_SIGNAL_SELF_THREAD 16016
#define TEST_SIGNAL_JOIN_SIGNAL_SELF_THREAD 16017
#define TEST_SIGNAL_RETVAL_SIGNAL_SELF 16018
#define TEST_SIGNAL_CREATE_REGULAR_THREAD 16019
#define TEST_SIGNAL_SIGNAL_REGULAR_THREAD 16020
#define TEST_SIGNAL_JOIN_REGULAR_THREAD 16021
#define TEST_SIGNAL_RETVAL_REGULAR 16022
#define TEST_SIGNAL_CREATE_MAIN_THREAD 16023
#define TEST_SIGNAL_REGISTER(X) ((X) + 16024)

#define TEST_CRITICAL_CREATE_THREADS_LOCAL(X) ((X) + 17000)
#define TEST_CRITICAL_JOIN_THREADS_LOCAL(X) ((X) + 1 + TEST_CRITICAL_CREATE_THREADS_LOCAL(10))
#define TEST_CRITICAL_VALUE_LOCAL (1 + TEST_CRITICAL_JOIN_THREADS_LOCAL(10))
#define TEST_CRITICAL_CREATE_THREADS_GLOBAL0(X) ((X) + 1 + TEST_CRITICAL_VALUE_LOCAL)
#define TEST_CRITICAL_JOIN_THREADS_GLOBAL0(X) ((X) + 1 + TEST_CRITICAL_CREATE_THREADS_GLOBAL0(10))
#define TEST_CRITICAL_VALUE_GLOBAL0 (1 + TEST_CRITICAL_JOIN_THREADS_GLOBAL0(10))
#define TEST_CRITICAL_CREATE_THREADS_GLOBAL1(X) ((X) + 1 + TEST_CRITICAL_VALUE_GLOBAL0)
#define TEST_CRITICAL_JOIN_THREADS_GLOBAL1(X) ((X) + 1 + TEST_CRITICAL_CREATE_THREADS_GLOBAL1(10))
#define TEST_CRITICAL_VALUE_GLOBAL1 (1 + TEST_CRITICAL_JOIN_THREADS_GLOBAL1(10))
#define TEST_CRITICAL_CREATE_TEST (1 + TEST_CRITICAL_VALUE_GLOBAL1)

#define TEST_ATOMICS_CREATE_THREADS_SPINLOCK(X) ((X) + 18000)
#define TEST_ATOMICS_JOIN_THREADS_SPINLOCK(X) ((X) + 1 + TEST_ATOMICS_CREATE_THREADS_SPINLOCK(10))
#define TEST_ATOMICS_VALUE_SPINLOCK (1 + TEST_ATOMICS_JOIN_THREADS_SPINLOCK(10))
#define TEST_ATOMICS_CREATE_THREADS_INC(X) ((X) + 1 + TEST_ATOMICS_VALUE_SPINLOCK)
#define TEST_ATOMICS_JOIN_THREADS_INC(X) ((X) + 1 + TEST_ATOMICS_CREATE_THREADS_INC(10))
#define TEST_ATOMICS_VALUE_INC (1 + TEST_ATOMICS_JOIN_THREADS_INC(10))
#define TEST_ATOMICS_CREATE_THREADS_DEC(X) ((X) + 1 + TEST_ATOMICS_VALUE_INC)
#define TEST_ATOMICS_JOIN_THREADS_DEC(X) ((X) + 1 + TEST_ATOMICS_CREATE_THREADS_DEC(10))
#define TEST_ATOMICS_VALUE_DEC (1 + TEST_ATOMICS_JOIN_THREADS_DEC(10))
#define TEST_ATOMICS_CREATE_TEST (1 + TEST_ATOMICS_VALUE_DEC)

/** @brief Current test name */
#define TEST_FRAMEWORK_TEST_NAME "Signals"

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/* None */

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/* None */

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/* None */

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

void panicTest(void);
void interruptTest(void);
void exceptionTest(void);
void kheapTest(void);
void queueTest(void);
void kqueueTest(void);
void vectorTest(void);
void uhashtableTest(void);
void devtreeTest(void);
void memmgrTest(void);
void futexTest(void);
void schedulerTest(void);
void semaphoreTest(void);
void mutexTest(void);
void interruptDefferTest(void);
void signalTest(void);
void vfsTest(void);
void atomicsTest(void);
void criticalTest(void);

#endif /* #ifdef _TESTING_FRAMEWORK_ENABLED */

#endif /* #ifndef __TEST_FRAMEWORK_TEST_LIST_H_ */

/************************************ EOF *************************************/