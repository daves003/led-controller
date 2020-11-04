#include <WiFi.h>
#include <NeoPixelBus.h>
#include <NeoPixelAnimator.h>
#include "tuple.hpp"

//////////////////////////////////////////////////////////
//////////////////////// SETTINGS ////////////////////////
//////////////////////////////////////////////////////////

// How many LED strips are connected
constexpr uint8_t NUM_LED_STRIPS = 2;
// what pins are the strips connected to
constexpr uint8_t LED_PINS[NUM_LED_STRIPS] = { 12, 14 };
// how many LEDs per strip
constexpr uint16_t LED_COUNTS[NUM_LED_STRIPS] = { 144, 300 };

// how many wifi networks
constexpr int NUM_NETWORKS = 2;
// wifi SSIDs
constexpr const char* SSIDS[NUM_NETWORKS] =
{
	"wifi-network-1",
	"wifi-network-2"
};
// wifi passwords
constexpr const char* PASSWORDS[NUM_NETWORKS] =
{
	"wifi-pass-1",
	"wifi-pass-2"
};
// how many milliseconds we will try to connect to each network
constexpr unsigned long WIFI_CONNECTION_TIMEOUT = 5000;
// what port to listen on
constexpr int PORT = 8000;


//all the strips
using Strip0_t = NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt0Ws2812xMethod>;
using Strip1_t = NeoPixelBus<NeoGrbFeature, NeoEsp32Rmt1Ws2812xMethod>;
Strip0_t g_strip_0{LED_COUNTS[0], LED_PINS[0]};
Strip1_t g_strip_1{LED_COUNTS[1], LED_PINS[1]};
tuple<Strip0_t*, Strip1_t*> g_strips { &g_strip_0, &g_strip_1 };


//////////////////////////////////////////////////////////
////////////////////////// CODE //////////////////////////
//////////////////////////////////////////////////////////

// global udp listener
WiFiUDP g_udp;


//////////// ANIMATIONS ////////////
void animateConnecting(AnimationParam param)
{
	// a yellow pixel scanning across the strips
	float progress = (param.progress>0.5) ? 2*(1-param.progress) : 2*(param.progress);
	progress = NeoEase::CubicInOut(progress);
	const RgbColor color(255, 255, 0);
    for_each_in(g_strips, [&](auto strip)
    {
        const auto led = strip->PixelCount() * progress;
        strip->ClearTo(RgbColor(0,0,0));
        strip->SetPixelColor(led, color);

    });
}

void animateConnectionFailure(AnimationParam param)
{
	// red eased flashing
	const float progress = (param.progress>0.5) ? 2*(1-param.progress) : 2*(param.progress);
	const float brightness = NeoEase::CubicIn(progress);
	const HsbColor color(0.f, 1.f, brightness);
    for_each_in(g_strips, [&](auto strip)
    {
        strip->ClearTo(color);
    });
}

void animateConnected(AnimationParam param)
{
	// two abrupt green flashes
	const bool on = (param.progress < 0.25) || (param.progress > 0.5 && param.progress < 0.75);
	const auto color = on ? RgbColor(0, 255, 0) : RgbColor(0,0,0);
    for_each_in(g_strips, [&](auto strip)
    {
        strip->ClearTo(color);
    });
}

void error()
{
	// display a neverending error animation
	NeoPixelAnimator animator(1); //only 1 animation
	animator.StartAnimation(0, 100, animateConnectionFailure);
	while(true)
	{
		if(!animator.IsAnimating()) animator.RestartAnimation(0);
		animator.UpdateAnimations();
        for_each_in(g_strips, [&](auto s){ s->Show(); });
    }
}

///////////// SETUP /////////////
void setup()
{
	Serial.begin(115200);
	Serial.println("Starting...");
	// start led strips
    for_each_in(g_strips, [&](auto strip)
    {
        strip->Begin();
        strip->ClearTo(RgbColor(0,0,0));
        strip->Show();
    });
	NeoPixelAnimator animator(1); //only 1 animation

	// connect to wifi
	animator.StartAnimation(0, 1000, animateConnecting);
	for(uint8_t i = 0; i < NUM_NETWORKS; ++i)
	{
		Serial.print("Connecting to: ");
		Serial.println(SSIDS[i]);
		WiFi.begin(SSIDS[i], PASSWORDS[i]);
		for(const auto end = millis() + WIFI_CONNECTION_TIMEOUT; millis() < end; )
		{
			if(WiFi.status() == WL_CONNECTED) goto Wifi_connected;
			// play connecting animation
			if (!animator.IsAnimating()) animator.RestartAnimation(0);
			animator.UpdateAnimations();
            for_each_in(g_strips, [&](auto s){ s->Show(); });
        }
		Serial.println("Network unreachable.");
	}
	// play failure animation
	Serial.println("Not connected to any network! Stoping...");
	animator.StartAnimation(0, 1000, animateConnectionFailure);
	while(true)
	{
		if(!animator.IsAnimating()) animator.RestartAnimation(0);
		animator.UpdateAnimations();
        for_each_in(g_strips, [&](auto s){ s->Show(); });
    }

	Wifi_connected:
	Serial.println("Connection established.");
	Serial.print  ("IP address: ");
	Serial.println(WiFi.localIP());
	bool error;
	do {
		error = g_udp.begin(PORT) == 0;
		if(error) animator.StartAnimation(0, 500, animateConnectionFailure);
		else      animator.StartAnimation(0, 500, animateConnected);
		while(animator.IsAnimating())
		{
			animator.UpdateAnimations();
            for_each_in(g_strips, [&](auto s){ s->Show(); });
        }
		animator.RestartAnimation(0);
	}
	while (error);
	Serial.print("UDP listening on port ");
	Serial.println(PORT);
}

///////////// LOOP /////////////
void loop()
{
	const auto available = g_udp.parsePacket();
	if(available == 0) return;

	auto* buf = new byte[available];
	for(size_t i = 0; i < available; ++i) buf[i] = g_udp.read();

	for(int stripIdx = 0; stripIdx < NUM_LED_STRIPS; ++stripIdx)
    for_each_in(g_strips, [available, &buf](auto strip)
	{
        const auto ledCount = strip->PixelCount();
		for(int ledIdx = 0; ledIdx < ledCount; ++ledIdx)
		{
			const auto dataIdx = available / 3 * ledIdx / ledCount * 3;
			if(dataIdx+2 > available) error();
			RgbColor col(buf[dataIdx], buf[dataIdx+1], buf[dataIdx+2]);
            strip->SetPixelColor(ledIdx, col);
		}
        strip->Show();
    });
	delete[] buf;
}
