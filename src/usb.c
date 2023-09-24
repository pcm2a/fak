#include "usb.h"
#include "ch552.h"
#include "math.h"

#include <string.h>

#define MIN(a, b) ((a > b) ? b : a)
#define MSB(u16) (u16 >> 8)
#define LSB(u16) (u16 & 0xFF)

#ifdef USB_MANUFACTURER_STR
#define USB_MANUFACTURER_STR_IDX 1
#else
#define USB_MANUFACTURER_STR_IDX 0
#endif

#ifdef USB_PRODUCT_STR
#define USB_PRODUCT_STR_IDX 2
#else
#define USB_PRODUCT_STR_IDX 0
#endif

#ifdef USB_SERIAL_NO_STR
#define USB_SERIAL_NO_STR_IDX 3
#else
#define USB_SERIAL_NO_STR_IDX 0
#endif

#if defined(USB_MANUFACTURER_STR) || defined(USB_PRODUCT_STR) || defined(USB_SERIAL_NO_STR)
#define USB_STRINGS_ENABLE
#endif

typedef struct {
    USB_CFG_DESCR cfg_descr;
    USB_ITF_DESCR itf_descr;
    USB_HID_DESCR hid_descr;
    USB_ENDP_DESCR endp1_descr;
    USB_ENDP_DESCR endp2_descr;
} USB_CFG1_DESCR;

__code uint8_t *p_usb_tx;
__xdata __at(XADDR_USB_TX_LEN) uint8_t usb_tx_len;
__bit usb_hid_protocol;

__xdata __at(XADDR_USB_EP0) uint8_t EP0_buffer[USB_EP0_SIZE];
__xdata __at(XADDR_USB_EP1O) uint8_t EP1O_buffer[USB_EP1_SIZE];
__xdata __at(XADDR_USB_EP1I) uint8_t EP1I_buffer[USB_EP1_SIZE];

__code USB_DEV_DESCR USB_DEVICE_DESCR = {
    .bLength = sizeof(USB_DEV_DESCR),
    .bDescriptorType = USB_DESCR_TYP_DEVICE,
    .bcdUSBL = 0x10,
    .bcdUSBH = 0x01,
    .bDeviceClass = 0,
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = USB_EP0_SIZE,
    .idVendorL = LSB(USB_VENDOR_ID),
    .idVendorH = MSB(USB_VENDOR_ID),
    .idProductL = LSB(USB_PRODUCT_ID),
    .idProductH = MSB(USB_PRODUCT_ID),
    .bcdDeviceL = LSB(USB_PRODUCT_VER),
    .bcdDeviceH = MSB(USB_PRODUCT_VER),
    .iManufacturer = USB_MANUFACTURER_STR_IDX,
    .iProduct = USB_PRODUCT_STR_IDX,
    .iSerialNumber = USB_SERIAL_NO_STR_IDX,
    .bNumConfigurations = 1
};

