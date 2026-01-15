obj-$(CONFIG_USB_CDNS_DRD_TC)			+= cdns-drd-tc.o

cdns-drd-tc-y					:= cdns_drd_tc.o otg_fsm.o 

ifneq ($(CONFIG_USB_CDNS_MISC),)
	cdns-drd-tc-y				+= cdns_misc.o
endif

obj-$(CONFIG_USB_CDNS_GADGET)			+= cdns-gadget.o
cdns-gadget-y				:= cdns_gadget.o debug.o

ifneq ($(CONFIG_USB_CDNS_MISC),)
	cdns-gadget-y				+= cdns_misc.o
endif

obj-$(CONFIG_USB_CDNS_DRD_PCI)			+= cdns-drd-pci.o
cdns-drd-pci-y				:= cdns_drd_pci.o

obj-$(CONFIG_USB_CDNS_HG_PCI)			+= cdns-hg-pci.o
cdns-hg-pci-y				:= cdns_hg_pci.o

##
# Platform-specific glue layers go here
#
# NOTICE: Make sure your glue layer doesn't depend on anything
# which is arch-specific and that it compiles on all situations.
#
# We want to keep this requirement in order to be able to compile
# the entire driver (with all its glue layers) on several architectures
# and make sure it compiles fine. This will also help with allmodconfig
# and allyesconfig builds.
##
