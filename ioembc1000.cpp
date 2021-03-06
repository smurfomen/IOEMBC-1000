#include "ioembc1000.h"
#include <stdbool.h>
#include <map>
#include <list>
#include <string>
#include <stdio.h>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>

#ifdef DEBUG
#define LOG(x) (std::cout << x << std::endl);
#else
#define LOG(x)
#endif


template< typename T >
std::string to_hex( T i )
{
  std::stringstream stream;
  stream << "0x"
         << std::setfill ('0') << std::setw(sizeof(T)*2)
         << std::hex << i;
  return stream.str();
}

#ifdef Q_OS_WIN
#include <windows.h>
using call_outw = void(__stdcall *)(short, short);   // тип указатель на функцию вывода
using call_inpw = short(__stdcall *)(short);			// тип указатель на функцию ввода
using call_condition = int(__stdcall *)(void);    // тип указатель на функцию, возвращающую состояние
using call_inpl = unsigned int (__stdcall *)(unsigned int);
using call_outl = void (__stdcall *)(unsigned int port, unsigned int value);

static call_outw Out32                        = 0;	// указатель на функцию вывода
static call_inpw Inp32                        = 0;	// указатель на функцию ввода
static call_outl Outl32                        = 0;	// указатель на функцию вывода
static call_inpl Inpl32                        = 0;	// указатель на функцию ввода

static call_condition IsInpOutDriverOpen   = 0;	// указатель на функцию готовности драйвера
static call_condition IsXP64Bit            = 0;	// указатель на функцию проверки разрядности

int IsDriverReady()
{
    return (IsInpOutDriverOpen) ? IsInpOutDriverOpen() : FALSE;
}

int iopl(int)
{
    static int success = -1;
    if(success >= 0)
        return success;

    HINSTANCE hDLL = ::LoadLibrary(
                     #ifdef Q_OS_WIN64
                         L"inpoutx64.dll"
                     #else
                         L"inpout32.dll"
                     #endif
                         );

    if(hDLL) {
        Out32               = (call_outw)      GetProcAddress(hDLL, "Out32");
        Inp32               = (call_inpw)      GetProcAddress(hDLL, "Inp32");
        Inpl32              = (call_inpl)      GetProcAddress(hDLL, "DlPortReadPortUlong");
        Outl32              = (call_outl)      GetProcAddress(hDLL, "DlPortWritePortUlong");

        IsInpOutDriverOpen   = (call_condition) GetProcAddress(hDLL, "IsInpOutDriverOpen");
        IsXP64Bit            = (call_condition) GetProcAddress(hDLL, "IsXP64Bit");


        if(IsDriverReady())
            success = 0;
    }

    return success;
}

uint8_t inb(uint16_t port)
{
    uint8_t ret = 0;
    if (Inp32 != NULL && IsDriverReady())
    {
        ret = Inp32(port) & 0xff;
    }
    return ret;
}

void outb(uint8_t value, uint16_t port)
{
    if (Out32 != NULL && IsDriverReady())
    {
        Out32(port, value);
    }
}

uint32_t inl(uint32_t port) {
    uint32_t ret = 0;
    if(Inpl32 != NULL && IsDriverReady()) {
        ret = Inpl32(port);
    }
    else
    {
        for(size_t i = 0; i < sizeof (ret); i++) {
             ret |= inb(port + i) << (i * 8);
        }
    }

    return ret;
}

void outl(uint32_t value, uint32_t port) {
    if(Outl32 != NULL && IsDriverReady()) {
        Outl32(port, value);
    }
    else
    {
        for(size_t i = 0; i < sizeof (value); i++) {
             outb((value >> (i*8)) & 0xff, port + i);
        }
    }
}


#else
    #include <sys/io.h>
    #include <unistd.h>
#endif



namespace embc {
    namespace bus {
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

            dwAddrVal= inl(PCI_CONFIG_DATA);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
            return dwAddrVal;
        };

        auto PCI_write(uint32_t dwDataVal) -> void
        {
            outl(dwDataVal,PCI_CONFIG_ADDR);
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        };


