#include "ioembc1000.h"
#include <stdbool.h>
#include <map>
#include <list>
#include <string>
#include <stdio.h>


#ifdef Q_OS_WIN
#include <windows.h>
typedef void	(__stdcall *lpOut32)(short, short);		// тип указатель на функцию вывода
typedef short	(__stdcall *lpInp32)(short);			// тип указатель на функцию ввода
typedef BOOL	(__stdcall *lpIsInpOutDriverOpen)(void);        // тип указатель на функцию готовности драйвера
typedef BOOL	(__stdcall *lpIsXP64Bit)(void);			// тип указатель на функцию проверки разрядности

static lpOut32 gfpOut32                             = NULL;	// указатель на функцию вывода
static lpInp32 gfpInp32                             = NULL;	// указатель на функцию ввода
static lpIsInpOutDriverOpen gfpIsInpOutDriverOpen   = NULL;	// указатель на функцию готовности драйвера
static lpIsXP64Bit gfpIsXP64Bit                     = NULL;	// указатель на функцию проверки разрядности

int IsDriverReady()
{
    return (gfpIsInpOutDriverOpen) ? gfpIsInpOutDriverOpen() : FALSE;
}

int iopl(int)
{
    static int success = -1;
    if(success >= 0)
        return success;

    HINSTANCE hDLL = LoadLibrary(
                     #ifdef Q_OS_WIN64
                         "inpoutx64.dll"
                     #else
                         "inpout32.dll"
                     #endif
                         );

    if(hDll) {
        gfpOut32                = (lpOut32)                 GetProcAddress(hDLL, "Out32");
        gfpInp32                = (lpInp32)                 GetProcAddress(hDLL, "Inp32");
        gfpIsInpOutDriverOpen   = (lpIsInpOutDriverOpen)    GetProcAddress(hDLL, "IsInpOutDriverOpen");
        gfpIsXP64Bit            = (lpIsXP64Bit)             GetProcAddress(hDLL, "IsXP64Bit");

        if(IsDriverReady())
            success = 0;
    }

    return success;
}

int inb(uint16_t port)
{
    short ret = 0;
    if (IsDriverReady())
    {
        ret = gfpInp32(port);
    }
    return ret;
}

void outb(uint16_t value, uint16_t port)
{
    if (IsDriverReady())
    {
        gfpOut32(port, value);
    }
}
#else
    #include <sys/io.h>
    #include <unistd.h>
#endif



namespace embc {
    namespace io {
        namespace bus {
            uint16_t base_bus_addr = 0x500;
#define	SMBHSTSTS           0x00	// SMBus Host  Status Register Offset
#define	SMBHSTSTS_BUSY		0x01	// SMBus Host -> 0000-0001 Busy
#define	SMBHSTSTS_INTR		0x02	// SMBus Host -> 0000-0010 Interrupt / complection
#define	SMBHSTSTS_ERROR		0x04	// SMBus Host -> 0000-0100 Error
#define	SMBHSTSTS_COLLISION	0x08	// SMBus Host -> 0000-1000 Collistion
#define	SMBHSTSTS_FAILED	0x10	// SMBus Host -> 0001-0000 Failed

#define	SMBHSTCNT           0x02	// SMBus Host Contorl Register Offset
#define	SMBHSTCNT_KILL		0x02	// SMBus Host Contorl -> 0000 0010 Kill
#define	SMBHSTCNT_QUICK		0x00	// SMBus Host Contorl -> 0000 0000 quick (default)
#define	SMBHSTCNT_SENDRECV	0x04	// SMBus Host Contorl -> 0000 0100 Byte
#define	SMBHSTCNT_BYTE		0x08	// SMBus Host Contorl -> 0000 1000 Byte Data
#define	SMBHSTCNT_WORD		0x0c	// SMBus Host Contorl -> 0000 1100 Word Data
#define	SMBHSTCNT_BLOCK		0x14	// SMBus Host Contorl -> 0001 0100 Block
#define	SMBHSTCNT_START		0x40	// SMBus Host Contorl -> 0100 0000 Start

#define	SMBHSTCMD           0x03	// SMBus Host Command 		Register Offset
#define	SMBHSTADD           0x04	// SMBus Host Address		Register Offset
#define	SMBHSTDAT0          0x05	// SMBus Host Data0		Register Offset
            // SMBus Host Block Counter 	Register Offset
#define	SMBHSTDAT1          0x06	// SMBus Host Data1		Register Offset
#define	SMBBLKDAT           0x07	// SMBus Host Block	Data	Register Offset

