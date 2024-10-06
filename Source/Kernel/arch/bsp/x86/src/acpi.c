/*******************************************************************************
 * @file acpi.c
 *
 * @see acpi.h
 *
 * @author Alexy Torres Aurora Dugo
 *
 * @date 04/06/2024
 *
 * @version 2.0
 *
 * @brief Kernel ACPI driver.
 *
 * @details Kernel ACPI driver, detects and parse the ACPI for the kernel.
 *
 * @copyright Alexy Torres Aurora Dugo
 ******************************************************************************/

/*******************************************************************************
 * INCLUDES
 ******************************************************************************/

/* Included headers */
#include <panic.h>        /* Kernel panic */
#include <kheap.h>        /* Kernel heap */
#include <stdint.h>       /* Standard int types */
#include <stddef.h>       /* Standard definition types */
#include <string.h>       /* Memory manipulation */
#include <kerror.h>       /* Kernel error types */
#include <memory.h>       /* Memory manager */
#include <syslog.h>       /* Kernel Syslog */
#include <devtree.h>      /* Device tree service */
#include <drivermgr.h>    /* Driver manager */

/* Configuration files */
#include <config.h>

/* Header file */
#include <acpi.h>

/* Unit test header */
#include <test_framework.h>

/*******************************************************************************
 * CONSTANTS
 ******************************************************************************/

/** @brief FDT property for regs  */
#define ACPI_FDT_REGS_PROP "reg"

/** @brief Module name */
#define MODULE_NAME "X86 ACPI"

/** @brief ACPI memory signature: RSDP. */
#define ACPI_RSDP_SIG 0x2052545020445352
/** @brief ACPI memory signature: RSDT. */
#define ACPI_RSDT_SIG 0x54445352
/** @brief ACPI memory signature: XSDT. */
#define ACPI_XSDT_SIG 0x54445358
/** @brief ACPI memory signature: FACP. */
#define ACPI_FACP_SIG 0x50434146
/** @brief ACPI memory signature: FACS. */
#define ACPI_FACS_SIG 0x53434146
/** @brief ACPI memory signature: APIC. */
#define ACPI_APIC_SIG 0x43495041
/** @brief ACPI memory signature: DSDT. */
#define ACPI_DSDT_SIG 0x54445344
/** @brief ACPI memory signature: HPET. */
#define ACPI_HPET_SIG 0x54455048

/** @brief APIC type: local APIC. */
#define APIC_TYPE_LOCAL_APIC 0x0
/** @brief APIC type: IO APIC. */
#define APIC_TYPE_IO_APIC 0x1
/** @brief APIC type: interrupt override. */
#define APIC_TYPE_INTERRUPT_OVERRIDE 0x2
/** @brief APIC type: NMI. */
#define APIC_TYPE_NMI 0x4

/** @brief HPET Flags HW Revision mask */
#define HPET_FLAGS_HW_REV_MASK 0x00FF
/** @brief HPET Flags comparator count mask */
#define HPET_FLAGS_CC_MASK 0x1F00
/** @brief HPET Flags counter size mask */
#define HPET_FLAGS_CS_MASK 0x2000
/** @brief HPET Flags legacy replacement IRQ mask */
#define HPET_FLAGS_IRQ_MASK 0x8000

/** @brief HPET Flags HW Revision shift */
#define HPET_FLAGS_HW_REV_SHIFT 0
/** @brief HPET Flags comparator count shift */
#define HPET_FLAGS_CC_SHIFT 8
/** @brief HPET Flags counter size shift */
#define HPET_FLAGS_CS_SHIFT 13
/** @brief HPET Flags legacy replacement IRQ shift */
#define HPET_FLAGS_IRQ_SHIFT 15

/*******************************************************************************
 * STRUCTURES AND TYPES
 ******************************************************************************/

/** @brief ACPI structure header.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief Header signature */
    char pSignature[4];

    /** @brief Structure length */
    uint32_t    length;

    /** @brief Structure revision version */
    uint8_t     revision;

    /** @brief Structure checksum */
    uint8_t     checksum;

    /** @brief OEM identifier */
    char        pOEM[6];

    /** @brief OEM talbe identifier */
    char        pOEMTableId[8];

    /** @brief OEM revision version */
    uint32_t    oemRevision;

    /** @brief Creator identifier */
    uint32_t    creatorId;

    /** @brief Creator revision version */
    uint32_t    creatorRevision;
} __attribute__((__packed__)) acpi_header_t;

/** @brief ACPI RSDP descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief RSDP Signature */
    char pSignature[8];

    /** @brief RSDP checksum */
    uint8_t checksum;

    /** @brief RSDP OEM identifiter */
    char oemid[6];

    /** @brief RSDP ACPI resivion version */
    uint8_t revision;

    /** @brief RSDT pointer address */
    uint32_t rsdtAddress;
} __attribute__ ((packed)) rsdp_descriptor_t;

/** @brief ACPI extended RSDP descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief RSDP base part (non extended) */
    rsdp_descriptor_t rsdpBasePart;

    /** @brief RSDP extension length */
    uint32_t length;

    /** @brief XSDT pointer address */
    uint64_t xsdtAddress;

    /** @brief RSDP extended checksum */
    uint8_t extendedChecksum;

    /** @brief Reserved memory */
    uint8_t reserved[3];
} __attribute__ ((packed)) rsdp_descriptor_2_t;

/** @brief ACPI RSDT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief RSDT header */
    acpi_header_t header;

    /** @brief Array of pointers to the ACPI DTs. */
    uint32_t *pDtPointers;
} __attribute__ ((packed)) rsdt_descriptor_t;

/** @brief ACPI XSDT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief XSDT header */
    acpi_header_t header;

    /** @brief Array of pointers to the ACPI DTs. */
    uint64_t *pDtPointers;
} __attribute__ ((packed)) xsdt_descriptor_t;

/** @brief ACPI address descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief Address space identifier */
    uint8_t addressSpace;

    /** @brief Bit width */
    uint8_t bitWidth;

    /** @brief Bit offset */
    uint8_t bitOffset;

    /** @brief Access size */
    uint8_t accessSize;

    /** @brief Plain address */
    uint64_t address;
} __attribute__((__packed__)) generic_address_t;

