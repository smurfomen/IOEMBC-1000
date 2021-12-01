#include "ioembc1000.h"
#include <stdbool.h>
#include <map>
#include <string>
#include <stdio.h>
#include <unistd.h>
#include <sys/io.h>

namespace embc {
    namespace io {
        namespace bus {
            uint16_t m_SMBusMapIoAddr = 0x500;
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


            int devid; //for SMBus Devices ID
            bool PCI_AutoDetect()
            {
                auto PCI_Read = []() -> uint32_t
                {
                    uint32_t dwAddrVal;

                    dwAddrVal=inl(PCI_CONFIG_DATA);
                    usleep(10);
                    return dwAddrVal;
                };

                auto PCI_Write = [](uint32_t dwDataVal)
                {
                    outl(dwDataVal,PCI_CONFIG_ADDR);
                    usleep(10);
                };

                auto initialize = [=](uint32_t configAddr, uint32_t enable_flag, uint32_t baseAddr, uint32_t busMaskAddr){
                    PCI_Write(configAddr);
                    if ( PCI_Read() & enable_flag )
                    {
                        PCI_Write(baseAddr);
                        m_SMBusMapIoAddr = 	(uint16_t)PCI_Read() & busMaskAddr ;
                        return true;
                    }

                    return false;
                };


                devid =0 ;
                int PCI_Bus = 0;
                int PCI_Device = 0;
                int PCI_Function = 0;
                for (PCI_Bus = 0;PCI_Bus<5;PCI_Bus++)
                {

                    for (PCI_Device = 0;PCI_Device<32;PCI_Device++)
                    {

                        for(PCI_Function = 0; PCI_Function<8 ; PCI_Function++ )
                        {

                            uint32_t dwResult;
                            uint32_t dwIOAddr = 0x80000000 + PCI_Bus*0x10000 + PCI_Device*0x800 + PCI_Function*0x100;
                            PCI_Write(dwIOAddr);
                            dwResult = PCI_Read();


                            if ((( dwResult & 0xFFFF) == VIA_VT8237_BUS_CTRL_VENDOR_ID )  &&
                                (( dwResult >> 16 )   == VIA_VT8237_BUS_CTRL_DEVICE_ID ))
                            {
                                devid = VIA_VT8237_BUS_CTRL_DEVICE_ID;
                                return initialize(dwIOAddr + VT8237_SMBUS_HOST_CONFIGURE,
                                                  VT8237_SMBUS_HOST_CONTROLER_ENABLE,
                                                  dwIOAddr + VT8237_SMBUS_HOST_IOBASE,
                                                  0xFF00);
                            }

                            if ((( dwResult & 0xFFFF) == VIA_CX700M_BUS_CTRL_VENDOR_ID )  &&
                                (( dwResult >> 16 )   == VIA_CX700M_BUS_CTRL_DEVICE_ID ))
                            {
                                devid = VIA_CX700M_BUS_CTRL_DEVICE_ID;
                                return initialize(dwIOAddr+CX700M_SMBUS_HOST_CONFIGURE,
                                                  CX700M_SMBUS_HOST_CONTROLER_ENABLE,
                                                  dwIOAddr+CX700M_SMBUS_HOST_IOBASE,
                                                  0xFF00);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_ICH4_VENDOR_ID )  &&
                                (( dwResult >> 16 )   == INTEL_ICH4_DEVICE_ID ))
                            {
                                devid = INTEL_ICH4_DEVICE_ID;
                                int PCI_Function=0;
                                for (PCI_Function=0;PCI_Function<8;PCI_Function++)
                                {
                                    dwIOAddr = dwIOAddr + PCI_Function*0x100;
                                    PCI_Write(dwIOAddr);
                                    dwResult = PCI_Read();

                                    if ((( dwResult & 0xFFFF) == INTEL_ICH4_SMBUS_VENDOR_ID )  &&
                                        (( dwResult >> 16   ) == INTEL_ICH4_SMBUS_DEVICE_ID ))
                                    {
                                        devid = INTEL_ICH4_SMBUS_DEVICE_ID;
                                        return initialize(dwIOAddr+ICH4_SMBUS_HOST_CONFIGURE,
                                                          ICH4_SMBUS_HOST_HST_EN,
                                                          dwIOAddr+ICH4_SMBUS_HOST_IOBASE,
                                                          0xFF00);
                                    }
                                }
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_ICH7_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_ICH7_SMBUS_DEVICE_ID ))						//	0x27DA
                            {
                                devid = INTEL_ICH7_SMBUS_DEVICE_ID;
                                return initialize(dwIOAddr+ICH7_SMBUS_HOST_CONFIGURE,
                                                  ICH7_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+ICH7_SMBUS_HOST_IOBASE,
                                                  0xFF00);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_ICH8_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_ICH8_SMBUS_DEVICE_ID ))						//	0x27DA
                            {
                                devid = INTEL_ICH8_SMBUS_DEVICE_ID;
                                return initialize(dwIOAddr+ICH8_SMBUS_HOST_CONFIGURE,
                                                  ICH8_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+ICH8_SMBUS_HOST_IOBASE,
                                                  0xFF00);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_ICH10_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_ICH10_SMBUS_DEVICE_ID ))						//	0x3A30
                            {
                                devid = INTEL_ICH10_SMBUS_DEVICE_ID;
                                return initialize(dwIOAddr+ICH10_SMBUS_HOST_CONFIGURE,
                                                  ICH10_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+ICH10_SMBUS_HOST_IOBASE,
                                                  0xFF00);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_NM10_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_NM10_SMBUS_DEVICE_ID ))						//	0x27DA
                            {
                                devid = INTEL_NM10_SMBUS_DEVICE_ID;
                                return initialize(dwIOAddr+NM10_SMBUS_HOST_CONFIGURE,
                                                  NM10_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+NM10_SMBUS_HOST_IOBASE,
                                                  0xFF00);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_QM67_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_QM67_SMBUS_DEVICE_ID ))						//	0x283E
                            {
                                devid = INTEL_QM67_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+QM67_SMBUS_HOST_CONFIGURE,
                                                  QM67_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+QM67_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_QM77_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_QM77_SMBUS_DEVICE_ID ))						//	0x283E
                            {
                                devid = INTEL_QM77_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+QM77_SMBUS_HOST_CONFIGURE,
                                                  QM77_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+QM77_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_HM65_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_HM65_SMBUS_DEVICE_ID ))						//	0x283E
                            {
                                devid = INTEL_HM65_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+HM65_SMBUS_HOST_CONFIGURE,
                                                  HM65_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+HM65_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_HM76_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_HM76_SMBUS_DEVICE_ID ))						//	0x283E
                            {
                                devid = INTEL_HM76_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+HM76_SMBUS_HOST_CONFIGURE,
                                                  HM76_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+HM76_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_SOC_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_SOC_SMBUS_DEVICE_ID ))						//	0x283E
                            {
                                devid = INTEL_SOC_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+SOC_SMBUS_HOST_CONFIGURE,
                                                  SOC_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+SOC_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }

                            if ((( dwResult & 0xFFFF) == INTEL_Apollo_Lake_SOC_SMBUS_VENDOR_ID )  &&					//	0x8086
                                (( dwResult >> 16   ) == INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID ))						//	0x9C22
                            {
                                devid = INTEL_Apollo_Lake_SOC_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+Apollo_Lake_SOC_SMBUS_HOST_CONFIGURE,
                                                  Apollo_Lake_SOC_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+Apollo_Lake_SOC_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }

                            //20170829 Add Sky_Lake  by Tracy↓↓↓
                            if ((( dwResult & 0xFFFF) == INTEL_Sky_Lake_SOC_SMBUS_VENDOR_ID )  &&
                                (( dwResult >> 16 )   == INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID ))
                            {
                                devid = INTEL_Sky_Lake_SOC_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+Sky_Lake_SOC_SMBUS_HOST_CONFIGURE,
                                                  Sky_Lake_SOC_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+Sky_Lake_SOC_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }
                            //20170829 Add Sky_Lake  by Tracy↑↑↑

                            //20170829 Add Sky_Lake U by Tracy↓↓↓
                            if ((( dwResult & 0xFFFF) == INTEL_Sky_Lake_U_SOC_SMBUS_VENDOR_ID )  &&
                                (( dwResult >> 16 )   == INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID ))
                            {
                                devid = INTEL_Sky_Lake_U_SOC_SMBUS_DEVICE_ID ;
                                return initialize(dwIOAddr+Sky_Lake_U_SOC_SMBUS_HOST_CONFIGURE,
                                                  Sky_Lake_U_SOC_SMBUS_HOST_HST_EN,
                                                  dwIOAddr+Sky_Lake_U_SOC_SMBUS_HOST_IOBASE,
                                                  0xFFF0);
                            }
                            //20170829 Add Sky_Lake U by Tracy↑↑↑



                            if ((( dwResult & 0xFFFF) == VIA_VX900_BUS_CTRL_VENDOR_ID )  &&					//	0x1106
                                (( dwResult >> 16   ) == VIA_VX900_BUS_CTRL_DEVICE_ID ))					//	0x8410
                            {
                                devid = VIA_VX900_BUS_CTRL_DEVICE_ID;
                                return initialize(dwIOAddr+VX900_SMBUS_HOST_CONFIGURE,
                                                  VX900_SMBUS_HOST_CONTROLER_ENABLE,
                                                  dwIOAddr+VX900_SMBUS_HOST_IOBASE,
                                                  0xFF00);
                            }
                        }
                    }
                }

                return false;
            }

            void out(uint8_t byteOffset, uint8_t byteData) {
                outb(byteData , m_SMBusMapIoAddr + byteOffset);
                usleep(10);
            }

            uint8_t inp(uint8_t byteOffset) {
                uint8_t res = inb(m_SMBusMapIoAddr + byteOffset);
                usleep(10);
                return res;
            }

            void clear() {
                out(SMBHSTSTS , 0xFF);
                out(SMBHSTDAT0, 0x0 );
            }

            bool is_busy() {
                return (inp(SMBHSTSTS) & SMBHSTSTS_BUSY) == 1;
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
                    uint64_t dwValue = inp(SMBHSTSTS) & 0xFF;

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
            bus::PCI_AutoDetect();
            return bus::status_device(bus::devid);
        }

        uint8_t read(uint8_t dataType) {
            uint8_t result = 0;
            bus::clear();

            if(bus::is_busy())
                return result;

            bus::out(SMBHSTADD, F75111_INTERNAL_ADDR | 1 );
            bus::out(SMBHSTCMD, dataType );
            bus::out(SMBHSTCNT, SMBHSTCNT_START | SMBHSTCNT_BYTE);

            if (bus::wait())
            {
                result = bus::inp(SMBHSTDAT0) & 0xFF;
            }

            return result;
        }

        bool write(uint8_t dataType, uint8_t v) {
            bus::clear();

            if (bus::is_busy())
                return false;

            bus::out(SMBHSTADD , F75111_INTERNAL_ADDR & ~1 );
            bus::out(SMBHSTCMD , dataType );
            bus::out(SMBHSTDAT0, v );
            bus::out(SMBHSTCNT , SMBHSTCNT_START | SMBHSTCNT_BYTE);

            return bus::wait();
        }
    }
}




