#include <iostream>
#include <thread>
#include <boost/asio.hpp>

// hsv2rgb from https://stackoverflow.com/a/6930407


using boost::asio::ip::udp;
using boost::asio::ip::address_v4;
static const udp::endpoint c_destination { address_v4::from_string("192.168.62.144"), 8000 };


struct rgb {
	double r;       // a fraction between 0 and 1
	double g;       // a fraction between 0 and 1
	double b;       // a fraction between 0 and 1
};

struct hsv {
	double h;       // angle in degrees
	double s;       // a fraction between 0 and 1
	double v;       // a fraction between 0 and 1
};

rgb hsv2rgb(hsv in)
{
	double      hh, p, q, t, ff;
	long        i;
	rgb         out;

	if(in.s <= 0.0) {       // < is bogus, just shuts up warnings
		out.r = in.v;
		out.g = in.v;
		out.b = in.v;
		return out;
	}
	hh = in.h;
	if(hh >= 360.0) hh = 0.0;
	hh /= 60.0;
	i = (long)hh;
	ff = hh - i;
	p = in.v * (1.0 - in.s);
	q = in.v * (1.0 - (in.s * ff));
	t = in.v * (1.0 - (in.s * (1.0 - ff)));

	switch(i) {
	case 0:
		out.r = in.v;
		out.g = t;
		out.b = p;
		break;
	case 1:
		out.r = q;
		out.g = in.v;
		out.b = p;
		break;
	case 2:
		out.r = p;
		out.g = in.v;
		out.b = t;
		break;

	case 3:
		out.r = p;
		out.g = q;
		out.b = in.v;
		break;
	case 4:
		out.r = t;
		out.g = p;
		out.b = in.v;
		break;
	case 5:
	default:
		out.r = in.v;
		out.g = p;
		out.b = q;
		break;
	}
	return out;
}


int main()
{
	//rainbow data
	char data[1920 / 10 * 3];
	for(unsigned i = 0; i < sizeof(data); i+= 3)
	{
		float h = 360.f * i / sizeof(data);
		const auto rgb = hsv2rgb({h, 1.f, 0.1f});
		data[i]   = 255 * rgb.r;
		data[i+1] = 255 * rgb.g;
		data[i+2] = 255 * rgb.b;
	}

	std::cout << "Starting Udp transmittion." << std::endl;
	boost::asio::io_service ioService;
	udp::socket socket(ioService);
	socket.open(udp::v4());

	//send rainbow to ESP
	while(true)
	{
		const auto sent = socket.send_to(boost::asio::buffer(data), c_destination);
		if(sent < sizeof(data))
		{
			std::cout << "Could only send " << float(sent)/sizeof(data) << "% of pixel data!" << std::endl;
		}
		std::rotate(std::begin(data), std::begin(data)+3, std::end(data));
		std::this_thread::sleep_for(std::chrono::milliseconds(3));
	}
}