/** @brief ACPI FADT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief FADT header */
    acpi_header_t header;

    /** @brief Firmware control */
    uint32_t firmwareControl;

    /** @brief DSDT pointer address*/
    uint32_t dsdt;

    /** @brief Reserved */
    uint8_t reserved0;

    /** @brief Prefered PM profile */
    uint8_t preferredPMProfile;

    /** @brief SCI interrupt */
    uint16_t sciInterrupt;

    /** @brief SMI command port */
    uint32_t smiCommandPort;

    /** @brief ACPI enable */
    uint8_t acpiEnable;

    /** @brief ACPI disable */
    uint8_t acpiDisable;

    /** @brief S4 BIOS request */
    uint8_t s4BIOSReq;

    /** @brief PSTATE control */
    uint8_t pstateControl;

    /** @brief PM1 A Event block */
    uint32_t pm1aEventBlock;

    /** @brief PM1 B Event block */
    uint32_t pm1bEventBlock;

    /** @brief PM1 A Control block */
    uint32_t pm1aControlBlock;

    /** @brief PM1 B Control block */
    uint32_t pm1bControlBlock;

    /** @brief PM2 Control block */
    uint32_t pm2ControlBlock;

    /** @brief PM Timer block */
    uint32_t pmTimerBlock;

    /** @brief GPE0 Block */
    uint32_t gpe0Block;

    /** @brief GPE1 Block */
    uint32_t gpe1Block;

    /** @brief PM1 Event length */
    uint8_t pm1EventLength;

    /** @brief PM1 Control length */
    uint8_t pm1ControlLength;

    /** @brief PM1 Control length */
    uint8_t pm2ControlLength;

    /** @brief PM Timer length*/
    uint8_t pmTimerLength;

    /** @brief GPE0 length */
    uint8_t gpe0Length;

    /** @brief GPE1 length */
    uint8_t gpe1Length;

    /** @brief GPE1 balse */
    uint8_t gpe1Base;

    /** @brief CState control */
    uint8_t cStateControl;

    /** @brief Worst C2 latency */
    uint16_t worstC2Latency;

    /** @brief Worst C3 latency */
    uint16_t worstC3Latency;

    /** @brief Flush size */
    uint16_t flushSize;

    /** @brief Flush stride */
    uint16_t flush_stride;

    /** @brief Duty offset */
    uint8_t dutyOffset;

    /** @brief Duty width */
    uint8_t dutyWidth;

    /** @brief Day alarm */
    uint8_t dayAlarm;

    /** @brief Month alarm */
    uint8_t monthAlarm;

    /** @brief Century */
    uint8_t century;

    /** @brief Boot architecture flags */
    uint16_t bootArchitectureFlags;

    /** @brief Reserved */
    uint8_t reserved1;

    /** @brief Flags */
    uint32_t flags;

    /** @brief Reset registers */
    generic_address_t resetReg;

    /** @brief Reset value */
    uint8_t resetValue;

    /** @brief Reserved */
    uint8_t reserved2[3];

    /** @brief Extended firmware control */
    uint64_t xFirmwareControl;

    /** @brief Extended DSDT */
    uint64_t xDsdt;

    /** @brief Extended PM1 A Event block  */
    generic_address_t xPM1aEventBlock;

    /** @brief Extended PM1 B Event block */
    generic_address_t xPM1bEventBlock;

    /** @brief Extended PM1 A Control block */
    generic_address_t xPM1aControlBlock;

    /** @brief Extended PM1 B Control block */
    generic_address_t xPM1bControlBlock;

    /** @brief Extended PM2 Control block */
    generic_address_t xPM2ControlBlock;

    /** @brief Extended PM Timer block */
    generic_address_t xPMTimerBlock;

    /** @brief Extended GPE0 block  */
    generic_address_t xGPE0Block;

    /** @brief Extended GPE1 block */
    generic_address_t xGPE1Block;

} __attribute__((__packed__)) acpi_fadt_t;

/** @brief ACPI DSDT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief DSDT header */
    acpi_header_t header;
}  __attribute__((__packed__)) acpi_dsdt_t;

/** @brief ACPI MADT descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief DSDT header */
    acpi_header_t header;

    /** @brief Local APIC ACPI address */
    uint32_t localApicAddr;

    /** @brief Local APIC flags */
    uint32_t flags;
} __attribute__((__packed__)) acpi_madt_t;

/** @brief ACPI HPET descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief The APIC header field. */
    acpi_header_t header;

    /** @brief HPET flags
     * @details [0:7] hardware revision ID
     *          [8:12] Comparator count
     *          [13] Counter size
     *          [14] Reserved
     *          [15] Legacy Replacement IRQ routine table
     */
    uint16_t flags;

    /** @brief PCI vendor ID */
    uint16_t pciVendorId;

    /** @brief HPET base adderss */
    generic_address_t address;

    /** @brief HPET sequence number */
    uint8_t hpetNumber;

    /** @brief Minimum number of ticks supported in periodic mode */
    uint16_t minimumTick;

    /** @brief Page protection attribute */
    uint8_t pageProtection;
} __attribute__((__packed__)) acpi_hpet_desc_t;

/**
 * @brief ACPI APIC descriptor header.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief The APIC descriptor type. */
    uint8_t type;

    /** @brief The APIC descriptor length. */
    uint8_t length;
} __attribute__((__packed__)) apic_header_t;

/**
 * @brief ACPI IO APIC descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief The APIC header field. */
    apic_header_t header;

    /** @brief Stores the IO-APIC identifier. */
    uint8_t ioApicId;

    /** @brief Reserved field. */
    uint8_t reserved;

    /** @brief Stores the IO-APIC MMIO address. */
    uint32_t ioApicAddr;

    /** @brief Stores the IO-APIC GSI base address. */
    uint32_t globalSystemInterruptBase;
} __attribute__((__packed__)) io_apic_t;

/**
 * @brief ACPI LAPIC descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief The APIC header field. */
    apic_header_t header;

    /** @brief Stores the LAPIC CPU identifier. */
    uint8_t cpuId;

    /** @brief Stores the LAPIC identifier.  */
    uint8_t lapicId;

    /** @brief Stores the LAPIC configuration flags. */
    uint32_t flags;
} __attribute__((__packed__)) lapic_t;

/** @brief ACPI Interrupt override descriptor.
 * Please check the ACPI standard for more information.
 */
