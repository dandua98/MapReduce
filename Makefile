wc: threadpool.o mapreduce.o distwc.o
	g++ -std=c++11 -g -o wordcount threadpool.o mapreduce.o distwc.o -Wall -Werror -pthread

compile: threadpool.o mapreduce.o distwc.o

threadpool.o: threadpool.cpp threadpool.h
	g++ -std=c++11 -c -g threadpool.cpp -Wall -Werror -pthread

mapreduce.o: mapreduce.cpp mapreduce.h threadpool.h
	g++ -std=c++11 -I./ -c -g mapreduce.cpp -Wall -Werror -pthread

distwc.o: distwc.cpp mapreduce.h
	g++ -std=c++11 -I./ -c -g distwc.cpp -Wall -Werror -pthread

compress: Makefile *.cpp *.h *.c README.md
	tar -czvf mapreduce.tar.gz Makefile *.cpp *.h *.c README.md

clean:
	rm -rf *.o wordcount *.tar.gz
