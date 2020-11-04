#define SHOW_DEBUG 0

#include <QGuiApplication>
#include <QScreen>
#include <QPixmap>
#include <QTimer>
#include <QDebug>
#if SHOW_DEBUG
	#include <QApplication>
	#include <QLabel>
	#include <QMainWindow>
#endif
#include <boost/asio.hpp>

using boost::asio::ip::udp;
using boost::asio::ip::address_v4;
static const udp::endpoint c_destination { address_v4::from_string("192.168.2.197"), 8000 };
constexpr unsigned c_resolution = 300;
constexpr float c_maxBrightness = 0.1f;
constexpr float c_smoothing = 0.0f;
constexpr int c_bottomPixelCount = 30;

int main(int argc, char *argv[])
{
#if SHOW_DEBUG
	QApplication a(argc, argv);
#else
	QGuiApplication a(argc, argv);
#endif

	boost::asio::io_service ioService;
	udp::socket socket(ioService);
	socket.open(udp::v4());

#if SHOW_DEBUG
	QMainWindow w;
	auto* l = new QLabel;
	w.setCentralWidget(l);
	w.show();
#endif

	QTimer timer;
	timer.setSingleShot(false);
//	timer.setInterval(1000.f / qApp->primaryScreen()->refreshRate());
//	timer.setInterval(50.f / qApp->primaryScreen()->refreshRate());
	timer.setInterval(1);
	qDebug() << "Interval: " << timer.interval() << "ms";

	char last_bytes[c_resolution*3] = {};
	QObject::connect(&timer, &QTimer::timeout, [&]
	{
		const auto geometry = qApp->primaryScreen()->geometry();
		const auto pixels = qApp->primaryScreen()->grabWindow(0, 0, geometry.height()-c_bottomPixelCount-1);
		Q_ASSERT(pixels.height() > 0);
		const auto img = pixels.scaled(c_resolution, 1).toImage();
#		if SHOW_DEBUG
		l->setPixmap(pixels);
#		endif

		char bytes[c_resolution*3];
		for(unsigned i = 0; i < c_resolution*3; i+=3)
		{
			const auto rgb = img.pixel(i/3, 0);
			QColor col{rgb};
			col.setHsv(col.hue(), std::min(255, col.saturation()*2), col.value());
			bytes[i]   = (1-c_smoothing) * c_maxBrightness * col.red  () + (c_smoothing * last_bytes[i  ]);
			bytes[i+1] = (1-c_smoothing) * c_maxBrightness * col.green() + (c_smoothing * last_bytes[i+1]);
			bytes[i+2] = (1-c_smoothing) * c_maxBrightness * col.blue () + (c_smoothing * last_bytes[i+2]);
		}
		std::memcpy(last_bytes, bytes, sizeof(bytes));
		const auto sent = socket.send_to(boost::asio::buffer(bytes), c_destination);
		if(sent < sizeof(bytes)) qDebug() << "Could only send " << float(sent)/sizeof(bytes) << "% of data.";
	});
	timer.start();
	return a.exec();
}