typedef struct
{
    /** @brief The APIC header field. */
    apic_header_t header;

    /** @brief Interrupt override bus */
    uint8_t bus;

    /** @brief Interrupt override source */
    uint8_t source;

    /** @brief Interrupt override destination */
    uint32_t interrupt;

    /** @brief Interrupt override flags */
    uint16_t flags;
} __attribute__((__packed__)) apic_interrupt_override_t;

/** @brief x86 ACPI driver controler. */
typedef struct
{
    /** @brief Detected CPU count. */
    uint8_t detectedCPUCount;

    /** @brief Detected IO APIC count. */
    uint8_t detectedIOAPICCount;

    /** @brief Detected interrupt override count */
    uint8_t detectedIntOverrideCount;

    /** @brief Detected HPET count */
    uint8_t detectedHpetCount;

    /** @brief Parsed local APIC address. */
    uintptr_t localApicAddress;

    /** @brief List of detected LAPIC. */
    lapic_node_t* pLapicList;

    /** @brief List of detected IO APIC. */
    io_apic_node_t* pIoApicList;

    /** @brief List of detected interrupt override */
    int_override_node_t* pIntOverrideList;

    /** @brief List of detected HPET devices */
    hpet_node_t* pHpetList;
} acpi_controler_t;

/*******************************************************************************
 * MACROS
 ******************************************************************************/

/**
 * @brief Assert macro used by the ACPI to ensure correctness of execution.
 *
 * @details Assert macro used by the ACPI to ensure correctness of execution.
 * Due to the critical nature of the ACPI, any error generates a kernel panic.
 *
 * @param[in] COND The condition that should be true.
 * @param[in] MSG The message to display in case of kernel panic.
 * @param[in] ERROR The error code to use in case of kernel panic.
 */
#define ACPI_ASSERT(COND, MSG, ERROR) {                     \
    if((COND) == false)                                     \
    {                                                       \
        PANIC(ERROR, MODULE_NAME, MSG);                     \
    }                                                       \
}

/**
 * @brief Adds a node to a list using a cursor.
 *
 * @param[out] LIST The list to update.
 * @param[out] CURSOR The cursor to use.
 * @param[out] NODE The node to add.
*/
#define ADD_TO_LIST(LIST, CURSOR, NODE) {       \
    NODE->pNext = NULL;                         \
    CURSOR = LIST;                              \
    if(CURSOR != NULL)                          \
    {                                           \
        while(CURSOR->pNext != NULL)            \
        {                                       \
            CURSOR = CURSOR->pNext;             \
        }                                       \
        CURSOR->pNext = NODE;                   \
    }                                           \
    else                                        \
    {                                           \
        LIST = NODE;                            \
    }                                           \
}

/*******************************************************************************
 * STATIC FUNCTIONS DECLARATIONS
 ******************************************************************************/
/**
 * @brief Attaches the ACPI driver to the system.
 *
 * @details Attaches the ACPI driver to the system. This function will use the
 * FDT to initialize the ACPI hardware and retreive the ACPI parameters.
 *
 * @param[in] pkFdtNode The FDT node with the compatible declared
 * by the driver.
 *
 * @return The success state or the error code.
 */
static OS_RETURN_E _acpiAttach(const fdt_node_t* pkFdtNode);

/**
 * @brief Use the APIC RSDP to parse the ACPI infomation.
 *
 * @details Use the APIC RSDP to parse the ACPI infomation. The function will
 * detect the RSDT or XSDT pointed and parse them.
 *
 * @param[in] kpRsdpDesc The RSDP to walk.
 */
static void _acpiParseRSDP(const rsdp_descriptor_t* kpRsdpDesc);

/**
 * @brief Parse the APIC RSDT table.
 *
 * @details Parse the APIC RSDT table. The function will detect the read each
 * entries of the RSDT and call the corresponding functions to parse the entries
 * correctly.
 *
 * @param[in] kpRsdtPtr The address of the RSDT entry to parse.
 */
static void _acpiParseRSDT(const rsdt_descriptor_t* kpRsdtPtr);

#ifdef ARCH_64_BITS
/**
 * @brief Parse the APIC XSDT table.
 *
 * @details The function will detect the read each entries of the XSDT and call
 * the corresponding functions to parse the entries correctly.
 *
 * @param[in] kpXsdtPtr The address of the XSDT entry to parse.
 */
static void _acpiParseXSDT(const xsdt_descriptor_t* kpXsdtPtr);
#endif

/**
 * @brief Parse the APIC SDT table.
 *
 * @details Parse the APIC SDT table. The function will detect the SDT given as
 * parameter thanks to the information contained in the header. Then, if the
 * entry is correctly detected and supported, the parsing function corresponding
 * will be called.
 *
 * @param[in] kpHeader The virtual address of the SDT entry to parse.
 * @param[in] kPhysAddr The physical address of the SDT entry.
 */
static void _acpiParseDT(const acpi_header_t* kpHeader,
                         const uintptr_t      kPhysAddr);

/**
 * @brief Parse the APIC FADT table.
 *
 * @details Parse the APIC FADT table. The function will save the FADT table
 * address in for further use.
 *
 * @param[in] kpFadtPtr The address of the FADT entry to parse.
 */
static void _acpiParseFADT(const acpi_fadt_t* kpFadtPtr);

/**
 * @brief Parses the APIC entries of the MADT table.
 *
 * @details Parse the APIC entries of the MADT table.The function will parse
 * each entry and detect three of the possible entry kind: the LAPIC entries,
 * which also determine the cpu count, the IO-APIC entries will detect the
 * different available IO-APIC of the system, the interrupt override will also
 * be detected.
 *
 * @param[in] kpApicPtr The address of the MADT entry to parse.
 */
static void _acpiParseMADT(const acpi_madt_t* kpApicPtr);

/**
 * @brief Parses the APIC entries of the HPET table.
 *
 * @details Parse the APIC entries of the HPET table.The function will parse
 * each entry and add the HPET node to the list of detected HPET entries.
 *
 * @param[in] kpHpetPtr The address of the HPET entry to parse.
 */
static void _acpiParseHPET(const acpi_hpet_desc_t* kpHpetPtr);

/**
 * @brief Returns the number of LAPIC detected in the system.
 *
 * @details Returns the number of LAPIC detected in the system.
 *
 * @return The number of LAPIC detected in the system is returned.
    */
static uint8_t _acpiGetLAPICCount(void);