        /* { device ID, { config offset, bit 'is enabled'} } */
        std::map <uint16_t, std::pair<uint8_t, uint8_t>> configs = {
            { VIA_VT8237_BUS_CTRL_DEVICE_ID,        { VT8237_SMBUS_HOST_CONFIGURE,          VT8237_SMBUS_HOST_CONTROLER_ENABLE      } },
            { VIA_CX700M_BUS_CTRL_DEVICE_ID,        { CX700M_SMBUS_HOST_CONFIGURE,          CX700M_SMBUS_HOST_CONTROLER_ENABLE      } },
            { VIA_VX900_BUS_CTRL_DEVICE_ID,         { VX900_SMBUS_HOST_CONFIGURE,           VX900_SMBUS_HOST_CONTROLER_ENABLE       } },
            { INTEL_ICH4_SMBUS_DEVICE_ID,           { ICH4_SMBUS_HOST_CONFIGURE,            ICH4_SMBUS_HOST_HST_EN                  } },
            { INTEL_ICH8_SMBUS_DEVICE_ID,           { ICH8_SMBUS_HOST_CONFIGURE,            ICH8_SMBUS_HOST_HST_EN                  } },
            { INTEL_ICH10_SMBUS_DEVICE_ID,          { ICH10_SMBUS_HOST_CONFIGURE,           ICH10_SMBUS_HOST_HST_EN                 } },
            { INTEL_NM10_SMBUS_DEVICE_ID,           { NM10_SMBUS_HOST_CONFIGURE,            NM10_SMBUS_HOST_HST_EN                  } },
            { INTEL_QM67_SMBUS_DEVICE_ID,           { QM67_SMBUS_HOST_CONFIGURE,            QM67_SMBUS_HOST_HST_EN                  } },
            { INTEL_QM77_SMBUS_DEVICE_ID,           { QM77_SMBUS_HOST_CONFIGURE,            QM77_SMBUS_HOST_HST_EN                  } },
            { INTEL_HM65_SMBUS_DEVICE_ID,           { HM65_SMBUS_HOST_CONFIGURE,            HM65_SMBUS_HOST_HST_EN                  } },
            { INTEL_HM76_SMBUS_DEVICE_ID,           { HM76_SMBUS_HOST_CONFIGURE,            HM76_SMBUS_HOST_HST_EN                  } },
            { INTEL_SOC_SMBUS_DEVICE_ID,            { SOC_SMBUS_HOST_CONFIGURE,             SOC_SMBUS_HOST_HST_EN                   } },
            { INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID,{ Apollo_Lake_SOC_SMBUS_HOST_CONFIGURE, Apollo_Lake_SOC_SMBUS_HOST_HST_EN       } },
            { INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID,   { Sky_Lake_SOC_SMBUS_HOST_CONFIGURE,    Sky_Lake_SOC_SMBUS_HOST_HST_EN          } },
            { INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID, { Sky_Lake_U_SOC_SMBUS_HOST_CONFIGURE,  Sky_Lake_U_SOC_SMBUS_HOST_HST_EN        } }
        };

        /* { device ID, vendor ID } */
        std::map <uint16_t, uint16_t> vendors = {
            { VIA_VT8237_BUS_CTRL_DEVICE_ID,            VIA_VT8237_BUS_CTRL_VENDOR_ID           },
            { VIA_CX700M_BUS_CTRL_DEVICE_ID,            VIA_CX700M_BUS_CTRL_VENDOR_ID           },
            { VIA_VX900_BUS_CTRL_DEVICE_ID,             VIA_VX900_BUS_CTRL_VENDOR_ID            },
            { INTEL_ICH4_SMBUS_DEVICE_ID,               INTEL_ICH4_SMBUS_VENDOR_ID              },
            { INTEL_ICH8_SMBUS_DEVICE_ID,               INTEL_ICH8_SMBUS_VENDOR_ID              },
            { INTEL_ICH10_SMBUS_DEVICE_ID,              INTEL_ICH10_SMBUS_VENDOR_ID             },
            { INTEL_NM10_SMBUS_DEVICE_ID,               INTEL_NM10_SMBUS_VENDOR_ID              },
            { INTEL_QM67_SMBUS_DEVICE_ID,               INTEL_QM67_SMBUS_VENDOR_ID              },
            { INTEL_QM77_SMBUS_DEVICE_ID,               INTEL_QM77_SMBUS_VENDOR_ID              },
            { INTEL_HM65_SMBUS_DEVICE_ID,               INTEL_HM65_SMBUS_VENDOR_ID              },
            { INTEL_HM76_SMBUS_DEVICE_ID,               INTEL_HM76_SMBUS_VENDOR_ID              },
            { INTEL_SOC_SMBUS_DEVICE_ID,                INTEL_SOC_SMBUS_VENDOR_ID               },
            { INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID,    INTEL_Apollo_Lake_SOC_SMBUS_VENDOR_ID   },
            { INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID,       INTEL_Sky_Lake_SOC_SMBUS_VENDOR_ID      },
            { INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID,     INTEL_Sky_Lake_U_SOC_SMBUS_VENDOR_ID    }
        };

