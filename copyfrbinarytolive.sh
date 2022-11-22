TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/frmud/frmud/bin/circle /home/frmud/frmud/bin/circle.$TIMEDATE
cp ../bin/circle /home/frmud/frmud/bin
echo Copied binary to frmud live port
cp /home/frmud/frmud/changelog /home/frmud/frmud/lib/text/news
echo Moved changelog over to news file

