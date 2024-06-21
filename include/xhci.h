
/* https://github.com/fysnet/FYSOS/blob/master/main/usb/utils/include/xhci.h */
#include "klib.h"
#include "pci.h"
#include "rtos.h"

#define XHCI_PCI_CLASS_CODE 0xC0330

#define XHCI_CAPS_CapLength 0x00
#define XHCI_CAPS_Reserved 0x01
#define XHCI_CAPS_IVersion 0x02
#define XHCI_CAPS_HCSParams1 0x04
#define XHCI_CAPS_HCSParams2 0x08
#define XHCI_CAPS_HCSParams3 0x0C
#define XHCI_CAPS_HCCParams1 0x10
#define XHCI_CAPS_DBOFF 0x14
#define XHCI_CAPS_RTSOFF 0x18
#define XHCI_CAPS_HCCParams2 0x1C

#define XHCI_OPS_USBCommand 0x00
#define XHCI_OPS_USBStatus 0x04
#define XHCI_OPS_USBPageSize 0x08
#define XHCI_OPS_USBDnctrl 0x14
#define XHCI_OPS_USBCrcr 0x18
#define XHCI_OPS_USBDcbaap 0x30
#define XHCI_OPS_USBConfig 0x38

#define XHCI_OPS_USBPortSt 0x400
#define XHCI_Port_PORTSC 0
#define XHCI_Port_PORTPMSC 4
#define XHCI_Port_PORTLI 8
#define XHCI_Port_PORTHLPMC 12

#define XHCI_PortUSB_CHANGE_BITS ((1 << 17) | (1 << 18) | (1 << 20) | (1 << 21) | (1 << 22))

#define XHCI_INTERRUPTER_PRIMARY 0

#define XHCI_INTERRUPTER_IMAN 0x00
#define XHCI_INTERRUPTER_IMOD 0x04
#define XHCI_INTERRUPTER_TAB_SIZE 0x08
#define XHCI_INTERRUPTER_RESV 0x0C
#define XHCI_INTERRUPTER_ADDRESS 0x10
#define XHCI_INTERRUPTER_DEQUEUE 0x18

#define xHCI_SPEED_FULL 1
#define xHCI_SPEED_LOW 2
#define xHCI_SPEED_HI 3
#define xHCI_SPEED_SUPER 4

#define xHCI_DIR_NO_DATA 0
#define xHCI_DIR_OUT 2
#define xHCI_DIR_IN 3

#define xHCI_DIR_OUT_B 0
#define xHCI_DIR_IN_B 1

#define xHCI_SLOT_CNTX 0
#define xHCI_CONTROL_EP 1
#define xHCI_EP1_OUT 2
#define xHCI_EP1_IN 3
#define xHCI_EP2_OUT 4
#define xHCI_EP2_IN 5
#define xHCI_EP3_OUT 6
#define xHCI_EP3_IN 7
#define xHCI_EP4_OUT 8
#define xHCI_EP4_IN 9
#define xHCI_EP5_OUT 10
#define xHCI_EP5_IN 11
#define xHCI_EP6_OUT 12
#define xHCI_EP6_IN 13
#define xHCI_EP7_OUT 14
#define xHCI_EP7_IN 15
#define xHCI_EP8_OUT 16
#define xHCI_EP8_IN 17
#define xHCI_EP9_OUT 18
#define xHCI_EP9_IN 19
#define xHCI_EP10_OUT 20
#define xHCI_EP10_IN 21
#define xHCI_EP11_OUT 22
#define xHCI_EP11_IN 23
#define xHCI_EP12_OUT 24
#define xHCI_EP12_IN 25
#define xHCI_EP13_OUT 26
#define xHCI_EP13_IN 27
#define xHCI_EP14_OUT 28
#define xHCI_EP14_IN 29
#define xHCI_EP15_OUT 30
#define xHCI_EP15_IN 31

#define xHCI_PROTO_INFO (1 << 0)
#define xHCI_PROTO_HSO (1 << 1)
#define xHCI_PROTO_HAS_PAIR (1 << 2)
#define xHCI_PROTO_ACTIVE (1 << 3)
#define xHCI_PROTO_USB2 0
#define xHCI_PROTO_USB3 1