        /* { device ID, vendor ID } */
        std::map <uint16_t, const char *> names = {
            { VIA_VT8237_BUS_CTRL_DEVICE_ID,            "VT8237"            },
            { VIA_CX700M_BUS_CTRL_DEVICE_ID,            "CX700M"            },
            { VIA_VX900_BUS_CTRL_DEVICE_ID,             "VX900"             },
            { INTEL_ICH4_SMBUS_DEVICE_ID,               "Intel ICH4"        },
            { INTEL_ICH8_SMBUS_DEVICE_ID,               "Intel ICH8"        },
            { INTEL_ICH10_SMBUS_DEVICE_ID,              "Intel ICH10"       },
            { INTEL_NM10_SMBUS_DEVICE_ID,               "Intel NM10"        },
            { INTEL_QM67_SMBUS_DEVICE_ID,               "Intel QM67"        },
            { INTEL_QM77_SMBUS_DEVICE_ID,               "Intel QM77"        },
            { INTEL_HM65_SMBUS_DEVICE_ID,               "Intel HM65"        },
            { INTEL_HM76_SMBUS_DEVICE_ID,               "Intel HM76"        },
            { INTEL_SOC_SMBUS_DEVICE_ID,                "Intel SOC"         },
            { INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID,    "Intel Apollo Lake" },
            { INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID,       "Intel Sky Lake"    },
            { INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID,     "Intel Sky Lake U"  }
        };

        /* { device ID , { host smbus offset, mask }}  */
        std::map <uint16_t, std::pair<uint8_t, uint16_t>> bus_host_offset = {
            { VIA_VT8237_BUS_CTRL_DEVICE_ID,            { VT8237_SMBUS_HOST_IOBASE          , 0xff00} },
            { VIA_CX700M_BUS_CTRL_DEVICE_ID,            { CX700M_SMBUS_HOST_IOBASE          , 0xff00} },
            { VIA_VX900_BUS_CTRL_DEVICE_ID,             { VX900_SMBUS_HOST_IOBASE           , 0xff00} },
            { INTEL_ICH4_SMBUS_DEVICE_ID,               { ICH4_SMBUS_HOST_IOBASE            , 0xff00} },
            { INTEL_ICH8_SMBUS_DEVICE_ID,               { ICH8_SMBUS_HOST_IOBASE            , 0xff00} },
            { INTEL_ICH10_SMBUS_DEVICE_ID,              { ICH10_SMBUS_HOST_IOBASE           , 0xff00} },
            { INTEL_NM10_SMBUS_DEVICE_ID,               { NM10_SMBUS_HOST_IOBASE            , 0xff00} },
            { INTEL_QM67_SMBUS_DEVICE_ID,               { QM67_SMBUS_HOST_IOBASE            , 0xfff0} },
            { INTEL_QM77_SMBUS_DEVICE_ID,               { QM77_SMBUS_HOST_IOBASE            , 0xfff0} },
            { INTEL_HM65_SMBUS_DEVICE_ID,               { HM65_SMBUS_HOST_IOBASE            , 0xfff0} },
            { INTEL_HM76_SMBUS_DEVICE_ID,               { HM76_SMBUS_HOST_IOBASE            , 0xfff0} },
            { INTEL_SOC_SMBUS_DEVICE_ID,                { SOC_SMBUS_HOST_IOBASE             , 0xfff0} },
            { INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID,    { Apollo_Lake_SOC_SMBUS_HOST_IOBASE , 0xfff0} },
            { INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID,       { Sky_Lake_SOC_SMBUS_HOST_IOBASE    , 0xfff0} },
            { INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID,     { Sky_Lake_U_SOC_SMBUS_HOST_IOBASE  , 0xfff0} }
        };


