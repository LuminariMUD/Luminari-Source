TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/aod/reawakening/dev/bin/circle /home/aod/reawakening/dev/bin/circle.$TIMEDATE
cp ../bin/circle /home/aod/reawakening/dev/bin
echo Copied binary to aod reawakening dev port
cp /home/aod/reawakening/dev/changelog /home/aod/reawakening/dev/lib/text/news
echo Moved changelog over to news file

