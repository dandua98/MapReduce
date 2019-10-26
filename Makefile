wc: threadpool.o mapreduce.o distwc.o
	g++ -std=c++11 -o wordcount threadpool.o mapreduce.o distwc.o

compile: threadpool.o mapreduce.o distwc.o

threadpool.o: threadpool.cpp threadpool.h
	g++ -std=c++11 -c threadpool.cpp -Wall -Werror -pthread

mapreduce.o: mapreduce.cpp mapreduce.h threadpool.h
	g++ -std=c++11 -I./ -c mapreduce.cpp -Wall -Werror -pthread

distwc.o: distwc.cpp mapreduce.h
	g++ -std=c++11 -I./ -c distwc.cpp -Wall -Werror -pthread

compress: Makefile *.cpp *.h *.c readme.md
	tar -czvf mapreduce.tar.gz Makefile *.cpp *.h *.c readme.md

clean:
	rm -rf *.o wordcount *.tar.gz
