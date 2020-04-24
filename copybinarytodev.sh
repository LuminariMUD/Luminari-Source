TIMEDATE=$(date +"%m-%d-%Y-%H-%M")
mv /home/luminari/dev/bin/circle /home/luminari/dev/bin/circle.$TIMEDATE
cp ../bin/circle /home/luminari/dev/bin
echo Copied binary to luminari dev port
