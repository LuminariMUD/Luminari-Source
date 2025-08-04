cp hlq/$1.hlq /home/frmud/frmud/lib/world/hlq
cp mob/$1.mob /home/frmud/frmud/lib/world/mob
cp obj/$1.obj /home/frmud/frmud/lib/world/obj
cp qst/$1.qst /home/frmud/frmud/lib/world/qst
cp shp/$1.shp /home/frmud/frmud/lib/world/shp
cp trg/$1.trg /home/frmud/frmud/lib/world/trg
cp wld/$1.wld /home/frmud/frmud/lib/world/wld
cp zon/$1.zon /home/frmud/frmud/lib/world/zon
cd /home/frmud/frmud/lib/world
chown -R frmud:frmud *
echo Zone $1 copied to faerun d20 port