/**
 * @brief Returns the list of detected LAPICs.
 *
 * @details Returns the list of detected LAPICs. This list should not be
 * modified and is generated during the attach of the ACPI while parsing
 * its tables.
 *
 * @return The list of detected LAPICs descritors is returned.
 */
static const lapic_node_t* _acpiGetLAPICList(void);

/**
 * @brief Returns the detected LAPIC base address.
 *
 * @details Returns the detected LAPIC base address.
 *
 * @return The detected LAPIC base address is returned.
 */
static uintptr_t _acpiGetLAPICBaseAddress(void);

/**
 * @brief Returns the number of IO-APIC detected in the system.
 *
 * @details Returns the number of IO-APIC detected in the system.
 *
 * @return The number of IO-APIC detected in the system is returned.
    */
static uint8_t _acpiGetIOAPICCount(void);

/**
 * @brief Returns the list of detected IO-APICs.
 *
 * @details Returns the list of detected IO-APICs. This list should not be
 * modified and is generated during the attach of the ACPI while parsing
 * its tables.
 *
 * @return The list of detected IO-APICs descritors is returned.
 */
static const io_apic_node_t* _acpiGetIOAPICList(void);

/**
 * @brief Returns the list of detected HPETs.
 *
 * @details Returns the list of detected HPETs. This list should not be
 * modified and is generated during the attach of the ACPI while parsing
 * its tables.
 *
 * @return The list of detected HPETs descritors is returned.
 */
static const hpet_node_t* _acpiGetHPETList(void);

/**
 * @brief Checks if the IRQ has been remaped in the IO-APIC structure.
 *
 * @details Checks if the IRQ has been remaped in the IO-APIC structure. This
 * function must be called after the init_acpi function.
 *
 * @param[in] kIrqNumber The initial IRQ number to check.
 *
 * @return The remapped IRQ number corresponding to the irq number given as
 * parameter.
 */
static uint32_t _acpiGetRemapedIrq(const uint32_t kIrqNumber);

/*******************************************************************************
 * GLOBAL VARIABLES
 ******************************************************************************/

/************************* Imported global variables **************************/
/* None */

/************************* Exported global variables **************************/
/* None */

/************************** Static global variables ***************************/
/** @brief ACPI driver instance. */
static driver_t sX86ACPIDriver = {
    .pName         = "X86 ACPI Driver",
    .pDescription  = "X86 ACPI Driver for roOs",
    .pCompatible   = "x86,x86-acpi",
    .pVersion      = "2.0",
    .pDriverAttach = _acpiAttach
};

/** @brief ACPI driver controler instance */
static acpi_controler_t sDrvCtrl = {
    .detectedCPUCount         = 0,
    .detectedIOAPICCount      = 0,
    .detectedIntOverrideCount = 0,
    .detectedHpetCount        = 0,
    .pIoApicList              = NULL,
    .pLapicList               = NULL,
    .pIntOverrideList         = NULL,
    .pHpetList                = NULL,
};

/** @brief ACPI external driver instance */
static acpi_driver_t sAPIDriver = {
    .pGetLAPICCount       = _acpiGetLAPICCount,
    .pGetLAPICList        = _acpiGetLAPICList,
    .pGetLAPICBaseAddress = _acpiGetLAPICBaseAddress,
    .pGetIOAPICCount      = _acpiGetIOAPICCount,
    .pGetIOAPICList       = _acpiGetIOAPICList,
    .pGetHPETList         = _acpiGetHPETList,
    .pGetRemapedIrq       = _acpiGetRemapedIrq,
};

/*******************************************************************************
 * FUNCTIONS
 ******************************************************************************/

static OS_RETURN_E _acpiAttach(const fdt_node_t* pkFdtNode)
{
    const uintptr_t* pUintptrProp;
    size_t           propLen;
    OS_RETURN_E      retCode;
    uintptr_t        searchRangeStart;
    uintptr_t        searchRangeEnd;
    size_t           mapSize;
    uintptr_t        mapBase;
    uint64_t         signature;



    /* Get the reg: the range to search for the ACPI structure */
    pUintptrProp = fdtGetProp(pkFdtNode, ACPI_FDT_REGS_PROP, &propLen);
    if(pUintptrProp == NULL || propLen != sizeof(uintptr_t) * 2)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }

#ifdef ARCH_32_BITS
    searchRangeStart = FDTTOCPU32(*pUintptrProp);
    searchRangeEnd   = searchRangeStart + FDTTOCPU32(*(pUintptrProp + 1));
#elif defined(ARCH_64_BITS)
    searchRangeStart = FDTTOCPU64(*pUintptrProp);
    searchRangeEnd   = searchRangeStart + FDTTOCPU64(*(pUintptrProp + 1));
#else
    #error "Invalid architecture"
#endif
#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "ACPI Range start: 0x%p Range end: 0x%p",
           searchRangeStart,
           searchRangeEnd);
#endif
    /* Map the memory */
    mapBase = searchRangeStart & ~PAGE_SIZE_MASK;
    mapSize = ((searchRangeEnd - mapBase) + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

    searchRangeStart = (uintptr_t)memoryKernelMap((void*)mapBase,
                                                  mapSize,
                                                  MEMMGR_MAP_HARDWARE |
                                                  MEMMGR_MAP_KERNEL   |
                                                  MEMMGR_MAP_RO,
                                                  &retCode);
    if(retCode != OS_NO_ERR)
    {
        goto ATTACH_END;
    }
    /* Search for the ACPI table */
    mapBase = searchRangeStart;
    searchRangeEnd = searchRangeStart + mapSize;
    while(searchRangeStart < searchRangeEnd)
    {
        signature = *(uint64_t*)searchRangeStart;

         /* Checking the RSDP signature */
        if(signature == ACPI_RSDP_SIG)
        {
#if ACPI_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "RSDP found at 0x%p",
                   searchRangeStart);
#endif
            /* Parse RSDP */
            _acpiParseRSDP((rsdp_descriptor_t*)searchRangeStart);
            break;
        }

        searchRangeStart += sizeof(uintptr_t);
    }

    /* Unmap the memory */
    retCode = memoryKernelUnmap((void*)mapBase, mapSize);
    if(retCode != OS_NO_ERR)
    {
        syslog(SYSLOG_LEVEL_ERROR,
               MODULE_NAME,
               "Failed to unmap ACPI memory");
    }

    if(searchRangeStart >= searchRangeEnd)
    {
        retCode = OS_ERR_INCORRECT_VALUE;
        goto ATTACH_END;
    }
    else
    {
        /* Set the API driver */
        retCode = driverManagerSetDeviceData(pkFdtNode, &sAPIDriver);
    }
