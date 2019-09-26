@ https://github.com/tianocore/tianocore.github.io/wiki/How-to-run-OVMF
qemu-system-x86_64.exe -L . -smp cpus=4,cores=4 -m size=512m,maxmem=8g -bios OVMF-pure-efi.fd -net none -hda fat:EFI_PATH

