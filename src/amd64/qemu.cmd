:<<"::CMDGOTO"
@echo off
goto :CMDENTRY
rem https://stackoverflow.com/questions/17510688/single-script-to-run-in-both-windows-batch-and-linux-bash
::CMDGOTO

echo "========== 3o3 qemu ${SHELL} ================="

qemu-system-x86_64 -D qemu.log -d int,cpu_reset -no-reboot -drive if=pflash,format=raw,unit=0,readonly=on,file="/usr/share/qemu/OVMF.fd" -drive if=none,id=stick,format=vpc,file=32gears.vhd -device nec-usb-xhci,id=xhci -device usb-storage,drive=stick,bus=xhci.0

exit $?
:CMDENTRY

echo ============= 3o3 qemu %COMSPEC% ============

"C:\Program Files\qemu\qemu-system-x86_64.exe" -D qemu.log -d int,cpu_reset -no-reboot -drive if=pflash,format=raw,unit=0,readonly=on,file="C:\Program Files\qemu\share\edk2-x86_64-code.fd" -drive if=none,id=stick,format=vpc,file=32gears.vhd -device nec-usb-xhci,id=xhci -device usb-storage,drive=stick,bus=xhci.0

exit 0


