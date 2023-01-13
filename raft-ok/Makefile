

CC = gcc -g

All: leader followers

# leader: arpc_leader.cpp appendentries.h
# 	$(CC)  arpc_leader.cpp -o leader

# followers: arpc_followers.cpp appendentries.h
# 	$(CC) arpc_followers.cpp -o followers


leader: al.c appendentries.h
	$(CC) al.c -o leader

followers: af.c appendentries.h
	$(CC) af.c -o followers

clean:
	rm -f leader followers

# $(CC) appendentriesRPC_leader.o appendentriesRPC_followers.o -o append