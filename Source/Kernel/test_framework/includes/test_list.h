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

#include <cpu_interrupt.h> /* CPU Interrupt test ids */

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/*************************************************
 * TESTING ENABLE FLAGS
 ************************************************/
#define TEST_KHEAP_ENABLED                        0
#define TEST_EXCEPTION_ENABLED                    0
#define TEST_KICKSTART_ENABLED                    0
#define TEST_INTERRUPT_ENABLED                    0
#define TEST_PANIC_ENABLED                        0
#define TEST_OS_KQUEUE_ENABLED                    0
#define TEST_OS_QUEUE_ENABLED                     0
#define TEST_OS_VECTOR_ENABLED                    0
#define TEST_OS_UHASHTABLE_ENABLED                0
#define TEST_DEVTREE_ENABLED                      1

/*************************************************
 * TEST IDENTIFIERS
 ************************************************/

#define TEST_KICKSTART_END_ID                           0

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
    (TEST_INTERRUPT_SW_REG0_SWINT_HANDLER(MAX_INTERRUPT_LINE) + IDVAL)
#define TEST_INTERRUPT_SW_REG1_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REM0_SWINT_HANDLER(MAX_INTERRUPT_LINE) + IDVAL)
#define TEST_INTERRUPT_SW_REM1_SWINT_HANDLER(IDVAL)     \
    (TEST_INTERRUPT_SW_REG1_SWINT_HANDLER(MAX_INTERRUPT_LINE) + IDVAL)

#define TEST_KHEAP_START_ID 3000
#define TEST_KHEAP_ALLOC0_ID(IDVAL) (TEST_KHEAP_START_ID + IDVAL + 1)
#define TEST_KHEAP_ALLOC1_ID(IDVAL) (TEST_KHEAP_ALLOC0_ID(201) + IDVAL)
#define TEST_KHEAP_MEM_FREE0_ID(IDVAL) (TEST_KHEAP_ALLOC1_ID(201) + IDVAL)
#define TEST_KHEAP_MEM_FREE1_ID(IDVAL) (TEST_KHEAP_MEM_FREE0_ID(201) + IDVAL)

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
/** @brief Current test name */
#define TEST_FRAMEWORK_TEST_NAME "Device Tree Lib Suite"

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

void interrupt_test(void);
void exception_test(void);
void kheap_test(void);
void queue_test(void);
void kqueue_test(void);
void vector_test(void);
void uhashtable_test(void);
void devtreeTest(void);

#endif

#endif /* #ifndef __TEST_FRAMEWORK_TEST_LIST_H_ */

/************************************ EOF *************************************/