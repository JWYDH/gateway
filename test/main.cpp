#include "test_tcp_client.hpp"
#include "test_tcp_server.hpp"

int main(int argc, char *argv[])
{
	if (argc <= 1)
	{
		JW_LOG(jw::LL_ERROR, "no params");
		return -1;
	}
	switch (std::stoi(argv[1]))
	{
	case 1:
		test_tcp_client();
		break;
	case 2:
		test_tcp_server();
		break;
	default:
		JW_LOG(jw::LL_ERROR, "invalid params");
		break;
	}
	return 0;
}