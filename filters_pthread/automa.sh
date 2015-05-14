#!/bin/bash          
echo Sending Executions to NSC. . . .  
echo Parameter : input image
echo second Parameter :  radius
echo last parameter: how many threads?!
declare -a arrayname=(1 2 3 4 8 16)

 for i in ${arrayname[@]}; 
	do
echo 	salloc -N 1 -t 1 ./blurc $2 $1 r.ppm $i
		#salloc -N 1 -t 1 
		./blurc $1 $2 r.ppm $i
	done
mv measures.csv measuresBlurfilter_$2_r$1.csv
      	
        