#define xHCI_PORT_INFO_FLAGS(x) ((port_info[(x)].offset_count_flags0_flags1[2] | ((port_info[(x)].offset_count_flags0_flags1[3] << 8))
#define xHCI_IS_USB3_PORT(x) ((xHCI_PORT_INFO_FLAGS(x) & xHCI_PROTO_INFO) == xHCI_PROTO_USB3)
#define xHCI_IS_USB2_PORT(x) ((xHCI_PORT_INFO_FLAGS(x) & xHCI_PROTO_INFO) == xHCI_PROTO_USB2)
#define xHCI_IS_USB2_HSO(x) ((xHCI_PORT_INFO_FLAGS(x) & xHCI_PROTO_HSO) == xHCI_PROTO_HSO)
#define xHCI_HAS_PAIR(x) ((xHCI_PORT_INFO_FLAGS(x) & xHCI_PROTO_HAS_PAIR) == xHCI_PROTO_HAS_PAIR)
#define xHCI_IS_ACTIVE(x) ((xHCI_PORT_INFO_FLAGS(x) & xHCI_PROTO_ACTIVE) == xHCI_PROTO_ACTIVE)

#define XHCI_TRB_ID_LINK 6
#define XHCI_TRB_ID_NOOP 8

struct XHCI_PORT_STATUS
{
    os_uint32 portsc;
    os_uint32 portpmsc;
    os_uint32 portli;
    os_uint32 porthlpmc;
};

#define XHCI_xECP_ID_NONE 0
#define XHCI_xECP_ID_LEGACY 1
#define XHCI_xECP_ID_PROTO 2
#define XHCI_xECP_ID_POWER 3
#define XHCI_xECP_ID_VIRT 4
#define XHCI_xECP_ID_MESS 5
#define XHCI_xECP_ID_LOCAL 6
#define XHCI_xECP_ID_DEBUG 10
#define XHCI_xECP_ID_EXT_MESS 17

#define XHCI_xECP_LEGACY_TIMEOUT 10 /* 10 milliseconds*/
#define XHCI_xECP_LEGACY_BIOS_OWNED (1 << 16)
#define XHCI_xECP_LEGACY_OS_OWNED (1 << 24)
#define XHCI_xECP_LEGACY_OWNED_MASK (XHCI_xECP_LEGACY_BIOS_OWNED | XHCI_xECP_LEGACY_OS_OWNED)
struct XHCI_xECP_LEGACY
{
    os_uint32 volatile id_next_owner_flags;
    os_uint32 volatile cntrl_status;
};

struct XHCI_xECP_PROTO
{
    os_uint8 id_next_minor_major[4];
    os_uint32 name;
    os_uint8 offset_count_flags0_flags1[4];
};

#define MAX_CONTEXT_SIZE 64                   /* Max Context size in bytes*/
#define MAX_SLOT_SIZE (MAX_CONTEXT_SIZE * 32) /*Max Total Slot size in bytes*/

/*Slot State*/
#define SLOT_STATE_DISABLED_ENABLED 0
#define SLOT_STATE_DEFAULT 1
#define SLOT_STATE_ADRESSED 2
#define SLOT_STATE_CONFIGURED 3

struct xHCI_SLOT_CONTEXT
{
    os_uint8 data[50];
    /* FIXME
    unsigned entries;
    os_uint8    hub;
    os_uint8    mtt;
    unsigned speed;
    os_uint32   route_string;
    unsigned num_ports;
    unsigned rh_port_num;
    unsigned max_exit_latency;
    unsigned int_target;
    unsigned ttt;
    unsigned tt_port_num;
    unsigned tt_hub_slot_id;
    unsigned slot_state;
    unsigned device_address; */
};

/*EP State*/
#define EP_STATE_DISABLED 0
#define EP_STATE_RUNNING 1
#define EP_STATE_HALTED 2
#define EP_STATE_STOPPED 3
#define EP_STATE_ERROR 4

struct xHCI_EP_CONTEXT
{
    os_uint8 data[51];
    /* FIXME
    unsigned interval;
    os_uint8    lsa;
    unsigned max_pstreams;
    unsigned mult;
    unsigned ep_state;
    unsigned max_packet_size;
    unsigned max_burst_size;
    os_uint8    hid;
    unsigned ep_type;
    unsigned cerr;
    os_uint32   tr_dequeue_pointer[2];
    os_uint8    dcs;
    unsigned max_esit_payload;
    unsigned average_trb_len;*/
};

struct xHCI_TRB
{
    os_uint32 param[2];
    os_uint32 status;
    os_uint32 command;
};

/*event ring specification*/
struct xHCI_EVENT_SEG_TABLE
{
    os_uint32 addr[2];
    os_uint32 size;
    os_uint32 resv;
};

#define XHCI_DIR_EP_OUT 0
#define XHCI_DIR_EP_IN 1
#define XHCI_GET_DIR(x) (((x) & (1 << 7)) >> 7)

