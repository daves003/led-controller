#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#include <boost/asio.hpp>

using boost::asio::ip::udp;
using boost::asio::ip::address_v4;
static const udp::endpoint c_destination { address_v4::from_string("192.168.62.144"), 8000 };
constexpr unsigned c_resolution = 144;
constexpr float c_maxBrightness = 0.2;

int main(int argc, char *argv[])
{
	QGuiApplication a(argc, argv);

	boost::asio::io_service ioService;
	udp::socket socket(ioService);
	socket.open(udp::v4());

	QTimer timer;
	timer.setSingleShot(false);
	timer.setInterval(1000.f / qApp->primaryScreen()->refreshRate());
	QObject::connect(&timer, &QTimer::timeout, [&socket]
	{
		const auto geometry = qApp->primaryScreen()->geometry();
		const auto pixels = qApp->primaryScreen()->grabWindow(0, geometry.bottomLeft().x(), geometry.bottomLeft().y());
		Q_ASSERT(pixels.height() == 1);
		const auto img = pixels.scaled(144, 1).toImage();
		char bytes[c_resolution*3];
		for(unsigned i = 0; i < c_resolution*3; i+=3)
		{
			const auto rgb = img.pixel(i/3, 0);
			QColor col{rgb};
			col.setHsv(col.hue(), std::min(255, col.saturation()*2), col.value());
			bytes[i]   = c_maxBrightness * col.red();
			bytes[i+1] = c_maxBrightness * col.green();
			bytes[i+2] = c_maxBrightness * col.blue();
		}
		const auto sent = socket.send_to(boost::asio::buffer(bytes), c_destination);
		if(sent < sizeof(bytes)) qDebug() << "Could only send " << float(sent)/sizeof(bytes) << "% of data.";
	});
	timer.start();
	return a.exec();
}
