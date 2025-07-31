# script to backup a single zone.
# command syntax is: ./backup-zone.sh (backup directory to create) (zone number)
# example: ./backup-zone.sh ashenport-backup 1030
# this will backup zone 1030 to the directory ashenport-backup
mkdir $1
mkdir $1/hlq
mkdir $1/mob
mkdir $1/obj
mkdir $1/qst
mkdir $1/shp
mkdir $1/trg
mkdir $1/wld
mkdir $1/zon
cp hlq/$2.hlq $1/hlq
cp mob/$2.mob $1/mob
cp obj/$2.obj $1/obj
cp qst/$2.qst $1/qst
cp shp/$2.shp $1/shp
cp trg/$2.trg $1/trg
cp wld/$2.wld $1/wld
cp zon/$2.zon $1/zon
echo Backup Up Zone $2 to Directory $1