__code USB_CFG1_DESCR USB_CONFIG1_DESCR = {
    .cfg_descr = {
        .bLength = sizeof(USB_CFG_DESCR),
        .bDescriptorType = USB_DESCR_TYP_CONFIG,
        .wTotalLengthL = LSB(sizeof(USB_CONFIG1_DESCR)),
        .wTotalLengthH = MSB(sizeof(USB_CONFIG1_DESCR)),
        .bNumInterfaces = 1,
        .bConfigurationValue = 1,
        .iConfiguration = 0,
        .bmAttributes = (1 << 7),
        .bMaxPower = 50
    },
    .itf_descr = {
        .bLength = sizeof(USB_ITF_DESCR),
        .bDescriptorType = USB_DESCR_TYP_INTERF,
        .bInterfaceNumber = 0,
        .bAlternateSetting = 0,
        .bNumEndpoints = 2,
        .bInterfaceClass = USB_DEV_CLASS_HID,
        .bInterfaceSubClass = 1, // Boot interface
        .bInterfaceProtocol = 1, // Keyboard
        .iInterface = 0
    },
    .hid_descr = {
        .bLength = sizeof(USB_HID_DESCR),
        .bDescriptorType = USB_DESCR_TYP_HID,
        .bcdHIDL = 0x11,
        .bcdHIDH = 0x01,
        .bCountryCode = 0,
        .bNumDescriptors = 1,
        .bDescriptorTypeX = USB_DESCR_TYP_REPORT,
        .wDescriptorLengthL = LSB(sizeof(USB_HID_REPORT_DESCR)),
        .wDescriptorLengthH = MSB(sizeof(USB_HID_REPORT_DESCR))
    },
    .endp1_descr = {
        .bLength = sizeof(USB_ENDP_DESCR),
        .bDescriptorType = USB_DESCR_TYP_ENDP,
        .bEndpointAddress = USB_ENDP_DIR_MASK | 1, // IN 1
        .bmAttributes = USB_ENDP_TYPE_INTER,
        .wMaxPacketSizeL = LSB(USB_EP1_SIZE),
        .wMaxPacketSizeH = MSB(USB_EP1_SIZE),
        .bInterval = 1
    },
    .endp2_descr = {
        .bLength = sizeof(USB_ENDP_DESCR),
        .bDescriptorType = USB_DESCR_TYP_ENDP,
        .bEndpointAddress = 1, // OUT 1
        .bmAttributes = USB_ENDP_TYPE_INTER,
        .wMaxPacketSizeL = LSB(USB_EP1_SIZE),
        .wMaxPacketSizeH = MSB(USB_EP1_SIZE),
        .bInterval = 1
    }
};

__code uint8_t USB_HID_REPORT_DESCR[] = {
    0x05, 0x01,
    0x09, 0x06,
    0xA1, 0x01,
    0x05, 0x07,
    0x19, 0xE0,
    0x29, 0xE7,
    0x15, 0x00,
    0x25, 0x01,
    0x75, 0x01,
    0x95, 0x08,
    0x81, 0x02,
    0x95, 0x01,
    0x75, 0x08,
    0x81, 0x01,
    0x95, 0x05,
    0x75, 0x01,
    0x05, 0x08,
    0x19, 0x01,
    0x29, 0x05,
    0x91, 0x02,
    0x95, 0x01,
    0x75, 0x03,
    0x91, 0x01,
    0x95, 0x06,
    0x75, 0x08,
    0x15, 0x00,
    0x25, 0x65,
    0x05, 0x07,
    0x19, 0x00,
    0x29, 0x65,
    0x81, 0x00,
    0xC0
};

#ifdef USB_STRINGS_ENABLE
__code uint8_t USB_STR0_DESCR[] = {
    sizeof(USB_STR0_DESCR),
    USB_DESCR_TYP_STRING,
    0x09, 0x04
};

#ifdef USB_MANUFACTURER_STR
__code uint8_t USB_STR1_DESCR[] = {
    sizeof(USB_STR1_DESCR),
    USB_DESCR_TYP_STRING,
    USB_MANUFACTURER_STR
};
#endif

#ifdef USB_PRODUCT_STR
__code uint8_t USB_STR2_DESCR[] = {
    sizeof(USB_STR2_DESCR),
    USB_DESCR_TYP_STRING,
    USB_PRODUCT_STR
};
#endif

#ifdef USB_SERIAL_NO_STR
__code uint8_t USB_STR3_DESCR[] = {
    sizeof(USB_STR3_DESCR),
    USB_DESCR_TYP_STRING,
    USB_SERIAL_NO_STR
};
#endif
#endif

static void USB_EP0_tx() {
    UEP0_T_LEN = MIN(usb_tx_len, USB_EP0_SIZE);

    if (UEP0_T_LEN) {
        memcpy(EP0_buffer, p_usb_tx, UEP0_T_LEN);
        usb_tx_len -= UEP0_T_LEN;
        p_usb_tx += UEP0_T_LEN;
    }
}

