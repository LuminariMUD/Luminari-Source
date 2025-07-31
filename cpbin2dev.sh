cp ../bin/circle /home/luminari/dev/bin
echo "Copied binary to luminari dev port"
cp /home/luminari/dev/changelog /home/luminari/dev/lib/text/news
echo "Moved changelog over to news file"
echo "1 second delay before starting server..."
sleep 1
echo "Starting dev server on port 4101"
cd /home/luminari/dev && ./checkmud.sh &
echo ""
echo "To change to the dev directory, run:"
echo "cd /home/luminari/dev"
