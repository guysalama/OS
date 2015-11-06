mkdir 200857233
cd 200857233
mkdir temp
echo "Guy Salama" > temp/guy
echo "Guy Salama" > temp/salama
echo "Guy Salama" > temp/guysalama
cp temp/guy salama
cp temp/salama guy
rm temp/guy
rm temp/salama
mv temp/guysalama guysalama
rmdir temp
echo "200857233 directory contains:"
ls -l -a
echo "guy file contains:"
cat guy
echo "salama file contains:"
cat salama
echo "guysalama file contains:"
cat guysalama
