#include "gic.h"

/*
 * This is for test gic routing.
 * From gicv2 archicture, some register need to init in secure Mode.
 * Init these registers before exit3 to El1 non-secure Rtos firmware or Linux Kernel for test.
 */
void gicv2_init_for_secure_boot(void)
{
	unsigned int i;

	/*gicv2 archicture: p3-69*/
	/*
	 * 1. programs the GICD_IGROUPRn registers to indicate which interrupts are Group 1, Non-secure
	 */
	for (i = 0 ; i < (MAX_INTR_NUM + 31) / 32; i++) {
		GICD_IGROUPR(i) = 0xffffffff;  /*set all to Group 1 for non-secure accesss*/
	}

	/*
	 * sets the Secure GICC_CTLR.FIQEn bit to 1 to configure the CPU interface to use FIQ for Group 0 interrupts.
	 * [0] EnableGrp0e, f Enable for the signaling of Group 0 interrupts by the CPU interface to the connected processor
	 * [2] AckCtl When the highest priority pending interrupt is a Group 1 interrupt, determines both
	 * [1] EnableGrp1d, e Enable for the signaling of Group 1 interrupts by the CPU interface to the connected processor
	 * [3] FIQEn Controls whether the CPU interface signals Group 0 interrupts to a target processor using the FIQ or the IRQ signal.
	 * [4] CBPRc Controls whether the GICC_BPR provides common control to Group 0 and Group 1 interrupts
	 */
	GICC_CTLR = (0x1 | (0x1 << 1) | (0x1 << 2) | (0x1 << 3) | (0x01 << 4));

	/*
	 * GICD_CTLR: bit[0]: EnableGrp0 Global enable for forwarding pending Group 0 interrupts from the Distributor to the CPU interfaces
	 *            bit[1]: EnableGrp1 Global enable for forwarding pending Group 1 interrupts from the Distributor to the CPU interfaces
	 */
	GICD_CTLR = 0x03; //Enable EnableGrp0, EnableGrp1

	/*
	* This behavior of Non-secure accesses applies only to the Priority value fields in the GICD_IPRIORITYRn:
	• if the Priority field in the GICC_PMR holds a value with bit [7] == 0, then the field is RAZ/WI to Non-secure accesses
	• if the Priority field in the GICC_RPR holds a value with bit [7] == 0, then the field is RAZ to Non-secure reads
	*/
	GICC_PMRn = 0xf0;
}

void gicv2_init_for_nonsecure_boot(void)
{
	unsigned int i;

	/*gicv2 archicture: p3-69*/
	/*
	 * 1. programs the GICD_IGROUPRn registers to indicate which interrupts are Group 1, Non-secure
	 */
	for (i = 0 ; i < (MAX_INTR_NUM + 31) / 32; i++) {
		GICD_IGROUPR(i) = 0x0;  /*set all to Group 1 for non-secure accesss*/
	}

	/*
	 * sets the Secure GICC_CTLR.FIQEn bit to 1 to configure the CPU interface to use FIQ for Group 0 interrupts.
	 * [0] EnableGrp0e, f Enable for the signaling of Group 0 interrupts by the CPU interface to the connected processor
	 * [2] AckCtl When the highest priority pending interrupt is a Group 1 interrupt, determines both
	 * [1] EnableGrp1d, e Enable for the signaling of Group 1 interrupts by the CPU interface to the connected processor
	 * [3] FIQEn Controls whether the CPU interface signals Group 0 interrupts to a target processor using the FIQ or the IRQ signal.
	 * [4] CBPRc Controls whether the GICC_BPR provides common control to Group 0 and Group 1 interrupts
	 */
	//GICC_CTLR = (0x1 | (0x1 << 1) | (0x1 << 2) | (0x1 << 3) | (0x01 << 4));

	/*
	 * GICD_CTLR: bit[0]: EnableGrp0 Global enable for forwarding pending Group 0 interrupts from the Distributor to the CPU interfaces
	 *            bit[1]: EnableGrp1 Global enable for forwarding pending Group 1 interrupts from the Distributor to the CPU interfaces
	 */
	//GICD_CTLR = 0x03; //Enable EnableGrp0, EnableGrp1

	/*
	* This behavior of Non-secure accesses applies only to the Priority value fields in the GICD_IPRIORITYRn:
	• if the Priority field in the GICC_PMR holds a value with bit [7] == 0, then the field is RAZ/WI to Non-secure accesses
	• if the Priority field in the GICC_RPR holds a value with bit [7] == 0, then the field is RAZ to Non-secure reads
	*/
	//GICC_PMRn = 0xf0;
}