ATTACH_END:
#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "ACPI Initialization end");
#endif

    return retCode;
}

static void _acpiParseRSDP(const rsdp_descriptor_t* kpRsdpDesc)
{
    uint8_t              sum;
    uint8_t              i;
    uintptr_t            descAddr;
    size_t               toMap;
    rsdp_descriptor_2_t* pExtendedRsdp;
    OS_RETURN_E          errCode;

    ACPI_ASSERT(kpRsdpDesc != NULL,
                "Tried to parse a NULL RSDP",
                OS_ERR_NULL_POINTER);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Parsing RSDP at 0x%p",
           kpRsdpDesc);
#endif

    /* Verify checksum */
    sum = 0;
    for(i = 0; i < sizeof(rsdp_descriptor_t); ++i)
    {
        sum += ((uint8_t*)kpRsdpDesc)[i];
    }

    ACPI_ASSERT((sum & 0xFF) == 0,
                "RSDP Checksum failed",
                OS_ERR_INCORRECT_VALUE);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Revision %d detected",
           kpRsdpDesc->revision);
#endif

    /* ACPI version check */
    descAddr = (uintptr_t)NULL;
    if(kpRsdpDesc->revision == 0)
    {
        /* Map pages for RSDT */
        toMap = ((uintptr_t)kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK) +
                 sizeof(rsdt_descriptor_t);
        toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

        descAddr = (uintptr_t)kpRsdpDesc->rsdtAddress & ~PAGE_SIZE_MASK;
        descAddr = (uintptr_t)memoryKernelMap((void*)descAddr,
                                              toMap,
                                              MEMMGR_MAP_HARDWARE |
                                              MEMMGR_MAP_KERNEL   |
                                              MEMMGR_MAP_RO,
                                              &errCode);
        ACPI_ASSERT(errCode == OS_NO_ERR && descAddr != (uintptr_t)NULL,
                    "Failed to map RSDT",
                    errCode);
        _acpiParseRSDT((rsdt_descriptor_t*)
                       (descAddr | (kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK)));
    }
    else if(kpRsdpDesc->revision == 2)
    {
        pExtendedRsdp = (rsdp_descriptor_2_t*)kpRsdpDesc;
        sum = 0;

        for(i = 0; i < sizeof(rsdp_descriptor_2_t); ++i)
        {
            sum += ((uint8_t*)pExtendedRsdp)[i];
        }
        if(sum == 0)
        {
#ifdef ARCH_64_BITS
            if(pExtendedRsdp->xsdtAddress)
            {
                /* Map pages for XSDT */
                toMap = ((uintptr_t)pExtendedRsdp->xsdtAddress & PAGE_SIZE_MASK) +
                         sizeof(xsdt_descriptor_t);
                toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

                descAddr = (uintptr_t)pExtendedRsdp->xsdtAddress &
                           ~PAGE_SIZE_MASK;
                descAddr = (uintptr_t)memoryKernelMap((void*)descAddr,
                                                    toMap,
                                                    MEMMGR_MAP_HARDWARE |
                                                    MEMMGR_MAP_KERNEL   |
                                                    MEMMGR_MAP_RO,
                                                    &errCode);
                ACPI_ASSERT(errCode == OS_NO_ERR && descAddr != (uintptr_t)NULL,
                            "Failed to map XSDT",
                            errCode);
                _acpiParseXSDT((xsdt_descriptor_t*)(descAddr |
                               ((uintptr_t)pExtendedRsdp->xsdtAddress &
                                 PAGE_SIZE_MASK)));
            }
            else
#endif
            {
                /* Map pages for RSDT */
                toMap = ((uintptr_t)kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK) +
                         sizeof(rsdt_descriptor_t);
                toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

                descAddr = (uintptr_t)kpRsdpDesc->rsdtAddress &
                           ~PAGE_SIZE_MASK;
                descAddr = (uintptr_t)memoryKernelMap((void*)descAddr,
                                                    KERNEL_PAGE_SIZE * 2,
                                                    MEMMGR_MAP_HARDWARE |
                                                    MEMMGR_MAP_KERNEL   |
                                                    MEMMGR_MAP_RO,
                                                    &errCode);
                ACPI_ASSERT(errCode == OS_NO_ERR && descAddr != (uintptr_t)NULL,
                            "Failed to map RSDT",
                            errCode);

                _acpiParseRSDT((rsdt_descriptor_t*)
                                (descAddr |
                                 (kpRsdpDesc->rsdtAddress & PAGE_SIZE_MASK)));
            }
        }
    }
    else
    {
        ACPI_ASSERT(false,
                    "Unsupported ACPI version",
                    OS_ERR_NOT_SUPPORTED);
    }

    /* Unmap */
    if(descAddr != (uintptr_t)NULL)
    {
        errCode = memoryKernelUnmap((void*)descAddr, toMap);
        ACPI_ASSERT(errCode == OS_NO_ERR,
                    "Failed to unmap RSDT",
                    errCode);
    }
}

static void _acpiParseRSDT(const rsdt_descriptor_t* kpRrsdtPtr)
{
    uintptr_t      rangeBegin;
    uintptr_t      rangeEnd;
    uintptr_t      descAddr;
    size_t         toMap;
    OS_RETURN_E    errCode;
    uint8_t        i;
    int8_t         sum;
    acpi_header_t* pAddress;

    ACPI_ASSERT(kpRrsdtPtr != NULL,
                "Tried to parse a NULL RSDT",
                OS_ERR_NULL_POINTER);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Parsing RSDT at 0x%p",
           kpRrsdtPtr);
