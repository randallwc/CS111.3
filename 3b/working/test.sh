#!/bin/bash

echo ""
echo "/////BEGIN MAKING *.TXT FILES/////"
echo ""

for i in {1..22}
do
	INPUT_FILE=test/P3B-test_$i.csv
	./lab3b $INPUT_FILE > test/$i.txt
	EXIT_CODE=$?
	ERROR_FILE=test/P3B-test_$i.err
	if [ -s $ERROR_FILE ]
	then
		if [ $EXIT_CODE -ne 2 ]
		then
			echo "test $i failed ... incorrrect exit code $EXIT_CODE"
		fi
	fi
done

echo ""
echo "/////BEGIN TRIVIAL TEST/////"
echo ""

if ./lab3b test/trivial.csv > test/0.txt
then
  echo "trivial.csv passed"
  cat test/0.txt
else
  echo "trivial.csv failed"
  echo "===correct==="
  echo "NO OUTPUT"
  echo "===incorrect==="
  cat test/0.txt
  echo "===end==="
fi

echo ""
echo "/////BEGIN DIFF TESTS/////"
echo ""

for i in {1..22}
do
	ERROR_FILE=test/P3B-test_$i.err
	OUTPUT_FILE=test/$i.txt
	DIFF_FILE=test/$i.diff
	diff -u $OUTPUT_FILE $ERROR_FILE > $DIFF_FILE
	if [ -s $DIFF_FILE ]
	then
		echo ""
		echo "test $i FAILED"
		echo "///DIFF///"
		cat $DIFF_FILE
		echo "///END///"
	else
    	echo "test $i passed"
	fi
done
