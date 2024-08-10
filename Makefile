INCLUDE := -I $(shell pwd) -I /usr/include -g -lpthread

main:main.o workload.o arch.o mapping.o eigenUtil.o debug.o singleLevelAnalysis.o multiLevelAnalysis.o transformSearchEngine.o timeline.o groupSearchEngine.o costAnalysis.o tileSearchEngine.o config.o
	g++ main.o workload.o arch.o mapping.o eigenUtil.o debug.o singleLevelAnalysis.o multiLevelAnalysis.o transformSearchEngine.o timeline.o groupSearchEngine.o costAnalysis.o tileSearchEngine.o config.o -o main ${INCLUDE} 
transformSearchEngine.o:src/searchEngine/transformSearchEngine.cpp
	g++ -c src/searchEngine/transformSearchEngine.cpp ${INCLUDE}
workload.o:src/datastruct/workload.cpp
	g++ -c src/datastruct/workload.cpp ${INCLUDE}
arch.o:src/datastruct/arch.cpp 
	g++ -c src/datastruct/arch.cpp ${INCLUDE}
mapping.o:src/datastruct/mapping.cpp
	g++ -c src/datastruct/mapping.cpp  ${INCLUDE}
eigenUtil.o:src/util/eigenUtil.cpp
	g++ -c src/util/eigenUtil.cpp ${INCLUDE}
debug.o:src/util/debug.cpp
	g++ -c src/util/debug.cpp ${INCLUDE}
singleLevelAnalysis.o: src/analysis/singleLevelAnalysis.cpp
	g++ -c src/analysis/singleLevelAnalysis.cpp ${INCLUDE}
multiLevelAnalysis.o: src/analysis/multiLevelAnalysis.cpp
	g++ -c src/analysis/multiLevelAnalysis.cpp ${INCLUDE}
timeline.o:src/util/timeline.cpp
	g++ -c src/util/timeline.cpp ${INCLUDE}
groupSearchEngine.o:src/searchEngine/groupSearchEngine.cpp
	g++ -c src/searchEngine/groupSearchEngine.cpp ${INCLUDE}
tileSearchEngine.o:src/searchEngine/tileSearchEngine.cpp
	g++ -c src/searchEngine/tileSearchEngine.cpp ${INCLUDE}
config.o:src/util/config.cpp
	g++ -c src/util/config.cpp ${INCLUDE}
costAnalysis.o:src/analysis/costAnalysis.cpp
	g++ -c src/analysis/costAnalysis.cpp ${INCLUDE}
main.o:main.cpp
	g++ -c main.cpp ${INCLUDE}
.PHONY:clean

clean:
	rm -r ./*.o
	rm -r ./*.json
