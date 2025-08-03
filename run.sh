make clean
make 
rm -f filesamples/makro1.am

echo "Running assembler on filesamples/makro1.as"
cat filesamples/makro1.as
echo "Output will be in filesamples/makro1.am"
./assembler filesamples/makro1.as