#endif

    /* Verify checksum */
    sum = 0;
    for(i = 0; i < kpRrsdtPtr->header.length; ++i)
    {
        sum += ((uint8_t*)kpRrsdtPtr)[i];
    }

    ACPI_ASSERT((sum & 0xFF) == 0,
                "RSDT Checksum failed",
                OS_ERR_INCORRECT_VALUE);

    ACPI_ASSERT(*((uint32_t*)kpRrsdtPtr->header.pSignature) == ACPI_RSDT_SIG,
                "Wrong RSDT Signature",
                OS_ERR_INCORRECT_VALUE);

    rangeBegin = (uintptr_t)(&kpRrsdtPtr->pDtPointers);
    rangeEnd   = ((uintptr_t)kpRrsdtPtr + kpRrsdtPtr->header.length);

    /* Parse each SDT of the RSDT */
    while(rangeBegin < rangeEnd)
    {
        pAddress = (acpi_header_t*)(uintptr_t)(*(uint32_t*)rangeBegin);

#if ACPI_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Detected SDT at 0x%p",
               pAddress);
#endif
        /* Map pages */
        toMap = ((uintptr_t)pAddress & PAGE_SIZE_MASK) + sizeof(acpi_header_t);
        toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

        descAddr = (uintptr_t)pAddress & ~PAGE_SIZE_MASK;
        descAddr = (uintptr_t)memoryKernelMap((void*)descAddr,
                                              toMap,
                                              MEMMGR_MAP_HARDWARE |
                                              MEMMGR_MAP_KERNEL   |
                                              MEMMGR_MAP_RO,
                                              &errCode);
        ACPI_ASSERT(errCode == OS_NO_ERR && descAddr != (uintptr_t)NULL,
                    "Failed to map DT",
                    errCode);

        _acpiParseDT((acpi_header_t*)
                     (descAddr | ((uintptr_t)pAddress & PAGE_SIZE_MASK)),
                     (uintptr_t)pAddress);

        /* Unmap */
        errCode = memoryKernelUnmap((void*)descAddr, toMap);
        ACPI_ASSERT(errCode == OS_NO_ERR,
                    "Failed to unmap DT",
                    errCode);

        rangeBegin += sizeof(uint32_t);
    }
}

#ifdef ARCH_64_BITS
static void _acpiParseXSDT(const xsdt_descriptor_t* kpXsdtPtr)
{
    uintptr_t      rangeBegin;
    uintptr_t      rangeEnd;
    uintptr_t      descAddr;
    size_t         toMap;
    OS_RETURN_E    errCode;
    uint8_t        i;
    int8_t         sum;
    acpi_header_t* pAddress;

    ACPI_ASSERT(kpXsdtPtr != NULL,
                "Tried to parse a NULL XSDT",
                OS_ERR_NULL_POINTER);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG, MODULE_NAME, "Parsing XSDT at 0x%p", kpXsdtPtr);
#endif
    /* Verify checksum */
    sum = 0;
    for(i = 0; i < kpXsdtPtr->header.length; ++i)
    {
        sum += ((uint8_t*)kpXsdtPtr)[i];
    }

    ACPI_ASSERT((sum & 0xFF) == 0,
                "XSDT Checksum failed",
                OS_ERR_INCORRECT_VALUE);

    ACPI_ASSERT(*((uint32_t*)kpXsdtPtr->header.pSignature) == ACPI_XSDT_SIG,
                "Wrong XSDT Signature",
                OS_ERR_INCORRECT_VALUE);

    rangeBegin = (uintptr_t)(&kpXsdtPtr->pDtPointers);
    rangeEnd   = ((uintptr_t)kpXsdtPtr + kpXsdtPtr->header.length);

    /* Parse each SDT of the RSDT */
    while(rangeBegin < rangeEnd)
    {
        pAddress = (acpi_header_t*)(*(uint64_t*)rangeBegin);

#if ACPI_DEBUG_ENABLED
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Detected SDT at 0x%p",
               pAddress);
#endif

        /* Map pages */
        toMap = ((uintptr_t)pAddress & PAGE_SIZE_MASK) + sizeof(acpi_header_t);
        toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

        descAddr = (uintptr_t)pAddress & ~PAGE_SIZE_MASK;
        descAddr = (uintptr_t)memoryKernelMap((void*)descAddr,
                                              toMap,
                                              MEMMGR_MAP_HARDWARE |
                                              MEMMGR_MAP_KERNEL   |
                                              MEMMGR_MAP_RO,
                                              &errCode);
        ACPI_ASSERT(errCode == OS_NO_ERR && descAddr != (uintptr_t)NULL,
                    "Failed to map DT x64",
                    errCode);

        _acpiParseDT((acpi_header_t*)
                     (descAddr | ((uintptr_t)pAddress & PAGE_SIZE_MASK)),
                     (uintptr_t)pAddress);

        /* Unmap */
        errCode = memoryKernelUnmap((void*)descAddr, toMap);
        ACPI_ASSERT(errCode == OS_NO_ERR,
                    "Failed to unmap DT",
                    errCode);

        rangeBegin += sizeof(uint64_t);
    }

}
#endif

static void _acpiParseDT(const acpi_header_t* kpHeader,
                         const uintptr_t      kPhysAddr)
{
    uintptr_t   descAddr;
    uintptr_t   descPtr;
    size_t      toMap;
    OS_RETURN_E errCode;

#if ACPI_DEBUG_ENABLED
    char        pSigStr[5];
#endif

    ACPI_ASSERT(kpHeader != NULL,
                "Tried to parse a NULL DT",
                OS_ERR_NULL_POINTER);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Parsing SDT at 0x%p",
           kpHeader);

    memcpy(pSigStr, kpHeader->pSignature, 4);
    pSigStr[4] = 0;

    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Signature: %s",
           pSigStr);
#endif

    /* Map memory */
    toMap = ((uintptr_t)kpHeader & PAGE_SIZE_MASK) + kpHeader->length;
    toMap = (toMap + PAGE_SIZE_MASK) & ~PAGE_SIZE_MASK;

    descAddr = (uintptr_t)kPhysAddr & ~PAGE_SIZE_MASK;
    descAddr = (uintptr_t)memoryKernelMap((void*)descAddr,
                                          toMap,
                                          MEMMGR_MAP_HARDWARE |
                                          MEMMGR_MAP_KERNEL   |
                                          MEMMGR_MAP_RO,
                                          &errCode);
    ACPI_ASSERT(errCode == OS_NO_ERR && descAddr != (uintptr_t)NULL,
                "Failed to remap DT",
                errCode);

    descPtr = descAddr | (kPhysAddr & PAGE_SIZE_MASK);
    if(*((uint32_t*)kpHeader->pSignature) == ACPI_FACP_SIG)
    {
        _acpiParseFADT((acpi_fadt_t*)descPtr);
    }
    else if(*((uint32_t*)kpHeader->pSignature) == ACPI_APIC_SIG)
    {
        _acpiParseMADT((acpi_madt_t*)descPtr);
    }
    else if(*((uint32_t*)kpHeader->pSignature) == ACPI_HPET_SIG)
    {
        _acpiParseHPET((acpi_hpet_desc_t*)descPtr);
    }
