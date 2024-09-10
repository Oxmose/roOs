# Size of the eMMC/SD in sectors
SECTORS=16384
SECTOR_SIZE=1

# Create zero-file to represent the eMMC (needed for fdisk)
dd if=/dev/zero of=mem_file seek=$((SECTORS - 1)) bs=$SECTOR_SIZE count=1