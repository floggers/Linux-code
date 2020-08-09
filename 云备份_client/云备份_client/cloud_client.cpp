#include "cloud_client.hpp"

#define STORE_FILE "./list.backup"
#define LISTEN_DIR "./backup/"
#define SERVER_IP "192.168.231.130"
#define SERVER_PORT 9000
using namespace std;


int main() {
	Cloud_Client client(LISTEN_DIR,STORE_FILE,SERVER_IP,SERVER_PORT);
	client.Start();
	return 0;
}