        struct device_i {
        private:
            uint16_t device_id;
            uint32_t pci_base;
            uint16_t smbus_addr;
        public:

            device_i(uint16_t device_id, uint32_t pci_base)
                : device_id(device_id),
                  pci_base(pci_base),
                  smbus_addr(0)
            {
                PCI_write(pci_base + bus_host_offset[device_id].first);
                smbus_addr = (uint16_t)PCI_read() & bus_host_offset[device_id].second;
                if(!isEnabled())
                    smbus_addr = 0;
            }

            device_i(uint16_t device_id)
                : device_id(device_id),
                  pci_base(0),
                  smbus_addr(0)
            {

            }

            void set_smbus(uint16_t addr)
            {
                smbus_addr = addr;
            }

            uint16_t id() const {
                return device_id;
            }

            uint16_t vendor() const {
                return vendors[device_id];
            }

            const char * name() const {
                return names[device_id];
            }

            std::string description() const {
                std::stringstream ss;
                ss << "Name: " << name() << " ID: " << to_hex(id()) << '\n'
                   << "Vendor: " << to_hex(vendor()) << '\n'
                   << "PCI address: " << to_hex(pci_base) << '\n'
                   << "BUS address: " << to_hex(smbus_addr) << '\n';
                return ss.str();
            }

            bool isEnabled() {
                if(!smbus_addr || !pci_base)
                    return false;

                PCI_write(pci_base + configs[device_id].first);
                return PCI_read() & configs[device_id].second;
            }

            void out(uint8_t byteOffset, uint8_t byteData) {
                outb(byteData , smbus_addr + byteOffset);
            }

            uint8_t inp(uint8_t byteOffset) {
                return smbus_addr ? inb(smbus_addr + byteOffset) : 0;
            }

            void clear() {
                out(SMBHSTSTS , 0xff);
                out(SMBHSTDAT0, 0x0 );
            }

            bool is_busy() {
                bool busy = inp(SMBHSTSTS) & SMBHSTSTS_BUSY;
                int timeout = 5;
                auto interval = std::chrono::milliseconds(5);
                while(busy && timeout--)
                {
                    std::this_thread::sleep_for(interval);
                    busy = inp(SMBHSTSTS) & SMBHSTSTS_BUSY;
                }

                return busy;
            }