namespace embc {
    namespace gpio {
        std::map<int, pin_t> num_aliases;
        std::map<std::string, pin_t> str_aliases;

        uint8_t read() {
            uint8_t byteGPIO1X = io::read(GPIO1X_INPUT_DATA);
            uint8_t byteGPIO3X = io::read(GPIO3X_INPUT_DATA);
            uint8_t value = (byteGPIO3X & 0x0f) | (byteGPIO1X & 0xf0);

            return value;
        }

        bool write(uint8_t value) {
            return io::write(GPIO2X_OUTPUT_DATA, value);
        }

        bool write(bool on, int pin_alias) {
            return num_aliases.count(pin_alias) && write(on, num_aliases[pin_alias]);
        }

        bool write(bool on, const char *pin_alias) {
            return str_aliases.count(pin_alias) && write(on, str_aliases[pin_alias]);
        }

        bool write(bool on, pin_t pin) {
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

        bool read(pin_t pin) {
            return read() & pin;
        }

        void set_alias(pin_t pin, const char * alias)
        {
            str_aliases.insert({std::string(alias), pin});
        }

        void set_alias(pin_t pin, int alias)
        {
            num_aliases.insert({alias, pin});
        }

        uint8_t read(int pin_alias) {
            return num_aliases.count(pin_alias) && read(num_aliases[pin_alias]);
        }

        uint8_t read(const char *pin_alias) {
            return str_aliases.count(pin_alias) && read(str_aliases[pin_alias]);
        }
    }

    void wdt_off() {
        io::write(WDT_CONFIGURATION, 0);
    }

    void wdt_on(uint8_t byteTimer) {
        io::write(WDT_TIMER_RANGE, byteTimer);
        io::write(WDT_CONFIGURATION, WDT_TIMEOUT_FLAG | WDT_ENABLE | WDT_PULSE | WDT_PSWIDTH_100MS);
    }

    bool init() {
        if(io::status()) {
            io::write(GPIO1X_CONTROL_MODE   , 0x00);
            io::write(GPIO3X_CONTROL_MODE   , 0x00);
            io::write(GPIO2X_CONTROL_MODE   , 0xFF);
            io::write(GPIO2X_OUTPUT_DRIVING , 0xFF);
            io::write(F75111_CONFIGURATION  , 0x07);
            io::write(0x06                  , 0x04);
            return true;
        }

        return false;
    }
}