#if ACPI_DEBUG_ENABLED
    else
    {
        syslog(SYSLOG_LEVEL_DEBUG,
               MODULE_NAME,
               "Signature not supported: %s",
               pSigStr);
    }
#endif

    /* Unmap memory */
    errCode = memoryKernelUnmap((void*)descAddr, toMap);
    ACPI_ASSERT(errCode == OS_NO_ERR,
                "Failed to unmap DT",
                errCode);
}

static void _acpiParseFADT(const acpi_fadt_t* kpFadtPtr)
{
    int32_t  sum;
    uint32_t i;

    ACPI_ASSERT(kpFadtPtr != NULL,
                "Tried to parse a NULL FADT",
                OS_ERR_NULL_POINTER);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Parsing FADT at 0x%p",
           kpFadtPtr);
#endif

    /* Verify checksum */
    sum = 0;
    for(i = 0; i < kpFadtPtr->header.length; ++i)
    {
        sum += ((uint8_t*)kpFadtPtr)[i];
    }

    ACPI_ASSERT((sum & 0xFF) == 0,
                "FADT Checksum failed",
                OS_ERR_INCORRECT_VALUE);

    ACPI_ASSERT(*((uint32_t*)kpFadtPtr->header.pSignature) == ACPI_FACP_SIG,
                "FADT Signature comparison failed",
                OS_ERR_INCORRECT_VALUE);
}

static void _acpiParseMADT(const acpi_madt_t* kpMadtPtr)
{
    int32_t              sum;
    uint32_t             i;
    uintptr_t            madtEntry;
    uintptr_t            madtLimit;
    apic_header_t*       pHeader;
    lapic_node_t*        pLapicNode;
    lapic_node_t*        pLapicListCursor;
    io_apic_node_t*      pIOApicNode;
    io_apic_node_t*      pIOApicListCursor;
    int_override_node_t* pIntOverrideNode;
    int_override_node_t* pIntOverrideListCursor;

    ACPI_ASSERT(kpMadtPtr != NULL,
                "Tried to parse a NULL APIC",
                OS_ERR_NULL_POINTER);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Parsing APIC at 0x%p",
           kpMadtPtr);
#endif

    /* Verify checksum */
    sum = 0;
    for(i = 0; i < kpMadtPtr->header.length; ++i)
    {
        sum += ((uint8_t*)kpMadtPtr)[i];
    }

    ACPI_ASSERT((sum & 0xFF) == 0,
                "APIC checksum failed",
                OS_ERR_INCORRECT_VALUE);

    ACPI_ASSERT(*((uint32_t*)kpMadtPtr->header.pSignature) == ACPI_APIC_SIG,
                "Invalid APIC signature",
                OS_ERR_INCORRECT_VALUE);

    madtEntry = (uintptr_t)(kpMadtPtr + 1);
    madtLimit = ((uintptr_t)kpMadtPtr) + kpMadtPtr->header.length;

    /* Get the LAPIC address */
    sDrvCtrl.localApicAddress = kpMadtPtr->localApicAddr;

    while (madtEntry < madtLimit)
    {
        /* Get entry header */
        pHeader = (apic_header_t*)madtEntry;

        /* Check entry type */
        if(pHeader->type == APIC_TYPE_LOCAL_APIC)
        {

#if ACPI_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Found LAPIC: CPU #%d | ID #%d | FLAGS %x",
                   ((lapic_t*)madtEntry)->cpuId,
                   ((lapic_t*)madtEntry)->lapicId,
                   ((lapic_t*)madtEntry)->flags);
#endif

            if(sDrvCtrl.detectedCPUCount < SOC_CPU_COUNT)
            {
                /* Create new LAPIC node */
                pLapicNode = kmalloc(sizeof(lapic_node_t));
                ACPI_ASSERT(pLapicNode != NULL,
                            "Could not allocate node for new LAPIC",
                            OS_ERR_NO_MORE_MEMORY);

                /* Fill the descriptor */
                pLapicNode->lapic.lapicId = ((lapic_t*)madtEntry)->lapicId;
                pLapicNode->lapic.cpuId   = ((lapic_t*)madtEntry)->cpuId;
                pLapicNode->lapic.flags   = ((lapic_t*)madtEntry)->flags;

                /* Link the node */
                ADD_TO_LIST(sDrvCtrl.pLapicList, pLapicListCursor, pLapicNode);
                ++sDrvCtrl.detectedCPUCount;
            }
            else
            {
                syslog(SYSLOG_LEVEL_INFO,
                       MODULE_NAME,
                       "Exceeded CPU count (%u), ignoring CPU %d",
                       SOC_CPU_COUNT,
                       ((lapic_t*)madtEntry)->cpuId);
            }
        }
        else if(pHeader->type == APIC_TYPE_IO_APIC)
        {

#if ACPI_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Found IO-APIC ADDR 0x%p | ID #%d | GSIB %x",
                   ((io_apic_t*)madtEntry)->ioApicAddr,
                   ((io_apic_t*)madtEntry)->ioApicId,
                   ((io_apic_t*)madtEntry)->globalSystemInterruptBase);
#endif
            /* Create new IO APIC node */
            pIOApicNode = kmalloc(sizeof(io_apic_node_t));
            ACPI_ASSERT(pIOApicNode != NULL,
                        "Could not allocate node for new IO APIC",
                        OS_ERR_NO_MORE_MEMORY);

            /* Fill the descriptor */
            pIOApicNode->ioApic.ioApicId = ((io_apic_t*)madtEntry)->ioApicId;
            pIOApicNode->ioApic.ioApicAddr =
                ((io_apic_t*)madtEntry)->ioApicAddr;
            pIOApicNode->ioApic.globalSystemInterruptBase =
                ((io_apic_t*)madtEntry)->globalSystemInterruptBase;

            /* Link the node */
            ADD_TO_LIST(sDrvCtrl.pIoApicList, pIOApicListCursor, pIOApicNode);
            ++sDrvCtrl.detectedIOAPICCount;
        }
        else if(pHeader->type == APIC_TYPE_INTERRUPT_OVERRIDE)
        {

#if ACPI_DEBUG_ENABLED
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Found Interrupt override %d -> %d",
                   ((apic_interrupt_override_t*)madtEntry)->source,
                   ((apic_interrupt_override_t*)madtEntry)->interrupt);
