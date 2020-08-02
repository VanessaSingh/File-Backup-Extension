all:
	g++ write_backup.cpp -o write_backup 
	gcc test.c -o test_ans
clean:	
	rm write_backup test_ans
