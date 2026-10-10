// readdir(/proc/self/fd) + fstat against fstat(0..FDSize), the measurement behind forEachFd().
//   c++ -O2 -std=gnu++26 todo/statusPage/fdbench.cpp -o /tmp/fdbench && /tmp/fdbench 10000
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <dirent.h>
#include <fstream>
#include <string>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>
static double ms(std::chrono::steady_clock::time_point a) { return std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - a).count(); }
int main(int argc, char** argv) {
	int n = atoi(argv[1]);
	for (int i = 0; i < n; ++i) socket(AF_INET, SOCK_STREAM, 0);
	for (int rep = 0; rep < 3; ++rep) {
		auto t = std::chrono::steady_clock::now();
		std::vector<int> fds; DIR* d = opendir("/proc/self/fd"); while (auto* e = readdir(d)) if (e->d_name[0] != '.') fds.push_back(atoi(e->d_name)); closedir(d);
		double tDir = ms(t);
		t = std::chrono::steady_clock::now(); int s1 = 0; for (int fd : fds) { struct stat st; if (fstat(fd, &st) == 0 && S_ISSOCK(st.st_mode)) ++s1; }
		double tStat = ms(t);
		t = std::chrono::steady_clock::now();
		std::ifstream f("/proc/self/status"); std::string l; long fdsize = 0; while (std::getline(f, l)) if (l.rfind("FDSize:", 0) == 0) fdsize = atol(l.c_str() + 7);
		int s2 = 0; for (long fd = 0; fd < fdsize; ++fd) { struct stat st; if (fstat((int)fd, &st) == 0 && S_ISSOCK(st.st_mode)) ++s2; }
		double tScan = ms(t);
		printf("n %d  readdir %.1f ms + fstat %.1f ms (%d sock)   |   FDSize %ld scan %.1f ms (%d sock)\n", n, tDir, tStat, s1, fdsize, tScan, s2);
	}
}
