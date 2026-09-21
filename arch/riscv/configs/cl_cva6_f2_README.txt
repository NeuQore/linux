AWS F2 cl_cva6_linux kernel options (Linux v6.12, NeuQore/linux branch cva6).

  make ARCH=riscv defconfig
  scripts/kconfig/merge_config.sh -m .config arch/riscv/configs/cl_cva6_f2.fragment
  make ARCH=riscv olddefconfig

Set CONFIG_INITRAMFS_SOURCE to your rootfs path before the final olddefconfig.

Full build flow: NeuQore/aws-fpga cl_cva6_linux/linux/build_linux.sh
Firmware siblings: NeuQore/opensbi and NeuQore/busybox @ cva6 (see linux/sources.env.sh).