            //	Register of VIA only
#define	SMBHSLVSTS          0x01	// SMBus Slave Status Register Offset

#define	SMBSLVCNT           0x08	// SMBus Slave  Control
#define	SMBSHDWCMD          0x09	// SMBus Shadow Command
#define	SMBSLVEVT           0x0a	// SMBus Slave  Event
#define	SMBSLVDAT           0x0c	// SMBus Slave  Data

            // SMBus Bus Status Code
#define SMBUS_OK            0x0     // SMBUS OK
#define SMBUS_BUSY          0x1     // SMBUS BUSY
#define SMBUS_INT           0x2     // SMBUS INTR
#define SMBUS_ERROR         0x4     // SMBUS ERROR

#define	SMBUS_TIMEOUT		100



            //----------------------------------------------------------------------------------
            //	PCI Device Read/Write I/O
            //----------------------------------------------------------------------------------
#define 	PCI_CONFIG_ADDR					0xCF8
#define 	PCI_CONFIG_DATA					0xCFC
            //----------------------------------------------------------------------------------
            //	Intel ICH4 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define 	INTEL_ICH4_VENDOR_ID				0x8086
#define 	INTEL_ICH4_DEVICE_ID				0x24C0
#define 	INTEL_ICH4_SMBUS_VENDOR_ID			0x8086
#define 	INTEL_ICH4_SMBUS_DEVICE_ID			0x24C3
            //----------------------------------------------------------------------------------
#define     ICH4_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define     ICH4_SMBUS_HOST_CONFIGURE			0x40
#define     ICH4_SMBUS_HOST_HST_EN				0x01
#define     ICH4_SMBUS_HOST_SMI_EN				0x02
#define     ICH4_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel 945 ICH7 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_ICH7_VENDOR_ID				0x8086
#define		INTEL_ICH7_DEVICE_ID				0x2448

#define		INTEL_ICH7_SMBUS_VENDOR_ID			0x8086
#define		INTEL_ICH7_SMBUS_DEVICE_ID			0x27DA
            //----------------------------------------------------------------------------------
#define		ICH7_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		ICH7_SMBUS_HOST_CONFIGURE			0x40
#define		ICH7_SMBUS_HOST_HST_EN				0x01
#define		ICH7_SMBUS_HOST_SMI_EN				0x02
#define		ICH7_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel 510&525 ICH8 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_ICH8_VENDOR_ID				0x8086
#define		INTEL_ICH8_DEVICE_ID				0x244E

#define		INTEL_ICH8_SMBUS_VENDOR_ID			0x8086
#define		INTEL_ICH8_SMBUS_DEVICE_ID			0x283E
            //----------------------------------------------------------------------------------
#define		ICH8_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		ICH8_SMBUS_HOST_CONFIGURE			0x40
#define		ICH8_SMBUS_HOST_HST_EN				0x01
#define		ICH8_SMBUS_HOST_SMI_EN				0x02
#define		ICH8_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel 275 ICH10 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_ICH10_VENDOR_ID				0x8086
#define		INTEL_ICH10_DEVICE_ID				0x244E

#define		INTEL_ICH10_SMBUS_VENDOR_ID			0x8086
#define		INTEL_ICH10_SMBUS_DEVICE_ID			0x3A30
            //----------------------------------------------------------------------------------
#define		ICH10_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		ICH10_SMBUS_HOST_CONFIGURE			0x40
#define		ICH10_SMBUS_HOST_HST_EN				0x01
#define		ICH10_SMBUS_HOST_SMI_EN				0x02
#define		ICH10_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel NM10 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_NM10_VENDOR_ID				0x8086
#define		INTEL_NM10_DEVICE_ID				0x2448

#define		INTEL_NM10_SMBUS_VENDOR_ID			0x8086
#define		INTEL_NM10_SMBUS_DEVICE_ID			0x27DA
            //----------------------------------------------------------------------------------
#define		NM10_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		NM10_SMBUS_HOST_CONFIGURE			0x40
#define		NM10_SMBUS_HOST_HST_EN				0x01
#define		NM10_SMBUS_HOST_SMI_EN				0x02
#define		NM10_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel QM67 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_QM67_SMBUS_VENDOR_ID			0x8086
#define		INTEL_QM67_SMBUS_DEVICE_ID			0x1C22
            //----------------------------------------------------------------------------------
#define		QM67_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		QM67_SMBUS_HOST_CONFIGURE			0x40
#define		QM67_SMBUS_HOST_HST_EN				0x01
#define		QM67_SMBUS_HOST_SMI_EN				0x02
#define		QM67_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel QM77 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_QM77_SMBUS_VENDOR_ID			0x8086
#define		INTEL_QM77_SMBUS_DEVICE_ID			0x1E22
            //----------------------------------------------------------------------------------
#define		QM77_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		QM77_SMBUS_HOST_CONFIGURE			0x40
#define		QM77_SMBUS_HOST_HST_EN				0x01
#define		QM77_SMBUS_HOST_SMI_EN				0x02
#define		QM77_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel HM65 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_HM65_SMBUS_VENDOR_ID			0x8086
#define		INTEL_HM65_SMBUS_DEVICE_ID			0x1C22
            //----------------------------------------------------------------------------------
#define		HM65_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		HM65_SMBUS_HOST_CONFIGURE			0x40
#define		HM65_SMBUS_HOST_HST_EN				0x01
#define		HM65_SMBUS_HOST_SMI_EN				0x02
#define		HM65_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel HM76 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_HM76_SMBUS_VENDOR_ID			0x8086
#define		INTEL_HM76_SMBUS_DEVICE_ID			0x1E22
            //----------------------------------------------------------------------------------
#define		HM76_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		HM76_SMBUS_HOST_CONFIGURE			0x40
#define		HM76_SMBUS_HOST_HST_EN				0x01
#define		HM76_SMBUS_HOST_SMI_EN				0x02
#define		HM76_SMBUS_HOST_I2C_EN				0x04

