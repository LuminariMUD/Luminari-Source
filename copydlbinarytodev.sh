TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/krynn/dev/bin/circle /home/krynn/dev/bin/circle.$TIMEDATE
cp ../bin/circle /home/krynn/dev/bin
echo Copied binary to aod reawakening dev port
cp /home/krynn/dev/changelog /home/krynn/dev/lib/text/news
echo Moved changelog over to news file