#define TRB_GET_STYPE(x) (((x) & (0x1F << 16)) >> 16)
#define TRB_SET_STYPE(x) (((x) & 0x1F) << 16)
#define TRB_GET_TYPE(x) (((x) & (0x3F << 10)) >> 10)
#define TRB_SET_TYPE(x) (((x) & 0x3F) << 10)
#define TRB_GET_COMP_CODE(x) (((x) & (0x7F << 24)) >> 24)
#define TRB_SET_COMP_CODE(x) (((x) & 0x7F) << 24)
#define TRB_GET_SLOT(x) (((x) & (0xFF << 24)) >> 24)
#define TRB_SET_SLOT(x) (((x) & 0xFF) << 24)
#define TRB_GET_TDSIZE(x) (((x) & (0x1F << 17)) >> 17)
#define TRB_SET_TDSIZE(x) (((x) & 0x1F) << 17)
#define TRB_GET_EP(x) (((x) & (0x1F << 16)) >> 16)
#define TRB_SET_EP(x) (((x) & 0x1F) << 16)

#define TRB_GET_TARGET(x) (((x) & (0x3FF << 22)) >> 22)
#define TRB_GET_TX_LEN(x) ((x) & 0x1FFFF)
#define TRB_GET_TOGGLE(x) (((x) & (1 << 1)) >> 1)

#define TRB_DC(x) (((x) & (1 << 9)) >> 9)
#define TRB_IS_IMMED_DATA(x) (((x) & (1 << 6)) >> 6)
#define TRB_IOC(x) (((x) & (1 << 5)) >> 5)
#define TRB_CHAIN(x) (((x) & (1 << 4)) >> 4)
#define TRB_SPD(x) (((x) & (1 << 2)) >> 2)
#define TRB_TOGGLE(x) (((x) & (1 << 1)) >> 1)

#define TRB_CYCLE_ON (1 << 0)
#define TRB_CYCLE_OFF (0 << 0)

#define TRB_TOGGLE_CYCLE_ON (1 << 1)
#define TRB_TOGGLE_CYCLE_OFF (0 << 1)

#define TRB_CHAIN_ON (1 << 4)
#define TRB_CHAIN_OFF (0 << 4)

#define TRB_IOC_ON (1 << 5)
#define TRB_IOC_OFF (0 << 5)

#define TRB_LINK_CMND (TRB_SET_TYPE(LINK) | TRB_IOC_OFF | TRB_CHAIN_OFF | TRB_TOGGLE_CYCLE_OFF | TRB_CYCLE_ON)

#define NORMAL 1
#define SETUP_STAGE 2
#define DATA_STAGE 3
#define STATUS_STAGE 4
#define ISOCH 5
#define LINK 6
#define EVENT_DATA 7
#define NO_OP 8
#define ENABLE_SLOT 9
#define DISABLE_SLOT 10
#define ADDRESS_DEVICE 11
#define CONFIG_EP 12
#define EVALUATE_CONTEXT 13
#define RESET_EP 14
#define STOP_EP 15
#define SET_TR_DEQUEUE 16
#define RESET_DEVICE 17
#define FORCE_EVENT 18
#define DEG_BANDWIDTH 19
#define SET_LAT_TOLERANCE 20
#define GET_PORT_BAND 21
#define FORCE_HEADER 22
#define NO_OP_CMD 23
#define TRANS_EVENT 32
#define COMMAND_COMPLETION 33
#define PORT_STATUS_CHANGE 34
#define BANDWIDTH_REQUEST 35
#define DOORBELL_EVENT 36
#define HOST_CONTROLLER_EVENT 37
#define DEVICE_NOTIFICATION 38
#define MFINDEX_WRAP 39

#define TRB_SUCCESS 1
#define DATA_BUFFER_ERROR 2
#define BABBLE_DETECTION 3
#define TRANSACTION_ERROR 4
#define TRB_ERROR 5
#define STALL_ERROR 6
#define RESOURCE_ERROR 7
#define BANDWIDTH_ERROR 8
#define NO_SLOTS_ERROR 9
#define INVALID_STREAM_TYPE 10
#define SLOT_NOT_ENABLED 11
#define EP_NOT_ENABLED 12
#define SHORT_PACKET 13
#define RING_UNDERRUN 14
#define RING_OVERRUN 15
#define VF_EVENT_RING_FULL 16
#define PARAMETER_ERROR 17
#define BANDWITDH_OVERRUN 18
#define CONTEXT_STATE_ERROR 19
#define NO_PING_RESPONSE 20,
#define EVENT_RING_FULL 21
#define INCOMPATIBLE_DEVICE 22
#define MISSED_SERVICE 23
#define COMMAND_RING_STOPPED 24
#define COMMAND_ABORTED 25
#define STOPPED 26
#define STOPPER_LENGTH_ERROR 27
#define RESERVED 28
#define ISOCH_BUFFER_OVERRUN 29
#define EVERN_LOST 32
#define UNDEFINED 33
#define INVALID_STREAM_ID 34,
#define SECONDARY_BANDWIDTH 35,
#define SPLIT_TRANSACTION 36

#define XHCI_IRQ_DONE (1 << 31)