inline static void USB_EP0_SETUP() {
    UEP0_CTRL = bUEP_R_TOG | bUEP_T_TOG;

    USB_SETUP_REQ *setupPacket = (USB_SETUP_REQ *) EP0_buffer;
    usb_tx_len = 0;

    switch (setupPacket->bRequest) {
        case USB_GET_DESCRIPTOR:
            switch (setupPacket->wValueH) {
                case USB_DESCR_TYP_DEVICE:
                    usb_tx_len = sizeof(USB_DEVICE_DESCR);
                    p_usb_tx = (__code uint8_t *) &USB_DEVICE_DESCR;
                    break;
                case USB_DESCR_TYP_CONFIG:
                    usb_tx_len = sizeof(USB_CONFIG1_DESCR);
                    p_usb_tx = (__code uint8_t *) &USB_CONFIG1_DESCR;
                    break;
#ifdef USB_STRINGS_ENABLE
                case USB_DESCR_TYP_STRING:
                    switch (setupPacket->wValueL) {
                        case 0:
                            usb_tx_len = sizeof(USB_STR0_DESCR);
                            p_usb_tx = (__code uint8_t *) &USB_STR0_DESCR;
                            break;
#ifdef USB_MANUFACTURER_STR
                        case 1:
                            usb_tx_len = sizeof(USB_STR1_DESCR);
                            p_usb_tx = (__code uint8_t *) &USB_STR1_DESCR;
                            break;
#endif
#ifdef USB_PRODUCT_STR
                        case 2:
                            usb_tx_len = sizeof(USB_STR2_DESCR);
                            p_usb_tx = (__code uint8_t *) &USB_STR2_DESCR;
                            break;
#endif
#ifdef USB_SERIAL_NO_STR
                        case 3:
                            usb_tx_len = sizeof(USB_STR3_DESCR);
                            p_usb_tx = (__code uint8_t *) &USB_STR3_DESCR;
                            break;
#endif
                    }
                    break;
#endif
                case USB_DESCR_TYP_REPORT:
                    usb_tx_len = sizeof(USB_HID_REPORT_DESCR);
                    p_usb_tx = (__code uint8_t *) &USB_HID_REPORT_DESCR;
                    break;
            }

            if (usb_tx_len) {
                if (usb_tx_len > setupPacket->wLengthL) usb_tx_len = setupPacket->wLengthL;
                UDEV_CTRL |= bUD_GP_BIT;
                USB_EP0_tx();
                return;
            }
            break;
        
        case USB_SET_ADDRESS:
            usb_tx_len = setupPacket->wValueL;
            UDEV_CTRL &= ~bUD_GP_BIT;
            return;
        
        case USB_SET_CONFIGURATION:
            USB_DEV_AD = (USB_DEV_AD & MASK_USB_ADDR) | (setupPacket->wValueL << 7); // bUDA_GP_BIT
            return;
        
        case USB_GET_CONFIGURATION:
            EP0_buffer[0] = USB_DEV_AD >> 7; // bUDA_GP_BIT
            UEP0_T_LEN = 1;
            return;

        case USB_GET_STATUS:
            EP0_buffer[0] = 0;
            EP0_buffer[1] = 0;
            UEP0_T_LEN = 2;
            return;
        
        case HID_GET_REPORT:
            if (setupPacket->bRequestType == (USB_REQ_TYP_IN | USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF)
             && setupPacket->wValueH == 1) {
                for (uint8_t i = 0; i < 8; i++) {
                    EP0_buffer[i] = EP1I_buffer[i];
                }
                UEP0_T_LEN = 8;
                return;
            }
            break;
        
        case HID_GET_PROTOCOL:
            if (setupPacket->bRequestType == (USB_REQ_TYP_IN | USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF)) {
                EP0_buffer[0] = usb_hid_protocol;
                UEP0_T_LEN = 1;
                return;
             }
             break;
        
        case HID_SET_PROTOCOL:
            if (setupPacket->bRequestType == (USB_REQ_TYP_OUT | USB_REQ_TYP_CLASS | USB_REQ_RECIP_INTERF)) {
                usb_hid_protocol = setupPacket->wValueL;
                return;
             }
             break;
    }

    UEP0_CTRL |= UEP_R_RES_STALL | UEP_T_RES_STALL;
}

