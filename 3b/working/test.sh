#!/bin/bash
for i in {1..22}
do
	INPUT_FILE=test/P3B-test_$i.csv
	./lab3b $INPUT_FILE > test/$i.txt
done

for i in {1..22}
do
	ERROR_FILE=test/P3B-test_$i.err
	OUTPUT_FILE=test/$i.txt
	DIFF_FILE
	diff -u $ERROR_FILE $OUTPUT_FILE > $DIFF_FILE
	if[ $? -eq 0 ]
	then
		echo "test $i passed"
	else
		echo "test $i failed"
	fi
done