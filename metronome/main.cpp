#include <chrono>
#include <thread>
// #include <iostream>
using namespace std::chrono_literals;

#include <cpprest/http_msg.h>
// #include <wiringPi.h>
#include <pigpio.h>

#include "metronome.hpp"
#include "rest.hpp"

// ** Remember to update these numbers to your personal setup. **
#define LED_RED   27
#define LED_GREEN 17
#define BTN_MODE  23
#define BTN_TAP   24

// Mark as volatile to ensure it works multi-threaded.
volatile bool btn_mode_pressed = false;

// Run an additional loop separate from the main one.
void blink() {
	gpioWrite(LED_RED, 1);
	std::this_thread::sleep_for(1s);
	gpioWrite(LED_RED, 0);
}

// This is called when a GET request is made to "/answer".
void get42(web::http::http_request msg) {
	msg.reply(200, web::json::value::number(42));
}

// --------------------metronome implementations----------------------------
void metronome::start_timing() {
	m_timing = true;
	m_beat_count = 0;
}

void metronome::stop_timing() {
	m_timing = false;
}

void metronome::tap() {
	if (m_timing && m_beat_count < beat_samples) {
		auto now = std::chrono::system_clock::now().time_since_epoch().count();
		m_beats[m_beat_count] = now;
	}
}

size_t metronome::get_bpm() const {
	if (m_beat_count < beat_samples)
		return 0;

	size_t sum = 0;
	for (size_t i = 0; i < beat_samples - 1; ++i)
		sum += m_beats[i + 1] - m_beats[i];
	return 6000000000 / (sum / (beat_samples - 1));

}

// This program shows an example of input/output with GPIO, along with
// a dummy REST service.
// ** You will have to replace this with your metronome logic, but the
//    structure of this program is very relevant to what you need. **

int main() {
	// WiringPi must be initialized at the very start.
	// This setup method uses the Broadcom pin numbers. These are the
	// larger numbers like 17, 24, etc, not the 0-16 virtual ones.
	// wiringPiSetupGpio();
	if (gpioInitialise() < 0) {
		return 1;
	}

	// ** This is where you would create an instance of your metronome. **
	metronome my_metronome;
	my_metronome.start_timing();


	// simulate beat tapping
	// for (int i = 0; i < metronome::beat_samples; ++i) {
	// 	metronome.tap();
	// 	blink();
	// 	std::this_thread::sleep_for(1s);
	// }

	// Set up the directions of the pins.
	// Be careful here, an input pin set as an output could burn out.
	gpioSetMode(LED_RED, PI_OUTPUT);
	gpioSetMode(LED_GREEN, PI_OUTPUT);
	gpioSetMode(BTN_MODE, PI_INPUT);
	gpioSetMode(BTN_TAP, PI_INPUT);
	// Note that you can also set a pull-down here for the button,
	// if you do not want to use the physical resistor.
	//pullUpDnControl(BTN_MODE, PUD_DOWN);

	// Configure the rest services after setting up the pins,
	// but before we start using them.
	// ** You should replace these with the BPM endpoints. **
	auto get42_rest = rest::make_endpoint("/answer");
	get42_rest.support(web::http::methods::GET, get42);

	// Start the endpoints in sequence.
	get42_rest.open().wait();

	// Use a separate thread for the blinking.
	// This way we do not have to worry about any delays
	// caused by polling the button state / etc.
	for (int i =0; i< metronome::beat_samples; ++i) {
		my_metronome.tap();
		std::thread blink_thread(blink);
		std::this_thread::sleep_for(1s);
	}

	my_metronome.stop_timing();
	size_t bpm = my_metronome.get_bpm();
	std::cout << "BPM: " << bpm << std::endl;

	// blink_thread.detach();

	gpioTerminate();

	return 0;
}