#endif

            /* Create new IO APIC node */
            pIntOverrideNode = kmalloc(sizeof(int_override_node_t));
            ACPI_ASSERT(pIntOverrideNode != NULL,
                        "Could not allocate node for new interrupt override",
                        OS_ERR_NO_MORE_MEMORY);

            /* Fill the descriptor */
            pIntOverrideNode->intOverride.bus =
                ((apic_interrupt_override_t*)madtEntry)->bus;
            pIntOverrideNode->intOverride.source =
                ((apic_interrupt_override_t*)madtEntry)->source;
            pIntOverrideNode->intOverride.interrupt =
                ((apic_interrupt_override_t*)madtEntry)->interrupt;
            pIntOverrideNode->intOverride.flags =
                ((apic_interrupt_override_t*)madtEntry)->flags;

            /* Link the node */
            ADD_TO_LIST(sDrvCtrl.pIntOverrideList,
                        pIntOverrideListCursor,
                        pIntOverrideNode);
            ++sDrvCtrl.detectedIntOverrideCount;
        }
#if ACPI_DEBUG_ENABLED
        else
        {
            syslog(SYSLOG_LEVEL_DEBUG,
                   MODULE_NAME,
                   "Unknown APIC type %d",
                   pHeader->type);
        }
#endif
        madtEntry += pHeader->length;
    }
}

static void _acpiParseHPET(const acpi_hpet_desc_t* kpHpetPtr)
{
    int32_t      sum;
    uint32_t     i;
    hpet_node_t* pHpetNode;
    hpet_node_t* pHpetListCursor;

    ACPI_ASSERT(kpHpetPtr != NULL,
                "Tried to parse a NULL HPET",
                OS_ERR_NULL_POINTER);

#if ACPI_DEBUG_ENABLED
    syslog(SYSLOG_LEVEL_DEBUG,
           MODULE_NAME,
           "Parsing HPET at 0x%p",
           kpHpetPtr);
#endif

    /* Verify checksum */
    sum = 0;
    for(i = 0; i < kpHpetPtr->header.length; ++i)
    {
        sum += ((uint8_t*)kpHpetPtr)[i];
    }

    ACPI_ASSERT((sum & 0xFF) == 0,
                "HPET Checksum failed",
                OS_ERR_INCORRECT_VALUE);

    ACPI_ASSERT(*((uint32_t*)kpHpetPtr->header.pSignature) == ACPI_HPET_SIG,
                "HPET Signature comparison failed",
                OS_ERR_INCORRECT_VALUE);

    /* Create the new HPET node */
    pHpetNode = kmalloc(sizeof(hpet_node_t));
    ACPI_ASSERT(pHpetNode != NULL,
                "Could not allocate node for new HPET",
                OS_ERR_NO_MORE_MEMORY);

    /* Fill the descriptor */
    pHpetNode->hpet.hwRev = (kpHpetPtr->flags & HPET_FLAGS_HW_REV_MASK) >>
                             HPET_FLAGS_HW_REV_SHIFT;
    pHpetNode->hpet.comparatorCount = (kpHpetPtr->flags & HPET_FLAGS_CC_MASK) >>
                                       HPET_FLAGS_CC_SHIFT;
    pHpetNode->hpet.counterSize = (kpHpetPtr->flags & HPET_FLAGS_CS_MASK) >>
                                  HPET_FLAGS_CS_SHIFT;
    pHpetNode->hpet.legacyRepIrq = (kpHpetPtr->flags & HPET_FLAGS_IRQ_MASK) >>
                                   HPET_FLAGS_IRQ_SHIFT;
    pHpetNode->hpet.pciVendorId = kpHpetPtr->pciVendorId;
    pHpetNode->hpet.hpetNumber = kpHpetPtr->hpetNumber;
    pHpetNode->hpet.minimumTick = kpHpetPtr->minimumTick;
    pHpetNode->hpet.pageProtection = kpHpetPtr->pageProtection;
    pHpetNode->hpet.address = (uintptr_t)kpHpetPtr->address.address;
    pHpetNode->hpet.addressSpace = kpHpetPtr->address.addressSpace;
    pHpetNode->hpet.bitWidth = kpHpetPtr->address.bitWidth;
    pHpetNode->hpet.bitOffset = kpHpetPtr->address.bitOffset;
    pHpetNode->hpet.accessSize = kpHpetPtr->address.accessSize;

    /* Link the node */
    ADD_TO_LIST(sDrvCtrl.pHpetList, pHpetListCursor, pHpetNode);
    ++sDrvCtrl.detectedHpetCount;
}

static uint8_t _acpiGetLAPICCount(void)
{
    return sDrvCtrl.detectedCPUCount;
}

const lapic_node_t* _acpiGetLAPICList(void)
{
    return sDrvCtrl.pLapicList;
}

static uintptr_t _acpiGetLAPICBaseAddress(void)
{
    return sDrvCtrl.localApicAddress;
}

static uint8_t _acpiGetIOAPICCount(void)
{
    return sDrvCtrl.detectedIOAPICCount;
}

static const io_apic_node_t* _acpiGetIOAPICList(void)
{
    return sDrvCtrl.pIoApicList;
}

static const hpet_node_t* _acpiGetHPETList(void)
{
    return sDrvCtrl.pHpetList;
}

static uint32_t _acpiGetRemapedIrq(const uint32_t kIrqNumber)
{
    const int_override_node_t* kpOverride;
    uint32_t                   retValue;

    /* Search for the override */
    retValue = kIrqNumber;
    kpOverride = sDrvCtrl.pIntOverrideList;
    while(kpOverride != NULL)
    {
        if(kpOverride->intOverride.source == kIrqNumber)
        {
            retValue = kpOverride->intOverride.interrupt;
            break;
        }
        kpOverride = kpOverride->pNext;
    }

    /* If we did not find the interrupt, there is no redirection. */
    return retValue;
}

/***************************** DRIVER REGISTRATION ****************************/
DRIVERMGR_REG_FDT(sX86ACPIDriver);

/************************************ EOF *************************************/