            //----------------------------------------------------------------------------------
            //	Intel Bay Trail-I SoC Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_SOC_SMBUS_VENDOR_ID			0x8086
#define		INTEL_SOC_SMBUS_DEVICE_ID			0x0F12
            //----------------------------------------------------------------------------------
#define		SOC_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		SOC_SMBUS_HOST_CONFIGURE			0x40
#define		SOC_SMBUS_HOST_HST_EN				0x01
#define		SOC_SMBUS_HOST_SMI_EN				0x02
#define		SOC_SMBUS_HOST_I2C_EN				0x04

            //	Intel Apollo Lake SoC Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_Apollo_Lake_SOC_SMBUS_VENDOR_ID			0x8086
#define		INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID			0x5AD4
            //----------------------------------------------------------------------------------
#define		Apollo_Lake_SOC_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		Apollo_Lake_SOC_SMBUS_HOST_CONFIGURE			0x40
#define		Apollo_Lake_SOC_SMBUS_HOST_HST_EN				0x01
#define		Apollo_Lake_SOC_SMBUS_HOST_SMI_EN				0x02
#define		Apollo_Lake_SOC_SMBUS_HOST_I2C_EN				0x04

            //20170829 Add Sky_Lake by Tracy↓↓↓
            //----------------------------------------------------------------------------------
            //	Intel Sky Lake SoC Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_Sky_Lake_SOC_SMBUS_VENDOR_ID			0x8086
#define		INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID			0xA123
            //----------------------------------------------------------------------------------
#define		Sky_Lake_SOC_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		Sky_Lake_SOC_SMBUS_HOST_CONFIGURE			0x40
#define		Sky_Lake_SOC_SMBUS_HOST_HST_EN				0x01
#define		Sky_Lake_SOC_SMBUS_HOST_SMI_EN				0x02
#define		Sky_Lake_SOC_SMBUS_HOST_I2C_EN				0x04
            //20170829 Add Sky_Lake by Tracy↑↑↑

