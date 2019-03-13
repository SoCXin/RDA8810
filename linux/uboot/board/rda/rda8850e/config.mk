#
#the spl code should not exceed the SPL_MAX_SIZE,and if 
#the spl don't up to the max size, paddint the last with
#zero(or 0xff?)

#48kbytes, 48*1024
#SPL_APPENDING_TO	:= 48
#SPL_MAX_SIZE	:= 49512

# space for spl(48KiB) & mtd partition table(4KiB),
# so uboot offset 52KiB(48 + 4).

SPL_APPENDING_TO   := 52

CONFIG_SPL_SIGNATURE_CHECK_IMAGE := y
