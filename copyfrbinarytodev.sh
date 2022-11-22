TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/frmud/dev/bin/circle /home/frmud/dev/bin/circle.$TIMEDATE
cp ../bin/circle /home/frmud/dev/bin
echo Copied binary to frmud dev port
cp /home/frmud/dev/changelog /home/frmud/dev/lib/text/news
echo Moved changelog over to news file