            //20170829 Add Sky_Lake-U by Tracy↓↓↓
            //----------------------------------------------------------------------------------
            //	Intel Sky Lake SoC Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define		INTEL_Sky_Lake_U_SOC_SMBUS_VENDOR_ID			0x8086
#define		INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID			0x9D23
            //----------------------------------------------------------------------------------
#define		Sky_Lake_U_SOC_SMBUS_HOST_IOBASE				0x20
            //----------------------------------------------------------------------------------
#define		Sky_Lake_U_SOC_SMBUS_HOST_CONFIGURE				0x40
#define		Sky_Lake_U_SOC_SMBUS_HOST_HST_EN				0x01
#define		Sky_Lake_U_SOC_SMBUS_HOST_SMI_EN				0x02
#define		Sky_Lake_U_SOC_SMBUS_HOST_I2C_EN				0x04
            //20170829 Add Sky_Lake-U by Tracy↑↑↑

            //----------------------------------------------------------------------------------
            //	VIA VT8237 Vendor ID & Device ID
            //----------------------------------------------------------------------------------
#define 	VIA_VT8237_BUS_CTRL_VENDOR_ID			0x1106
#define 	VIA_VT8237_BUS_CTRL_DEVICE_ID			0x3227
            //----------------------------------------------------------------------------------
#define		VT8237_SMBUS_HOST_IOBASE			0xD0
            //----------------------------------------------------------------------------------
#define		VT8237_SMBUS_HOST_CONFIGURE			0xD2
#define		VT8237_SMBUS_HOST_CONTROLER_ENABLE		0x01
#define		VT8237_SMBUS_HOST_INTERRUPT_ENABLE		0x02
#define		VT8237_SMBUS_HOST_INTERRUPT_TYPE		0x08
            //----------------------------------------------------------------------------------
#define 	VIA_CX700M_BUS_CTRL_VENDOR_ID			0x1106
#define 	VIA_CX700M_BUS_CTRL_DEVICE_ID			0x8324
            //----------------------------------------------------------------------------------
#define		CX700M_SMBUS_HOST_IOBASE			0xD0
            //----------------------------------------------------------------------------------
#define		CX700M_SMBUS_HOST_CONFIGURE			0xD2
#define		CX700M_SMBUS_HOST_CONTROLER_ENABLE		0x01
#define		CX700M_SMBUS_HOST_INTERRUPT_ENABLE		0x02
#define		CX700M_SMBUS_HOST_INTERRUPT_TYPE		0x08
#define		CX700M_SMBUS_HOST_CLOCK				0x04
            //----------------------------------------------------------------------------------
            //	VIA VX900 Vendor ID / Devices ID & Bus-Specific configuration register
            //----------------------------------------------------------------------------------
#define 	VIA_VX900_BUS_CTRL_VENDOR_ID			0x1106
#define 	VIA_VX900_BUS_CTRL_DEVICE_ID			0x8410
#define		VX900_SMBUS_HOST_IOBASE				0xD0
#define		VX900_SMBUS_HOST_CONFIGURE			0xD2
#define		VX900_SMBUS_HOST_CONTROLER_ENABLE		0x01

            auto PCI_read() -> uint32_t
            {
                uint32_t dwAddrVal;

                dwAddrVal=inl(PCI_CONFIG_DATA);
                usleep(10);
                return dwAddrVal;
            };

            auto PCI_write(uint32_t dwDataVal) -> void
            {
                outl(dwDataVal,PCI_CONFIG_ADDR);
                usleep(10);
            };

            struct device_i {
                device_i(uint16_t vendor_id, uint16_t device_id, uint32_t pci_base, uint8_t offset_bus, uint16_t mask_bus_addr, uint8_t offset_conf, uint8_t flag_isenable)
                    : vendor_id(vendor_id),
                      device_id(device_id),
                      pci_base(pci_base),
                      offset_bus(offset_bus),
                      mask_bus_addr(mask_bus_addr),
                      offset_conf(offset_conf),
                      fenable(flag_isenable)
                {

                }