            bool wait() {
                int	timeout = SMBUS_TIMEOUT;
                int status = SMBUS_OK;
                auto interval = std::chrono::milliseconds(5);

                while (true)
                {
                    if(!timeout--) {
                        status = SMBUS_BUSY;
                        break;
                    }

                    std::this_thread::sleep_for(interval);

                    // Read Host Status Register
                    uint64_t r = inp(SMBHSTSTS) & 0xff;

                    if(r & SMBHSTSTS_INTR)
                    {
                        status = SMBUS_OK;
                        break;
                    }

                    if(r & SMBHSTSTS_FAILED)
                    {
                        status = SMBHSTSTS_FAILED;
                        break;
                    }

                    if(r & SMBHSTSTS_COLLISION)
                    {
                        status = SMBHSTSTS_COLLISION;
                        break;
                    }

                    if( r & SMBHSTSTS_ERROR)
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

            uint8_t read(uint8_t target_addr) {
                uint8_t result = 0;
                clear();

                if(is_busy())
                    return result;

                out(SMBHSTADD, F75111_INTERNAL_ADDR | 1 );
                out(SMBHSTCMD, target_addr );
                out(SMBHSTCNT, SMBHSTCNT_START | SMBHSTCNT_BYTE);

                if (wait())
                {
                    result = inp(SMBHSTDAT0) & 0xff;
                }

                return result;
            }

            bool write(uint8_t target_addr, uint8_t v) {
                clear();

                if (is_busy())
                    return false;

                out(SMBHSTADD , F75111_INTERNAL_ADDR & ~1 );
                out(SMBHSTCMD , target_addr );
                out(SMBHSTDAT0, v );
                out(SMBHSTCNT , SMBHSTCNT_START | SMBHSTCNT_BYTE);

                return wait();
            }
        };

        device_i * active_device = nullptr;
        device_i & dev() {
            return * active_device;
        }

        std::list<device_i> PCI_AutoDetect()
        {
            std::list<device_i> detected_devices;

            auto init = [&detected_devices](uint32_t pci_base){
                PCI_write(pci_base);

                uint32_t result = PCI_read();

                uint16_t venID = result & 0xffff;
                uint16_t devID = (result >> 16) & 0xffff;

                if(vendors.count(devID))
                {
                    if(vendors[devID] == venID)
                    {
                        detected_devices.push_back(device_i(devID, pci_base));
                    }
                }
                else if (venID == INTEL_ICH4_VENDOR_ID && devID == INTEL_ICH4_DEVICE_ID)
                {
                    int stride = 0x100;
                    for (int i = 0; i < 8; i ++)
                    {
                        int base = pci_base + i * stride;
                        PCI_write(base);
                        {
                            uint32_t result = PCI_read();

                            venID = result & 0xffff;
                            devID = (result >> 16) & 0xffff;

                            if(vendors.count(devID))
                            {
                                if(vendors[devID] == venID)
                                {
                                    device_i d(devID, base);
                                    if(d.isEnabled()) {
                                        LOG(d.description());
                                        detected_devices.push_back(d);
                                    }
                                }
                            }
                        }
                    }
                }
            };

            uint32_t pci_base = 0;
            for (int PCI_Bus = 0; PCI_Bus < 5; PCI_Bus++)
            {
                for (int PCI_Device = 0; PCI_Device<32; PCI_Device++)
                {
                    for(int PCI_Function = 0; PCI_Function < 8 ; PCI_Function++ )
                    {
                        pci_base = 0x80000000 + PCI_Bus*0x10000 + PCI_Device*0x800 + PCI_Function*0x100;
                        init(pci_base);
                    }
                }
            }


            return detected_devices;
        }


        bool status() {
            return active_device && active_device->status_device(F75111_INTERNAL_ADDR);
        }
    }
}




namespace embc {
    namespace gpio {
        bool inverted = true;
        uint8_t read() {
            uint8_t byteGPIO1X = bus::dev().read(GPIO1X_INPUT_DATA);
            uint8_t byteGPIO3X = bus::dev().read(GPIO3X_INPUT_DATA);
            uint8_t value = (byteGPIO1X & 0xf0) | (byteGPIO3X & 0x0f);

            return inverted ? ~value : value;
        }

        bool write(uint8_t value) {
            return bus::dev().write(GPIO2X_OUTPUT_DATA, value);
        }

        bool write(opin_t pin, bool on) {
            static uint8_t prev = inverted ? ~0 : 0;
            uint8_t val = prev;

            if(on)
            {
                (inverted ? val &= ~pin : val |= pin);
            }
            else
            {
                (inverted ? val |= pin : val &= ~pin);
            }

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
            static uint8_t prev = ~read();

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
        bus::dev().write(WDT_CONFIGURATION, 0);
    }

    void wdt_on(uint8_t timer) {
        bus::dev().write(WDT_TIMER_RANGE, timer);
        bus::dev().write(WDT_CONFIGURATION, WDT_TIMEOUT_FLAG | WDT_ENABLE | WDT_PULSE | WDT_PSWIDTH_100MS);
    }

    bool init(uint16_t deviceID, uint16_t smbus_addr) {
        if(iopl(3) != -1) {
            if(deviceID && smbus_addr)
            {
                bus::active_device = new bus::device_i(deviceID);
                bus::active_device->set_smbus(smbus_addr);
            }
            else
            {
                static std::list<bus::device_i> devices = bus::PCI_AutoDetect();
                for(auto & d : devices) {
                    if(d.status_device(F75111_INTERNAL_ADDR))
                    {
                        bus::active_device = &d;
                    }
                }
            }

            if(bus::status()) {
                bus::dev().write(GPIO1X_CONTROL_MODE   , 0x00);
                bus::dev().write(GPIO3X_CONTROL_MODE   , 0x00);
                bus::dev().write(GPIO2X_CONTROL_MODE   , 0xff);
                bus::dev().write(GPIO2X_OUTPUT_DRIVING , 0xff);
                bus::dev().write(F75111_CONFIGURATION  , 0x07);
                bus::dev().write(0x06                  , 0x04);
                return true;
            }
        }

        return false;
    }
}