inline static void USB_EP0_IN() {
    if (!usb_tx_len) return;
    
    if (UDEV_CTRL & bUD_GP_BIT) {
        // USB_GET_DESCRIPTOR
        USB_EP0_tx();
        UEP0_CTRL ^= bUEP_T_TOG;
    } else {
        // USB_SET_ADDRESS
        USB_DEV_AD = USB_DEV_AD & ~MASK_USB_ADDR | usb_tx_len;
    }
}

inline static void USB_EP0_OUT() {}

uint8_t USB_EP1I_read(uint8_t idx) {
    IE_USB = 0;
    uint8_t value = EP1I_buffer[idx];
    IE_USB = 1;
    return value;
}

void USB_EP1I_write(uint8_t idx, uint8_t value) {
    IE_USB = 0;
    EP1I_buffer[idx] = value;
    IE_USB = 1;
    USB_EP1I_ready_send();
}

inline void USB_EP1I_ready_send() {
    UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_ACK;
}

inline void USB_EP1I_send_now() {
    USB_EP1I_ready_send();
    while (!(UEP1_CTRL & UEP_T_RES_NAK));
}

inline static void USB_EP1_IN() {
    UEP1_CTRL = UEP1_CTRL & ~MASK_UEP_T_RES | UEP_T_RES_NAK;
}

inline static void USB_EP1_OUT() {}

#pragma save
#pragma nooverlay
void USB_interrupt() {
    if (UIF_TRANSFER) {
        UEP0_T_LEN = 0;
        uint8_t endp = USB_INT_ST & MASK_UIS_ENDP;

        switch (USB_INT_ST & MASK_UIS_TOKEN) {
            case UIS_TOKEN_SETUP:
                if (endp == 0) USB_EP0_SETUP();
                break;
            
            case UIS_TOKEN_IN:
                switch (endp) {
                    case 0: USB_EP0_IN(); break;
                    case 1: USB_EP1_IN(); break;
                }
                break;
            
            case UIS_TOKEN_OUT:
                switch (endp) {
                    case 0: USB_EP0_OUT(); break;
                    case 1: USB_EP1_OUT(); break;
                }
                break;
        }

        UIF_TRANSFER = 0;
    }
}
#pragma restore

void USB_init() {
    // Reset USB
    USB_CTRL |= bUC_RESET_SIE | bUC_CLR_ALL;
    USB_CTRL &= ~bUC_CLR_ALL;

    // Flush endpoints
    for (uint8_t i = 0; i < 8; i++) {
        EP0_buffer[i] = 0;
        EP1I_buffer[i] = 0;
        EP1O_buffer[i] = 0;
    }

    usb_tx_len = 0;
    usb_hid_protocol = 1;

    // Main init
    USB_CTRL = bUC_DEV_PU_EN | bUC_INT_BUSY | bUC_DMA_EN; 
    UDEV_CTRL = bUD_PD_DIS | bUD_PORT_EN;

    UEP0_T_LEN = 0;
    UEP0_DMA = XADDR_USB_EP0;

    UEP1_T_LEN = USB_EP1_SIZE;
    UEP1_DMA = XADDR_USB_EP1;
    UEP1_CTRL = bUEP_AUTO_TOG | UEP_T_RES_NAK | UEP_R_RES_ACK;
    UEP4_1_MOD = bUEP1_RX_EN | bUEP1_TX_EN;

    USB_INT_EN = bUIE_TRANSFER;
    IE_USB = 1;
}