                device_i()
                    : vendor_id(0),
                      device_id(0),
                      pci_base(0),
                      offset_bus(0),
                      mask_bus_addr(0),
                      offset_conf(0),
                      fenable(0)
                {

                }

                uint16_t vendor_id;
                uint16_t device_id;
                uint32_t pci_base;
                uint8_t  offset_bus;
                uint16_t mask_bus_addr;
                uint8_t  offset_conf;
                uint8_t  fenable;

                uint16_t bus_address() {
                    uint16_t ba = 0;

                    if(pci_base) {
                        PCI_write(pci_base + offset_bus);
                        ba = (uint16_t)PCI_read() & mask_bus_addr;
                    }

                    return ba;
                }

                uint32_t config_address() {
                    return pci_base ? pci_base + offset_conf : 0;
                }

                bool isEnabled() {
                    bool enable = false;

                    uint32_t ca = config_address();
                    if(ca)
                    {
                        PCI_write(ca);
                        enable = PCI_read() & fenable;
                    }
                    return enable;
                }
            };

            std::map<uint16_t, device_i> devices_spr = {
                { VIA_VT8237_BUS_CTRL_DEVICE_ID,        device_i(
                                                            VIA_VT8237_BUS_CTRL_VENDOR_ID,
                                                            0,
                                                            VIA_VT8237_BUS_CTRL_DEVICE_ID,
                                                            VT8237_SMBUS_HOST_IOBASE,
                                                            0xff00,
                                                            VT8237_SMBUS_HOST_CONFIGURE,
                                                            VT8237_SMBUS_HOST_CONTROLER_ENABLE
                                                        )},

                { VIA_CX700M_BUS_CTRL_DEVICE_ID,        device_i(
                                                            VIA_CX700M_BUS_CTRL_VENDOR_ID,
                                                            0,
                                                            VIA_CX700M_BUS_CTRL_DEVICE_ID,
                                                            CX700M_SMBUS_HOST_IOBASE,
                                                            0xff00,
                                                            CX700M_SMBUS_HOST_CONFIGURE,
                                                            CX700M_SMBUS_HOST_CONTROLER_ENABLE
                                                        )},

                { VIA_VX900_BUS_CTRL_DEVICE_ID,         device_i(
                                                            VIA_VX900_BUS_CTRL_VENDOR_ID,
                                                            0,
                                                            VIA_VX900_BUS_CTRL_DEVICE_ID,
                                                            VX900_SMBUS_HOST_IOBASE,
                                                            0xff00,
                                                            VX900_SMBUS_HOST_CONFIGURE,
                                                            VX900_SMBUS_HOST_CONTROLER_ENABLE
                                                        )},

                { INTEL_ICH4_SMBUS_DEVICE_ID,           device_i(
                                                            INTEL_ICH4_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_ICH4_SMBUS_DEVICE_ID,
                                                            ICH4_SMBUS_HOST_IOBASE,
                                                            0xff00,
                                                            ICH4_SMBUS_HOST_CONFIGURE,
                                                            ICH4_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_ICH8_SMBUS_DEVICE_ID,           device_i(
                                                            INTEL_ICH8_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_ICH8_SMBUS_DEVICE_ID,
                                                            ICH8_SMBUS_HOST_IOBASE,
                                                            0xff00,
                                                            ICH8_SMBUS_HOST_CONFIGURE,
                                                            ICH8_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_ICH10_SMBUS_DEVICE_ID,          device_i(
                                                            INTEL_ICH10_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_ICH10_SMBUS_DEVICE_ID,
                                                            ICH10_SMBUS_HOST_IOBASE,
                                                            0xff00,
                                                            ICH10_SMBUS_HOST_CONFIGURE,
                                                            ICH10_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_NM10_SMBUS_DEVICE_ID,           device_i(
                                                            INTEL_NM10_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_NM10_SMBUS_DEVICE_ID,
                                                            NM10_SMBUS_HOST_IOBASE,
                                                            0xff00,
                                                            NM10_SMBUS_HOST_CONFIGURE,
                                                            NM10_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_QM67_SMBUS_DEVICE_ID,           device_i(
                                                            INTEL_QM67_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_QM67_SMBUS_DEVICE_ID,
                                                            QM67_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            QM67_SMBUS_HOST_CONFIGURE,
                                                            QM67_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_QM77_SMBUS_DEVICE_ID,           device_i(
                                                            INTEL_QM77_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_QM77_SMBUS_DEVICE_ID,
                                                            QM77_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            QM77_SMBUS_HOST_CONFIGURE,
                                                            QM77_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_HM65_SMBUS_DEVICE_ID,           device_i(
                                                            INTEL_HM65_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_HM65_SMBUS_DEVICE_ID,
                                                            HM65_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            HM65_SMBUS_HOST_CONFIGURE,
                                                            HM65_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_HM76_SMBUS_DEVICE_ID,           device_i(
                                                            INTEL_HM76_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_HM76_SMBUS_DEVICE_ID,
                                                            HM76_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            HM76_SMBUS_HOST_CONFIGURE,
                                                            HM76_SMBUS_HOST_HST_EN
                                                         )},

                { INTEL_SOC_SMBUS_DEVICE_ID,            device_i(
                                                            INTEL_SOC_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_SOC_SMBUS_DEVICE_ID,
                                                            SOC_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            SOC_SMBUS_HOST_CONFIGURE,
                                                            SOC_SMBUS_HOST_HST_EN
                                                         )},

                { INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID, device_i(
                                                            INTEL_Apollo_Lake_SOC_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID,
                                                            Apollo_Lake_SOC_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            Apollo_Lake_SOC_SMBUS_HOST_CONFIGURE,
                                                            Apollo_Lake_SOC_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID,   device_i(
                                                            INTEL_Sky_Lake_SOC_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID,
                                                            Sky_Lake_SOC_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            Sky_Lake_SOC_SMBUS_HOST_CONFIGURE,
                                                            Sky_Lake_SOC_SMBUS_HOST_HST_EN
                                                        )},

                { INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID, device_i(
                                                            INTEL_Sky_Lake_U_SOC_SMBUS_VENDOR_ID,
                                                            0,
                                                            INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID,
                                                            Sky_Lake_U_SOC_SMBUS_HOST_IOBASE,
                                                            0xfff0,
                                                            Sky_Lake_U_SOC_SMBUS_HOST_CONFIGURE,
                                                            Sky_Lake_U_SOC_SMBUS_HOST_HST_EN
                                                        )},
            };


            std::list<device_i> PCI_AutoDetect()
            {
                std::list<device_i> detected_devices;

                uint16_t vendorID = 0;
                uint16_t deviceID = 0;
                for (int PCI_Bus = 0; PCI_Bus < 5; PCI_Bus++)
                {
                    for (int PCI_Device = 0; PCI_Device<32; PCI_Device++)
                    {
                        for(int PCI_Function = 0; PCI_Function < 8 ; PCI_Function++ )
                        {
                            uint32_t pci_base = 0x80000000 + PCI_Bus*0x10000 + PCI_Device*0x800 + PCI_Function*0x100;
                            PCI_write(pci_base);

                            uint32_t result = PCI_read();

                            vendorID = result & 0xffff;
                            deviceID = (result >> 16) & 0xffff;


                            if(devices_spr.count(deviceID))
                            {
                                if(devices_spr[deviceID].vendor_id == vendorID)
                                {
                                    devices_spr[deviceID].pci_base = pci_base;
                                }
                            }
                            else if (vendorID == INTEL_ICH4_VENDOR_ID && deviceID == INTEL_ICH4_DEVICE_ID)
                            {
                                int stride = 0x100;
                                for (int i = 0; i < 8; i ++)
                                {
                                    int base = pci_base + i * stride;
                                    PCI_write(base);
                                    {
                                        uint32_t result = PCI_read();

                                        vendorID = result & 0xffff;
                                        deviceID = (result >> 16) & 0xffff;

                                        if(devices_spr.count(deviceID))
                                        {
                                            if(devices_spr[deviceID].vendor_id == vendorID)
                                            {
                                                devices_spr[deviceID].pci_base = base;
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }

                for(auto dev : devices_spr) {
                    if(dev.second.isEnabled()) {
                        detected_devices.push_back(dev.second);
                    }
                }

                return detected_devices;
            }


            void out(uint8_t byteOffset, uint8_t byteData) {
                outb(byteData , base_bus_addr + byteOffset);
                usleep(10);
            }

            uint8_t inp(uint8_t byteOffset) {
                uint8_t res = inb(base_bus_addr + byteOffset);
                usleep(10);
                return res;
            }

            void clear() {
                out(SMBHSTSTS , 0xff);
                out(SMBHSTDAT0, 0x0 );
            }

            bool is_busy() {

                bool busy = false;
                int timeout = 5;
                do {
                   busy = inp(SMBHSTSTS) & SMBHSTSTS_BUSY;
                   usleep(10);
                } while(!busy && timeout--);

                return busy;
            }

            bool wait() {
                int	timeout = SMBUS_TIMEOUT;
                int status = SMBUS_OK;
                while (true)
                {
                    if(!timeout--) {
                        status = SMBUS_BUSY;
                        break;
                    }

                    // I/O Delay
                    usleep(100);

                    // Read Host Status Register
                    uint64_t dwValue = inp(SMBHSTSTS) & 0xff;

                    if(dwValue & SMBHSTSTS_INTR)
                    {
                        status = SMBUS_OK;
                        break;
                    }

                    if(dwValue & SMBHSTSTS_FAILED)
                    {
                        status = SMBHSTSTS_FAILED;
                        break;
                    }

                    if(dwValue & SMBHSTSTS_COLLISION)
                    {
                        status = SMBHSTSTS_COLLISION;
                        break;
                    }

                    if( dwValue & SMBHSTSTS_ERROR)
                    {
                        status = SMBHSTSTS_ERROR;
                        break;
                    }
                }

                return status == SMBUS_OK;
            }

            bool status_device(uint8_t deviceAddress) {
                clear();

                if(is_busy())
                    return false;

                out(SMBHSTADD  , deviceAddress  & ~1 );
                out(SMBHSTCNT  , SMBHSTCNT_START | SMBHSTCNT_QUICK);

                return wait();
            }
        }


#define F75111_INTERNAL_ADDR    0x9C 	//	OnBoard  F75111 Chipset
#define F75111_EXTERNAL_ADDR    0x6E	//	External F75111 Chipset

#define F75111_CONFIGURATION    0x03	//  Configure GPIO13 to WDT2 Function

#define GPIO1X_CONTROL_MODE     0x10	//  Select Output Mode or Input Mode
#define GPIO2X_CONTROL_MODE     0x20	//  Select GPIO2X Output Mode or Input Mode
#define GPIO3X_CONTROL_MODE     0x40	//  Select GPIO3X Output Mode or Input Mode

#define GPIO1X_INPUT_DATA       0x12	//  GPIO1X Input
#define GPIO3X_INPUT_DATA       0x42	//  GPIO3X Input

#define GPIO2X_OUTPUT_DATA      0x21	//  GPIO2X Output

#define GPIO2X_OUTPUT_DRIVING   0x2B	//  Select GPIO2X Output Mode or Input Mode

#define WDT_TIMER_RANGE         0x37	//  0-255 (secord or minute program by WDT_UNIT)

#define	WDT_CONFIGURATION       0x36	//  Configure WDT Function
#define	WDT_TIMEOUT_FLAG        0x40	//	When watchdog timeout.this bit will be set to 1.
#define	WDT_ENABLE              0x20	//	Enable watchdog timer
#define	WDT_PULSE               0x10	//	Configure WDT output mode
                                        //	0:Level Mode
                                        //	1:Pulse	Mode

#define	WDT_UNIT                0x08	//	Watchdog unit select.
                                        //	0:Select second.
                                        //	1:Select minute.

#define	WDT_LEVEL               0x04	//	When select level output mode:
                                        //	0:Level low
                                        //	1:Level high

#define	WDT_PSWIDTH_1MS         0x00	//	When select Pulse mode:	1	ms.
#define	WDT_PSWIDTH_20MS        0x01	//	When select Pulse mode:	20	ms.
#define	WDT_PSWIDTH_100MS       0x02	//	When select Pulse mode:	100	ms.
#define	WDT_PSWIDTH_4000MS      0x03	//	When select Pulse mode:	4	 s.

        bool status() {
            static std::list<bus::device_i> devices = bus::PCI_AutoDetect();
            bool initialized = false;
            if(!devices.empty()) {
                bus::base_bus_addr = devices.front().bus_address();
                initialized = bus::status_device(F75111_INTERNAL_ADDR);
            }

            /* PCI not detected */
            return initialized;
        }

        uint8_t read(uint8_t target_addr) {
            uint8_t result = 0;
            bus::clear();

            if(bus::is_busy())
                return result;

            bus::out(SMBHSTADD, F75111_INTERNAL_ADDR | 1 );
            bus::out(SMBHSTCMD, target_addr );
            bus::out(SMBHSTCNT, SMBHSTCNT_START | SMBHSTCNT_BYTE);

            if (bus::wait())
            {
                result = bus::inp(SMBHSTDAT0) & 0xff;
            }

            return result;
        }

        bool write(uint8_t target_addr, uint8_t v) {
            bus::clear();

            if (bus::is_busy())
                return false;

            bus::out(SMBHSTADD , F75111_INTERNAL_ADDR & ~1 );
            bus::out(SMBHSTCMD , target_addr );
            bus::out(SMBHSTDAT0, v );
            bus::out(SMBHSTCNT , SMBHSTCNT_START | SMBHSTCNT_BYTE);

            return bus::wait();
        }
    }
}




namespace embc {
    namespace gpio {
        uint8_t read() {
            uint8_t byteGPIO1X = io::read(GPIO1X_INPUT_DATA);
            uint8_t byteGPIO3X = io::read(GPIO3X_INPUT_DATA);
            uint8_t value = (byteGPIO1X & 0xf0) | (byteGPIO3X & 0x0f);

            return value;
        }

        bool write(uint8_t value) {
            return io::write(GPIO2X_OUTPUT_DATA, value);
        }

        bool write(opin_t pin, bool on) {
            static uint8_t prev = 0;
            uint8_t val = prev;
            if(((uint8_t)(val ^ pin)) == 0)
                return true;

            (on ? val |= pin : val &= ~pin);

            bool ok = write(val);
            if(ok)
                prev = val;

            return ok;
        }

        bool read(ipin_t pin) {
            return read() & pin;
        }

#ifdef IOEMBC1000_QT
        GpiWatcher * GpiWatcher::instance()
        {
            static GpiWatcher * i = new GpiWatcher();
            return i;
        }

        GpiWatcher::GpiWatcher() {
            startTimer(10);
        }

        void GpiWatcher::timerEvent(QTimerEvent *)
        {
            static uint8_t prev = read();

            uint8_t r = read();
            uint8_t changes = r ^ prev;
            if(changes) {
                for(int i = 0; i < 8; i++)
                {
                    uint8_t mask = (1 << i);
                    Q_EMIT pinChanged((ipin_t)(changes & mask), r & mask);
                }
                prev = r;
            }
        }
#endif
    }

    void wdt_off() {
        io::write(WDT_CONFIGURATION, 0);
    }

    void wdt_on(uint8_t timer) {
        io::write(WDT_TIMER_RANGE, timer);
        io::write(WDT_CONFIGURATION, WDT_TIMEOUT_FLAG | WDT_ENABLE | WDT_PULSE | WDT_PSWIDTH_100MS);
    }

    bool init() {
        if(iopl(3) != -1) {
            if(io::status()) {
                io::write(GPIO1X_CONTROL_MODE   , 0x00);
                io::write(GPIO3X_CONTROL_MODE   , 0x00);
                io::write(GPIO2X_CONTROL_MODE   , 0xff);
                io::write(GPIO2X_OUTPUT_DRIVING , 0xff);
                io::write(F75111_CONFIGURATION  , 0x07);
                io::write(0x06                  , 0x04);
                return true;
            }
        }

        return false;
    }
}
