obj-$(CONFIG_PCIE_DW) += pcie-designware.o
obj-$(CONFIG_PCIE_DW_EP) += pcie-designware-ep.o
obj-$(CONFIG_PCIE_DW_PLAT) += pcie-designware-plat.o
ifneq ($(filter y, $(CONFIG_PCI_SCA200V100_HOST) $(CONFIG_PCI_SCA200V100_EP)),)
	obj-$(CONFIG_PCI_SCA200V100) += pci-smartchip.o
endif
