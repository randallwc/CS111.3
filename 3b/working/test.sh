#!/bin/bash
for i in {1..22}
do
	INPUT_FILE=test/P3B-test_$i.csv
	./lab3b $INPUT_FILE > test/$i.txt
done

if ! ./lab3b test/trivial.csv > test/23.txt
then
  echo "trivial.csv passed"
else
  echo "trivial.csv failed"
  echo "===correct==="
  echo "NO OUTPUT"
  echo "===incorrect==="
  cat test/23.txt
  echo "===end==="
fi

for i in {1..22}
do
    ERROR_FILE=test/P3B-test_$i.err
    OUTPUT_FILE=test/$i.txt
    DIFF_FILE=test/$i.diff
    diff -u $ERROR_FILE $OUTPUT_FILE > $DIFF_FILE
    if [ -s $DIFF_FILE ]
    then
      echo "test $i FAILED"
        echo "===correct==="
        cat $ERROR_FILE
        echo "===incorrect==="
        cat $OUTPUT_FILE
        echo "===end==="
    else
    echo "test $i passed"
    fi
done
